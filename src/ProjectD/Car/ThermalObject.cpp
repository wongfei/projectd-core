#include "Car/ThermalObject.h"

namespace D {

ThermalObject::ThermalObject()
{}

ThermalObject::~ThermalObject()
{}

void ThermalObject::step(float dt, float ambientTemp, const Speed& speed)
{
	float fOneDivMass = 1.0f / tmass;
	float fCool = 1.0f - (coolSpeedK * speed.value);

	t += (((((fCool * ambientTemp) - t) * fOneDivMass) * dt) * coolFactor);

	float fAccum = heatAccumulator;
	heatAccumulator = 0;

	if (fAccum != 0.0f)
		t += ((((fAccum - t) * fOneDivMass) * dt) * heatFactor);
}

void ThermalObject::addHeadSource(float heat)
{
	heatAccumulator += heat;
}

}
