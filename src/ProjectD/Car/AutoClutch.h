#pragma once

#include "Car/CarCommon.h"

namespace D {

struct ClutchSequence
{
	Curve clutchCurve;
	float currentTime = 0.0f;
	bool isDone = false;

	ClutchSequence();
	ClutchSequence(const Curve& curve);
};

struct AutoClutch : public NonCopyable
{
	AutoClutch();
	~AutoClutch();

	void init(Car* car);
	void step(float dt);
	void stepSequence(float dt);
	void onGearRequest(const struct OnGearRequestEvent& ev);

	Car* car = nullptr;
	ClutchSequence clutchSequence;
	Curve upshiftProfile;
	Curve downshiftProfile;
	float rpmMin = 1500;
	float rpmMax = 2500;
	float clutchSpeed = 1.0;
	float clutchValueSignal = 0;
	bool useAutoOnStart = false;
	bool useAutoOnChange = false;
	bool isForced = false;
};

}
