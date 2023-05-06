#include "PlaygrounD.h"
#include "Car/ScoringSystem.h"

namespace D {

void PlaygrounD::initCharts()
{
	const int ChartN = 512;
	float y0 = 0;
	float y1 = 10;
	chartDT_.init(ChartN, 64);
	serSim_.init(ChartN, y0, y1);
	serDraw_.init(ChartN, y0, y1);
	serSwap_.init(ChartN, y0, y1);
	serIdle_.init(ChartN, y0, y1);

	y0 = -1.1f;
	y1 = 1.1f;
	chartInput_.init(ChartN, 64);
	serSteer_.init(ChartN, y0, y1);
	serClutch_.init(ChartN, y0, y1);
	serBrake_.init(ChartN, y0, y1);
	serGas_.init(ChartN, y0, y1);

	y0 = -20.0f;
	y1 = 20.0f;
	chartSA_.init(ChartN, 64);
	for (int i = 0; i < 4; ++i)
		serSA_[i].init(ChartN, y0, y1);

	y0 = -1.0f;
	y1 = 1.0f;
	chartFF_.init(ChartN, 64);
	serFF_.init(ChartN, y0, y1);
}

void PlaygrounD::renderCharts()
{
	float dx = 550;
	float x = 200;
	float y = 10;

	chartDT_.clear();
	chartDT_.plot(serSim_, ChartColor(255, 165, 0, 255)); // orange
	chartDT_.plot(serDraw_, ChartColor(255, 0, 255, 255)); // magenta
	chartDT_.plot(serSwap_, ChartColor(0, 0, 255, 0));
	chartDT_.plot(serIdle_, ChartColor(0, 255, 0, 0));
	chartDT_.present(x, y, 1.0f); x += dx;

	chartInput_.clear();
	chartInput_.plot(serSteer_, ChartColor(255, 255, 0, 255));
	chartInput_.plot(serClutch_, ChartColor(0, 0, 255, 255));
	chartInput_.plot(serBrake_, ChartColor(255, 0, 0, 255));
	chartInput_.plot(serGas_, ChartColor(0, 255, 0, 255));
	chartInput_.present(x, y, 1.0f); x += dx;

	chartSA_.clear();
	chartSA_.plot(serSA_[0], ChartColor(255, 0, 0, 255));
	chartSA_.plot(serSA_[1], ChartColor(255, 140, 0, 255));
	chartSA_.plot(serSA_[2], ChartColor(0, 128, 0, 255));
	chartSA_.plot(serSA_[3], ChartColor(0, 128, 128, 255));
	chartSA_.present(x, y, 1.0f); x += dx;

	chartFF_.clear();
	chartFF_.plot(serFF_, ChartColor(255, 0, 0, 255));
	chartFF_.present(200, height_ - 80.0f, 1.0f);
}

void PlaygrounD::renderStats()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	font_.bind();
	font_.setHeight(14);
	glColor3ub(255, 255, 255);

	int simCount = 0;
	if (simManager_)
		simCount = simManager_->getSimCount();
	else
		simCount = sim_ ? 1 : 0;

	int carCount = sim_ ? (int)sim_->cars.size() : 0;

	float x = 1, y = 1, dy = 20;
	font_.draw(x, y, "simId %d of %d", activeSimId_, simCount); y += dy;
	font_.draw(x, y, "carId %d of %d", activeCarId_, carCount); y += dy;
	font_.draw(x, y, "gameTime %.2f s", gameTime_); y += dy;
	font_.draw(x, y, "maxDt %.3f ms", statMaxDt_); y += dy;
	font_.draw(x, y, "maxSim %.3f ms", statMaxSim_); y += dy;
	font_.draw(x, y, "maxDraw %.3f ms", statMaxDraw_); y += dy;
	font_.draw(x, y, "simHZ %d (%llu)", (int)statSimRate_, simId_); y += dy;
	font_.draw(x, y, "drawHZ %d (%llu)", (int)statDrawRate_, drawId_); y += dy;
	font_.draw(x, y, "simHitches %d", (int)statSimHitches_); y += dy;
	font_.draw(x, y, "drawHitches %d", (int)statDrawHitches_); y += dy;
	font_.draw(x, y, "lastHitch %.2f", lastHitchTime_); y += dy;

	font_.draw(x, y, "camera %.2f %.2f %.2f", camPos_.x, camPos_.y, camPos_.z); y += dy;

	if (lookatSurf_) {
		font_.draw(x, y, "surf sector=%u cat=%u istrack=%d grip=%.3f",
			lookatSurf_->sectorID,
			lookatSurf_->collisionCategory,
			(int)lookatSurf_->isValidTrack,
			lookatSurf_->gripMod
		);
		y += dy;
	}
	else {
		font_.draw(x, y, "surf ?"); y += dy;
	}

