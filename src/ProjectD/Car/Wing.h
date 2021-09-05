#pragma once

#include "Car/CarCommon.h"

namespace D {

struct WingData
{
	std::wstring name;
	Curve lutAOA_CL;
	Curve lutAOA_CD;
	Curve lutGH_CL;
	Curve lutGH_CD;
	vec3f position;
	float clGain = 0;
	float cdGain = 0;
	float yawGain = 0;
	float chord = 0;
	float span = 0;
	float area = 0;
	bool isVertical = false;
	bool hasController = false;
};

struct WingState
{
	float inputAngle = 0;
	float angleMult = 1.0f;
	float angle = 0;
	float frontShare = 0;
	float groundHeight = 0;
	float aoa = 0;
	float yawAngle = 0;
	float cd = 0;
	float cl = 0;
	float dragKG = 0;
	float liftKG = 0;
	float groundEffectDrag = 0;
	float groundEffectLift = 0;
	vec3f liftVector;
};

struct WingOverrideDef
{
	float overrideAngle = 0;
	bool isActive = false;
};

struct Wing : public NonCopyable
{
	Wing();
	~Wing();
	Wing(Car* car, INIReader* ini, int index, bool isVertical);
	void init(Car* car, INIReader* ini, int index, bool isVertical);
	void step(float dt);
	void stepDynamicControllers(float dt);
	void addDrag(const vec3f& lv);
	void addLift(const vec3f& lv);
	float getPointFrontShare(const vec3f& p);

	// config
	std::vector<std::unique_ptr<WingDynamicController>> dynamicControllers;
	WingData data;
	float SPEED_DAMAGE_COEFF = 300.0f;
	float SURFACE_DAMAGE_COEFF = 300.0f;
	float damageCL[5] = {};
	float damageCD[5] = {};
	bool hasDamage = false;

	// runtime
	Car* car = nullptr;
	WingState status;
	WingOverrideDef overrideStatus;
};

}
