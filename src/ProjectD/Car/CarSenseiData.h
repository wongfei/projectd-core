#pragma once

#include "Car/CarControls.h"
#include "Core/Math.h"
#include <array>

namespace D {

#pragma pack(push, 4)

struct CarSenseiData
{
	CarControls controls;

	mat44f bodyMatrix;
	vec3f worldSplinePosition;
	vec3f velocity;
	vec3f localVelocity;
	vec3f angularVelocity;
	vec3f localAngularVelocity;
	
	int32_t trackPointId = 0;
	float trackLocation = 0;
	float bodyVsTrack = 0;
	float velocityVsTrack = 0;

	int32_t gear = 0;
	float engineRPM = 0;
	float speedMS = 0;
};

#pragma pack(pop)

}
