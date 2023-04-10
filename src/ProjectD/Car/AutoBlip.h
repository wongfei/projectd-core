#pragma once

#include "Car/CarCommon.h"

namespace D {

struct AutoBlip : public NonCopyable
{
	AutoBlip();
	~AutoBlip();

	void init(Car* car);
	void step(float dt);

	Car* car = nullptr;
	Curve blipProfile;
	double blipStartTime = 0;
	double blipPerformTime = 0;
	bool isActive = false;
	bool isElectronic = false;
};

}
