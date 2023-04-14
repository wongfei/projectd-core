#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <gl/GLU.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Sim/Simulator.h"
#include "Sim/Track.h"
#include "Car/CarImpl.h"

#include "Renderer/GLPrimitive.h"
#include "Renderer/GLFont.h"
#include "Renderer/GLChart.h"
#include "Renderer/GLSkyBox.h"
#include "Renderer/GLTrack.h"
#include "Renderer/GLCar.h"

#include "Audio/FmodAudioRenderer.h"
#include "Input/DICarController.h"
#include "Input/KeyboardCarController.h"

#include <chrono>

inline D::vec3f v(const glm::vec3& in) { return D::vec3f(in.x, in.y, in.z); }
inline glm::vec3 v(const D::vec3f& in) { return glm::vec3(in.x, in.y, in.z); }

using namespace D;

//=======================================================================================

double simTickRate_ = 1 / 333.0f;
double drawTickRate_ = 1 / 100.0;

bool fullscreen_ = false;
bool recordTelemetry_ = false;
int width_ = 1280;
int height_ = 720;
int swapInterval_ = -1;

SDL_Window* appWindow_ = nullptr;
SDL_GLContext glContext_ = nullptr;
HWND sysWindow_ = nullptr;
bool exitFlag_ = false;
bool keys_[SDL_NUM_SCANCODES] = {};
bool asynckeys_[SDL_NUM_SCANCODES] = {};

SimulatorPtr sim_;
TrackPtr track_;
CarPtr car_;
CarPtr carB_;
mat44f pitPos_;

std::unique_ptr<FmodAudioRenderer> audioRenderer_;
std::unique_ptr<DICarController> controlsProvider_;
std::unique_ptr<KeyboardCarController> kbControlsProvider_;

GLFont font_;
GLSkyBox sky_;
GLTrack trackAvatar_;
GLCar carAvatar_;

Surface* lookatSurf_ = nullptr;
vec3f lookatPos_;

glm::vec3 camPos_ = {};
glm::vec3 camYpr_ = {90, 0, 0};
glm::vec3 camFront_ = {};
glm::vec3 camRight_ = {};
glm::vec3 camUp_ = {};
glm::mat4 camView_ = {};

enum class ECamMode {
	Free = 0,
	Eye,
	Rear,
	COUNT // LAST
};
int inpCamMode_ = (int)ECamMode::Eye;
vec3f eyeOff_(-0.33f, 0.48f, -0.15f);
float fov_ = 56.0f;
float eyeTuneSpeed_ = 0.2f;
float fovTuneSpeed_ = 5.0f;

glm::vec3 inpMove_(0, 0, 0);
bool inpMouseBtn_[2] = { 0, 0 };
float inpMoveSpeed_ = 10.0f;
float inpMoveSpeedScale_ = 1.0f;
float inpMouseSens_ = 0.35f;
bool inpSimFlag_ = true;
bool inpSimOnce_ = false;
bool inpDrawSky_ = true;
bool inpWireframe_ = false;
bool inpDrawTrackPoints_ = false;
bool inpDrawNearbyPoints_ = false;
bool inpDrawCarProbes_ = false;

double dt_ = 0;
double gameTime_ = 0;
double physicsTime_ = 0;
double simAccum_ = 0;
uint64_t simId_ = 0;
uint64_t drawId_ = 0;

float statMaxDt_ = 0;
float statMaxSim_ = 0;
float statMaxDraw_ = 0;
int statSimRate_ = 0;
int statDrawRate_ = 0;
int statSimHitches_ = 0;
int statDrawHitches_ = 0;

float timeSinceShift_ = 0;
int lastGear_ = 0;

//=======================================================================================

#include "DemoSDL.h"
#include "DemoDiag.h"
#include "DemoTelemetry.h"

//=======================================================================================

static void initEngine(int argc, char** argv)
{
	srand(666);
	log_init(L"demo.log");

	auto ini(std::make_unique<INIReader>(L"cfg/demo.ini"));
	if (ini->ready)
	{
		fullscreen_ = ini->getInt(L"SYS", L"FULLSCREEN") != 0;
		width_ = ini->getInt(L"SYS", L"RESX");
		height_ = ini->getInt(L"SYS", L"RESY");

		int iFpsLimit = ini->getInt(L"SYS", L"FPS_LIMIT");
		drawTickRate_ = iFpsLimit > 0 ? (1.0f / (float)iFpsLimit) : 0.0f;
		swapInterval_ = tclamp(ini->getInt(L"SYS", L"SWAP_INTERVAL"), -1, 1);
		inpMouseSens_ = ini->getFloat(L"SYS", L"MOUSE_SENS");
	}

	initSDL();
	clearViewport();
	swap();

	font_.initDefault();
}

