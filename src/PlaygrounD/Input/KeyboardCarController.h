#pragma once

#include "Car/CarCommon.h"
#include "Car/ICarControlsProvider.h"
#include "Input/InputVariable.h"
#include <vector>

namespace D {

struct KeyboardCarController : public ICarControlsProvider
{
	KeyboardCarController(const std::wstring& basePath, Car* car, void* windowHandle = nullptr);
	~KeyboardCarController();

	// ICarControlsProvider
	void acquireControls(CarControlsInput* input, CarControls* controls, float dt) override;
	void sendFF(float ff, float damper, float userGain) override {}
	void setVibrations(VibrationDef* vib) override {}
	float getFeedbackFilter() override { return 0; }

	float getKeyboardSteering(CarControlsInput* input, float dt);
	float getMouseSteering(CarControlsInput* input, float dt);
	float computeGasCoefficient(CarControlsInput* input, float dt);

	// config
	bool keyboardEnabled = true;
	bool mouseSteering = false;
	bool mouseAcceleratorBrake = false;

	int keyGas = 0x57; // W
	int keyBrake = 0x53; // S
	int keyLeft = 0x41; // A
	int keyRight = 0x44; // D
	int keyGearUp = 0x52; // R
	int keyGearDn = 0x46; // F

	float gasPedalSpeed = 1;
	float mouseSpeed = 1000;
	Curve mouseSpeedCurve;
	Curve kbSteerSpeedCurve;
	Curve kbSteerCenterCurve;

	// runtime
	std::wstring basePath;
	Car* car = nullptr;
	void* windowHandle = nullptr;
	float oldSteer = 0;
	float oldMouseValue = 0;
	float kbSteer = 0;
	float intGas = 0;
};

}
