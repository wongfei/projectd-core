#pragma once

#include "Car/CarCommon.h"

namespace D {

struct Damper : public NonCopyable
{
	Damper();
	~Damper();
	float getForce(float speed);

	// config
	float bumpSlow = 2000.0f;
	float reboundSlow = 5000.0f;
	float bumpFast = 300.0f;
	float reboundFast = 300.0f;
	float fastThresholdBump = 0.2f;
	float fastThresholdRebound = 0.2f;
};

}
