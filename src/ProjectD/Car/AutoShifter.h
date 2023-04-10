#pragma once

#include "Car/CarCommon.h"

namespace D {

struct AutoShifter : public NonCopyable
{
	AutoShifter();
	~AutoShifter();

	void init(Car* car);
	void step(float dt);

	Car* car = nullptr;
	int changeUpRpm = 0;
	int changeDnRpm = 4000;
	float slipThreshold = 0.8f;
	float gasCutoff = 0.0f;
	float gasCutoffTime = 0.5f;
	bool isActive = false;
	bool butGearUp = false;
	bool butGearDn = false;
};

}
