#include "Input/DICarController.h"
#include "Input/DIContext.h"
#include "Input/DIDevice.h"

namespace D {

DICarController::DICarController(void* windowHandle)
{
	TRACE_CTOR(DICarController);

	initVars();

	if (!windowHandle)
		windowHandle = osFindProcessWindow(0);

	diContext.reset(new DIContext());
	diContext->init((HWND)windowHandle);

	loadConfig();
}

DICarController::~DICarController()
{
	TRACE_DTOR(DICarController);
}

void DICarController::initVars()
{
	addVar(&varSteer, L"STEER");
	addVar(&varClutch, L"CLUTCH");
	addVar(&varBrake, L"BRAKE");
	addVar(&varHbrake, L"HBRAKE");
	addVar(&varGas, L"GAS");

	addVar(&varGearUp, L"GEAR_UP");
	addVar(&varGearDown, L"GEAR_DOWN");

	for (int i = 0; i < 6; ++i)
		addVar(&varGear[i], strwf(L"GEAR_%d", i + 1));

	addVar(&varGearR, L"GEAR_R");
}

void DICarController::loadConfig()
{
	auto ini(std::make_unique<INIReader>(L"cfg/dinput.ini"));
	if (!ini->ready)
		return;

	const auto strSteerDevice = ini->getString(L"STEER", L"DEVICE");
	diWheel = diContext->findDeviceBySubstring(strSteerDevice);
	if (!diWheel)
	{
		log_printf(L"Input device not found: %s", strSteerDevice.c_str());
		return;
	}

	steerLock = ini->getFloat(L"STEER", L"STEER_LOCK") * 0.5f;
	steerScale = ini->getFloat(L"STEER", L"STEER_SCALE");
	steerLinearity = ini->getFloat(L"STEER", L"STEER_LINEARITY");
	steerFilter = ini->getFloat(L"STEER", L"STEER_FILTER");
	speedSensitivity = ini->getFloat(L"STEER", L"SPEED_SENSITIVITY");
	brakeGamma = ini->getFloat(L"BRAKE", L"GAMMA");

	ffEnabled = (ini->getInt(L"STEER", L"FF_ENABLED") != 0);
	ffSoftLock = (ini->getInt(L"STEER", L"FF_SOFT_LOCK") != 0);
	ffGain = ini->getFloat(L"STEER", L"FF_GAIN");
	ffFilter = ini->getFloat(L"STEER", L"FF_FILTER");
	ffMin = ini->getFloat(L"STEER", L"FF_MIN");
	ffUpgrades.curbsGain = ini->getFloat(L"STEER", L"FF_CURBS");
	ffUpgrades.gforceGain = ini->getFloat(L"STEER", L"FF_ROAD");
	ffUpgrades.slipsGain = ini->getFloat(L"STEER", L"FF_SLIPS");
	ffUpgrades.absGain = ini->getFloat(L"STEER", L"FF_ABS");
	centerBoostGain = ini->getFloat(L"STEER", L"FF_CENTER_BOOST_GAIN");
	centerBoostRange = ini->getFloat(L"STEER", L"FF_CENTER_BOOST_RANGE");

	for (auto& var : inputVars)
	{
		auto strInput = ini->getString(var->name, L"INPUT");
		auto strDevice = ini->getString(var->name, L"DEVICE", false);
		auto fMin = ini->getFloat(var->name, L"MIN", false);
		auto fMax = ini->getFloat(var->name, L"MAX", false);

		if (strDevice.empty())
			strDevice = strSteerDevice;

		auto* pDev = diContext->findDeviceBySubstring(strDevice);
		if (!pDev)
		{
			log_printf(L"Input device not found: %s", strDevice.c_str());
			continue;
		}

		EInputType eType = EInputType(0);
		std::wstring strId;

		if (strInput.find(L"AXIS_") == 0) { eType = EInputType::Axis; strId = strInput.substr(5, std::wstring::npos); }
		else if (strInput.find(L"BTN_") == 0) { eType = EInputType::Button; strId = strInput.substr(4, std::wstring::npos); }
		else if (strInput.find(L"POV_") == 0) { eType = EInputType::Pov; strId = strInput.substr(4, std::wstring::npos); }
		else
		{
			log_printf(L"Invalid INPUT: %s", strInput.c_str());
			continue;
		}

		if (eType == EInputType::Axis)
			var->setRange(fMin, fMax);

		int iId = std::stoi(strId);

		log_printf(L"bind %s -> %s (%s)", var->name.c_str(), strInput.c_str(), pDev->name.c_str());
		bind(var, pDev, eType, iId);
	}

	if (ffEnabled)
	{
		diWheel->initFF();
	}
}

template<typename T>
inline void insert_uniq(std::vector<T>& vec, T item)
{
	if (std::find(vec.begin(), vec.end(), item) == vec.end())
		vec.emplace_back(item);
}

void DICarController::addVar(InputVariable* var, std::wstring name)
{
	var->name = std::move(name);
	insert_uniq(inputVars, var);
}

void DICarController::bind(InputVariable* var, DIDevice* device, EInputType type, int id)
{
	var->device = device;
	var->type = type;
	var->id = id;
	insert_uniq(activeDevices, device);
}

void DICarController::acquireControls(CarControlsInput* input, CarControls* controls, float dt)
{
	lastSpeed = input->speed;

	for (auto* dev : activeDevices)
		dev->poll();

	const float fSteerAxis = tclamp(varSteer.getInput(), -1.0f, 1.0f);
	const float fClutchAxis = tclamp(varClutch.getInput(), 0.0f, 1.0f);
	const float fBrakeAxis = tclamp(varBrake.getInput(), 0.0f, 1.0f);
	const float fHandBrake = tclamp(varHbrake.getInput(), 0.0f, 1.0f);
	const float fGasAxis = tclamp(varGas.getInput(), 0.0f, 1.0f);

	float fSteer = fSteerAxis * steerScale;

	if (steerLinearity != 1.0f)
	{
		float fSteerPow = powf(fabsf(fSteer), steerLinearity);
		fSteer = fSteerPow * signf(fSteer);
	}

	if (steerLock > input->steerLock && input->steerLock != 0.0f)
		fSteer *= (steerLock / input->steerLock);

	if (speedSensitivity != 0.0f)
		fSteer /= ((speedSensitivity * input->speed) + 1.0f);

	if (steerFilter > 0.0f)
	{
		float fFilterDt = tclamp(steerFilter * dt, 0.0f, 1.0f);
		fSteer = ((fSteer - controls->steer) * fFilterDt) + controls->steer;
	}

	currentLock = fSteer;

	controls->steer = tclamp(fSteer, -1.0f, 1.0f);
	controls->clutch = 1.0f - fClutchAxis;
	controls->brake = tclamp(powf(fBrakeAxis, brakeGamma), 0.0f, 1.0f);
	controls->handBrake = fHandBrake;
	controls->gas = fGasAxis;

	int manualGearId = 1; // R=0, N=1, G1=2, ...

	for (int i = 0; i < 6; ++i)
	{
		if (varGear[i].getInputBool())
		{
			manualGearId = i + 2;
			break;
		}
	}

	if (varGearR.getInput())
		manualGearId = 0;

	if (isShifterManual)
	{
		controls->requestedGearIndex = (int8_t)manualGearId;

		if ((varGearUp.getInputBool() || varGearDown.getInputBool()) && manualGearId == 1)
		{
			// switch to sequental
			isShifterManual = false;
			controls->requestedGearIndex = -1;
		}
	}
	else // sequental
	{
		if (manualGearId == 1)
		{
			controls->gearUp = varGearUp.getInputBool();
			controls->gearDn = varGearDown.getInputBool();
		}
		else
		{
			// switch to manual
			isShifterManual = true;
		}
	}
}

void DICarController::sendFF(float ff, float damper, float userGain)
{
	if (!ffEnabled || !varSteer.device)
		return;

	const float fSteerAxis = varSteer.getInput();

	if (centerBoostGain != 0.0f)
	{
		float fSteerAxisAbs = fabsf(fSteerAxis);
		if (fSteerAxisAbs < centerBoostRange)
		{
			ff *= ((1.0f - (fSteerAxisAbs / centerBoostRange)) * centerBoostGain);
		}
	}

	float v11 = ((lastSpeed * 3.6f) - 1.0f) * 0.2f;
	v11 = tclamp(v11, 0.0f, 1.0f);
	float v12 = v11 * ffMin;

	if (fabsf(ff) < v12)
	{
		float v14 = signf(ff);
		ff = v14 * v12;
	}

	if (ffCounter < ffInterval)
	{
		ffCounter++;
	}
	else
	{
		ffCounter = 0;
		if (!ffEnabled)
			ff = 0;

		if (ffSoftLock && fabsf(currentLock) > 1.0f)
		{
			float v = signf(currentLock);
			varSteer.device->sendFF(v, 1.0f);
		}
		else
		{
			float v = ((ff + currentVibration) * ffGain) * userGain;
			// TODO: FFPostProcessor
			varSteer.device->sendFF(v, damper);
		}
	}
}

void DICarController::setVibrations(VibrationDef* vib)
{
	float v = 
		vib->curbs * ffUpgrades.curbsGain +
		vib->gforce * ffUpgrades.gforceGain +
		vib->slips * ffUpgrades.slipsGain +
		vib->abs * ffUpgrades.absGain;

	currentVibration = tclamp(v, -1.0f, 1.0f);
}

}
