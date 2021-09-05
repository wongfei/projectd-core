#pragma once

#include <inttypes.h>

GLChart chartDT_;
ChartSeries serSim_;
ChartSeries serDraw_;

GLChart chartInput_;
ChartSeries serSteer_;
ChartSeries serClutch_;
ChartSeries serBrake_;
ChartSeries serGas_;

GLChart chartSA_;
ChartSeries serSA_[4];

GLChart chartFF_;
ChartSeries serFF_;

static void initCharts()
{
	const int ChartN = 512;
	float y0 = 0;
	float y1 = 5;
	chartDT_.init(ChartN, 64);
	serSim_.init(ChartN, y0, y1);
	serDraw_.init(ChartN, y0, y1);

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

static void drawCharts()
{
	float dx = 550;
	float x = 200;
	float y = 10;

	chartDT_.clear();
	chartDT_.plot(serSim_, ChartColor(255, 165, 0, 255)); // orange
	chartDT_.plot(serDraw_, ChartColor(255, 0, 255, 255)); // magenta
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

static void renderText()
{
	begin2d();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	font_.bind();
	font_.setHeight(14);
	glColor3ub(255, 255, 255);

	int gear = car_->drivetrain->currentGear - 1;
	float rpm = ((float)car_->drivetrain->engine.velocity * 0.15915507f) * 60.0f;

	float x = 1, y = 1, dy = 25;
	font_.draw(x, y, "TS %.3f", gameTime_); y += dy;
	font_.draw(x, y, "maxDt %.3f millis", statMaxDt_); y += dy;
	font_.draw(x, y, "maxSim %.3f millis", statMaxSim_); y += dy;
	font_.draw(x, y, "maxDraw %.3f millis", statMaxDraw_); y += dy;
	font_.draw(x, y, "simHZ %d %" PRIu64, (int)statSimRate_, simId_); y += dy;
	font_.draw(x, y, "drawHZ %d %" PRIu64, (int)statDrawRate_, drawId_); y += dy;

	if (lookatSurf_) {
		font_.draw(x, y, "surf cat=%d istrack=%d grip=%.3f",
			(int)lookatSurf_->collisionCategory,
			(int)lookatSurf_->isValidTrack,
			lookatSurf_->gripMod
		);
		y += dy;
	}
	else {
		font_.draw(x, y, "surf ?"); y += dy;
	}

	font_.draw(x, y, "steer %.3f", car_->controls.steer); y += dy;
	font_.draw(x, y, "gas %.3f", car_->controls.gas); y += dy;
	font_.draw(x, y, "brake %.3f", car_->controls.brake); y += dy;
	font_.draw(x, y, "handBrake %.3f", car_->controls.handBrake); y += dy;
	font_.draw(x, y, "clutch %.3f", car_->controls.clutch); y += dy;

	font_.draw(x, y, "geartime #%d %.3f", gear, timeSinceShift_); y += dy;
	font_.draw(x, y, "collisions %d", (int)sim_->collisions.size()); y += dy;

	for (int i = 0; i < 4; ++i)
	{
		auto s = car_->suspensions[i]->getStatus();
		font_.draw(x, y, "travel%d %3.2f mm", i, (s.travel * 1000.0f)); y += dy;
	}

	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "psi%d %3.2f", i, (car_->tyres[i]->status.pressureDynamic)); y += dy;
	}

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

	#if 1
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "SR%d %+10.5f", i, car_->tyres[i]->status.slipRatio); y += dy;
	}
	for (int i = 0; i < 4; ++i)
	{
		font_.draw(x, y, "ND%d %+10.5f", i, car_->tyres[i]->status.ndSlip); y += dy;
	}
	#endif

	// FF
	#if 0
	font_.draw(x, y, "mzCurrent %.3f", car_->mzCurrent); y += dy;
	font_.draw(x, y, "lastPureMZFF %.3f", car_->lastPureMZFF); y += dy;
	font_.draw(x, y, "lastFF %.3f", car_->lastFF); y += dy;
	font_.draw(x, y, "lastDamp %.3f", car_->lastDamp); y += dy;
	font_.draw(x, y, "lastGyroFF %.3f", car_->lastGyroFF); y += dy;
	font_.draw(x, y, "lastSteerPosition %.3f", car_->lastSteerPosition); y += dy;
	#endif

	// CAM POS
	#if 1
	auto vcar = car_->body->worldToLocal({ 0, 0, 0 });
	auto off = car_->body->worldToLocal(vec3f(camPos_.x, camPos_.y, camPos_.z));
	font_.draw(x, y, "CAR %.2f %.2f %.2f", vcar.x, vcar.y, vcar.z); y += dy;
	font_.draw(x, y, "CAM %.2f %.2f %.2f", camPos_.x, camPos_.y, camPos_.z); y += dy;
	font_.draw(x, y, "REL %.2f %.2f %.2f", off.x, off.y, off.z); y += dy;
	font_.draw(x, y, "FOV %.3f", fov_); y += dy;

	auto euler = car_->body->getWorldMatrix(0).getEulerAngles();
	font_.draw(x, y, "Euler %.2f %.2f %.2f", euler.x, euler.y, euler.z); y += dy;

	#endif

	// STATUS BAR

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
	font_.draw(w * 0.45f, h * 0.7f, "%d", gear);

	glColor3ub(255, 255, 255);
	font_.draw(w * 0.55f, h * 0.7f, "%.1f", car_->speed.kmh());

	if (car_->drivetrain->isGearGrinding)
	{
		glColor3ub(255, 0, 0);
		font_.draw(width_ * 0.4f, height_ * 0.6f, "MONEY SHIFT!");
	}

	//===

	glDisable(GL_BLEND);

	drawCharts();

	glDisable(GL_TEXTURE_2D);
}
