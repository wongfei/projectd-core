#pragma once

#include "Core/Core.h"
#include <cmath>

namespace D {

struct SinSignalGenerator
{
	inline void step(float dt)
	{
		value += ((dt * 1000.0f) * freqScale);
	}

	inline float getValue() const
	{
		return sinf(value * 0.001f);
	}

	float freqScale = 1.0f;
	float value = 0;
};

}
