#pragma once

#include "Car/CarCommon.h"

namespace D {

struct AeroMap : public NonCopyable
{
	AeroMap();
	~AeroMap();
	void init(Car* car, const vec3f& frontAP, const vec3f& rearAP);
	void step(float dt);
	void addDrag(const vec3f& lv);
	void addLift(const vec3f& lv);
	float getCurrentDragKG();
	float getCurrentLiftKG();

	// config
	std::vector<std::unique_ptr<Wing>> wings;
	float referenceArea = 1.0f;
	float frontShare = 0.5f;
	float CD = 0;
	float CL = 0;
	float CDX = 0;
	float CDY = 0;
	float CDA = 0.1f;

	// runtime
	Car* car = nullptr;
	IRigidBody* carBody = nullptr;
	vec3f frontApplicationPoint;
	vec3f rearApplicationPoint;
	float airDensity = 1.221f;
	float dynamicCD = 0;
	float dynamicCL = 0;
};

}
