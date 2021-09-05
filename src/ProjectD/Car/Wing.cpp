#include "Car/Wing.h"
#include "Car/Car.h"
#include "Car/AeroMap.h"
#include "Car/WingDynamicController.h"

namespace D {

Wing::Wing()
{}

Wing::~Wing()
{}

Wing::Wing(Car* _car, INIReader* ini, int index, bool isVertical)
{
	init(_car, ini, index, isVertical);
}

void Wing::init(Car* _car, INIReader* ini, int index, bool isVertical)
{
	car = _car;

	auto strId = strwf(L"%s_%d", (isVertical ? L"FIN" : L"WING"), index);

	data.isVertical = isVertical;
	data.name = ini->getString(strId, L"NAME");
	data.chord = ini->getFloat(strId, L"CHORD");
	data.span = ini->getFloat(strId, L"SPAN");
	data.area = data.chord * data.span;
	data.position = ini->getFloat3(strId, L"POSITION");

	auto strPath = car->carDataPath + ini->getString(strId, L"LUT_AOA_CL");
	data.lutAOA_CL.load(strPath);

	strPath = car->carDataPath + ini->getString(strId, L"LUT_AOA_CD");
	data.lutAOA_CD.load(strPath);

	strPath = car->carDataPath + ini->getString(strId, L"LUT_GH_CL");
	if (osFileExists(strPath))
		data.lutGH_CL.load(strPath);

	strPath = car->carDataPath + ini->getString(strId, L"LUT_GH_CD");
	if (osFileExists(strPath))
		data.lutGH_CD.load(strPath);

	data.cdGain = ini->getFloat(strId, L"CD_GAIN");
	data.clGain = ini->getFloat(strId, L"CL_GAIN");

	status.angle = ini->getFloat(strId, L"ANGLE");
	status.inputAngle = status.angle;
	status.frontShare = getPointFrontShare(data.position);

	int iVer = ini->getInt(L"HEADER", L"VERSION");
	if (iVer >= 2)
	{
		const wchar_t* strZones[] = {L"FRONT", L"REAR", L"LEFT", L"RIGHT", L"CENTER"};
		for (int i = 0; i < 5; ++i)
		{
			damageCD[i] = ini->getFloat(strId, strwf(L"ZONE_%s_CD", strZones[i]));
			damageCL[i] = ini->getFloat(strId, strwf(L"ZONE_%s_CL", strZones[i]));
		}
		hasDamage = true;
	}

	if (iVer >= 3)
	{
		data.yawGain = ini->getFloat(strId, L"YAW_CL_GAIN");
	}
}

void Wing::step(float dt)
{
	if (!overrideStatus.isActive)
		stepDynamicControllers(dt);

	vec3f vGroundWind = car->getGroundWindVector();
	vec3f vWorldVel = car->body->getLocalPointVelocity(data.position);
	vec3f vLocalVel = car->body->worldToLocalNormal(vWorldVel + vGroundWind);
	vec3f vWingWorld = car->body->localToWorld(data.position);
	status.groundHeight = car->getPointGroundHeight(vWingWorld);
  
	float fAngle = status.angle;
	if (overrideStatus.isActive)
		status.angle = overrideStatus.overrideAngle;
	
	if (vLocalVel.z == 0.0f)
	{
		status.aoa = 0;
		status.yawAngle = 0;
		status.cd = 0;
		status.cl = 0;
	}
	else
	{
		status.aoa = atanf((1.0f / vLocalVel.z) * vLocalVel.y) * 57.29578f;
		status.yawAngle = atanf((1.0f / vLocalVel.z) * vLocalVel.x) * 57.29578f;
		addDrag(vLocalVel);
		addLift(vLocalVel);
	}
  
	if (overrideStatus.isActive) // TODO: ???
		status.angle = fAngle;
}

void Wing::stepDynamicControllers(float dt)
{
	float fAngle = status.inputAngle;
	for (auto& dc : dynamicControllers)
	{
		dc->step();
		
		if (dc->combinatorMode == WingControllerCombinator::Add)
		{
			fAngle += dc->outputAngle;
		}
		else if (dc->combinatorMode == WingControllerCombinator::Mult)
		{
			fAngle *= dc->outputAngle;
		}

		fAngle = tclamp(fAngle, dc->downLimit, dc->upLimit);
	}
	status.angle = fAngle;
}

void Wing::addDrag(const vec3f& lv)
{
	float fAngleOff = data.isVertical ? status.yawAngle : status.aoa;
	status.cd = data.lutAOA_CD.getValue((status.angleMult * status.angle) + fAngleOff) * data.cdGain;

	if (hasDamage) // TODO: implement
	{
	}

	if (data.lutGH_CD.getCount())
	{
		float fLut = data.lutGH_CD.getValue(status.groundHeight);
		status.groundEffectDrag = fLut;
		status.cd *= fLut;
	}

	float fDot = lv.sqlen();
	float fDrag = (((fDot * status.cd) * car->aeroMap->airDensity) * data.area) * 0.5f;
	status.dragKG = fDrag * 0.10197838f;

	if (fDot != 0.0f)
	{
		car->body->addLocalForceAtLocalPos(lv.get_norm() * -fDrag, data.position);
	}
}

void Wing::addLift(const vec3f& lv)
{
	float fAngleOff, fAxis;

	if (data.isVertical)
	{
		fAngleOff = status.yawAngle;
		fAxis = lv.x;
	}
	else
	{
		fAngleOff = status.aoa;
		fAxis = lv.y;
	}

	status.cl = data.lutAOA_CL.getValue((status.angleMult * status.angle) + fAngleOff) * data.clGain;

	if (lv.z < 0.0f)
		status.cl = 0;

	if (!data.isVertical && data.yawGain != 0.0f)
	{
		float v8 = (sinf(fabsf(status.yawAngle) * 0.017453f) * data.yawGain) + 1.0f;
		status.cl *= tclamp(v8, 0.0f, 1.0f);
	}
  
	if (data.lutGH_CL.getCount())
	{
		float fLut = data.lutGH_CL.getValue(status.groundHeight);
		status.groundEffectLift = fLut;
		status.cl *= fLut;
	}
  
	if (hasDamage) // TODO: implement
	{
	}

	float fDot = (fAxis * fAxis) + (lv.z * lv.z);
	float fLift = (((fDot * status.cl) * car->aeroMap->airDensity) * data.area) * 0.5f;
	status.liftKG = fLift * 0.10197838f;
  
	if (fDot != 0.0f)
	{
		// TODO: check

		vec3f vNorm = lv.get_norm();
		vec3f vOut = data.isVertical ? vec3f(-vNorm.z, 0, vNorm.x) : vec3f(0, vNorm.z, -vNorm.y);
		vec3f vForce = vOut * -fLift;

		status.liftVector = vForce;
		car->body->addLocalForceAtLocalPos(vForce, data.position);
	}
}

float Wing::getPointFrontShare(const vec3f& p)
{
	auto p0 = car->suspensions[0]->getBasePosition();
	auto p2 = car->suspensions[2]->getBasePosition();
	return (1.0f - (p.z - p0.z) / (p2.z - p0.z));
}

}
