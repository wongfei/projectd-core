#include "PlaygrounD.h"
#include "Audio/FmodContext.h"

namespace D {

//=======================================================================================

PlaygrounD::PlaygrounD() { TRACE_CTOR(PlaygrounD); }
PlaygrounD::~PlaygrounD() { TRACE_DTOR(PlaygrounD); closeWindow(); }

void PlaygrounD::runDemo()
{
	init("", false);
	loadDemo();

	while (!exitFlag_)
	{
		tick();
	}

	log_printf(L"SHUTDOWN..");

	sim_.reset();
	controlsProvider_.reset();

	closeWindow();
}

void PlaygrounD::init(const std::string& basePath, bool loadedByPython)
{
	initEngine(basePath, loadedByPython);
	initCharts();
	tick0_ = clock::now();
}

void PlaygrounD::tick()
{
	{
		const auto tick1 = clock::now();
		gameTime_ = std::chrono::duration_cast<std::chrono::microseconds>(tick1 - tick0_).count() * 1e-6;
		const float gameTime = (float)gameTime_;

		const double dt = gameTime_ - prevTime;
		prevTime = gameTime_;
		dt_ = dt;
		maxDt = tmax(maxDt, (float)dt * 1e3f);
		bool hitchFlag = false;

		if (car_)
			car_->lockControls = (inpCamMode_ == (int)ECamMode::Free);

		//===========================================================================
		// SIMULATE

		if (simTime_ + simTickRate_ * hitchRate < gameTime_)
		{
			++statSimHitches_;
			lastHitchTime_ = gameTime_;
			simTime_ = gameTime_;
			hitchFlag = true;
		}

		bool simStepDone = false;
		const auto sim0 = clock::now();
		while (simTime_ + simTickRate_ <= gameTime_)
		{
			simTime_ += simTickRate_;

			if (sim_ && (simStepOnce_ || simEnabled_))
			{
				simStepOnce_ = false;

				sim_->step((float)simTickRate_, physicsTime_, gameTime_);

				if (car_)
				{
					timeSinceShift_ += (float)simTickRate_;
					if (lastGear_ != car_->drivetrain->currentGear && car_->drivetrain->currentGear > 1)
					{
						lastGear_ = car_->drivetrain->currentGear;
						timeSinceShift_ = 0;
					}

					for (int i = 0; i < 4; ++i)
						serSA_[i].add_value(gameTime, car_->tyres[i]->status.slipAngleRAD * 57.295779513082320876798154814105f);

					serFF_.add_value(gameTime, car_->lastFF);
				}

				physicsTime_ += simTickRate_;
				simId_++;
				simCount++;
				simStepDone = true;
			}
		}
		const auto sim1 = clock::now();
		const float simMillis = std::chrono::duration_cast<std::chrono::microseconds>(sim1 - sim0).count() * 1e-3f;
		maxSim = tmax(maxSim, simMillis);
		if (simStepDone)
			serSim_.add_value(gameTime, simMillis);

		//===========================================================================
		// DRAW

		float drawDt = 0;
		bool redraw = false;

		if (drawTickRate_ <= 0.0)
		{
			drawDt = (float)dt;
			redraw = true;
		}
		else
		{
			if (drawTime_ + drawTickRate_ * hitchRate < gameTime_)
			{
				++statDrawHitches_;
				lastHitchTime_ = gameTime_;
				drawTime_ = gameTime_;
				hitchFlag = true;
			}

			if (drawTime_ + drawTickRate_ <= gameTime_)
			{
				drawTime_ += drawTickRate_;

				drawDt = (float)drawTickRate_;
				redraw = true;
			}
		}

		if (redraw)
		{
			processEvents();

			processInput(drawDt);

			updateCamera(drawDt);

			const auto draw0 = clock::now();
			{
				render();
				drawId_++;
				drawCount++;
			}
			const auto draw1 = clock::now();
			const float drawMillis = std::chrono::duration_cast<std::chrono::microseconds>(draw1 - draw0).count() * 1e-3f;
			maxDraw = tmax(maxDraw, drawMillis);

			const auto swap0 = clock::now();
			swap();
			const auto swap1 = clock::now();
			const float swapMillis = std::chrono::duration_cast<std::chrono::microseconds>(swap1 - swap0).count() * 1e-3f;

			serDraw_.add_value(gameTime, drawMillis);
			serSwap_.add_value(gameTime, swapMillis);

			if (car_)
			{
				serSteer_.add_value(gameTime, car_->controls.steer);
				serClutch_.add_value(gameTime, car_->controls.clutch);
				serBrake_.add_value(gameTime, car_->controls.brake);
				serGas_.add_value(gameTime, car_->controls.gas);
			}

			if (sim_)
				sim_->dbgCollisions.clear();

			if (!isFirstFrameDone && skipFrames == 0)
			{
				isFirstFrameDone = true;

				if (fullscreen_)
				{
					SDL_ShowWindow(appWindow_);
					SDL_SetRelativeMouseMode(SDL_TRUE);
				}
			}

			if (skipFrames > 0) --skipFrames;
		}

		//===========================================================================
		// FMOD

		if (fmodContext_)
			fmodContext_->update();

		//===========================================================================
		// STATS

		if (statTime_ + 1.0 * hitchRate < gameTime_)
		{
			statTime_ = gameTime_;
		}

		if (statTime_ + 1.0 <= gameTime_)
		{
			statTime_ += 1.0;
			statDrawRate_ = drawCount; drawCount = 0;
			statSimRate_ = simCount; simCount = 0;
			statMaxDt_ = maxDt; maxDt = 0;
			statMaxSim_ = maxSim; maxSim = 0;
			statMaxDraw_ = maxDraw; maxDraw = 0;
		}

		//===========================================================================
		// SLEEP

		if (!hitchFlag)
		{
			#if 0
				::Sleep(0);
			#else
				// estimate time to next frame
				const auto tick2 = clock::now();
				const double curTime = std::chrono::duration_cast<std::chrono::microseconds>(tick2 - tick0_).count() * 1e-6;
				const double simIdle = tmax(0.0, ((simTime_ + simTickRate_) - curTime));
				const double drawIdle = tmax(0.0, ((drawTime_ + drawTickRate_) - curTime));
				const int idleMs = floorToInt((float)(tmin(simIdle, drawIdle) * 1000.0));
				serIdle_.add_value(gameTime, (float)idleMs);
				::Sleep((DWORD)idleMs);
			#endif
		}
	}
}

//=======================================================================================
// INIT
//=======================================================================================

void PlaygrounD::initEngine(const std::string& basePath, bool loadedByPython)
{
	if (!loadedByPython)
		srand(666);

	auto exePath = osGetModuleFullPath();
	auto exeDir = osGetDirPath(exePath);

	if (loadedByPython)
		appDir_ = strw(basePath);
	else
		appDir_ = osCombinePath(exeDir, L"..\\");

	replace(appDir_, L'\\', L'/');
	if (!ends_with(appDir_, L'/'))
		appDir_.append(L"/");

	if (loadedByPython)
	{
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		AddDllDirectory(osCombinePath(appDir_, L"bin").c_str());
	}

	if (!loadedByPython)
	{
		const std::wstring logPath = appDir_ + L"projectd.log";
		log_init(logPath.c_str());
	}

	log_printf(L"Start PlaygrounD");
	log_printf(L"exePath: %s", exePath.c_str());
	log_printf(L"appDir: %s", appDir_.c_str());

	#if defined(DEBUG)
	INIReader::_debug = true;
	#endif

	auto ini(std::make_unique<INIReader>(appDir_ + L"cfg/demo.ini"));
	if (ini->ready)
	{
		posx_ = ini->getInt(L"SYS", L"POSX");
		posy_ = ini->getInt(L"SYS", L"POSY");
		width_ = ini->getInt(L"SYS", L"RESX");
		height_ = ini->getInt(L"SYS", L"RESY");
		fullscreen_ = ini->getInt(L"SYS", L"FULLSCREEN") != 0;

		int iFpsLimit = ini->getInt(L"SYS", L"FPS_LIMIT");
		drawTickRate_ = iFpsLimit > 0 ? (1.0 / (double)iFpsLimit) : 0.0;
		swapInterval_ = tclamp(ini->getInt(L"SYS", L"SWAP_INTERVAL"), -1, 1);
		mouseSens_ = ini->getFloat(L"SYS", L"MOUSE_SENS");
	}

	initWindow();

	auto rc = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	log_printf(L"IMG_Init: %d", rc);

	font_.initDefault(appDir_);

	fmodContext_.reset(new FmodContext());
	fmodContext_->init(stra(appDir_));

	clearViewport();
	swap();
}

void PlaygrounD::loadDemo()
{
	log_printf(L"loadDemo");

	std::wstring trackName = L"ek_akina";
	std::wstring carModel = L"ks_toyota_ae86_drift";

	auto ini(std::make_unique<INIReader>(appDir_ + L"cfg/demo.ini"));
	if (ini->ready)
	{
		trackName = ini->getString(L"TRACK", L"NAME");
		carModel = ini->getString(L"CAR", L"MODEL");
	}

	auto sim = std::make_shared<Simulator>();
	sim->init(appDir_);
	setSimulator(sim);

	auto track = sim->loadTrack(trackName);

	const int maxCars = tmin(sim->maxCars, (int)track->pits.size());
	if (maxCars <= 0)
		return;

	for (int i = 0; i < maxCars; ++i)
	{
		auto* car = sim->addCar(carModel);
		car->teleport(track->pits[i]);
	}

	pitPos_ = track->pits[0];
	camPos_ = glm::vec3(pitPos_.M41, pitPos_.M42, pitPos_.M43);

	setActiveCar(0, true, true);

	// CAR_TUNE

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

		std::wstring eyeOffStr;
		if (ini->tryGetString(L"CAR_TUNE", L"EYE_OFFSET", eyeOffStr))
		{
			auto parts = split(eyeOffStr, L",");
			if (parts.size() == 3)
				eyeOff_ = vec3f(std::stof(parts[0]), std::stof(parts[1]), std::stof(parts[2]));
		}
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
}

void PlaygrounD::initCarController()
{
	log_printf(L"initCarController");

	bool forceKeyboard = false;
	auto dinputIni(std::make_unique<INIReader>(appDir_ + L"cfg/dinput.ini"));
	if (dinputIni->ready)
	{
		auto inputDev = dinputIni->getString(L"STEER", L"DEVICE");
		forceKeyboard = (inputDev == L"KEYBOARD");
	}

	if (forceKeyboard)
	{
		controlsProvider_ = std::make_shared<KeyboardCarController>(appDir_, sysWindow_);
	}
	else
	{
		auto controlsProvider = std::make_shared<DICarController>(appDir_, sysWindow_);
		if (controlsProvider->diWheel)
		{
			controlsProvider_ = controlsProvider;
		}
		else
		{
			controlsProvider_ = std::make_shared<KeyboardCarController>(appDir_, sysWindow_);
		}
	}

	CarControlsInput input;
	CarControls controls;

	for (int i = 0; i < 3; ++i)
		controlsProvider_->acquireControls(&input, &controls, (float)simTickRate_);
}

void PlaygrounD::setSimulator(SimulatorPtr newSim)
{
	log_printf(L"setSimulator %p -> %p", sim_.get(), newSim.get());

	if (sim_ == newSim)
		return;

	sim_ = newSim;
	if (!sim_)
		return;

	if (!sim_->avatar)
		sim_->avatar.reset(new GLSimulator(sim_.get()));

	track_ = sim_->track.get();

	if (track_ && !track_->pits.empty())
	{
		pitPos_ = track_->pits[0];
		camPos_ = glm::vec3(pitPos_.M41, pitPos_.M42, pitPos_.M43);
	}
}

void PlaygrounD::setActiveCar(int carId, bool takeControls, bool enableSound)
{
	log_printf(L"setActiveCar id=%d takeControls=%d enableSound=%d", carId, (int)takeControls, (int)enableSound);

	if (sim_ && carId >= 0 && carId < (int)sim_->cars.size())
	{
		auto* old = car_;
		car_ = sim_->cars[carId];
		inpCamMode_ = (int)ECamMode::Eye;

		if (old)
		{
			old->lockControls = true;
			old->controlsProvider.reset();

			if (old->avatar)
				((GLCar*)old->avatar.get())->drawBody = true;

			old->audioRenderer.reset();
		}

		if (takeControls && enableCarController_)
		{
			if (!controlsProvider_)
				initCarController();

			controlsProvider_->setCar(car_);
			car_->controlsProvider = controlsProvider_;
			car_->lockControls = false;
		}

		if (enableSound && enableCarSound_)
		{
			if (!car_->audioRenderer)
				car_->audioRenderer = std::make_shared<FmodAudioRenderer>(fmodContext_, stra(appDir_), car_);
		}
	}
}

//=======================================================================================
// UPDATE
//=======================================================================================

void PlaygrounD::processInput(float dt)
{
	if (asyncKeydown(SDL_SCANCODE_ESCAPE)) { exitFlag_ = true; }

	if (asyncKeydown(SDL_SCANCODE_C)) { simEnabled_ = !simEnabled_; }
	if (asyncKeydown(SDL_SCANCODE_X)) { simStepOnce_ = true; }
	if (asyncKeydown(SDL_SCANCODE_T)) { if (car_) car_->teleport(pitPos_); }

	if (asyncKeydown(SDL_SCANCODE_M)) { drawWorld_ = !drawWorld_; }
	if (asyncKeydown(SDL_SCANCODE_N)) { drawSky_ = !drawSky_; }
	if (asyncKeydown(SDL_SCANCODE_B)) { wireframeMode_ = !wireframeMode_; }
	if (asyncKeydown(SDL_SCANCODE_P)) { drawTrackPoints_ = !drawTrackPoints_; }
	if (asyncKeydown(SDL_SCANCODE_O)) { drawNearbyPoints_ = !drawNearbyPoints_; }
	if (asyncKeydown(SDL_SCANCODE_I)) { drawCarProbes_ = !drawCarProbes_; }

	if (sim_)
	{
		const int n = (int)sim_->cars.size();
		if (asyncKeydown(SDL_SCANCODE_PAGEUP)) { ++activeCarId_; if (activeCarId_ >= n) { activeCarId_ = 0; }; setActiveCar(activeCarId_, true, true); }
		if (asyncKeydown(SDL_SCANCODE_PAGEDOWN)) { --activeCarId_; if (activeCarId_ < 0) { activeCarId_ = (n ? n - 1 : 0); }; setActiveCar(activeCarId_, true, true); }
	}

	if (asyncKeydown(SDL_SCANCODE_V))
	{
		if (++inpCamMode_ >= (int)ECamMode::COUNT)
			inpCamMode_ = 0;
	}

	camMove_[0] = getkeyf(SDL_SCANCODE_W) + getkeyf(SDL_SCANCODE_S) * -1.0f;
	camMove_[2] = getkeyf(SDL_SCANCODE_D) + getkeyf(SDL_SCANCODE_A) * -1.0f;

	moveSpeedScale_ = 1;
	if (getkey(SDL_SCANCODE_LSHIFT)) { moveSpeedScale_ = 5; }
	else if (getkey(SDL_SCANCODE_LALT)) { moveSpeedScale_ = 10; }

	if (keys_[SDL_SCANCODE_PAGEUP]) eyeOff_.y += eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_PAGEDOWN]) eyeOff_.y -= eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_4]) eyeOff_.x += eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_6]) eyeOff_.x -= eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_5]) eyeOff_.z -= eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_8]) eyeOff_.z += eyeTuneSpeed_ * dt;
	if (keys_[SDL_SCANCODE_KP_7]) fov_ = tclamp(fov_ - fovTuneSpeed_ * dt, 10.0f, 100.0f);
	if (keys_[SDL_SCANCODE_KP_9]) fov_ = tclamp(fov_ + fovTuneSpeed_ * dt, 10.0f, 100.0f);
}