static void initInput()
{
	bool forceKeyboard = false;
	auto dinputIni(std::make_unique<INIReader>(L"cfg/dinput.ini"));
	if (dinputIni->ready)
	{
		auto inputDev = dinputIni->getString(L"STEER", L"DEVICE");
		forceKeyboard = (inputDev == L"KEYBOARD");
	}

	if (forceKeyboard)
	{
		kbControlsProvider_ = std::make_unique<KeyboardCarController>(car_.get(), sysWindow_);
		car_->controlsProvider = kbControlsProvider_.get();
	}
	else
	{
		controlsProvider_ = std::make_unique<DICarController>(sysWindow_);
		if (controlsProvider_->diWheel)
		{
			car_->controlsProvider = controlsProvider_.get();
		}
		else
		{
			kbControlsProvider_ = std::make_unique<KeyboardCarController>(car_.get(), sysWindow_);
			car_->controlsProvider = kbControlsProvider_.get();
		}
	}

	CarControlsInput input;
	CarControls controls;

	for (int i = 0; i < 3; ++i)
		car_->controlsProvider->acquireControls(&input, &controls, (float)simTickRate_);
}

static void initDemo(int argc, char** argv)
{
	std::wstring basePath = L"";
	std::wstring trackName = L"ek_akina";
	std::wstring carModel = L"ks_toyota_ae86_drift";

	auto ini(std::make_unique<INIReader>(L"cfg/demo.ini"));
	if (ini->ready)
	{
		trackName = ini->getString(L"TRACK", L"NAME");
		carModel = ini->getString(L"CAR", L"MODEL");
	}

	sky_.load(10000, "content/demo", "sky", "jpg");

	sim_ = std::make_shared<Simulator>();
	sim_->init(basePath);

	track_ = sim_->initTrack(trackName);
	car_ = sim_->initCar(track_, carModel);
	//carB_ = sim_->initCar(track_, carModel);

	const float frontRayLength = track_->fatPointsHash.cellSize;
	car_->probes.push_back(ray3f(vec3f(0, 0, 0), vec3f(0, 0, 1), frontRayLength)); // forward
	car_->probes.push_back(ray3f(vec3f(0, 0, 0), vec3f(1, 0, 0), 10)); // left
	car_->probes.push_back(ray3f(vec3f(0, 0, 0), vec3f(-1, 0, 0), 10)); // right

	audioRenderer_ = std::make_unique<FmodAudioRenderer>(car_.get(), basePath, carModel);
	car_->audioRenderer = audioRenderer_.get();

	trackAvatar_.init(track_.get());
	carAvatar_.init(car_.get());

	// CAR TUNE

	int tyreCompound = 1;
	auto tyresIni(std::make_unique<INIReader>(car_->carDataPath + L"tyres.ini"));
	if (ini->ready)
	{
		tyresIni->tryGetInt(L"COMPOUND_DEFAULT", L"INDEX", tyreCompound);
	}

	if (ini->ready)
	{
		car_->autoClutch->useAutoOnStart = (ini->getInt(L"ASSISTS", L"AUTO_CLUTCH") != 0);
		car_->autoClutch->useAutoOnChange = (ini->getInt(L"ASSISTS", L"AUTO_CLUTCH") != 0);
		car_->autoShift->isActive = (ini->getInt(L"ASSISTS", L"AUTO_SHIFT") != 0);
		car_->autoBlip->isActive = (ini->getInt(L"ASSISTS", L"AUTO_BLIP") != 0);

		float diffPowerRamp = (float)car_->drivetrain->diffPowerRamp;
		float diffCoastRamp = (float)car_->drivetrain->diffCoastRamp;

		ini->tryGetInt(L"CAR_TUNE", L"TYRE_COMPOUND", tyreCompound);
		ini->tryGetFloat(L"CAR_TUNE", L"DIFF_POWER", diffPowerRamp);
		ini->tryGetFloat(L"CAR_TUNE", L"DIFF_COAST", diffCoastRamp);
		ini->tryGetFloat(L"CAR_TUNE", L"BRAKE_POWER", car_->brakeSystem->brakePowerMultiplier);
		ini->tryGetFloat(L"CAR_TUNE", L"BRAKE_FRONT_BIAS", car_->brakeSystem->frontBias);

		car_->drivetrain->diffPowerRamp = diffPowerRamp;
		car_->drivetrain->diffCoastRamp = diffCoastRamp;
	}

	for (int i = 0; i < 4; ++i)
	{
		car_->tyres[i]->setCompound(tyreCompound);
	}

	#if 0
	((SuspensionStrut*)car_->suspensions[0])->damper.reboundSlow = 7500;
	((SuspensionStrut*)car_->suspensions[1])->damper.reboundSlow = 7500;
	((SuspensionAxle*)car_->suspensions[2])->damper.reboundSlow = 7000;
	((SuspensionAxle*)car_->suspensions[2])->damper.bumpSlow = 5600;
	((SuspensionAxle*)car_->suspensions[3])->damper.reboundSlow = 7000;
	((SuspensionAxle*)car_->suspensions[3])->damper.bumpSlow = 5600;
	#endif

	// TELEPORT

	if (track_->pits.size() > 0)
		pitPos_ = track_->pits[0];
	car_->teleport(pitPos_);
	camPos_ = glm::vec3(pitPos_.M41, pitPos_.M42, pitPos_.M43);
	
	auto pitB = pitPos_;
	if (track_->pits.size() > 1)
		pitB = track_->pits[1];
	else
		pitB.M41 += 3;

	if (carB_)
		carB_->teleport(pitB);
}

