#pragma once

#include "Car/CarControls.h"
#include "Core/Math.h"
#include <array>

namespace D {

#pragma pack(push, 4)

struct CarState
{
	int32_t carId = 0;
	int32_t simId = 0;

	CarControls controls;

	float engineRPM = 0;
	float speedMS = 0;
	int32_t gear = 0; // 0=R, 1=N, 2=H1, 3=H2, 4=H3, 5=H4, 6=H5, 7=H6
	int32_t gearGrinding = 0;
	
	float trackLocation = 0;
	int32_t trackPointId = 0;
	int32_t collisionFlag = 0;
	int32_t outOfTrackFlag = 0;

	float bodyVsTrack = 0;
	float velocityVsTrack = 0;
	float agentDriftReward = 0;

	mat44f bodyMatrix;
	vec3f bodyPos;
	vec3f bodyEuler;
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
};

#pragma pack(pop)

}
