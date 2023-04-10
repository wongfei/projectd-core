#include "Car/AutoShifter.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"
#include "Car/Engine.h"

namespace D {

AutoShifter::AutoShifter()
{}

AutoShifter::~AutoShifter()
{}

void AutoShifter::init(Car* _car)
{
	car = _car;

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"drivetrain.ini"));
	if (!ini->ready)
		return;

	if (ini->hasSection(L"AUTO_SHIFTER"))
	{
		changeUpRpm = ini->getInt(L"AUTO_SHIFTER", L"UP");
		changeDnRpm = ini->getInt(L"AUTO_SHIFTER", L"DOWN");
		slipThreshold = ini->getFloat(L"AUTO_SHIFTER", L"SLIP_THRESHOLD");
		gasCutoffTime = ini->getFloat(L"AUTO_SHIFTER", L"GAS_CUTOFF_TIME");
	}
}

void AutoShifter::step(float dt)
{
	if (!isActive)
		return;

	if (car->drivetrain->currentGear)
	{
		if (!changeUpRpm)
		{
			auto* pEngine = car->drivetrain->engineModel.get();

			float fMaxPowerRpm = pEngine->maxPowerRPM;
			float fLimiterRpm = (float)pEngine->getLimiterRPM();
			float fMaxRpm;

			if (fLimiterRpm >= fMaxPowerRpm)
				fMaxRpm = fMaxPowerRpm;
			else
				fMaxRpm = fLimiterRpm;

			changeUpRpm = (int)(fMaxRpm * 0.98f);
			changeDnRpm = (int)(pEngine->maxTorqueRPM * 1.1f);
		}

		if (!car->controls.gearUp && !car->controls.gearDn)
		{
			car->controls.gearDn = 0;
			car->controls.gearUp = 0;

			bool bIsSlipping = false;
			if (car->getDrivingTyresSlip() > slipThreshold)
			{
				if (car->speed.value > 5.0f)
					bIsSlipping = true;
			}

			if (!car->drivetrain->isChangingGear())
			{
				if ((car->controls.clutch > 0.99f || car->drivetrain->currentGear == 1) && !bIsSlipping)
				{
					int iEngineRpm = (int)car->drivetrain->getEngineRPM();
					if (iEngineRpm > changeUpRpm)
					{
						// TODO: suspicious
						if (car->drivetrain->currentGear < ((int)car->drivetrain->gears.size() - 1)
							&& car->controls.gas > 0.2f
							&& gasCutoff <= 0.0f)
						{
							car->controls.gearUp = true;
							gasCutoff = gasCutoffTime;
						}
					}

					int iCurGear = car->drivetrain->currentGear;
					int iChangeDnRpm;

					if (iCurGear == 3)
						iChangeDnRpm = (int)(changeDnRpm * 0.65f); // WTF???
					else
						iChangeDnRpm = changeDnRpm;

					if (iEngineRpm < iChangeDnRpm
						&& iCurGear > 2
						&& car->controls.clutch > 0.85f
						&& gasCutoff <= 0.0f)
					{
						car->controls.gearDn = true;
					}
				}
			}

			bool bLowSpeed = car->speed.value < 2.0f;
			if (bLowSpeed && !car->drivetrain->isChangingGear())
			{
				if (car->controls.gas < 0.1f && gasCutoff <= 0.0f && car->drivetrain->currentGear > 2)
					car->controls.gearDn = true;
			}

			float fCutoff = gasCutoff;
			if (fCutoff > 0.0f)
			{
				gasCutoff = fCutoff - dt;
				car->controls.gas = 0.0f;
			}
		}
	}
}

}