static void processInput(float dt)
{
	if (asyncKeydown(SDL_SCANCODE_ESCAPE)) { exitFlag_ = true; }

	if (asyncKeydown(SDL_SCANCODE_X)) { inpSimOnce_ = true; }
	if (asyncKeydown(SDL_SCANCODE_C)) { inpSimFlag_ = !inpSimFlag_; }
	if (asyncKeydown(SDL_SCANCODE_T)) { car_->teleport(pitPos_); }

	if (asyncKeydown(SDL_SCANCODE_N)) { inpDrawSky_ = !inpDrawSky_; }
	if (asyncKeydown(SDL_SCANCODE_B)) { inpWireframe_ = !inpWireframe_; }
	if (asyncKeydown(SDL_SCANCODE_P)) { inpDrawTrackPoints_ = !inpDrawTrackPoints_; }
	if (asyncKeydown(SDL_SCANCODE_O)) { inpDrawNearbyPoints_ = !inpDrawNearbyPoints_; }
	if (asyncKeydown(SDL_SCANCODE_I)) { inpDrawCarProbes_ = !inpDrawCarProbes_; }

	if (asyncKeydown(SDL_SCANCODE_V))
	{
		if (++inpCamMode_ >= (int)ECamMode::COUNT)
			inpCamMode_ = 0;
		if (kbControlsProvider_)
			kbControlsProvider_->keyboardEnabled = (inpCamMode_ != (int)ECamMode::Free);
	}

	inpMove_[0] = getkeyf(SDL_SCANCODE_W) + getkeyf(SDL_SCANCODE_S) * -1.0f;
	inpMove_[2] = getkeyf(SDL_SCANCODE_D) + getkeyf(SDL_SCANCODE_A) * -1.0f;

	inpMoveSpeedScale_ = 1;
	if (getkey(SDL_SCANCODE_LSHIFT)) { inpMoveSpeedScale_ = 5; }
	else if (getkey(SDL_SCANCODE_LALT)) { inpMoveSpeedScale_ = 10; }

	if (keys_[SDL_SCANCODE_PAGEUP]) eyeOff_.y += eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_PAGEDOWN]) eyeOff_.y -= eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_4]) eyeOff_.x += eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_6]) eyeOff_.x -= eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_5]) eyeOff_.z -= eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_8]) eyeOff_.z += eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_7]) fov_ = tclamp(fov_ - fovTuneSpeed_ * dt, 10.0f, 100.0f);
	if (keys_[SDL_SCANCODE_KP_9]) fov_ = tclamp(fov_ + fovTuneSpeed_ * dt, 10.0f, 100.0f);
}

static void updateCamera(float dt)
{
	if (camYpr_[1] > 89.0f) camYpr_[1] = 89.0f;
	else if (camYpr_[1] < -89.0f) camYpr_[1] = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(camYpr_[0])) * cos(glm::radians(camYpr_[1]));
	direction.y = sin(glm::radians(camYpr_[1]));
	direction.z = sin(glm::radians(camYpr_[0])) * cos(glm::radians(camYpr_[1]));

	glm::vec3 world_up(0, 1, 0);
	camFront_ = glm::normalize(direction);
	camRight_ = glm::normalize(glm::cross(camFront_, world_up));
	camUp_ = glm::normalize(glm::cross(camRight_, camFront_));

	camPos_ += camFront_ * inpMove_[0] * inpMoveSpeed_ * inpMoveSpeedScale_ * (float)dt;
	camPos_ += camRight_ * inpMove_[2] * inpMoveSpeed_ * inpMoveSpeedScale_ * (float)dt;

	camView_ = glm::lookAt(camPos_, camPos_ + camFront_, camUp_);
}

