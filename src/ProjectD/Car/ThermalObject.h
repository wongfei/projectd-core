#pragma once

#include "Car/CarCommon.h"

namespace D {

struct ThermalObject : public NonCopyable
{
	ThermalObject();
	~ThermalObject();
	void step(float dt, float ambientTemp, const Speed& speed);
	void addHeadSource(float heat);

	// config
	float tmass = 1.0f;
	float coolSpeedK = 0.002f;
	float coolFactor = 0.2f;
	float heatFactor = 1.0f;

	// runtime
	float heatAccumulator = 0;
	float t = 0;
};

}
