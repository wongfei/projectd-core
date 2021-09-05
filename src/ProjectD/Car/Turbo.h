#pragma once

#include "Car/CarCommon.h"

namespace D {

struct TurboDef
{
	float maxBoost = 0;
	float wastegate = 0;
	float rpmRef = 0;
	float gamma = 0;
	float lagUP = 0;
	float lagDN = 0;
	bool isAdjustable = 0;
};

struct Turbo : public NonCopyable
{
	Turbo(const TurboDef& data);
	~Turbo();
	void step(float gas, float rpms, float dt);
	void reset();
	void setTurboBoostLevel(float value);
	float getBoost() const;

	// config
	TurboDef data;

	// runtime
	float userSetting = 1.0f;
	float rotation = 0;
};

}