static void render(float dt)
{
	clearViewport();
	begin3d();

	if (inpDrawSky_)
		sky_.draw();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	trackAvatar_.setLight(v(camPos_), v(camFront_));
	trackAvatar_.drawOrigin();
	trackAvatar_.drawTrack(inpWireframe_);

	if (inpDrawTrackPoints_)
		trackAvatar_.drawTrackPoints();

	if (inpDrawNearbyPoints_)
	{
		trackAvatar_.drawNearbyPoints(v(camPos_));

		float obstacleDist = track_->rayCastTrackBounds(v(camPos_) + vec3f(0, -0.5f, 0), v(camFront_));
		if (obstacleDist > 0.0f)
		{
			auto interPos = camPos_ + camFront_ * obstacleDist;

			TrackRayCastHit hit;
			if (track_->rayCast(v(interPos), vec3f(0, -1, 0), 10, hit))
			{
				glBegin(GL_LINES);
					glColor3f(1.0f, 0.0f, 0.0f);
					glVertex3fv(&interPos.x);
					glVertex3fv(&hit.pos.x);
				glEnd();
			}

			glPointSize(6);
			glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv(&interPos.x);
			glEnd();
		}
	}

	bool drawBody = (inpCamMode_ != (int)ECamMode::Eye);
	carAvatar_.draw(drawBody, inpDrawCarProbes_);

	if (carB_)
		carAvatar_.drawInstance(carB_.get(), true, true);

	glDisable(GL_DEPTH_TEST);

	if (!sim_->collisions.empty())
	{
		glPointSize(3);
		glBegin(GL_POINTS);
		for (auto& c : sim_->collisions)
		{
			glColor3f(1.0f, 1.0f, 0.0f);
			glVertex3fv(&c.pos.x);
		}
		glEnd();
		sim_->collisions.clear();
	}

	// lookat rayhit point
	if (inpCamMode_ == (int)ECamMode::Free)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glPointSize(3);
		glBegin(GL_POINTS);
		{
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3fv(&lookatPos_.x);
		}
		glEnd();
	}

	renderText();
	swap();
}

