#include "Car/SetupManager.h"
#include "Car/CarImpl.h"
#include <fstream>

namespace D {

SetupManager::SetupManager() {}
SetupManager::~SetupManager() { removeVars(); }

void SetupManager::init(struct Car* car)
{
	//addVar("ENGINE_LIMITER", "", 0.01, 1.0f);
	//addVar("FF_GAIN", "%", 1.0, 1.0, &car->userFFGain);
	//addVar("STEER_ASSIST", "", 0.01, 1.0, car->steerAssist);

	// BRAKES

	addVar("FRONT_BIAS", "%", 0.01, 1.0, &car->brakeSystem->frontBias);
	addVar("BRAKE_POWER_MULT", "%", 0.01, 1.0, &car->brakeSystem->brakePowerMultiplier);

	// DRIVETRAIN

	addVar("DIFF_POWER", "%", 0.01, 1.0, &car->drivetrain->diffPowerRamp); // differential lock under power. 1.0=100% lock - 0 0% lock 
	addVar("DIFF_COAST", "%", 0.01, 1.0, &car->drivetrain->diffCoastRamp); // differential lock under coasting. 1.0=100% lock 0=0% lock
	addVar("DIFF_PRELOAD", "Nm", 1.0, 1.0, &car->drivetrain->diffPreLoad); // preload torque
	addVar("FRONT_DIFF_POWER", "%", 0.01, 1.0, &car->drivetrain->awdFrontDiff.power);
	addVar("FRONT_DIFF_COAST", "%", 0.01, 1.0, &car->drivetrain->awdFrontDiff.coast);
	addVar("FRONT_DIFF_PRELOAD", "Nm", 1.0, 1.0, &car->drivetrain->awdFrontDiff.preload);
	addVar("REAR_DIFF_POWER", "%", 0.01, 1.0, &car->drivetrain->awdRearDiff.power);
	addVar("REAR_DIFF_COAST", "%", 0.01, 1.0, &car->drivetrain->awdRearDiff.coast);
	addVar("REAR_DIFF_PRELOAD", "Nm", 1.0, 1.0, &car->drivetrain->awdRearDiff.preload);
	addVar("CENTER_DIFF_POWER", "%", 0.01, 1.0, &car->drivetrain->awdCenterDiff.power);
	addVar("CENTER_DIFF_COAST", "%", 0.01, 1.0, &car->drivetrain->awdCenterDiff.coast);
	addVar("CENTER_DIFF_PRELOAD", "Nm", 1.0, 1.0, &car->drivetrain->awdCenterDiff.preload);
	addVar("AWD_FRONT_TORQUE_DISTRIBUTION", "%", 0.01, 1.0, &car->drivetrain->awdFrontShare);
	//addVar("COAST_TORQUE_MULT", "", 0.01, 1.0, &car->drivetrain->engineModel->coastTorqueMultiplier);

	const int ngears = (int)car->drivetrain->gears.size();
	for (int i = 0; i < ngears; ++i)
	{
		std::string name(straf("INTERNAL_GEAR_%d", i));
		addVar(name, "", 1.0, 1.0, &car->drivetrain->gears[i].ratio);
	}

	addVar("FINAL_RATIO", "", 1.0, 1.0, &car->drivetrain->finalRatio);

	// SUSPENSION

	addVar("ARB_FRONT", "N/m", 1.0, 1.0, &car->antirollBars[0]->k);
	addVar("ARB_REAR", "N/m", 1.0, 1.0, &car->antirollBars[1]->k);

	std::string type;
	const char* types[] = {"LF", "RF", "LR", "RR"};
	const double sides[] = {-1, 1, -1, 1};

	for (int i = 0; i < 4; ++i)
	{
		type.assign(types[i]);

		auto* susp = (SuspensionBase*)car->suspensions[i];
		auto* damper = susp->getDamper();

		addVar("DAMP_FAST_BUMP_" + type, "", 1.0, 1.0, &damper->bumpFast);
		addVar("DAMP_BUMP_" + type, "", 1.0, 1.0, &damper->bumpSlow); // Damper wheel rate stifness in N sec/m in compression
		addVar("DAMP_FAST_REBOUND_" + type, "", 1.0, 1.0, &damper->reboundFast);
		addVar("DAMP_REBOUND_" + type, "", 1.0, 1.0, &damper->reboundSlow); // Damper wheel rate stifness in N sec/m in rebound

		addVar("BUMP_STOP_RATE_" + type, "", 1000.0, 1.0, &susp->bumpStopRate); // Bump stop spring rate
		addVar("SPRING_RATE_" + type, "N/mm", 1000.0, 1.0, &susp->k); // Wheel rate stifness in Nm
		addVar("PROGRESSIVE_SPRING_RATE_" + type, "", 1000.0, 1.0, &susp->progressiveK); // Progressive spring rate in N/m
		addVar("ROD_LENGTH_" + type, "mm", 0.0001, 1.0, &susp->rodLength); // Push rod length in meters, positive raises ride height, negative lowers ride height
		addVar("CAMBER_" + type, "deg", 0.0017453292 * sides[i], 0.1f, &susp->staticCamber); // Static Camber in degrees
		addVar("TOE_OUT_" + type, "", 0.00001, 1.0, &susp->toeOUT_Linear); // Toe-out expressed as the length of the steering arm in meters
		addVar("PACKER_RANGE_" + type, "mm", 0.001, 1.0, &susp->packerRange); // Total suspension movement range, before hitting packers
	}

	// TYRES

	for (int i = 0; i < 4; ++i)
	{
		type.assign(types[i]);

		addVar("PRESSURE_" + type, "psi", 1.0, 1.0, &car->tyres[i]->status.pressureStatic);
	}

	// ENGINE

	addVar("ENGINE_LIMITER", "%", 0.01, 1.0f, &car->drivetrain->engineModel->limiterMultiplier);

	const int nturbos = (int)car->drivetrain->engineModel->turbos.size();
	for (int i = 0; i < nturbos; ++i)
	{
		std::string name(straf("TURBO_%d", i));

		auto* turbo = car->drivetrain->engineModel->turbos[i].get();
		auto* var = addVar(name.c_str(), "%", 0.01, 1.0, &turbo->userSetting);
		var->tab = "ENGINE";

		var->minV = 0.0f;
		var->maxV = 1.0f;
		var->step = 0.1f;
		var->tunable = true;
	}

	// WINGS

	const int nwings = (int)car->aeroMap->wings.size();
	for (int i = 0; i < nwings; ++i)
	{
		std::string name(straf("WING_%d", i));

		auto* wing = car->aeroMap->wings[i].get();
		if (wing->data.hasController)
		{
			addVar(name.c_str(), "deg", 1.0, 1.0, &wing->status.inputAngle);
		}
		else
		{
			addVar(name.c_str(), "deg", 1.0, 1.0, &wing->status.angle);
		}
	}

	// setup.ini

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"setup.ini"));
	if (ini->ready)
	{
		std::wstring ratiosFile;
		if (ini->tryGetString(L"FINAL_GEAR_RATIO", L"RATIOS", ratiosFile))
		{
			auto ratios = SetupGearRatio::load(car->carDataPath + ratiosFile);
			if (!ratios.empty())
			{
				auto* var = getVar("FINAL_RATIO");
				var->tab = "GEARS";
				var->spinnerType = SpinnerType::RawPredefined;
				var->spinnerValues.clear();

				float minR = FLT_MAX;
				float maxR = 0;

				for (const auto& r : ratios)
				{
					minR = std::min(minR, r.value);
					maxR = std::max(maxR, r.value);
					var->spinnerValues.push_back(r.value);
				}

				var->mult = 1;
				var->dispMult = 1;
				var->minV = minR;
				var->maxV = maxR;
				var->step = 0.01f;
				var->tunable = true;
			}
		}

		for (auto* var : vvars)
		{
			std::wstring section(strw(var->name));
			if (ini->hasSection(section))
			{
				std::wstring tab;
				if (ini->tryGetString(section, L"TAB", tab))
					var->tab = stra(tab);

				bool tunable = 
					ini->tryGetFloat(section, L"MIN", var->minV) &&
					ini->tryGetFloat(section, L"MAX", var->maxV) &&
					ini->tryGetFloat(section, L"STEP", var->step);

				int iShowClicks = 0;
				if (ini->tryGetInt(section, L"SHOW_CLICKS", iShowClicks))
				{
					switch (iShowClicks)
					{
						case (int)SpinnerType::Default:       var->spinnerType = SpinnerType::Default; break;
						case (int)SpinnerType::Clicks:        var->spinnerType = SpinnerType::Clicks; break;
						case (int)SpinnerType::ClicksAbs:     var->spinnerType = SpinnerType::ClicksAbs; break;
						case (int)SpinnerType::RawFloat:      var->spinnerType = SpinnerType::RawFloat; break;
						case (int)SpinnerType::RawPredefined: var->spinnerType = SpinnerType::RawPredefined; break;
						default: log_printf(L"INVALID setup.ini [%S] SHOW_CLICKS=%d", var->name.c_str(), iShowClicks); break;
					}
				}

				if (tunable)
					var->tunable = true;
			}
		}
	}

	for (auto* var : vvars)
	{
		if (var->tunable)
		{
			if (var->minV >= var->maxV || fabs(var->maxV - var->minV) < 0.01)
			{
				log_printf(L"INVALID setup.ini [%S] MIN=%f MAX=%f", var->name.c_str(), var->minV, var->maxV);
				var->minV = 0;
				var->maxV = 0;
				var->tunable = false;
			}

			if (var->step < 0.0f)
			{
				log_printf(L"INVALID setup.ini [%S] STEP=%f", var->name.c_str(), var->step);
				var->step = 0.0f;
				var->tunable = false;
			}
		}

		if (!var->tunable)
		{
			var->tab = "_HIDDEN";
			var->spinnerType = SpinnerType::RawFloat;
		}
	}

	std::sort(vvars.begin(), vvars.end(), [](SetupVar* a, SetupVar* b) -> bool
	{
		return (a->tab < b->tab) 
			|| (a->tab == b->tab && a->order < b->order) 
			|| (a->tab == b->tab && a->order == b->order && a->name < b->name);
	});
}

