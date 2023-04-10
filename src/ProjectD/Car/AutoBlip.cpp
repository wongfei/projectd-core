#include "Car/AutoBlip.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"
#include "Sim/Simulator.h"

namespace D {

AutoBlip::AutoBlip()
{}

AutoBlip::~AutoBlip()
{}

void AutoBlip::init(Car* _car)
{
	car = _car;

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"drivetrain.ini"));
	if (!ini->ready)
		return;

	float fLevel = ini->getFloat(L"AUTOBLIP", L"LEVEL");
	float fPoint0 = ini->getFloat(L"AUTOBLIP", L"POINT_0");
	float fPoint1 = ini->getFloat(L"AUTOBLIP", L"POINT_1");
	float fPoint2 = ini->getFloat(L"AUTOBLIP", L"POINT_2");

	blipProfile.reset();
	blipProfile.addValue(0.0, 0.0);
	blipProfile.addValue(fPoint0, fLevel);
	blipProfile.addValue(fPoint1, fLevel);
	blipProfile.addValue(fPoint2, 0.0);

	blipPerformTime = blipProfile.getMaxReference();

	isElectronic = ini->getInt(L"AUTOBLIP", L"ELECTRONIC") != 0;

	auto* pContext = this;
	car->drivetrain->evOnGearRequest.add(this, [pContext](const OnGearRequestEvent& ev)
	{
		if (ev.request == GearChangeRequest::eChangeDown)
		{
			auto* pCar = pContext->car;
			if (pCar->controls.clutch > 0.1f)
			{
				pContext->blipStartTime = (pCar->sim->physicsTime * 1000.0); // sec -> millisec
			}
		}
	});
}

void AutoBlip::step(float dt)
{
	if ((isActive || isElectronic) && (car->speed.kmh() > 5.0f))
	{
		double fBlipElapsed = (car->sim->physicsTime * 1000.0) - blipStartTime;
		if (fBlipElapsed >= 0.0 && fBlipElapsed < blipPerformTime && blipProfile.getCount() == 4)
		{
			float fGas = car->controls.gas;
			float fProfile = blipProfile.getValue(fBlipElapsed);

			if (fGas <= fProfile)
				fGas = fProfile;

			float fNewGas = 1.0;
			if (fGas <= 1.0)
			{
				fNewGas = 0.0;
				if (fGas >= 0.0)
					fNewGas = fGas;
			}

			car->controls.gas = fNewGas;
		}
	}
}

}
