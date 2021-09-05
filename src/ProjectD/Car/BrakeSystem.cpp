#include "Car/BrakeSystem.h"
#include "Car/Car.h"
#include "Car/Tyre.h"
#include "Sim/Simulator.h"

namespace D {

BrakeSystem::BrakeSystem()
{}

BrakeSystem::~BrakeSystem()
{}

void BrakeSystem::init(Car* _car)
{
	car = _car;

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"brakes.ini"));
	GUARD_FATAL(ini->ready);

	brakePower = ini->getFloat(L"DATA", L"MAX_TORQUE");
	frontBias = ini->getFloat(L"DATA", L"FRONT_SHARE");
	handBrakeTorque = ini->getFloat(L"DATA", L"HANDBRAKE_TORQUE");
	hasCockpitBias = (ini->getInt(L"DATA", L"COCKPIT_ADJUSTABLE") != 0);
	biasStep = ini->getFloat(L"DATA", L"ADJUST_STEP") * 0.01f;
	
	if (ini->hasSection(L"EBB")) // TODO: what cars?
	{
		ebbMode = EBBMode::Internal;
		ebbFrontMultiplier = tmax(1.1f, ini->getFloat(L"EBB", L"FRONT_SHARE_MULTIPLIER"));
	}

	auto strPath = car->carDataPath + L"steer_brake_controller.ini";
	if (osFileExists(strPath)) // TODO: what cars?
	{
		steerBrake.controller.init(car, strPath);
		steerBrake.isActive = true;
	}

	hasBrakeTempsData = (ini->hasSection(L"TEMPS_FRONT") && ini->hasSection(L"TEMPS_REAR"));

	if (hasBrakeTempsData) // cars: F40
	{
		std::wstring strId[] = {L"FRONT", L"FRONT", L"REAR", L"REAR"};
		for (int id = 0; id < 4; ++id)
		{
			auto strTemps = strwf(L"TEMPS_") + strId[id];
			discs[id].perfCurve = ini->getCurve(strTemps, L"PERF_CURVE");
			discs[id].torqueK = ini->getFloat(strTemps, L"TORQUE_K");
			discs[id].coolTransfer = ini->getFloat(strTemps, L"COOL_TRANSFER");
			discs[id].coolSpeedFactor = ini->getFloat(strTemps, L"COOL_SPEED_FACTOR");
		}
	}

	ini->load(car->carDataPath + L"setup.ini");
	if (ini->ready)
	{
		if (ini->hasSection(L"FRONT_BIAS"))
		{
			biasMin = ini->getFloat(L"FRONT_BIAS", L"MIN") * 0.01f;
			biasMax = ini->getFloat(L"FRONT_BIAS", L"MAX") * 0.01f;
		}
	}

	strPath = car->carDataPath + L"ctrl_ebb.ini";
	if (osFileExists(strPath)) // TODO: what cars?
	{
		ebbMode = EBBMode::DynamicController;
		ebbController.init(car, strPath);
	}
}

void BrakeSystem::reset()
{
	biasOverride = -1.0f;
	for (auto& iter : discs)
	{
		iter.t = car->sim->ambientTemperature;
	}
}

void BrakeSystem::step(float dt)
{
	float fFrontBias = frontBias;

	if (biasOverride != -1.0f)
		fFrontBias = biasOverride;

	if (ebbMode != EBBMode::Internal)
	{
		if (ebbMode == EBBMode::DynamicController)
			fFrontBias = ebbController.eval();
	}
	else
	{
		float fLoadFront = car->tyres[1]->status.load + car->tyres[0]->status.load;
		float fLoadAWD = (car->tyres[3]->status.load + car->tyres[2]->status.load) + fLoadFront;
		bool bFlag = false;

		if (fLoadAWD != 0.0f)
		{
			if (car->speed.kmh() > 10.0f)
				bFlag = true;
		}

		if (bFlag)
		{
			ebbInstant = tclamp(((fLoadFront / fLoadAWD) * ebbFrontMultiplier), 0.0f, 1.0f);
		}
		else
		{
			ebbInstant = frontBias;
		}

		fFrontBias = ebbInstant;
	}

	fFrontBias = tclamp(fFrontBias, biasMin, biasMax);

	float fBrakeInput = tmax(car->controls.brake, brakeOverride);
	float fBrakeTorq = (brakePower * brakePowerMultiplier) * fBrakeInput;

	car->tyres[0]->inputs.brakeTorque = fBrakeTorq * fFrontBias;
	car->tyres[1]->inputs.brakeTorque = fBrakeTorq * fFrontBias;

	float fRearBrakeTorq = ((1.0f - fFrontBias) * fBrakeTorq) - rearCorrectionTorque;
	if (fRearBrakeTorq < 0.0f)
		fRearBrakeTorq = 0;

	car->tyres[2]->inputs.brakeTorque = fRearBrakeTorq;
	car->tyres[3]->inputs.brakeTorque = fRearBrakeTorq;

	car->tyres[2]->inputs.handBrakeTorque = car->controls.handBrake * handBrakeTorque;
	car->tyres[3]->inputs.handBrakeTorque = car->controls.handBrake * handBrakeTorque;

	if (steerBrake.isActive)
	{
		float fSteerBrake = steerBrake.controller.eval();
		if (fSteerBrake >= 0.0f)
			car->tyres[3]->inputs.brakeTorque += fSteerBrake;
		else
			car->tyres[2]->inputs.brakeTorque -= fSteerBrake;
	}

	if (hasBrakeTempsData && car->tyres[0]->aiMult <= 1.0f)
		stepTemps(dt);

	brakeOverride = 0;
}

void BrakeSystem::stepTemps(float dt)
{
	float fAmbientTemp = car->sim->ambientTemperature;
	float fSpeed = car->speed.kmh();

	for (int i = 0; i < 4; ++i)
	{
		Tyre* pTyre = car->tyres[i].get();
		BrakeDisc* pDisc = &discs[i];

		pTyre->inputs.brakeTorque = pDisc->perfCurve.getValue(pDisc->t) * pTyre->inputs.brakeTorque;

		float fCool = ((fSpeed * pDisc->coolSpeedFactor) + 1.0f) * pDisc->coolTransfer;

		pDisc->t += (((fAmbientTemp - pDisc->t) * fCool) * dt);
		pDisc->t += (((fabsf(pTyre->status.angularVelocity) * (pTyre->inputs.brakeTorque * pDisc->torqueK)) * 0.001f) * dt);
	}
}

}
