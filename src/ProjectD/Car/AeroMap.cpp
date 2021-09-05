#include "Car/AeroMap.h"
#include "Car/Car.h"
#include "Car/Wing.h"
#include "Car/WingDynamicController.h"
#include "Sim/SlipStream.h"

namespace D {

AeroMap::AeroMap()
{}

AeroMap::~AeroMap()
{}

void AeroMap::init(Car* _car, const vec3f& frontAP, const vec3f& rearAP)
{
	car = _car;
	carBody = car->body.get();
	frontApplicationPoint = frontAP;
	rearApplicationPoint = rearAP;

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"aero.ini"));
	GUARD_FATAL(ini->ready);

	if (ini->hasSection(L"SLIPSTREAM")) // TODO: what cars?
	{
		car->slipStream->effectGainMult = ini->getFloat(L"SLIPSTREAM", L"EFFECT_GAIN_MULT");
		car->slipStream->speedFactorMult = ini->getFloat(L"SLIPSTREAM", L"SPEED_FACTOR_MULT");
	}

	for (int id = 0; ; ++id)
	{
		auto strId = strwf(L"WING_%d", id);
		if (!ini->hasSection(strId))
			break;

		wings.emplace_back(std::make_unique<Wing>(car, ini.get(), id, false));
	}

	for (int id = 0; ; ++id)
	{
		auto strId = strwf(L"FIN_%d", id);
		if (!ini->hasSection(strId))
			break;

		wings.emplace_back(std::make_unique<Wing>(car, ini.get(), id, true));
	}

	if (wings.empty()) // TODO: what cars?
	{
		GUARD_FATAL(ini->hasSection(L"DATA"));
		referenceArea = ini->getFloat(L"DATA", L"REFERENCE_AREA");
		frontShare = ini->getFloat(L"DATA", L"FRONT_SHARE");
		CD = ini->getFloat(L"DATA", L"CD");
		CL = ini->getFloat(L"DATA", L"CL");
		CDX = ini->getFloat(L"DATA", L"CDX");
		CDY = ini->getFloat(L"DATA", L"CDY");
	}
	else
	{
		GUARD_WARN(!ini->hasSection(L"DATA")); // redundant
	}

	for (int id = 0; ; ++id)
	{
		auto strId = strwf(L"DYNAMIC_CONTROLLER_%d", id);
		if (!ini->hasSection(strId))
			break;

		int iWing = ini->getInt(strId, L"WING");
		if (iWing >= 0 && iWing < (int)wings.size())
		{
			wings[iWing]->dynamicControllers.emplace_back(std::make_unique<WingDynamicController>(car, ini.get(), strId));
			wings[iWing]->data.hasController = true;
		}
		else
		{
			SHOULD_NOT_REACH_WARN;
		}
	}
}

void AeroMap::step(float dt)
{
	if (wings.empty())
	{
		vec3f lv = carBody->getLocalVelocity();
		addDrag(lv);
		addLift(lv);
	}

	for (auto& wing : wings)
	{
		wing->step(dt);
	}
}

void AeroMap::addDrag(const vec3f& lv)
{
	float fDot = lv.sqlen();
	if (fDot != 0.0f)
	{
		vec3f vNorm = lv / sqrtf(fDot);
		dynamicCD = (((fabsf(vNorm.x) * CD) * CDX) + CD) + ((fabsf(vNorm.y) * CD) * CDY);

		float fDrag = ((dynamicCD * fDot) * airDensity) * referenceArea;
		carBody->addLocalForce(vNorm * -(fDrag * 0.5f));

		vec3f vAngVel = carBody->getAngularVelocity();
		fDot = vAngVel.sqlen();
		if (fDot != 0.0f)
		{
			vNorm = vAngVel / sqrtf(fDot);
			carBody->addLocalTorque(vNorm * -(fDot * CDA));
		}
	}
}

void AeroMap::addLift(const vec3f& lv)
{
	float fZZ = lv.z * lv.z;
	if (fZZ != 0.0f)
	{
		float fLift = (((fZZ * CL) * airDensity) * referenceArea) * 0.5f;

		float fFrontLift = fLift * frontShare;
		carBody->addLocalForceAtLocalPos(vec3f(0, -fFrontLift, 0), frontApplicationPoint);

		float fRearLift = fLift * (1.0f - frontShare);
		carBody->addLocalForceAtLocalPos(vec3f(0, -fRearLift, 0), rearApplicationPoint);
	}
}

float AeroMap::getCurrentDragKG()
{
	float fSum = 0;
	for (auto& wing : wings)
	{
		fSum += wing->status.dragKG;
	}
	return fSum;
}

float AeroMap::getCurrentLiftKG()
{
	float fSum = 0;
	for (auto& wing : wings)
	{
		fSum += wing->status.liftKG;
	}
	return fSum;
}

}
