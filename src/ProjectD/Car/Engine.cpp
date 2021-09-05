#include "Car/Engine.h"
#include "Car/Car.h"
#include "Car/Turbo.h"
#include "Car/ITorqueGenerator.h"
#include "Car/ICoastGenerator.h"
#include "Sim/Simulator.h"

namespace D {

Engine::Engine()
{}

Engine::~Engine()
{}

void Engine::init(Car* _car)
{
	car = _car;
	sim = car->sim;

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"engine.ini"));
	GUARD_FATAL(ini->ready);

	auto strPowerCurve = ini->getString(L"HEADER", L"POWER_CURVE");
	data.powerCurve.load(car->carDataPath + strPowerCurve);

	data.minimum = ini->getInt(L"ENGINE_DATA", L"MINIMUM");
	if (!data.minimum)
		data.minimum = 1000;

	auto strCoastCurve = ini->getString(L"HEADER", L"COAST_CURVE");
	if (strCoastCurve == L"FROM_COAST_REF")
	{
		float fRpm = ini->getFloat(L"COAST_REF", L"RPM");
		float fTorque = ini->getFloat(L"COAST_REF", L"TORQUE");
		float fNL = ini->getFloat(L"COAST_REF", L"NON_LINEARITY");

		float v13 = ((1.0f - fNL) * fRpm) - data.minimum;
		float v14 = fNL * fRpm;

		data.coast1 = (v13 == 0.0f) ? 0.0f : -(fTorque / v13);
		data.coast2 = (v14 == 0.0f) ? 0.0f : fTorque / (v14 * v14);
	}

	inertia = ini->getFloat(L"ENGINE_DATA", L"INERTIA");
	data.limiter = ini->getInt(L"ENGINE_DATA", L"LIMITER");
	defaultEngineLimiter = data.limiter;

	if (defaultEngineLimiter)
	{
		rpmDamageThreshold = defaultEngineLimiter * 1.05f;
		rpmDamageK = 10.0f;
	}

	data.limiterCycles = ini->getInt(L"ENGINE_DATA", L"LIMITER_HZ");
	if (data.limiterCycles)
		data.limiterCycles = 1000 / data.limiterCycles / 3; // TODO: check
	else
		data.limiterCycles = 50;

	if (ini->hasSection(L"COAST_SETTINGS"))
	{
		gasCoastOffsetCurve = ini->getCurve(L"COAST_SETTINGS", L"LUT");
		coastSettingsDefaultIndex = ini->getInt(L"COAST_SETTINGS", L"DEFAULT");
		setCoastSettings(coastSettingsDefaultIndex);
		coastEntryRpm = data.minimum + ini->getInt(L"COAST_SETTINGS", L"ACTIVATION_RPM");
	}

	for (int id = 0; ; id++)
	{
		auto strTurbo = strwf(L"TURBO_%d", id);
		if (!ini->hasSection(strTurbo))
			break;

		TurboDef td;
		td.lagDN = (1.0f - ini->getFloat(strTurbo, L"LAG_DN")) * 1.333333f * 333.3333f;
		td.lagUP = (1.0f - ini->getFloat(strTurbo, L"LAG_UP")) * 1.333333f * 333.3333f;
		td.maxBoost = ini->getFloat(strTurbo, L"MAX_BOOST");
		td.wastegate = ini->getFloat(strTurbo, L"WASTEGATE");
		td.rpmRef = ini->getFloat(strTurbo, L"REFERENCE_RPM");
		td.gamma = ini->getFloat(strTurbo, L"GAMMA");
		td.isAdjustable = (ini->getInt(strTurbo, L"COCKPIT_ADJUSTABLE") != 0);

		if (td.isAdjustable)
			turboAdjustableFromCockpit = true;

		turbos.emplace_back(std::make_unique<Turbo>(td));
	}

	if (turboAdjustableFromCockpit)
	{
		float fBoost = ini->getFloat(L"ENGINE_DATA", L"DEFAULT_TURBO_ADJUSTMENT");
		setTurboBoostLevel(fBoost);
	}

	if (ini->hasSection(L"OVERLAP"))
	{
		data.overlapFreq = ini->getFloat(L"OVERLAP", L"FREQUENCY");
		data.overlapGain = ini->getFloat(L"OVERLAP", L"GAIN");
		data.overlapIdealRPM = ini->getFloat(L"OVERLAP", L"IDEAL_RPM");
	}

	throttleResponseCurve.load(car->carDataPath + L"throttle.lut");

	if (ini->hasSection(L"DAMAGE"))
	{
		if (!turbos.empty())
		{
			turboBoostDamageThreshold = ini->getFloat(L"DAMAGE", L"TURBO_BOOST_THRESHOLD");
			turboBoostDamageK = ini->getFloat(L"DAMAGE", L"TURBO_DAMAGE_K");
		}

		rpmDamageThreshold = ini->getFloat(L"DAMAGE", L"RPM_THRESHOLD");
		rpmDamageK = ini->getFloat(L"DAMAGE", L"RPM_DAMAGE_K");
	}

	if (ini->hasSection(L"BOV"))
	{
		bovThreshold = ini->getFloat(L"BOV", L"PRESSURE_THRESHOLD");
	}

	if (!turbos.empty())
	{
		int id = 0;
		for (auto& turbo : turbos)
		{
			auto strIniPath = car->carDataPath + strwf(L"ctrl_turbo%d.ini", id);
			if (osFileExists(strIniPath))
			{
				auto tdc(std::make_unique<TurboDynamicController>());
				tdc->controller.init(car, strIniPath);
				tdc->turbo = turbo.get();
				turboControllers.emplace_back(std::move(tdc));
			}

			strIniPath = car->carDataPath + strwf(L"ctrl_wastegate%d.ini", id);
			if (osFileExists(strIniPath))
			{
				auto tdc(std::make_unique<TurboDynamicController>());
				tdc->controller.init(car, strIniPath);
				tdc->turbo = turbo.get();
				tdc->isWastegate = true;
				turboControllers.emplace_back(std::move(tdc));
			}

			id++;
		}
	}

	if (ini->hasSection(L"THROTTLE_RESPONSE"))
	{
		throttleResponseCurveMaxRef = ini->getFloat(L"THROTTLE_RESPONSE", L"RPM_REFERENCE");
		throttleResponseCurveMax = ini->getCurve(L"THROTTLE_RESPONSE", L"LUT");
	}

	reset();
	precalculatePowerAndTorque();
}