void PlaygrounD::updateCamera(float dt)
{
	if (inpCamMode_ == (int)ECamMode::Free || !car_)
	{
		freeFlyCamera(dt);
	}

	if (car_)
	{
		if (inpCamMode_ == (int)ECamMode::Rear)
		{
			const auto eyePos = car_->body->localToWorld(vec3f(0, 2, -5));
			const auto targPos = car_->body->localToWorld(vec3f(0, 0, 0));

			camPos_ = v(eyePos);
			camView_ = glm::lookAt(camPos_, v(targPos), glm::vec3(0, 1, 0));
		}
		else if (inpCamMode_ == (int)ECamMode::Eye)
		{
			const auto bodyR = car_->body->getWorldMatrix(0).getRotator();
			const auto bodyFront = (vec3f(0, 0, 1) * bodyR).get_norm();
			const auto bodyUp = (vec3f(0, 1, 0) * bodyR).get_norm();
			const auto eyePos = car_->body->localToWorld(eyeOff_);

			camFront_ = v(bodyFront);
			camUp_ = v(bodyUp);
			camPos_ = v(eyePos);
			camView_ = glm::lookAt(camPos_, camPos_ + camFront_, camUp_);
		}
	}
}

void PlaygrounD::freeFlyCamera(float dt)
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

	camPos_ += camFront_ * camMove_[0] * moveSpeed_ * moveSpeedScale_ * (float)dt;
	camPos_ += camRight_ * camMove_[2] * moveSpeed_ * moveSpeedScale_ * (float)dt;

	#pragma warning(push)
	#pragma warning(disable: 4127)
	camView_ = glm::lookAt(camPos_, camPos_ + camFront_, camUp_);
	#pragma warning(pop)
}

