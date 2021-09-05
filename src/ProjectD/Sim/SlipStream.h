#pragma once

#include "Sim/SimulatorCommon.h"

namespace D {

struct SlipStreamTriangle
{
	vec3f points[3];
	plane4f plane;
};

struct SlipStream
{
	SlipStream();
	~SlipStream();
	float getSlipEffect(const vec3f& p);
	void setPosition(const vec3f& pos, const vec3f& vel);

	// config
	float effectGainMult = 1.0f;
	float speedFactorMult = 1.0f;
	float speedFactor = 0.25f;

	// runtime
	SlipStreamTriangle triangle;
	vec3f dir;
	float length = 0;
};

}