void SetupManager::removeVars()
{
	std::vector<SetupVar*> tmp;
	std::swap(tmp, vvars);
	mvars.clear();

	for (auto* var : tmp)
		delete var;
}

SetupVar* SetupManager::addVar(const std::string& name, const std::string& units, double mult, double dispMult, float* fvalue)
{
	auto* var = new SetupVar(name, units, fvalue, (float)mult, (float)dispMult);
	addVar(var);
	return var;
}

SetupVar* SetupManager::addVar(const std::string& name, const std::string& units, double mult, double dispMult, double* dvalue)
{
	auto* var = new SetupVar(name, units, dvalue, (float)mult, (float)dispMult);
	addVar(var);
	return var;
}

void SetupManager::addVar(SetupVar* var)
{
	GUARD_FATAL(var->fvalue || var->dvalue);
	GUARD_FATAL(var->mult != 0.0);
	GUARD_FATAL(var->dispMult != 0.0);

	vvars.push_back(var);
	mvars[var->name] = var;
	var->defV = var->getRaw();

	if (var->name.find("_LR") != std::string::npos 
		|| var->name.find("_RR") != std::string::npos)
	{
		var->order = 2;
	}
}

SetupVar* SetupManager::getVar(const std::string& name)
{
	auto iter = mvars.find(name);
	if (iter != mvars.end())
		return iter->second;
	return nullptr;
}