	if (sim_)
	{
		#if 1
		if (sim_->interopEnabled && sim_->interopInput && sim_->interopState)
		{
			auto* header = (SimInteropHeader*)sim_->interopInput->data();
			font_.draw(x, y, "interopInput %llu / %llu", header->producerId, header->consumerId); y += dy;

			header = (SimInteropHeader*)sim_->interopState->data();
			font_.draw(x, y, "interopState %llu / %llu", header->producerId, header->consumerId); y += dy;
		}
		#endif

		font_.draw(x, y, "collisions %d", (int)sim_->dbgCollisions.size()); y += dy;
	}

	if (car_)
		renderCarStats(x, y, dy);

	glDisable(GL_BLEND);
}

void PlaygrounD::renderCarStats(float x, float y, float dy)
{
	int gear = car_->drivetrain->currentGear - 1;
	float rpm = ((float)car_->drivetrain->engine.velocity * 0.15915507f) * 60.0f;

	font_.draw(x, y, "steer %.3f", car_->controls.steer); y += dy;
	font_.draw(x, y, "ssteer %.3f (%.3f)", car_->smoothSteerValue, (car_->smoothSteerTarget - car_->smoothSteerValue)); y += dy;
	font_.draw(x, y, "clutch %.3f", car_->controls.clutch); y += dy;
	font_.draw(x, y, "brake %.3f", car_->controls.brake); y += dy;
	font_.draw(x, y, "handBrake %.3f", car_->controls.handBrake); y += dy;
	font_.draw(x, y, "gas %.3f", car_->controls.gas); y += dy;

	font_.draw(x, y, "gear %d", car_->state->gear); y += dy;
	font_.draw(x, y, "gearReq %d", car_->controls.requestedGearIndex); y += dy;
	font_.draw(x, y, "gearTime #%d %.3f", gear, timeSinceShift_); y += dy;
	font_.draw(x, y, "trackPoint %d", car_->nearestTrackPointId); y += dy;
	font_.draw(x, y, "trackLocation %.2f", car_->trackLocation); y += dy;
	font_.draw(x, y, "trackWidth %.2f", car_->track->computedTrackWidth); y += dy;

	#if 1
	//font_.draw(x, y, "drifting %d", car_->drifting); y += dy;
	//font_.draw(x, y, "currentDriftAngle %.2f", car_->currentDriftAngle); y += dy;
	//font_.draw(x, y, "instantDrift %.2f", car_->instantDrift); y += dy;
	font_.draw(x, y, "driftPoints %.2f", car_->scoring->driftPoints); y += dy;
	font_.draw(x, y, "driftCombo %d", car_->scoring->driftComboCounter); y += dy;
	font_.draw(x, y, "agentReward %.2f", car_->scoring->agentDriftReward); y += dy;
	#endif

	#if 1
	font_.draw(x, y, "fuel %.2f", car_->fuel); y += dy;
	font_.draw(x, y, "engineLife %.2f", car_->drivetrain->engineModel->lifeLeft); y += dy;
	for (int i = 0; i < 5; ++i)
	{
		font_.draw(x, y, "dmg %d %.3f", i, car_->damageZoneLevel[i]); y += dy;
	}
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "flatSpot %d %.3f", i, car_->tyres[i]->status.flatSpot); y += dy;
	}
	#endif

	#if 0
	for (int i = 0; i < 4; ++i)
	{
		auto s = car_->suspensions[i]->getStatus();
		font_.draw(x, y, "travel%d %3.2f mm", i, (s.travel * 1000.0f)); y += dy;
	}

	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "psi%d %3.2f", i, (car_->tyres[i]->status.pressureDynamic)); y += dy;
	}
	#endif

	#if 0
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "load%d %10.2f", i, (car_->tyres[i]->status.load)); y += dy;
	}
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "SA%d %+10.4f", i, car_->tyres[i]->status.slipAngleRAD * 57.2958f); y += dy;
	}
	#endif

	#if 0
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "SR%d %+10.5f", i, car_->tyres[i]->status.slipRatio); y += dy;
	}
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "ND%d %+10.5f", i, car_->tyres[i]->status.ndSlip); y += dy;
	}
	#endif

	#if 0
	for (int i = 0; i < 2; ++i)
	{
		font_.draw(x, y, "ST #%d %.4f", i, car_->suspensions[i]->getSteerTorque()); y += dy;
	}
	#endif

	#if 0
	for (int i = 0; i < 2; ++i)
	{
		font_.draw(x, y, "FX FY MZ FT #%d %.4f %.4f %.4f %.4f", i, car_->tyres[i]->status.Fx, car_->tyres[i]->status.Fy, car_->tyres[i]->status.Mz, car_->tyres[i]->status.feedbackTorque); y += dy;
		font_.draw(x, y, "DX DY #%d %.4f %.4f", i, car_->tyres[i]->status.Dx, car_->tyres[i]->status.Dy); y += dy;
	}
	#endif

	// FF
	#if 0
	float fSteerTorq = car_->suspensions[0]->getSteerTorque() + car_->suspensions[1]->getSteerTorque();
	font_.draw(x, y, "SteerPosition %.3f", car_->lastSteerPosition); y += dy;
	font_.draw(x, y, "SteerTorqSum %.3f", fSteerTorq); y += dy;
	font_.draw(x, y, "CurrentMZ %.3f", car_->mzCurrent); y += dy;
	font_.draw(x, y, "PureMZFF %.3f", car_->lastPureMZFF); y += dy;
	font_.draw(x, y, "GyroFF %.3f", car_->lastGyroFF); y += dy;
	font_.draw(x, y, "FF %.3f", car_->lastFF); y += dy;
	font_.draw(x, y, "Damp %.3f", car_->lastDamp); y += dy;
	#endif

	#if 0
	font_.draw(x, y, "abs %.4f", car_->lastVibr.abs); y += dy;
	font_.draw(x, y, "curbs %.4f", car_->lastVibr.curbs); y += dy;
	font_.draw(x, y, "engine %.4f", car_->lastVibr.engine); y += dy;
	font_.draw(x, y, "gforce %.4f", car_->lastVibr.gforce); y += dy;
	font_.draw(x, y, "slips %.4f", car_->lastVibr.slips); y += dy;
	#endif

	#if 0
	auto vcar = car_->body->worldToLocal({ 0, 0, 0 });
	auto off = car_->body->worldToLocal(vec3f(camPos_.x, camPos_.y, camPos_.z));
	font_.draw(x, y, "CAR %.2f %.2f %.2f", vcar.x, vcar.y, vcar.z); y += dy;
	font_.draw(x, y, "REL %.2f %.2f %.2f", off.x, off.y, off.z); y += dy;
	font_.draw(x, y, "FOV %.3f", fov_); y += dy;

	auto euler = car_->body->getWorldMatrix(0).getEulerAngles();
	font_.draw(x, y, "Euler %.2f %.2f %.2f", euler.x, euler.y, euler.z); y += dy;
	#endif

	// STATUS BAR

	if (inpCamMode_ == (int)ECamMode::Eye || inpCamMode_ == (int)ECamMode::Rear)
	{
		if (rpm < 4000)
			glColor3ub(0, 255, 0);
		else if (rpm < 6000)
			glColor3ub(255, 255, 0);
		else
			glColor3ub(255, 0, 0);

		float w = (float)width_;
		float h = (float)height_;

		font_.setHeight(25);
		font_.draw(w * 0.35f, h * 0.7f, "%d", (int)rpm);
		font_.draw(w * 0.45f, h * 0.7f, "G%d", gear);

		glColor3ub(255, 255, 255);
		font_.draw(w * 0.55f, h * 0.7f, "%.1f kmh", car_->speed.kmh());

		if (car_->drivetrain->isGearGrinding)
		{
			glColor3ub(255, 0, 0);
			font_.draw(width_ * 0.4f, height_ * 0.4f, "MONEY SHIFT!");
		}
	}

	// DRIFT BAR

	if (inpCamMode_ == (int)ECamMode::Eye || inpCamMode_ == (int)ECamMode::Rear)
	{
		float w = (float)width_;
		float h = (float)height_;

		font_.setHeight(25);

		glColor3ub(255, 255, 255);
		if (car_->scoring->driftExtreme) glColor3ub(0, 255, 0);
		if (car_->scoring->driftInvalid) glColor3ub(255, 0, 0);

		font_.draw(w * 0.35f, h * 0.65f, "DA %d", (int)(car_->scoring->currentDriftAngle * M_RAD2DEG));
		//font_.draw(w * 0.45f, h * 0.65f, "%d", (int)car_->driftComboCounter);
		//font_.draw(w * 0.55f, h * 0.65f, "DP %d", (int)car_->instantDrift);
		font_.draw(w * 0.55f, h * 0.65f, "DD %.3f", car_->scoring->instantDriftDelta);
		font_.draw(w * 0.65f, h * 0.65f, "R %.3f", car_->scoring->agentDriftReward);
	}
}

}
