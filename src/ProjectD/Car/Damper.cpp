#include "Car/Damper.h"

namespace D {

Damper::Damper()
{}

Damper::~Damper()
{}

float Damper::getForce(float speed) // TODO: check
{
	float fForce;
	if (speed <= 0.0f)
	{
		if (fabsf(speed) <= fastThresholdRebound)
			fForce = -(speed * reboundSlow);
		else
			fForce = (fastThresholdRebound * reboundSlow) - ((fastThresholdRebound + speed) * reboundFast);
	}
	else
	{
		if (speed <= fastThresholdBump)
			fForce = -(speed * bumpSlow);
		else
			fForce = -(((speed - fastThresholdBump) * bumpFast) + (fastThresholdBump * bumpSlow));
	}
	return fForce;
}

}
