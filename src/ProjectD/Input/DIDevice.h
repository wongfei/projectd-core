#pragma once

#include "Input/DICommon.h"
#include "Input/IGameController.h"

namespace D {

struct DIDevice : public IGameController
{
	DIDevice();
	~DIDevice();

	bool init(HWND window, LPDIRECTINPUT8 di, const DIDEVICEINSTANCE& info);
	bool initFF();
	void release();
	bool poll();

	// IGameController
	float getInput(EInputType type, int id) override;
	void sendFF(float v, float damper) override;

	DIDEVICEINSTANCE info = {};
	std::wstring name;
	std::wstring guid;

	std::unique_ptr<IDirectInputDevice8, ComDtor> device;
	std::unique_ptr<IDirectInputEffect, ComDtor> fxConstantForce;
	std::unique_ptr<IDirectInputEffect, ComDtor> fxDamper;

	enum { AxisCount = 8, PovCount = 4, ButtonCount = 32 };
	DIJOYSTATE joystate = {};
	float axes[AxisCount] = {};
	uint32_t povs[PovCount] = {};
	uint8_t buttons[ButtonCount] = {};
	
	bool isNormalized = true;
	bool isFFInitialized = false;
	bool constantForceStarted = false;
	bool isDamperStarted = false;
	float lastConstantForce = 0;
	float lastDamper = 0;
};

}
