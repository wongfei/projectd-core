#pragma once

#include "Car/CarControls.h"
#include "Core/Math.h"

namespace D {

struct CarState
{
	CarControls controls;

	mat44f bodyMatrix = {};
	vec3f bodyPos = {};
	vec3f bodyEuler = {};

	mat44f graphicsMatrix = {};
	vec3f graphicsPos = {};
	vec3f graphicsEuler = {};

	mat44f hubMatrix[4] = {};
	vec3f tyreContacts[4] = {};
	
	float speedMS = 0;
	float engineRPM = 0;
	int gear = 0;
};

}
