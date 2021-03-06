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

int width_ = 1920;
int height_ = 1080;
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
bool inpSimOnce_ = false;
bool inpSimFlag_ = false;
bool inpDrawSky_ = true;
bool inpWireWalls_ = true;

double dt_ = 0;
double gameTime_ = 0;
double physicsTime_ = 0;
uint64_t simId_ = 0;
uint64_t drawId_ = 0;

float statMaxDt_ = 0;
float statMaxSim_ = 0;
float statMaxDraw_ = 0;
int statSimRate_ = 0;
int statDrawRate_ = 0;

float timeSinceShift_ = 0;
int lastGear_ = 0;

//=======================================================================================

#include "DemoSDL.h"
#include "DemoDiag.h"

static void initDemo(int argc, char** argv)
{
	std::wstring basePath = L"";
	std::wstring trackName = L"akina";
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
	carB_ = sim_->initCar(track_, carModel);

	audioRenderer_ = std::make_unique<FmodAudioRenderer>(car_.get(), basePath, carModel);
	car_->audioRenderer = audioRenderer_.get();

	trackAvatar_.init(track_.get());
	carAvatar_.init(car_.get());

	// CAR TUNE

	if (ini->ready)
	{
		car_->drivetrain->diffPowerRamp = ini->getFloat(L"CAR_TUNE", L"DIFF_POWER");
		car_->drivetrain->diffCoastRamp = ini->getFloat(L"CAR_TUNE", L"DIFF_COAST");
		car_->brakeSystem->brakePowerMultiplier = ini->getFloat(L"CAR_TUNE", L"BRAKE_POWER");
		car_->brakeSystem->frontBias = ini->getFloat(L"CAR_TUNE", L"BRAKE_FRONT_BIAS");

		int iCompound = ini->getInt(L"CAR_TUNE", L"TYRE_COMPOUND");
		for (int i = 0; i < 4; ++i)
			car_->tyres[i]->setCompound(iCompound);
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
	carB_->teleport(pitB);
}

static void processInput(float dt)
{
	if (asyncKeydown(SDL_SCANCODE_ESCAPE)) { exitFlag_ = true; }

	if (asyncKeydown(SDL_SCANCODE_X)) { inpSimOnce_ = true; }
	if (asyncKeydown(SDL_SCANCODE_C)) { inpSimFlag_ = !inpSimFlag_; }
	if (asyncKeydown(SDL_SCANCODE_T)) { car_->teleport(pitPos_); }

	if (asyncKeydown(SDL_SCANCODE_N)) { inpDrawSky_ = !inpDrawSky_; }
	if (asyncKeydown(SDL_SCANCODE_B)) { inpWireWalls_ = !inpWireWalls_; }

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

	trackAvatar_.drawOrigin();
	trackAvatar_.drawSurfaces(v(camPos_), v(camFront_));
	if (inpWireWalls_)
		trackAvatar_.drawWalls();

	bool drawBody = (inpCamMode_ != (int)ECamMode::Eye);
	carAvatar_.draw(drawBody);
	carAvatar_.drawInstance(carB_.get(), true);

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

	renderText();
	swap();
}

int main(int argc, char** argv)
{
	srand(666);
	log_init(L"demo.log");

	auto ini(std::make_unique<INIReader>(L"cfg/demo.ini"));
	if (ini->ready)
	{
		int iFpsLimit = ini->getInt(L"SYS", L"FPS_LIMIT");
		drawTickRate_ = iFpsLimit > 0 ? (1.0f / (float)iFpsLimit) : 0.0f;
		swapInterval_ = tclamp(ini->getInt(L"SYS", L"SWAP_INTERVAL"), -1, 1);
		inpMouseSens_ = ini->getFloat(L"SYS", L"MOUSE_SENS");
	}

	initSDL();
	clearViewport();
	swap();
	log_printf(L"hwnd=%p", sysWindow_);

	font_.initDefault();
	initDemo(argc, argv);
	initCharts();

	double prevTime = 0;
	double simAccum = 0;
	double drawAccum = 0;
	double statAccum = 0;
	float maxDt = 0;
	float maxSim = 0;
	float maxDraw = 0;
	int simCount = 0;
	int drawCount = 0;
	bool isWindowVisible = false;

	typedef std::chrono::high_resolution_clock clock;
	auto t0 = clock::now();
	auto t1 = t0;

	while (!exitFlag_)
	{
		auto t2 = clock::now();
		gameTime_ = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t0).count() * 1e-6;
		const float gameTime = (float)gameTime_;

		const double dt = gameTime_ - prevTime;
		prevTime = gameTime_;
		dt_ = dt;
		maxDt = tmax(maxDt, (float)dt * 1e3f);

		processEvents();

		// SIMULATE

		bool simStepDone = false;
		auto sim0 = clock::now();
		simAccum += dt;
		while (simAccum >= simTickRate_)
		{
			simAccum -= simTickRate_;
			if (simAccum > simTickRate_ * 3.0)
			{
				simAccum = 0;
				log_printf(L"RESET: simAccum");
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
				if (drawAccum > drawTickRate_ * 3.0)
				{
					drawAccum = 0;
					log_printf(L"RESET: drawAccum");
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

			if (!isWindowVisible)
			{
				isWindowVisible = true;
				SDL_ShowWindow(appWindow_);

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

	car_.reset();
	track_.reset();
	sim_.reset();

	shutSDL();

	return 0;
}
