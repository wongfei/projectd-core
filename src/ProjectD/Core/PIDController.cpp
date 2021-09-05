#include "Core/PIDController.h"
#include <cmath>

namespace D {

float PIDController::eval(float targetv, float currentv, float dt)
{
	float fErr = targetv - currentv;
	currentError = fErr;

	float fIntegral = (fErr * dt) + integral;
	integral = fIntegral;

	float fValue = (((fErr - currentError) / dt) * D) + ((fIntegral * I) + (fErr * P));

	if (isfinite(fErr) && isfinite(fIntegral) && isfinite(fValue))
	{
		return fValue;
	}

	reset();
	return 0;
}

void PIDController::setPID(float p, float i, float d)
{
	P = p;
	I = i;
	D = d;
}

void PIDController::reset()
{
	currentError = 0;
	integral = 0;
}

}
