#include "Car/DynamicController.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"
#include "Car/Tyre.h"

namespace D {

inline float lagToLerpDeltaK(float lag, float orgdt, float dt) { return (((1.0f / dt) * orgdt) * (1.0f - lag)) * (1.0f / dt); }

DynamicController::DynamicController(Car* _car, const std::wstring& filename)
{
	init(_car, filename);
}

DynamicController::DynamicController()
{}

DynamicController::~DynamicController()
{}

void DynamicController::init(Car* _car, const std::wstring& filename)
{
	car = _car;

	auto ini(std::make_unique<INIReader>(filename));
	GUARD_FATAL(ini->ready);

	std::map<std::wstring, DynamicControllerVariable> inputMap;

	#define ADD_KV(str, val)\
		inputMap.insert({str, DynamicControllerVariable::val});

	ADD_KV(L"BRAKE", Brake);
	ADD_KV(L"GAS", Gas);
	ADD_KV(L"LATG", LatG);
	ADD_KV(L"LONG", LonG);
	ADD_KV(L"STEER", Steer);
	ADD_KV(L"SPEED_KMH", Speed);
	ADD_KV(L"GEAR", Gear);
	ADD_KV(L"SLIPRATIO_MAX", SlipRatioMAX);
	ADD_KV(L"SLIPRATIO_AVG", SlipRatioAVG);
	ADD_KV(L"SLIPANGLE_FRONT_AVG", SlipAngleFrontAVG);
	ADD_KV(L"SLIPANGLE_REAR_AVG", SlipAngleRearAVG);
	ADD_KV(L"SLIPANGLE_FRONT_MAX", SlipAngleFrontMAX);
	ADD_KV(L"SLIPANGLE_REAR_MAX", SlipAngleRearMAX);
	ADD_KV(L"OVERSTEER_FACTOR", OversteerFactor);
	ADD_KV(L"REAR_SPEED_RATIO", RearSpeedRatio);
	ADD_KV(L"STEER_DEG", SteerDEG);
	ADD_KV(L"CONST", Const);
	ADD_KV(L"RPMS", RPMS);
	ADD_KV(L"WHEEL_STEER_DEG", WheelSteerDEG);
	ADD_KV(L"LOAD_SPREAD_LF", LoadSpreadLF);
	ADD_KV(L"LOAD_SPREAD_RF", LoadSpreadRF);
	ADD_KV(L"AVG_TRAVEL_REAR", AvgTravelRear);
	ADD_KV(L"SUS_TRAVEL_LR", SusTravelLR);
	ADD_KV(L"SUS_TRAVEL_RR", SusTravelRR);
	#undef ADD_KV

	for (int id = 0; ; ++id)
	{
		auto strId = strwf(L"CONTROLLER_%d", id);
		if (!ini->hasSection(strId))
			break;

		auto strInput = ini->getString(strId, L"INPUT");
		auto iterInput = inputMap.find(strInput);
		if (iterInput == inputMap.end())
		{
			SHOULD_NOT_REACH_WARN;
			continue;
		}

		auto eCombinator = DynamicControllerCombinator::Undefined;
		auto strCombinator = ini->getString(strId, L"COMBINATOR");

		if (strCombinator == L"ADD")
			eCombinator = DynamicControllerCombinator::Add;
		else if (strCombinator == L"MULT")
			eCombinator = DynamicControllerCombinator::Mult;
		else
		{
			SHOULD_NOT_REACH_WARN;
			continue;
		}

		auto pStage(std::make_unique<DynamicControllerStage>());
		
		pStage->inputVar = iterInput->second;
		pStage->combinatorMode = eCombinator;

		pStage->filter = lagToLerpDeltaK(ini->getFloat(strId, L"FILTER"), 0.004f, 0.003f); // TODO: check
		pStage->upLimit = ini->getFloat(strId, L"UP_LIMIT");
		pStage->downLimit = ini->getFloat(strId, L"DOWN_LIMIT");

		if (pStage->inputVar == DynamicControllerVariable::Const)
		{
			pStage->constValue = ini->getFloat(strId, L"CONST_VALUE");
		}
		else
		{
			pStage->lut = ini->getCurve(strId, L"LUT");
		}

		stages.emplace_back(std::move(pStage));
	}

	ready = !stages.empty();
}

float DynamicController::eval()
{
	float fRes = 0;

	for (auto& pStage : stages)
	{
		float fCurValue = pStage->currentValue;
		float fNewValue = 0;

		if (pStage->inputVar == DynamicControllerVariable::Const)
		{
			fNewValue = pStage->constValue;
		}
		else
		{
			float fInput = getInput(pStage->inputVar);
			fNewValue = pStage->lut.getValue(fInput);
		}

		if (fabsf(fNewValue - fCurValue) >= 0.001f)
		{
			float fFilter = tclamp(pStage->filter * 0.003f, 0.0f, 1.0f);
			fNewValue = ((fNewValue - fCurValue) * fFilter) + fCurValue;
		}

		pStage->currentValue = fNewValue;

		switch (pStage->combinatorMode)
		{
			case DynamicControllerCombinator::Add:
				fRes += fNewValue;
				break;
			case DynamicControllerCombinator::Mult:
				fRes *= fNewValue;
				break;
		}

		if (pStage->downLimit != 0.0f || pStage->upLimit != 0.0f)
		{
			fRes = tclamp(fRes, pStage->downLimit, pStage->upLimit);
		}
	}

	return fRes;
}

float DynamicController::getInput(DynamicControllerVariable input)
{
	switch (input)
	{
		case DynamicControllerVariable::Brake:
			return car->controls.brake;

		case DynamicControllerVariable::Gas:
			return car->controls.gas;

		case DynamicControllerVariable::LatG:
			return car->accG.x;

		case DynamicControllerVariable::LonG:
			return car->accG.z;

		case DynamicControllerVariable::Steer:
			return car->controls.steer;

		case DynamicControllerVariable::Speed:
			return car->speed.kmh();

		case DynamicControllerVariable::Gear:
			return (float)(car->drivetrain->currentGear - 1);

		case DynamicControllerVariable::SlipRatioMAX:
			if (car->drivetrain->tractionType == TractionType::RWD)
			{
				return tmax(car->tyres[2]->status.slipRatio, car->tyres[3]->status.slipRatio);
			}
			else if (car->drivetrain->tractionType == TractionType::FWD)
			{
				return tmax(car->tyres[0]->status.slipRatio, car->tyres[1]->status.slipRatio);
			}
			else
			{
				return tmax(
					tmax(car->tyres[0]->status.slipRatio, car->tyres[1]->status.slipRatio),
					tmax(car->tyres[2]->status.slipRatio, car->tyres[3]->status.slipRatio));
			}

		case DynamicControllerVariable::SlipRatioAVG:
			if (car->drivetrain->tractionType == TractionType::RWD)
			{
				return (car->tyres[2]->status.slipRatio + car->tyres[3]->status.slipRatio) * 0.5f;
			}
			else if (car->drivetrain->tractionType == TractionType::FWD)
			{
				return (car->tyres[0]->status.slipRatio + car->tyres[1]->status.slipRatio) * 0.5f;
			}
			else
			{
				return (
					car->tyres[0]->status.slipRatio + car->tyres[1]->status.slipRatio +
					car->tyres[2]->status.slipRatio + car->tyres[3]->status.slipRatio) * 0.25f;
			}

		case DynamicControllerVariable::SlipAngleFrontAVG:
			return ((car->tyres[0]->status.slipAngleRAD + car->tyres[1]->status.slipAngleRAD) * 57.29578f) * 0.5f;

		case DynamicControllerVariable::SlipAngleRearAVG:
			return ((car->tyres[2]->status.slipAngleRAD + car->tyres[3]->status.slipAngleRAD) * 57.29578f) * 0.5f;

		case DynamicControllerVariable::SlipAngleFrontMAX:
			return tmax<float>(fabsf(car->tyres[0]->status.slipAngleRAD), fabsf(car->tyres[1]->status.slipAngleRAD)) * 57.29578f;

		case DynamicControllerVariable::SlipAngleRearMAX:
			return tmax<float>(fabsf(car->tyres[2]->status.slipAngleRAD), fabsf(car->tyres[3]->status.slipAngleRAD)) * 57.29578f;

		case DynamicControllerVariable::OversteerFactor:
			return DynamicController::getOversteerFactor(car);

		case DynamicControllerVariable::RearSpeedRatio:
			return DynamicController::getRearSpeedRatio(car);

		case DynamicControllerVariable::SteerDEG:
			return car->steerLock * car->controls.steer;

		case DynamicControllerVariable::RPMS:
			return car->drivetrain->getEngineRPM();

		case DynamicControllerVariable::WheelSteerDEG:
			return car->finalSteerAngleSignal;

		case DynamicControllerVariable::LoadSpreadLF:
			return car->tyres[0]->status.load / (car->tyres[0]->status.load + car->tyres[1]->status.load);

		case DynamicControllerVariable::LoadSpreadRF:
			return car->tyres[1]->status.load / (float)(car->tyres[1]->status.load + car->tyres[0]->status.load);

		case DynamicControllerVariable::AvgTravelRear:
			return (car->suspensions[2]->getStatus().travel + car->suspensions[3]->getStatus().travel) * 0.5f * 1000.0f;

		case DynamicControllerVariable::SusTravelLR:
			return car->suspensions[2]->getStatus().travel * 1000.0f;

		case DynamicControllerVariable::SusTravelRR:
			return car->suspensions[3]->getStatus().travel * 1000.0f;

		default:
			SHOULD_NOT_REACH_FATAL;
	}
}

float DynamicController::getOversteerFactor(Car* car)
{
	return
		((fabsf(car->tyres[2]->status.slipAngleRAD) + fabsf(car->tyres[3]->status.slipAngleRAD) * 0.5f) -
		(fabsf(car->tyres[0]->status.slipAngleRAD) + fabsf(car->tyres[1]->status.slipAngleRAD) * 0.5f)) * 57.29578f;
}

float DynamicController::getRearSpeedRatio(Car* car)
{
	float result = 0;
	float front = (car->tyres[1]->status.angularVelocity + car->tyres[0]->status.angularVelocity) * 0.5f;
	if (front != 0.0f)
		result = ((car->tyres[3]->status.angularVelocity + car->tyres[2]->status.angularVelocity) * 0.5f) / front;
	return result;
}

}
