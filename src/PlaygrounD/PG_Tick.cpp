#include "PlaygrounD.h"
#include "Audio/FmodContext.h"

namespace D {

void PlaygrounD::tick()
{
	{
		const auto tickStart = clock::now();
		gameTime_ = std::chrono::duration_cast<std::chrono::microseconds>(tickStart - gameStartTicks_).count() * 1e-6;
		const float gameTime = (float)gameTime_;

		const double dt = gameTime_ - prevTime;
		prevTime = gameTime_;
		dt_ = dt;
		maxDt = tmax(maxDt, (float)dt * 1e3f);
		bool hitchFlag = false;

		const auto tickThreadId = osGetCurrentThreadId();

		//===========================================================================
		// CHANGE SIM/CAR

		if (newSimId_ != -1 && simManager_)
		{
			newSim_ = simManager_->getSim(newSimId_);
			newSimId_ = -1;
		}

		if (newSim_)
		{
			setSimulator(newSim_);
			newSim_.reset();
		}

		if (newCarId_ != -1)
		{
			setActiveCar(newCarId_);
			newCarId_ = -1;
		}

		if (car_ && car_->controlsProvider)
		{
			car_->lockControls = (inpCamMode_ == (int)ECamMode::Free);
		}

		//===========================================================================
		// SIMULATE

		simTickRate_ = simHz_ > 0 ? (1.0 / (double)simHz_) : 0.0;
		const double simDt = simTickRate_ > 0.0 ? simTickRate_ : dt;

		if (simTime_ + simDt * hitchRate < gameTime_)
		{
			++statSimHitches_;
			lastHitchTime_ = gameTime_;
			simTime_ = gameTime_;
			hitchFlag = true;
		}

		bool simStepDone = false;
		const auto sim0 = clock::now();
		while (simTime_ + simDt <= gameTime_)
		{
			simTime_ += simDt;

			if (sim_ && (simEnabled_ || simStepOnce_) && (sim_->physicsThreadId == tickThreadId))
			{
				simStepOnce_ = false;

				sim_->step((float)simDt, physicsTime_, gameTime_);

				updateSimStats((float)simDt, gameTime);

				physicsTime_ += simDt;
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

		drawTickRate_ = drawHz_ > 0 ? (1.0 / (double)drawHz_) : 0.0;

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

			// RENDER
			const auto draw0 = clock::now();
			{
				render();
				drawId_++;
				drawCount++;
			}
			const auto draw1 = clock::now();
			const float drawMillis = std::chrono::duration_cast<std::chrono::microseconds>(draw1 - draw0).count() * 1e-3f;
			maxDraw = tmax(maxDraw, drawMillis);

			// SWAP
			const auto swap0 = clock::now();
			{
				swap();
			}
			const auto swap1 = clock::now();
			const float swapMillis = std::chrono::duration_cast<std::chrono::microseconds>(swap1 - swap0).count() * 1e-3f;

			serDraw_.add_value(gameTime, drawMillis);
			serSwap_.add_value(gameTime, swapMillis);

			// show window after rendering few frames
			if (!isFirstFrameDone && skipFrames == 0)
			{
				isFirstFrameDone = true;

				if (fullscreen_)
				{
					//SDL_ShowWindow(appWindow_);
					SDL_SetRelativeMouseMode(SDL_TRUE);
				}
			}
			if (skipFrames > 0) --skipFrames;

			// clear collision accumulator after rendering it
			if (sim_)
				sim_->dbgCollisions.clear();

			DebugGL::get().clear();
		}

		//===========================================================================
		// FMOD

		if (fmodContext_)
			fmodContext_->update();

		//===========================================================================
		// STATS

		const auto tickEnd = clock::now();
		const float tickMillis = std::chrono::duration_cast<std::chrono::microseconds>(tickEnd - tickStart).count() * 1e-3f;
		maxTick = tmax(maxTick, tickMillis);

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
			statMaxTick_ = maxTick; maxTick = 0;
		}

		//===========================================================================
		// SLEEP

		if (enableSleep_)
		{
			#if 0
				::Sleep(0);
			#else
				// estimate time to next frame
				//const auto tick2 = clock::now();
				const double curTime = std::chrono::duration_cast<std::chrono::microseconds>(tickEnd - gameStartTicks_).count() * 1e-6;
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

void PlaygrounD::updateSimStats(float dt, float gameTime)
{
	if (car_)
	{
		serSteer_.add_value(gameTime, car_->controls.steer);
		serClutch_.add_value(gameTime, car_->controls.clutch);
		serBrake_.add_value(gameTime, car_->controls.brake);
		serGas_.add_value(gameTime, car_->controls.gas);

		for (int i = 0; i < 4; ++i)
			serSA_[i].add_value(gameTime, car_->tyres[i]->status.slipAngleRAD * 57.295779513082320876798154814105f);

		serFF_.add_value(gameTime, car_->lastFF);
		serReward_.add_value(gameTime, car_->scoring->totalReward);

		timeSinceShift_ += (float)dt;
		if (lastGear_ != car_->drivetrain->currentGear && car_->drivetrain->currentGear > 1)
		{
			lastGear_ = car_->drivetrain->currentGear;
			timeSinceShift_ = 0;
		}
	}
}

//=======================================================================================

void PlaygrounD::processInput(float dt)
{
	if (asyncKeydown(SDL_SCANCODE_ESCAPE)) { exitFlag_ = true; }

	if (asyncKeydown(SDL_SCANCODE_C)) { simEnabled_ = !simEnabled_; }
	if (asyncKeydown(SDL_SCANCODE_X)) { simStepOnce_ = true; }
	if (asyncKeydown(SDL_SCANCODE_T)) { if (car_) car_->teleport(pitPos_); }

	if (asyncKeydown(SDL_SCANCODE_COMMA)) { drawWorld_ = !drawWorld_; }
	//if (asyncKeydown(SDL_SCANCODE_PERIOD)) { drawSky_ = !drawSky_; }
	if (asyncKeydown(SDL_SCANCODE_SLASH)) { draw2D_ = !draw2D_; }
	if (asyncKeydown(SDL_SCANCODE_B)) { wireframeMode_ = !wireframeMode_; }

	if (asyncKeydown(SDL_SCANCODE_P)) { drawTrackPoints_ = !drawTrackPoints_; }
	if (asyncKeydown(SDL_SCANCODE_O)) { drawNearbyPoints_ = !drawNearbyPoints_; }
	if (asyncKeydown(SDL_SCANCODE_I)) { drawCarProbes_ = !drawCarProbes_; }

	if (asyncKeydown(SDL_SCANCODE_F5)) { car_->track->saveSenseiPoints(car_->unixName); }
	if (asyncKeydown(SDL_SCANCODE_F9)) { car_->track->loadSenseiPoints(car_->unixName); }

	if (simManager_)
	{
		const int n = simManager_->getSimCount();
		if (asyncKeydown(SDL_SCANCODE_MINUS)) { if (activeSimId_ > 0) { newSimId_ = activeSimId_ - 1; } }
		if (asyncKeydown(SDL_SCANCODE_EQUALS)) { if (activeSimId_ + 1 < n) { newSimId_ = activeSimId_ + 1; } }
	}

	if (sim_)
	{
		const int n = (int)sim_->cars.size();
		if (asyncKeydown(SDL_SCANCODE_LEFTBRACKET)) { if (activeCarId_ > 0) { newCarId_ = activeCarId_ - 1; } }
		if (asyncKeydown(SDL_SCANCODE_RIGHTBRACKET)) { if (activeCarId_ + 1 < n) { newCarId_ = activeCarId_ + 1; } }
	}

	if (asyncKeydown(SDL_SCANCODE_F1) && activeCarId_ != -1)
	{
		enableCarControls(true);
		enableCarSound(true);
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

//=======================================================================================

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

			// TODO: compute camYpr_
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

//=======================================================================================

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

}