void Engine::reset()
{
	lifeLeft = 1000.0f;

	for (auto& turbo : turbos)
		turbo->reset();
}

void Engine::precalculatePowerAndTorque()
{
	const float fMaxRef = data.powerCurve.getMaxReference();
	float fRpm = 0;

	while (fRpm <= fMaxRef)
	{
		float fTorq = data.powerCurve.getValue(fRpm);
		if (fTorq > maxTorqueNM)
		{
			maxTorqueRPM = fRpm;
			maxTorqueNM = fTorq;
		}

		float fPower = fRpm * fTorq * 0.1047f;
		if (fPower > maxPowerW)
		{
			maxPowerRPM = fRpm;
			maxPowerW = fPower;
		}

		fRpm += 50.0f;
	}
}

void Engine::step(const EngineInput& input, float dt)
{
	lastInput = input;
	lastInput.gasInput = getThrottleResponseGas(input.gasInput, input.rpm);

	if (gasCoastOffset > 0.0f)
	{
		float fGas1 = (input.rpm - (float)data.minimum) / (float)coastEntryRpm;
		fGas1 = tclamp(fGas1, 0.0f, 1.0f);

		float fGas2 = ((1.0f - (gasCoastOffset * fGas1)) * lastInput.gasInput) + (gasCoastOffset * fGas1);
		fGas2 = tclamp(fGas2, 0.0f, 1.0f);

		lastInput.gasInput = fGas2;
	}

	int iLimiter = data.limiter;
	if (iLimiter && (iLimiter * limiterMultiplier) < lastInput.rpm)
	{
		limiterOn = data.limiterCycles;
	}

	if (limiterOn > 0)
	{
		lastInput.gasInput = 0;
		limiterOn--;
	}

	if (lifeLeft <= 0.0f)
	{
		fuelPressure = 0;
	}

	float fGas = lastInput.gasInput * electronicOverride;
	lastInput.gasInput = fGas;
	gasUsage = fGas;

	float fPower = data.powerCurve.getValue(lastInput.rpm);
	float fCoastTorq = 0;

	if (data.coast1 != 0.0f)
	{
		fCoastTorq = (lastInput.rpm - (float)data.minimum) * data.coast1;
	}

	stepTurbos();

	if (status.turboBoost != 0.0f)
	{
		fPower *= (status.turboBoost + 1.0f);
	}

	if (data.coast2 != 0.0f)
	{
		float fRpmDelta = lastInput.rpm - (float)data.minimum;
		fCoastTorq -= (((fRpmDelta * fRpmDelta) * data.coast2) * signf(lastInput.rpm));
	}

	status.externalCoastTorque = 0;
	for (auto* pCoastGen : coastGenerators)
	{
		status.externalCoastTorque += pCoastGen->getCoastTorque();
	}
	fCoastTorq += (float)status.externalCoastTorque;

	if (lastInput.rpm <= (float)data.minimum)
	{
		status.externalCoastTorque = 0;
		fCoastTorq = 0;
	}

	float fTurboBoost = status.turboBoost;
	if (((1.0f - lastInput.gasInput) * fTurboBoost) <= bovThreshold)
		bov = 0;
	else
		bov = 1.0f;

	float fTbDamageThresh = turboBoostDamageThreshold;
	if (fTbDamageThresh != 0.0f && fTurboBoost > fTbDamageThresh)
	{
		lifeLeft -= ((((fTurboBoost - fTbDamageThresh) * turboBoostDamageK) * 0.003f) * sim->mechanicalDamageRate);
	}

	float fRpmDamageThresh = rpmDamageThreshold;
	if (fRpmDamageThresh != 0.0f && lastInput.rpm > fRpmDamageThresh)
	{
		lifeLeft -= ((((lastInput.rpm - fRpmDamageThresh) * rpmDamageK) * 0.003f) * sim->mechanicalDamageRate);
	}

	float fAirAmount = sim->getAirDensity() * 0.82630974f;
	float fRestrictor = restrictor;
	if (fRestrictor > 0.0f)
	{
		fAirAmount -= (((fRestrictor * input.rpm) * 0.0001f) * fGas);
		if (fAirAmount < 0.0f)
			fAirAmount = 0;
	}

	float fOutTorq = ((((fPower - fCoastTorq) * fGas) + fCoastTorq) * fAirAmount);
	status.outTorque = fOutTorq;

	bool bHasFuelPressure = (fuelPressure > 0.0f);
	if (bHasFuelPressure)
	{
		float fRpm = lastInput.rpm;
		if (fRpm >= (float)data.minimum)
		{
			float fOverlapGain = data.overlapGain;
			if (fOverlapGain != 0.0f)
			{
				float fOverlap = sinf((float)sim->physicsTime * 0.001f * data.overlapFreq * fRpm * 0.0003333333333333333f) * 0.5f - 0.5f;
				status.outTorque = (fOverlap * fabsf(fRpm - data.overlapIdealRPM) * fOverlapGain) + fOutTorq;
			}
		}
		else if (isEngineStallEnabled)
		{
			float fStallTorq = fRpm * -0.01f;
			#if 0
			if (GetAsyncKeyState(VK_BACK)) // LOL
				fStallTorq = starterTorque;
			#endif
			status.outTorque = fStallTorq;
		}
		else
		{
			status.outTorque = tmax(15.0f, fOutTorq);
		}
	}

	if (fuelPressure < 1.0f)
	{
		status.outTorque = (status.outTorque - lastInput.rpm * -0.01f) * fuelPressure + lastInput.rpm * -0.01f;
	}

	for (auto* pTorqGen : torqueGenerators)
	{
		status.outTorque += pTorqGen->getOutputTorque();
	}

	bool bLimiterOn = (limiterOn != 0);
	status.isLimiterOn = bLimiterOn;
	electronicOverride = 1.0f;

	float fMaxPowerDyn = maxPowerW_Dynamic;
	float fCurPower = input.rpm * (float)status.outTorque * 0.1047f;
	if (fCurPower > fMaxPowerDyn)
	{
		maxPowerW_Dynamic = fCurPower;
	}
}

