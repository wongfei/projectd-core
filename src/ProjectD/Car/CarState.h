#pragma once

#include "Car/CarControls.h"
#include "Core/Math.h"

namespace D {

struct CarState
{
	int32_t carId = 0;
	int32_t gear = 0;
	float engineRPM = 0;
	float speedMS = 0;

	CarControls controls;

	mat44f bodyMatrix;
	vec3f bodyPos;
	vec3f bodyEuler;

	mat44f graphicsMatrix;
	vec3f graphicsPos;
	vec3f graphicsEuler;

	mat44f hubMatrix[4];
	vec3f tyreContacts[4];
};

}
