#pragma once

#include "Car/CarCommon.h"

namespace D {

enum class WingControllerVariable
{
	Undefined = 0x0,
	Brake = 0x1,
	Gas = 0x2,
	LatG = 0x3,
	LonG = 0x4,
	Steer = 0x5,
	Speed = 0x6,
	SusTravelLR = 0x7,
	SusTravelRR = 0x8,
};

enum class WingControllerCombinator
{
	Undefined = 0x0,
	Add = 0x1,
	Mult = 0x2,
};

struct WingDynamicController : public NonCopyable
{
	WingDynamicController();
	~WingDynamicController();
	WingDynamicController(Car* car, INIReader* ini, const std::wstring& section);
	void init(Car* car, INIReader* ini, const std::wstring& section);
	void step();
	float getInput();

	// config
	WingControllerVariable inputVar = WingControllerVariable(0);
	WingControllerCombinator combinatorMode = WingControllerCombinator(0);
	Curve lut;
	float filter = 0;
	float upLimit = 0;
	float downLimit = 0;

	// runtime
	Car* car = nullptr;
	float outputAngle = 0;
};

}