//=======================================================================================
// RENDER
//=======================================================================================

void PlaygrounD::render()
{
	#if 1
		if (track_)
		{
			TrackRayCastHit hit;
			track_->rayCast(v(camPos_), v(camFront_), 1000.0f, hit);
			lookatSurf_ = hit.surface;
			lookatHit_ = hit.pos;
		}
	#endif

	clearViewport();

	if (drawWorld_)
		renderWorld();

	if (drawUI_)
		renderUI();
}

void PlaygrounD::renderWorld()
{
	projPerspective();

	if (sim_ && sim_->avatar)
	{
		auto simAvatar = (GLSimulator*)sim_->avatar.get();
		simAvatar->drawSkybox = drawSky_;

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		if (sim_->track)
		{
			auto trackAvatar = (GLTrack*)sim_->track->avatar.get();
			if (trackAvatar)
			{
				trackAvatar->setCamera(v(camPos_), v(camFront_));
				trackAvatar->wireframe = wireframeMode_;
				trackAvatar->drawFatPoints = drawTrackPoints_;
				trackAvatar->drawNearbyPoints = drawNearbyPoints_;
			}
		}

		if (car_ && car_->avatar)
		{
			auto carAvatar = (GLCar*)car_->avatar.get();
			carAvatar->drawBody = (inpCamMode_ != (int)ECamMode::Eye);
			carAvatar->drawProbes = drawCarProbes_;
		}

		sim_->avatar->draw();

		glDisable(GL_DEPTH_TEST);
	}

	if (sim_ && !sim_->dbgCollisions.empty())
	{
		glPointSize(3);
		glBegin(GL_POINTS);
		for (auto& c : sim_->dbgCollisions)
		{
			glColor3f(1.0f, 1.0f, 0.0f);
			glVertex3fv(&c.pos.x);
		}
		glEnd();
	}

	if (inpCamMode_ == (int)ECamMode::Free)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glPointSize(3);
		glBegin(GL_POINTS);
		{
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3fv(&lookatHit_.x);
		}
		glEnd();
	}
}

void PlaygrounD::renderUI()
{
	projOrtho();
	glEnable(GL_TEXTURE_2D);

	renderCharts();
	renderStats();

	glDisable(GL_TEXTURE_2D);
}

}
