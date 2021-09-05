#pragma once

#include "Car/CarCommon.h"
#include "Car/DynamicController.h"

namespace D {

struct SteeringSystem : public NonCopyable
{
	SteeringSystem();
	~SteeringSystem();
	void init(Car* car);
	void step(float dt);

	// config
	DynamicController ctrl4ws;
	float linearRatio = 0.003f;
	bool has4ws = false;

	// runtime
	Car* car = nullptr;
};

}
