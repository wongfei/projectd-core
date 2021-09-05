#pragma once

#include "Car/CarCommon.h"

namespace D {

struct GearChanger : public NonCopyable
{
	GearChanger();
	~GearChanger();
	void init(Car* car);
	void step(float dt);

	// runtime
	Car* car = nullptr;
	bool wasGearUpTriggered = 0;
	bool wasGearDnTriggered = 0;
	bool lastGearUp = 0;
	bool lastGearDn = 0;
};

}