int main(int argc, char** argv)
{
	initEngine(argc, argv);
	initDemo(argc, argv);
	initCharts();

	if (recordTelemetry_)
		_telemetry.reserve(10000);

	double prevTime = 0;
	double drawAccum = 0;
	double statAccum = 0;
	double telemAccum = 0;
	double telemRate = 1 / 100.0f;
	float maxDt = 0;
	float maxSim = 0;
	float maxDraw = 0;
	int simCount = 0;
	int drawCount = 0;
	
	// flicker fix
	int skipFrames = 3;
	bool isFirstFrameDone = false;

	typedef std::chrono::high_resolution_clock clock;
	auto tick0 = clock::now();

	while (!exitFlag_)
	{
		auto tick1 = clock::now();
		gameTime_ = std::chrono::duration_cast<std::chrono::microseconds>(tick1 - tick0).count() * 1e-6;
		const float gameTime = (float)gameTime_;

		const double dt = gameTime_ - prevTime;
		prevTime = gameTime_;
		dt_ = dt;
		maxDt = tmax(maxDt, (float)dt * 1e3f);

		processEvents();

		car_->lockControls = (inpCamMode_ == (int)ECamMode::Free);

		// SIMULATE

		bool simStepDone = false;
		auto sim0 = clock::now();
		simAccum_ += dt;
		while (simAccum_ >= simTickRate_)
		{
			simAccum_ -= simTickRate_;
			if (simAccum_ > simTickRate_ * 10.0)
			{
				simAccum_ = 0;
				++statSimHitches_;
				//log_printf(L"RESET: simAccum");
			}

			if (inpSimOnce_ || inpSimFlag_)
			{
				inpSimOnce_ = false;

				sim_->step((float)simTickRate_, physicsTime_, gameTime_);

				timeSinceShift_ += (float)simTickRate_;
				if (lastGear_ != car_->drivetrain->currentGear && car_->drivetrain->currentGear > 1)
				{
					lastGear_ = car_->drivetrain->currentGear;
					timeSinceShift_ = 0;
				}

				for (int i = 0; i < 4; ++i)
					serSA_[i].add_value(gameTime, car_->tyres[i]->status.slipAngleRAD * 57.295779513082320876798154814105f);

				serFF_.add_value(gameTime, car_->lastFF);

				physicsTime_ += simTickRate_;
				simId_++;
				simCount++;
				simStepDone = true;
			}
		}
		auto sim1 = clock::now();
		const float simMillis = std::chrono::duration_cast<std::chrono::microseconds>(sim1 - sim0).count() * 1e-3f;
		maxSim = tmax(maxSim, simMillis);
		if (simStepDone)
			serSim_.add_value(gameTime, simMillis);

		// TELEMETRY

		if (recordTelemetry_)
		{
			telemAccum += dt;
			if (telemAccum >= telemRate)
			{
				telemAccum -= telemRate;
				if (telemAccum >= telemRate)
					telemAccum = 0;
				record_telemetry();
			}
		}

		// DRAW

		float drawDt = 0;
		bool redraw = false;

		if (drawTickRate_ <= 0.0f)
		{
			drawDt = (float)dt;
			redraw = true;
		}
		else
		{
			drawAccum += dt;
			if (drawAccum >= drawTickRate_)
			{
				drawAccum -= drawTickRate_;
				if (drawAccum > drawTickRate_ * 10.0)
				{
					drawAccum = 0;
					++statDrawHitches_;
					//log_printf(L"RESET: drawAccum");
				}

				drawDt = (float)drawTickRate_;
				redraw = true;
			}
		}

		if (redraw)
		{
			processInput(drawDt);

			if (inpCamMode_ == (int)ECamMode::Free)
			{
				updateCamera(drawDt);
			}
			else if (inpCamMode_ == (int)ECamMode::Rear)
			{
				auto eyePos = car_->body->localToWorld(vec3f(0, 2, -5));
				auto targPos = car_->body->localToWorld(vec3f(0, 0, 0));

				camPos_ = v(eyePos);
				camView_ = glm::lookAt(camPos_, v(targPos), glm::vec3(0, 1, 0));
			}
			else if (inpCamMode_ == (int)ECamMode::Eye)
			{
				auto bodyR = car_->body->getWorldMatrix(0).getRotator();
				auto bodyFront = (vec3f(0, 0, 1) * bodyR).get_norm();
				auto bodyUp = (vec3f(0, 1, 0) * bodyR).get_norm();
				auto eyePos = car_->body->localToWorld(eyeOff_);

				camFront_ = v(bodyFront);
				camUp_ = v(bodyUp);
				camPos_ = v(eyePos);
				camView_ = glm::lookAt(camPos_, camPos_ + camFront_, camUp_);
			}

			TrackRayCastHit hit;
			track_->rayCast(v(camPos_), v(camFront_), 1000.0f, hit);
			lookatSurf_ = hit.surface;
			lookatPos_ = hit.pos;

			auto draw0 = clock::now();
			{
				render(drawDt);
				drawId_++;
				drawCount++;
			}
			auto draw1 = clock::now();
			const float drawMillis = std::chrono::duration_cast<std::chrono::microseconds>(draw1 - draw0).count() * 1e-3f;
			maxDraw = tmax(maxDraw, drawMillis);

			serDraw_.add_value(gameTime, drawMillis);
			serSteer_.add_value(gameTime, car_->controls.steer);
			serClutch_.add_value(gameTime, car_->controls.clutch);
			serBrake_.add_value(gameTime, car_->controls.brake);
			serGas_.add_value(gameTime, car_->controls.gas);

			if (!isFirstFrameDone && skipFrames == 0)
			{
				isFirstFrameDone = true;

				if (fullscreen_)
				{
					SDL_ShowWindow(appWindow_);
					SDL_SetRelativeMouseMode(SDL_TRUE);
				}

				initInput();
			}

			if (skipFrames > 0) --skipFrames;
		}

		// STATS

		statAccum += dt;
		if (statAccum >= 1.0)
		{
			statAccum -= 1.0;
			if (statAccum > 1.0)
				statAccum = 0;
			statDrawRate_ = drawCount; drawCount = 0;
			statSimRate_ = simCount; simCount = 0;
			statMaxDt_ = maxDt; maxDt = 0;
			statMaxSim_ = maxSim; maxSim = 0;
			statMaxDraw_ = maxDraw; maxDraw = 0;
		}
	}

	if (recordTelemetry_)
	{
		dump_telemetry();
	}

	car_.reset();
	track_.reset();
	sim_.reset();

	shutSDL();

	return 0;
}
