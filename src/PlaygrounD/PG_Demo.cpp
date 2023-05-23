#include "PlaygrounD.h"

namespace D {

void PlaygrounD::loadDemo()
{
	log_printf(L"PlaygrounD: loadDemo");

	// auto activate controls/sound when switching
	newCarControls_ = true;
	newCarSound_ = true;

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

	auto track = sim->loadTrack(trackName);

	const int maxCars = tmin(sim->maxCars, (int)track->pits.size());
	if (maxCars <= 0)
		return;

	for (int i = 0; i < maxCars; ++i)
	{
		auto* car = sim->addCar(carModel);
		car->teleport(track->pits[i]);
	}

	setSimulator(sim);
	GUARD_FATAL(car_);

	car_->teleportToSpline(0.0f);
	car_->senseiEnabled = true;

	// SECOND SIM

	#if 0
	auto sim2 = std::make_shared<Simulator>();
	sim2->simulatorId = 1;
	sim2->init(appDir_);
	auto track2 = sim2->loadTrack(trackName);
	auto* car2 = sim2->addCar(carModel);
	car2->teleportToSpline(0.5f);

	struct DbgSimulatorManager : public D::ISimulatorManager
	{
		std::vector<SimulatorPtr> sims;
		virtual int getSimCount() const override { return (int)sims.size(); }
		virtual D::SimulatorPtr getSim(int simId) override { return sims[simId]; }
	};

	auto* sm = new DbgSimulatorManager();
	sm->sims.push_back(sim);
	sm->sims.push_back(sim2);
	simManager_ = sm;
	#endif

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

}