void SetupManager::setRawTune(const std::string& name, float value)
{
	auto* var = getVar(name);
	if (var)
		var->setRaw(value);
}

void SetupManager::setTune(const std::string& name, float value)
{
	auto* var = getVar(name);
	if (var)
		var->setTune(value);
}

//=================================================================================================
// SetupVar
//=================================================================================================

inline float truncToFloat(float x) { return (float)truncToInt(x); }

SetupSpinner SetupVar::getSpinner() const
{
	return getSpinner(spinnerType);
}

SetupSpinner SetupVar::getSpinner(SpinnerType type) const
{
	SetupSpinner spinner;

	float rawValue = getRaw();
	float uiValue = rawValue / mult;
	float outValue = 0;

	// dumb and dumber..
	switch (type)
	{
		case SpinnerType::Default:
			spinner.minV = truncToFloat(minV);
			spinner.maxV = truncToFloat(maxV);
			spinner.step = truncToFloat(tmax(1.0f, step));
			outValue = truncToFloat(uiValue + 0.5f);
			break;

		case SpinnerType::Clicks:
			spinner.minV = truncToFloat(minV / step);
			spinner.maxV = truncToFloat(maxV / step);
			spinner.step = 1.0f;
			outValue = truncToFloat(uiValue / step + 0.5f);
			break;

		case SpinnerType::ClicksAbs:
			spinner.minV = 0.0f;
			spinner.maxV = truncToFloat((maxV - minV) / step);
			spinner.step = 1.0f;
			outValue = truncToFloat((uiValue - minV) / step + 0.5f);
			break;

		case SpinnerType::RawFloat:
		case SpinnerType::RawPredefined:
			spinner.minV = minV;
			spinner.maxV = maxV;
			spinner.step = step;
			outValue = rawValue;
			break;
	}

	spinner.value = outValue;
	spinner.type = type;

	return spinner;
}

