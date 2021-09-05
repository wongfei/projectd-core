#pragma once

#include "Car/CarCommon.h"

namespace D {

struct CarCollisionBox
{
	size_t id = 0;
	vec3f centre;
	vec3f size;
};

struct CarColliderManager : public NonCopyable
{
	CarColliderManager();
	~CarColliderManager();
	void init(Car* car);

	std::vector<CarCollisionBox> boxes;
};

}