float Engine::getThrottleResponseGas(float gas, float rpm)
{
	float result = 0;

	if (throttleResponseCurve.getCount() && throttleResponseCurveMax.getCount())
	{
		float fTrc = tclamp(throttleResponseCurve.getValue(gas * 100.0f) * 0.01f, 0.0f, 1.0f);
		float fTrcMax =  tclamp(throttleResponseCurveMax.getValue(gas * 100.0f) * 0.01f, 0.0f, 1.0f);
		float fTrcScale = tclamp(rpm / throttleResponseCurveMaxRef, 0.0f, 1.0f);

		result = ((fTrcMax - fTrc) * fTrcScale) + fTrc;
	}
	else if (throttleResponseCurve.getCount())
	{
		result = tclamp(throttleResponseCurve.getValue(gas * 100.0f) * 0.01f, 0.0f, 1.0f);
	}
	else
	{
		result = gas;
	}

	return result;
}

void Engine::stepTurbos()
{
	for (auto& tdc : turboControllers)
	{
		if (tdc->isWastegate)
			tdc->turbo->data.wastegate = tdc->controller.eval();
		else
			tdc->turbo->data.maxBoost = tdc->controller.eval();
	}

	status.turboBoost = 0;
	for (auto& turbo : turbos)
	{
		turbo->step(lastInput.gasInput, lastInput.rpm, 0.003f); // TODO: check
		status.turboBoost += (turbo->getBoost() * fuelPressure);
	}
}

void Engine::setTurboBoostLevel(float value)
{
	for (auto& turbo : turbos)
	{
		turbo->setTurboBoostLevel(value);
	}
}

void Engine::setCoastSettings(int id)
{
	if (id >= 0 && id < gasCoastOffsetCurve.getCount())
	{
		gasCoastOffset = gasCoastOffsetCurve.getValue((float)id);
	}
	else
	{
		SHOULD_NOT_REACH_WARN;
	}
}

void Engine::blowUp()
{
	lifeLeft = -100.0f;
}

int Engine::getLimiterRPM() const
{
	return (int)(data.limiter * limiterMultiplier);
}

}
