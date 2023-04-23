#pragma once

#include "Car/CarControls.h"
#include "Core/Math.h"
#include <array>

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

	std::array<mat44f, 4> hubMatrix;
	std::array<vec3f, 4> tyreContacts;
	std::array<float, 4> tyreSlip;
	std::array<float, 4> tyreLoad;
	std::array<float, 4> tyreAngularSpeed;

	enum { MaxProbes = 10 };
	std::array<float, MaxProbes> probes;
	
	float trackLocation = 0;
	float agentScore = 0;

	#if 0
	mat44f graphicsMatrix;
	vec3f bodyPos;
	vec3f bodyEuler;
	vec3f graphicsPos;
	vec3f graphicsEuler;
	#endif
};

}
