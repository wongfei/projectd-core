#pragma once

#include "Core/Core.h"

namespace D {

struct PIDController
{
	float eval(float targetv, float currentv, float dt);
	void setPID(float p, float i, float d);
	void reset();

	float P = 0;
	float I = 0;
	float D = 0;
	float currentError = 0;
	float integral = 0;
};

}
