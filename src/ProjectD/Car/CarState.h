#pragma once

#include "Car/CarControls.h"
#include "Core/Math.h"

namespace D {

struct CarState
{
	int32_t carId = 0;

	CarControls controls;

	float engineRPM = 0;
	float speedMS = 0;
	int32_t gear = 0;
	int32_t gearGrinding = 0;

	mat44f bodyMatrix;
	vec3f accG;
	vec3f velocity;
	vec3f localVelocity;
	vec3f angularVelocity;
	vec3f localAngularVelocity;

	mat44f hubMatrix[4];
	vec3f tyreContacts[4];
	float tyreSlip[4] = {};
	float tyreLoad[4] = {};
	float tyreAngularSpeed[4] = {};

	enum { MaxProbes = 10 };
	float probes[MaxProbes] = {};
	int32_t nearestTrackPointId = 0;

	#if 0
	mat44f graphicsMatrix;
	vec3f bodyPos;
	vec3f bodyEuler;
	vec3f graphicsPos;
	vec3f graphicsEuler;
	#endif
};

}
