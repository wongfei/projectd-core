#pragma once

#include "Car/CarCommon.h"
#include "Car/DynamicController.h"

namespace D {

struct AntirollBar : public NonCopyable
{
	AntirollBar();
	~AntirollBar();
	void init(IRigidBody* carBody, ISuspension* s0, ISuspension* s1);
	void step(float dt);

	// config
	DynamicController ctrl;

	// runtime
	IRigidBody* carBody = nullptr;
	ISuspension* hubs[2] = {};
	float k = 0;
};

}
