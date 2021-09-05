#pragma once

#include "Car/CarCommon.h"
#include "Car/DynamicController.h"

namespace D {

enum class EBBMode
{
	Disabled = 0x0,
	Internal = 0x1,
	DynamicController = 0x2,
};

struct BrakeDisc
{
	// config
	Curve perfCurve;
	float torqueK = 0.1f;
	float coolTransfer = 0.005f;
	float coolSpeedFactor = 0.001f;

	// runtime
	float t = 0;
};

struct SteerBrake
{
	DynamicController controller;
	bool isActive = false;
};

struct BrakeSystem : public NonCopyable
{
	BrakeSystem();
	~BrakeSystem();
	void init(Car* car);
	void reset();
	void step(float dt);
	void stepTemps(float dt);

	// config
	float brakePower = 2000.0f;
	float brakePowerMultiplier = 1.0f;
	float handBrakeTorque = 0;
	float frontBias = 0.7f;
	float biasStep = 0.005f;
	float biasMin = 0;
	float biasMax = 1.0f;
	bool hasCockpitBias = true;

	// optional
	EBBMode ebbMode = EBBMode(0);
	float ebbFrontMultiplier = 1.1f;
	DynamicController ebbController;
	SteerBrake steerBrake;
	BrakeDisc discs[4];
	bool hasBrakeTempsData = false;

	// runtime
	Car* car = nullptr;
	float brakeOverride = 0;
	float biasOverride = -1.0f;
	float ebbInstant = 0.5f;
	float rearCorrectionTorque = 0;
};

}
