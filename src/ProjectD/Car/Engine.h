#pragma once

#include "Car/CarCommon.h"
#include "Car/DynamicController.h"

namespace D {

struct EngineData
{
	Curve powerCurve;
	Curve coastCurve; // TODO: ???
	int minimum = 1000;
	int limiter = 18000;
	int limiterCycles = 50;
	float coast2 = 0.000001f;
	float coast1 = 0;
	float coast0 = 0;
	float overlapFreq = 1.0f;
	float overlapGain = 0;
	float overlapIdealRPM = 6000.0f;
};

struct EngineInput
{
	float gasInput = 0;
	float carSpeed = 0;
	float altitude = 0;
	float rpm = 0;
};

struct EngineStatus
{
	double outTorque = 0;
	double externalCoastTorque = 0;
	float turboBoost = 0;
	bool isLimiterOn = false;
};

struct TurboDynamicController
{
	DynamicController controller;
	Turbo* turbo = nullptr;
	bool isWastegate = false;
};

struct Engine : public NonCopyable
{
	Engine();
	~Engine();
	void init(Car* car);
	void reset();
	void precalculatePowerAndTorque();
	void step(const EngineInput& input, float dt);
	float getThrottleResponseGas(float gas, float rpm);
	void stepTurbos();
	void setTurboBoostLevel(float value);
	void setCoastSettings(int id);
	void blowUp();
	int getLimiterRPM() const;

	// config|engine
	EngineData data;
	float inertia = 1.0f;
	float starterTorque = 20.0f;
	float limiterMultiplier = 1.0f;
	int defaultEngineLimiter = 0;
	bool isEngineStallEnabled = false;

	// config|coast
	Curve gasCoastOffsetCurve;
	int coastSettingsDefaultIndex = 0;
	int coastEntryRpm = 0;

	// config|throttle
	Curve throttleResponseCurve;
	Curve throttleResponseCurveMax;
	float throttleResponseCurveMaxRef = 6000.0f;

	// config|turbo
	std::vector<std::unique_ptr<Turbo>> turbos;
	std::vector<std::unique_ptr<TurboDynamicController>> turboControllers;
	float bovThreshold = 0.2f;
	bool turboAdjustableFromCockpit = false;

	// config|damage
	float rpmDamageThreshold = 0;
	float rpmDamageK = 0;
	float turboBoostDamageThreshold = 0;
	float turboBoostDamageK = 0;

	// runtime
	Car* car = nullptr;
	Simulator* sim = nullptr;
	std::vector<ITorqueGenerator*> torqueGenerators;
	std::vector<ICoastGenerator*> coastGenerators;

	EngineInput lastInput;
	EngineStatus status;

	float restrictor = 0;
	float fuelPressure = 1.0f;
	float gasCoastOffset = 0;
	float electronicOverride = 1.0f;

	float lifeLeft = 0;
	float gasUsage = 0;
	float bov = 0;
	int limiterOn = 0;

	float maxPowerRPM = 0;
	float maxPowerW = 0;
	float maxPowerW_Dynamic = -1.0f;
	float maxTorqueRPM = 0;
	float maxTorqueNM = 0;
};

}
