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
	float timestamp = 0;

	CarControls controls;

	int32_t collisionFlag = 0;
	int32_t outOfTrackFlag = 0;
	int32_t trackPointId = 0;
	float lastTrackPointTimestamp = 0;
	float trackLocation = 0;
	float bodyVsTrack = 0;
	float velocityVsTrack = 0;

	float engineRPM = 0;
	float speedMS = 0;
	int32_t gear = 0; // 0=R, 1=N, 2=H1, 3=H2, 4=H3, 5=H4, 6=H5, 7=H6
	int32_t gearGrinding = 0;

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
	std::array<float, 4> tyreLoad;
	std::array<float, 4> tyreAngularSpeed;
	std::array<float, 4> tyreSlipRatio;
	std::array<float, 4> tyreNdSlip;

	enum { MaxProbes = 10 };
	std::array<float, MaxProbes> probes;

	enum { MaxLookAhead = 5 };
	std::array<float, MaxLookAhead> lookAhead;

	float stepReward = 0;
	float totalReward = 0;
};

#pragma pack(pop)

}
