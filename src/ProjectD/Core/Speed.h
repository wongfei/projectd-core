#pragma once

#include "Core/Core.h"

namespace D {

struct Speed
{
	inline Speed() = default;
	inline explicit Speed(float ms) : value(ms) {}
	inline float ms() const { return value; }
	inline float kmh() const { return value * 3.6f; }
	inline static Speed fromMS(float ms) { return Speed(ms); }
	inline static Speed fromKMH(float kmh) { return Speed(kmh / 3.6f); }

	float value = 0;
};

}
