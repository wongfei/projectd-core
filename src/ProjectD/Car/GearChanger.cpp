#include "Car/GearChanger.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"

namespace D {

GearChanger::GearChanger()
{}

GearChanger::~GearChanger()
{}

void GearChanger::init(Car* _car)
{
	car = _car;
}

void GearChanger::step(float dt)
{
	wasGearUpTriggered = false;
	wasGearDnTriggered = false;

	int gearId = car->controls.requestedGearIndex;

	if (gearId == -1)
	{
		if (car->controls.gearUp && !lastGearUp)
			wasGearUpTriggered = car->drivetrain->gearUp();

		if (car->controls.gearDn && !lastGearDn)
			wasGearDnTriggered = car->drivetrain->gearDown();

		lastGearUp = car->controls.gearUp;
		lastGearDn = car->controls.gearDn;
	}
	else
	{
		car->drivetrain->setCurrentGear(gearId, false);
	}
}

}
