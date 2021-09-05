#include "Car/WingDynamicController.h"
#include "Car/Car.h"

namespace D {

WingDynamicController::WingDynamicController()
{}

WingDynamicController::~WingDynamicController()
{}

WingDynamicController::WingDynamicController(Car* _car, INIReader* ini, const std::wstring& section)
{
	init(_car, ini, section);
}

void WingDynamicController::init(Car* _car, INIReader* ini, const std::wstring& section)
{
	car = _car;

	inputVar = WingControllerVariable::Undefined;
	combinatorMode = WingControllerCombinator::Undefined;

	std::map<std::wstring, WingControllerVariable> inputMap;

	#define ADD_KV(str, val)\
		inputMap.insert({str, WingControllerVariable::val});

	ADD_KV(L"BRAKE", Brake);
	ADD_KV(L"GAS", Gas);
	ADD_KV(L"LATG", LatG);
	ADD_KV(L"LONG", LonG);
	ADD_KV(L"STEER", Steer);
	ADD_KV(L"SPEED_KMH", Speed);
	ADD_KV(L"SUS_TRAVEL_LR", SusTravelLR);
	ADD_KV(L"SUS_TRAVEL_RR", SusTravelRR);
	#undef ADD_KV

	auto strInput = ini->getString(section, L"INPUT");
	auto strCombinator = ini->getString(section, L"COMBINATOR");

	auto iterInput = inputMap.find(strInput);
	if (iterInput != inputMap.end())
		inputVar = iterInput->second;

	if (strCombinator == L"ADD")
		combinatorMode = WingControllerCombinator::Add;
	else if (strCombinator == L"MULT")
		combinatorMode = WingControllerCombinator::Mult;

	if (inputVar == WingControllerVariable::Undefined || 
		combinatorMode == WingControllerCombinator::Undefined)
	{
		SHOULD_NOT_REACH_WARN;
		return;
	}

	auto strLut = car->carDataPath + ini->getString(section, L"LUT");
	lut.load(strLut);

	filter = ((1.0f - ini->getFloat(section, L"FILTER")) * 1.3333334f) * 333.33334f;
	upLimit = ini->getFloat(section, L"UP_LIMIT");
	downLimit = ini->getFloat(section, L"DOWN_LIMIT");
}

void WingDynamicController::step()
{
	float fInput = getInput();
	float fAngle = lut.getValue(fInput);

	if (fabsf(fAngle - outputAngle) >= 0.001f)
	{
		fAngle = outputAngle + (fAngle - outputAngle) * tclamp(filter * 0.003f, 0.0f, 1.0f);
	}

	outputAngle = fAngle;
}

float WingDynamicController::getInput()
{
	switch (inputVar)
	{
		case WingControllerVariable::Brake:
			return car->controls.brake;

		case WingControllerVariable::Gas:
			return car->controls.gas;

		case WingControllerVariable::LatG:
			return car->accG.x;

		case WingControllerVariable::LonG:
			return car->accG.z;

		case WingControllerVariable::Steer:
			return car->controls.steer;

		case WingControllerVariable::Speed:
			return car->speed.kmh();

		case WingControllerVariable::SusTravelLR:
			return car->suspensions[2]->getStatus().travel;

		case WingControllerVariable::SusTravelRR:
			return car->suspensions[3]->getStatus().travel;

		default:
			SHOULD_NOT_REACH_FATAL;
	}
}

}
