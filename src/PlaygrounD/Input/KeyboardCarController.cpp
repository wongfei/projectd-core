#include "Input/KeyboardCarController.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"
#include "Car/Tyre.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace D {

KeyboardCarController::KeyboardCarController(const std::wstring& _basePath, void* _windowHandle)
{
	TRACE_CTOR(KeyboardCarController);

	basePath = _basePath;
	windowHandle = _windowHandle;

	auto ini(std::make_unique<INIReader>(basePath + L"cfg/keyboard.ini"));
	if (ini->ready)
	{
		mouseSteering = (ini->getInt(L"BASIC", L"MOUSE_STEER") != 0);
		mouseAcceleratorBrake = (ini->getInt(L"BASIC", L"MOUSE_GAS_BRAKE") != 0);

		keyGas = ini->getInt(L"KEYS", L"GAS");
		keyBrake = ini->getInt(L"KEYS", L"BRAKE");
		keyLeft = ini->getInt(L"KEYS", L"LEFT");
		keyRight = ini->getInt(L"KEYS", L"RIGHT");
		keyGearUp = ini->getInt(L"KEYS", L"GEAR_UP");
		keyGearDn = ini->getInt(L"KEYS", L"GEAR_DOWN");
	}

	std::wstring strName(L"cfg/mouse_steer.lut");
	if (osFileExists(strName))
	{
		mouseSpeedCurve.load(strName);
	}

	strName = L"cfg/kb_steer.lut";
	if (osFileExists(strName))
	{
		kbSteerSpeedCurve.load(strName);
	}

	strName = L"cfg/kb_steer_center.lut";
	if (osFileExists(strName))
	{
		kbSteerCenterCurve.load(strName);
	}
}

KeyboardCarController::~KeyboardCarController()
{
	TRACE_DTOR(KeyboardCarController);
}

void KeyboardCarController::setCar(struct Car* _car)
{
	car = _car;
}

void KeyboardCarController::acquireControls(CarControlsInput* input, CarControls* controls, float dt)
{
	if (!car)
		return;

	// TODO: no one cares about peasants playing simulator on keyboard :D

	float fSteer;
	if (mouseSteering)
		fSteer = getMouseSteering(input, dt);
	else
		fSteer = getKeyboardSteering(input, dt);

	oldSteer = controls->steer;
	controls->steer = tclamp(fSteer, -1.0f, 1.0f);

	controls->clutch = 1.0f;

	if (keyboardEnabled && GetAsyncKeyState(keyGas)
		|| mouseSteering && mouseAcceleratorBrake && GetAsyncKeyState(1))
	{
		float fGas = computeGasCoefficient(input, dt);
		controls->gas = tclamp(fGas, 0.0f, 1.0f);
	}
	else
	{
		controls->gas = 0.0;
		intGas = 0.0;
	}

	if (keyboardEnabled && GetAsyncKeyState(keyBrake)
		|| mouseSteering && mouseAcceleratorBrake && GetAsyncKeyState(2))
	{
		float fBrake = car->getOptimalBrake();
		controls->brake = tclamp(fBrake, 0.0f, 1.0f);
	}
	else
	{
		controls->brake = 0.0;
	}

	controls->gearUp = keyboardEnabled && GetAsyncKeyState(keyGearUp);
	controls->gearDn = keyboardEnabled && GetAsyncKeyState(keyGearDn);
}

float KeyboardCarController::getKeyboardSteering(CarControlsInput* input, float dt)
{
	float fSteerDir = 0.0f;
	if (keyboardEnabled && GetAsyncKeyState(keyLeft))
	{
		fSteerDir = -1.0f;
	}
	else if (keyboardEnabled && GetAsyncKeyState(keyRight))
	{
		fSteerDir = 1.0f;
	}

	float fCarSpeed = car->speed.kmh();
	float fSteerSpeed = kbSteerSpeedCurve.getCount() > 0 ? kbSteerSpeedCurve.getValue(fCarSpeed) : 1;
	float fCenterSpeed =  kbSteerCenterCurve.getCount() > 0 ? kbSteerCenterCurve.getValue(fCarSpeed) : 1;

	kbSteer += fSteerDir * dt * fSteerSpeed;

	if (fabsf(fSteerDir) < 0.01f)
		kbSteer -= kbSteer * dt * fCenterSpeed;

	return kbSteer;
}

float KeyboardCarController::getMouseSteering(CarControlsInput* input, float dt)
{
	RECT rect;
	POINT mousePos;

	GetWindowRect((HWND)windowHandle, &rect);
	GetCursorPos(&mousePos);

	float fCarSpeed = car->speed.kmh();
	float fMouseSpeed = mouseSpeedCurve.getCount() > 0 ? mouseSpeedCurve.getValue(fCarSpeed) : mouseSpeed;

	LONG v5 = (rect.left + (rect.right - rect.left) / 2);
	float v6 = ((mousePos.x - v5) / fMouseSpeed) + oldMouseValue;
	oldMouseValue = tclamp(v6, -1.0f, 1.0f);

	SetCursorPos((int)v5, rect.bottom - 100);

	return oldMouseValue;
}

float KeyboardCarController::computeGasCoefficient(CarControlsInput* input, float dt)
{
	float fGasDt = dt * gasPedalSpeed;

	float fMaxSlip;
	if (car->drivetrain->tractionType == TractionType::FWD)
	{
		fMaxSlip = tmax(car->tyres[0]->status.ndSlip, car->tyres[1]->status.ndSlip);
	}
	else
	{
		fMaxSlip = tmax(car->tyres[2]->status.ndSlip, car->tyres[3]->status.ndSlip);
	}

	bool v11;
	if (car->speed.kmh() < 100.0f)
		v11 = (fMaxSlip <= 0.99);
	else
		v11 = (fMaxSlip <= 2.0);

	if (v11)
	{
		intGas = tclamp(intGas + fGasDt, 0.0f, 1.0f);
	}
	else
	{
		intGas = tclamp(intGas - fGasDt, 0.65f, 1.0f);
	}

	return intGas;
}

}
