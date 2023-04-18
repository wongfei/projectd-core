#pragma once

#include "Car/CarCommon.h"
#include "Car/ICarControlsProvider.h"
#include "Input/InputVariable.h"
#include <vector>

namespace D {

struct DIContext;
struct DIDevice;

struct FFUpgrades
{
	float curbsGain = 0.08f;
	float gforceGain = 0.08f;
	float slipsGain = 0.08f;
	float absGain = 0.08f;
};

struct DICarController : public ICarControlsProvider
{
	DICarController(const std::wstring& basePath, void* windowHandle = nullptr);
	~DICarController();

	void initVars();
	void loadConfig();
	void addVar(InputVariable* var, std::wstring name);
	void bind(InputVariable* var, DIDevice* device, EInputType type, int id);

	// ICarControlsProvider
	void acquireControls(CarControlsInput* input, CarControls* controls, float dt) override;
	void sendFF(float ff, float damper, float userGain) override;
	void setVibrations(VibrationDef* vib) override;
	float getFeedbackFilter() override { return ffFilter; }

	// config
	int pollSkipFrames = 1;
	float steerLock = 900;
	float steerScale = 1;
	float steerLinearity = 1;
	float steerFilter = 0;
	float speedSensitivity = 0;
	float brakeGamma = 2.4f;

	bool ffEnabled = false;
	bool ffSoftLock = false;
	float ffGain = 0.85f;
	float ffFilter = 0;
	float ffMin = 0.08f;
	FFUpgrades ffUpgrades;
	float centerBoostGain = 0;
	float centerBoostRange = 0.1f;

	// runtime
	std::wstring basePath;
	std::unique_ptr<DIContext> diContext;
	DIDevice* diWheel = nullptr;
	DIDevice* diHandbrake = nullptr;

	InputVariable varSteer;
	InputVariable varClutch;
	InputVariable varBrake;
	InputVariable varHbrake;
	InputVariable varGas;

	InputVariable varGearUp;
	InputVariable varGearDown;
	InputVariable varGear[6];
	InputVariable varGearR;

	std::vector<InputVariable*> inputVars;
	std::vector<DIDevice*> activeDevices;

	float lastSpeed = 0;
	float currentLock = 0;
	float currentVibration = 0;
	int ffInterval = 0;
	int ffCounter = 0;
	bool isShifterManual = true;
	int pollSkipCounter = 0;
};

}