void SetupVar::setValue(const SetupSpinner& spinner)
{
	float rawValue = 0;

	switch (spinner.type)
	{
		case SpinnerType::Default:         rawValue = spinner.value * mult; break;
		case SpinnerType::Clicks:          rawValue = (spinner.value * step) * mult; break;
		case SpinnerType::ClicksAbs:       rawValue = (spinner.value * step + minV) * mult; break;
		case SpinnerType::RawFloat:        rawValue = spinner.value; break;
		case SpinnerType::RawPredefined:   rawValue = spinner.value; break;
		default: return;
	}

	if (spinner.type == SpinnerType::RawPredefined)
	{
		const int nvalues = (int)spinnerValues.size();
		if (nvalues > 0)
		{
			int id = getSpinnerValueIdx(rawValue);
			id = tclamp(id, 0, nvalues - 1);
			rawValue = spinnerValues[id];
		}
	}

	setRaw(rawValue);
}

int SetupVar::getSpinnerValueIdx(float value) const
{
	int bestId = 0;
	float bestDist = FLT_MAX;

	for (int i = 0; i < (int)spinnerValues.size(); ++i)
	{
		const float diff = fabsf(spinnerValues[i] - value);
		if (diff < bestDist)
		{
			bestDist = diff;
			bestId = i;
		}
	}

	return bestId;
}

void SetupVar::setTune(float v)
{
	auto spinner = getSpinner();
	spinner.value = tclamp(v, spinner.minV, spinner.maxV);
	setValue(spinner);
}

float SetupVar::getTune() const
{
	return getSpinner().value;
}

//=================================================================================================
// SetupGearRatio
//=================================================================================================

std::vector<SetupGearRatio> SetupGearRatio::load(const std::wstring& filename)
{
	std::vector<SetupGearRatio> res;

	std::wifstream file(filename);
	if (!file.is_open())
	{
		log_printf(L"File not found: %s", filename.c_str());
		return res;
	}

	std::wstring line;
	while (std::getline(file, line))
	{
		// 10//50|5.0000
		if (!line.empty())
		{
			auto kv = split(line, L"|");
			if (kv.size() == 2)
			{
				res.push_back(SetupGearRatio{stra(kv[0]), std::stof(kv[1])});
			}
			else
			{
				log_printf(L"Invalid GearRatio: %s", line.c_str());
			}
		}
	}

	std::sort(res.begin(), res.end(), [](const SetupGearRatio& a, const SetupGearRatio& b) -> bool { return a.value < b.value; });

	return res;
}

}
