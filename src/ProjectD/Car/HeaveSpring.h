#pragma once

#include "Car/CarCommon.h"
#include "Car/Damper.h"

namespace D {

struct HeaveSpringStatus
{
	float travel = 0;
};

struct HeaveSpring : public NonCopyable
{
	HeaveSpring();
	~HeaveSpring();
	void init(IRigidBody* carBody, SuspensionDW* s1, SuspensionDW* s2, bool isFront, const std::wstring& dataPath);
	void step(float dt);

	// config
	bool isFront = 0;
	bool isPresent = 0;
	float k = 0;
	float progressiveK = 0;
	float bumpStopUp = 0;
	float bumpStopDn = 0;
	float bumpStopRate = 0;
	float rodLength = 0;
	float packerRange = 0;
	Damper damper;

	// runtime
	IRigidBody* carBody = nullptr;
	SuspensionDW* suspensions[2] = {};
	HeaveSpringStatus status;
};

}
