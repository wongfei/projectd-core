#pragma once

#include "Car/CarCommon.h"
#include "Car/ISuspension.h"
#include "Car/Damper.h"
#include "Car/CarDebug.h"

namespace D {

struct SuspensionDamage
{
	float minVelocity = 0;
	float maxDamage = 0;
	float damageGain = 0;
	float damageDirection = 0;
	float damageAmount = 0; // TODO: ???
	float lastAmount = 0; // TODO: ???
};

struct SuspensionBase : public ISuspension
{
	SuspensionType getType() const override { return type; }
	SuspensionStatus getStatus() const override { return status; }

	// config
	SuspensionType type = SuspensionType(0);
	int index = 0;
	float k = 0;
	float progressiveK = 0;
	float bumpStopUp = 0;
	float bumpStopDn = 0;
	float bumpStopRate = 0;
	float bumpStopProgressive = 0;
	float rodLength = 0;
	float toeOUT_Linear = 0;
	float staticCamber = 0;
	float packerRange = 0;
	float baseCFM = 0;
	Damper damper;

	// runtime
	IPhysicsEnginePtr core;
	IRigidBodyPtr carBody;
	SuspensionStatus status;
};

inline float randDamageDirection()
{
	float fRnd = ((rand() * 0.000030518509f) * 100.0f);
	return (fRnd >= 50.0f) ? 1.0f : -1.0f;
}

}
