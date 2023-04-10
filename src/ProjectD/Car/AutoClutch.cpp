#include "Car/AutoClutch.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"
//#include "Sim/Simulator.h"

namespace D {

ClutchSequence::ClutchSequence()
{
}

ClutchSequence::ClutchSequence(const Curve& curve) : 
	clutchCurve(curve)
{
}

AutoClutch::AutoClutch()
{
	clutchSequence.currentTime = 0.0f;
	clutchSequence.isDone = true;
}

AutoClutch::~AutoClutch()
{}

void AutoClutch::init(Car* _car)
{
	car = _car;

	upshiftProfile.reset();
	downshiftProfile.reset();

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"drivetrain.ini"));
	if (!ini->ready)
		return;

	if (ini->hasSection(L"HEADER"))
	{
		int iVersion = ini->getInt(L"HEADER", L"VERSION");
		if (iVersion >= 3)
		{
			//isForced = ini->getInt(L"AUTOCLUTCH", L"FORCED_ON") != 0;
		}
	}

	auto upProfileName = ini->getString(L"AUTOCLUTCH", L"UPSHIFT_PROFILE");
	auto dnProfileName = ini->getString(L"AUTOCLUTCH", L"DOWNSHIFT_PROFILE");

	useAutoOnChange = ini->getInt(L"AUTOCLUTCH", L"USE_ON_CHANGES") != 0;

	if (upProfileName != L"NONE" && ini->hasSection(upProfileName))
	{
		float fPoint0 = ini->getFloat(upProfileName, L"POINT_0");
		float fPoint1 = ini->getFloat(upProfileName, L"POINT_1");
		float fPoint2 = ini->getFloat(upProfileName, L"POINT_2");

		upshiftProfile.addValue(0.0f, 1.0f);
		upshiftProfile.addValue(fPoint0 * 0.001f, 0.0f);
		upshiftProfile.addValue(fPoint1 * 0.001f, 0.0f);
		upshiftProfile.addValue(fPoint2 * 0.001f, 1.0f);
	}

	if (dnProfileName != L"NONE" && ini->hasSection(dnProfileName))
	{
		float fPoint0 = ini->getFloat(dnProfileName, L"POINT_0");
		float fPoint1 = ini->getFloat(dnProfileName, L"POINT_1");
		float fPoint2 = ini->getFloat(dnProfileName, L"POINT_2");

		downshiftProfile.addValue(0.0f, 1.0f);
		downshiftProfile.addValue(fPoint0 * 0.001f, 0.0f);
		downshiftProfile.addValue(fPoint1 * 0.001f, 0.0f);
		downshiftProfile.addValue(fPoint2 * 0.001f, 1.0f);
	}

	rpmMin = ini->getFloat(L"AUTOCLUTCH", L"MIN_RPM");
	rpmMax = ini->getFloat(L"AUTOCLUTCH", L"MAX_RPM");

	if (rpmMin == 0.0f || rpmMax == 0.0f)
	{
		rpmMin = 1500.0f;
		rpmMax = 2500.0f;
	}

	auto* pContext = this;
	car->drivetrain->evOnGearRequest.add(this, [pContext](const OnGearRequestEvent& request)
	{
		pContext->onGearRequest(request);
	});
}

void AutoClutch::step(float dt) // TODO: fix this BRAINFUCK
{
	if (!clutchSequence.isDone)
	{
		bool bIsMoving = car->speed.kmh() >= 5.0f;
		if (bIsMoving)
		{
			stepSequence(dt);
			return;
		}
		clutchSequence.isDone = true;
	}

	if (useAutoOnStart || isForced)
	{
		float fNewClutchInput = 1.0f;
		float fNewClutchValueSignal = 1.0f;
		float fEngineRpm = car->drivetrain->getEngineRPM();
		int iCurGear = car->drivetrain->currentGear;

		bool bIsStationary = car->speed.kmh() < 5.0f;
		bool bRpmValid = false;

		float fClutchValueSignal;
		float fClutchSpeedDt;

		if ((iCurGear & 0xFFFFFFFD) != 0) // TODO: WTF???
		{
			if (iCurGear == 1)
			{
				if (!bIsStationary)
				{
LABEL_21:
					fClutchValueSignal = clutchValueSignal;
					fClutchSpeedDt = dt * clutchSpeed;
					
					if (fabsf(fNewClutchValueSignal - fClutchValueSignal) >= fClutchSpeedDt)
					{
						if (fNewClutchValueSignal <= fClutchValueSignal)
							clutchValueSignal = fClutchValueSignal - fClutchSpeedDt;
						else
							clutchValueSignal = fClutchSpeedDt + fClutchValueSignal;
					}
					else
					{
						clutchValueSignal = fNewClutchValueSignal;
					}

					if (clutchValueSignal <= 1.0f)
					{
						if (clutchValueSignal >= 0.0f)
							fNewClutchInput = clutchValueSignal;
						else
							fNewClutchInput = 0.0f;
					}

					car->controls.clutch = fNewClutchInput;
					return;
				}

				if (car->controls.gas > 0.2f)
				{
					clutchValueSignal = 1.0f;
					goto LABEL_21;
				}

LABEL_20:
				fNewClutchValueSignal = 0.0f;
				clutchValueSignal = 0.0f;
				goto LABEL_21;
			}

			bRpmValid = fEngineRpm < rpmMin;
		}
		else
		{
			if (fEngineRpm >= rpmMin)
			{
				if (fEngineRpm <= rpmMax)
				{
					fNewClutchValueSignal = (fEngineRpm - rpmMin) / (rpmMax - rpmMin);
					clutchValueSignal = fNewClutchValueSignal;
				}
			}

			if (fEngineRpm > rpmMax)
				fNewClutchValueSignal = 1.0f;

			bRpmValid = fEngineRpm < rpmMin;
		}

		if (!bRpmValid)
			goto LABEL_21;

		goto LABEL_20;
	}
}

void AutoClutch::stepSequence(float dt)
{
	float fClutchSignal = clutchSequence.clutchCurve.getValue(clutchSequence.currentTime);
	clutchValueSignal = fClutchSignal;
	clutchSequence.currentTime += dt;

	if (clutchSequence.currentTime > clutchSequence.clutchCurve.getMaxReference())
		clutchSequence.isDone = true;

	// TODO: check
	car->controls.clutch = tclamp(clutchValueSignal, 0.0f, 1.0f);
}

void AutoClutch::onGearRequest(const struct OnGearRequestEvent& ev)
{
	if (useAutoOnChange)
	{
		if (ev.request == GearChangeRequest::eChangeDown && clutchValueSignal > 0.01f && downshiftProfile.getCount() == 4)
		{
			//ClutchSequence::ClutchSequence(&v6, &this->downshiftProfile);
			//ClutchSequence::operator=(&this->clutchSequence, v4);
			//Curve::~Curve(&v6.clutchCurve);
			clutchSequence = ClutchSequence(downshiftProfile);
		}
		if (ev.request == GearChangeRequest::eChangeUp && clutchValueSignal > 0.01f && upshiftProfile.getCount() == 4)
		{
			//ClutchSequence::ClutchSequence(&v6, &this->upshiftProfile);
			//ClutchSequence::operator=(&this->clutchSequence, v5);
			//Curve::~Curve(&v6.clutchCurve);
			clutchSequence = ClutchSequence(upshiftProfile);
		}
	}
}

}
