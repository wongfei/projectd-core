#pragma once

#include "Core/Curve.h"
#include <memory>
#include <vector>

namespace D {

struct Car;

enum class DynamicControllerVariable
{
	Undefined = 0x0,
	Brake = 0x1,
	Gas = 0x2,
	LatG = 0x3,
	LonG = 0x4,
	Steer = 0x5,
	Speed = 0x6,
	Gear = 0x7,
	SlipRatioMAX = 0x8,
	SlipRatioAVG = 0x9,
	SlipAngleFrontAVG = 0xA,
	SlipAngleRearAVG = 0xB,
	SlipAngleFrontMAX = 0xC,
	SlipAngleRearMAX = 0xD,
	OversteerFactor = 0xE,
	RearSpeedRatio = 0xF,
	SteerDEG = 0x10,
	Const = 0x11,
	RPMS = 0x12,
	WheelSteerDEG = 0x13,
	LoadSpreadLF = 0x14,
	LoadSpreadRF = 0x15,
	AvgTravelRear = 0x16,
	SusTravelLR = 0x17,
	SusTravelRR = 0x18,
};

enum class DynamicControllerCombinator
{
	Undefined = 0x0,
	Add = 0x1,
	Mult = 0x2,
};

struct DynamicControllerStage
{
	// config
	DynamicControllerVariable inputVar = DynamicControllerVariable(0);
	DynamicControllerCombinator combinatorMode = DynamicControllerCombinator(0);
	float filter = 0;
	float upLimit = 0;
	float downLimit = 0;
	float constValue = 0;
	Curve lut;

	// runtime
	float currentValue = 0;
};

struct DynamicController : public NonCopyable
{
	DynamicController(Car* car, const std::wstring& filename);
	DynamicController();
	~DynamicController();

	void init(Car* car, const std::wstring& filename);
	float eval();
	float getInput(DynamicControllerVariable input);
	static float getOversteerFactor(Car* car);
	static float getRearSpeedRatio(Car* car);

	// config
	std::vector<std::unique_ptr<DynamicControllerStage>> stages;
	bool ready = false;

	// runtime
	Car* car = nullptr;
};

}
