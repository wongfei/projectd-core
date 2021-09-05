#include "Input/DIDevice.h"

namespace D {

DIDevice::DIDevice()
{
	TRACE_CTOR(DIDevice);
}

DIDevice::~DIDevice()
{
	TRACE_DTOR(DIDevice);

	release();
}

bool DIDevice::init(HWND window, LPDIRECTINPUT8 di, const DIDEVICEINSTANCE& _info)
{
	release();

	info = _info;
	name = info.tszInstanceName;
	guid = strw(info.guidInstance);

	log_printf(L"DIDevice: init: %s", name.c_str());

	do
	{
		IDirectInputDevice8* _device = nullptr;
		auto hr = di->CreateDevice(info.guidInstance, &_device, nullptr);
		HR_GUARD_WARN(hr, break);
		device.reset(_device);

		hr = device->SetDataFormat(&c_dfDIJoystick);
		HR_GUARD_WARN(hr, break);

		hr = device->SetCooperativeLevel(window, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
		if (FAILED(hr))
		{
			hr = device->SetCooperativeLevel(window, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
			if (FAILED(hr))
			{
				hr = device->SetCooperativeLevel(window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
			}
		}
		HR_GUARD_WARN(hr, break);

		DI_MAKE_PROP(DIPROPRANGE, p);
		p.lMin = -10000;
		p.lMax = 10000;
		hr = device->SetProperty(DIPROP_RANGE, &p.diph);
		HR_GUARD_WARN(hr, break);

		// SUCCESS
		return true;
	}
	while (0);

	// ERROR
	release();
	return false;
}

void DIDevice::release()
{
	if (constantForceStarted || isDamperStarted)
		sendFF(0.0f, 0.0f);

	constantForceStarted = false;
	isDamperStarted = false;
	isFFInitialized = false;

	fxConstantForce.reset();
	fxDamper.reset();
	device.reset();
}

bool DIDevice::poll()
{
	if (!device)
		return false;

	auto hr = device->Poll();
	if (FAILED(hr))
	{
		constantForceStarted = false;
		isDamperStarted = false;

		hr = device->Acquire();
		HR_GUARD_WARN(hr, return false);

		log_printf(L"DIDevice: acquired device: %s", name.c_str());
	}

	memzero(joystate);
	hr = device->GetDeviceState(sizeof(joystate), &joystate);
	HR_GUARD_WARN(hr, return false);

	const float scale = isNormalized ? 0.0001f : 1.0f;

	axes[0] = joystate.lX * scale;
	axes[1] = joystate.lY * scale;
	axes[2] = joystate.lZ * scale;
	axes[3] = joystate.lRx * scale;
	axes[4] = joystate.lRy * scale;
	axes[5] = joystate.lRz * scale;
	axes[6] = joystate.rglSlider[0] * scale;
	axes[7] = joystate.rglSlider[1] * scale;

	for (int i = 0; i < PovCount; ++i)
		povs[i] = joystate.rgdwPOV[i];

	for (int i = 0; i < ButtonCount; ++i)
		buttons[i] = joystate.rgbButtons[i];

	return true;
}

template<typename T>
inline T getElem(const T* data, int id, int max)
{
	if (id >= 0 && id < max)
		return data[id];
	return T(0);
}

float DIDevice::getInput(EInputType type, int id)
{
	switch (type)
	{
		case EInputType::Axis: return (float)getElem(axes, id, AxisCount);
		case EInputType::Pov: return (float)getElem(povs, id, PovCount);
		case EInputType::Button: return (float)getElem(buttons, id, ButtonCount);
	}
	return 0;
}

bool DIDevice::initFF()
{
	if (isFFInitialized)
		return true;

	log_printf(L"DIDevice: initFF: %s", name.c_str());

	// autocenter

	DI_MAKE_PROP(DIPROPDWORD, p);
	p.diph.dwHow = DIPH_DEVICE;
	p.dwData = FALSE;
	
	auto hr = device->SetProperty(DIPROP_AUTOCENTER, &p.diph);
	if (FAILED(hr))
	{
		log_printf(L"DIDevice: SetProperty failed: DIPROP_AUTOCENTER (hr=0x%X)", hr);
		// ignore error
	}

	// constant

	DWORD axes[] = {0};
	LONG dir[] = {0};
	DICONSTANTFORCE cf = {0};

	DIEFFECT fx;
	memzero(fx);
	fx.dwSize = sizeof(fx);
	fx.dwFlags = 18;
	fx.dwDuration = INFINITE;
	fx.dwSamplePeriod = 0;
	fx.dwGain = 10000;
	fx.dwTriggerButton = DIEB_NOTRIGGER;
	fx.dwTriggerRepeatInterval = 0;
	fx.cAxes = 1;
	fx.rgdwAxes = axes;
	fx.rglDirection = dir;
	fx.lpEnvelope = 0;
	fx.cbTypeSpecificParams = sizeof(cf);
	fx.lpvTypeSpecificParams = &cf;
	fx.dwStartDelay = 0;

	IDirectInputEffect* pFx = nullptr;
	hr = device->CreateEffect(GUID_ConstantForce, &fx, &pFx, nullptr);
	if (FAILED(hr))
	{
		log_printf(L"DIDevice: CreateEffect failed: GUID_ConstantForce (hr=0x%X)", hr);
		return false;
	}
	fxConstantForce.reset(pFx);

	// damper

	DICONDITION cond = {
		0,
		10000,
		10000,
		10000,
		10000,
		0
	};

	memzero(fx);
	fx.dwSize = sizeof(fx);
	fx.dwFlags = 18;
	fx.dwDuration = INFINITE;
	fx.dwSamplePeriod = 0;
	fx.dwGain = 10000;
	fx.dwTriggerButton = DIEB_NOTRIGGER;
	fx.dwTriggerRepeatInterval = 0;
	fx.cAxes = 1;
	fx.rgdwAxes = axes;
	fx.rglDirection = dir;
	fx.lpEnvelope = 0;
	fx.cbTypeSpecificParams = sizeof(cond);
	fx.lpvTypeSpecificParams = &cond;
	fx.dwStartDelay = 0;

	pFx = nullptr;
	hr = device->CreateEffect(GUID_Damper, &fx, &pFx, nullptr);
	if (FAILED(hr))
	{
		log_printf(L"DIDevice: CreateEffect failed: GUID_Damper (hr=0x%X)", hr);
		return false;
	}
	fxDamper.reset(pFx);

	isFFInitialized = true;
	return true;
}

void DIDevice::sendFF(float v, float damper)
{
	if (!isFFInitialized)
		return;

	// constant

	v = tclamp(v * 10000.0f, -10000.0f, 10000.0f);

	DWORD axes[] = {0};
	LONG dir[] = {1};
	DICONSTANTFORCE cf = {(LONG)v};

	DIEFFECT fx;
	memzero(fx);
	fx.dwSize = sizeof(fx);
	fx.dwFlags = 18;
	fx.cAxes = 1;
	fx.rgdwAxes = axes;
	fx.rglDirection = dir;
	fx.cbTypeSpecificParams = sizeof(cf);
	fx.lpvTypeSpecificParams = &cf;

    if (constantForceStarted)
	{
		auto hr = fxConstantForce->SetParameters(&fx, DIEP_TYPESPECIFICPARAMS);
		GUARD_FATAL(hr == S_OK);
	}
	else if (fabsf(v) > 0.0001f)
	{
		auto hr = fxConstantForce->SetParameters(&fx, DIEP_START | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS);
		GUARD_FATAL(hr == S_OK);
		constantForceStarted = true;
	}

	lastConstantForce = v;

	// damper

	DICONDITION cond = {
		0,
		(LONG)(damper * 10000.0f),
		(LONG)(damper * 10000.0f),
		10000,
		10000,
		0
	};

	memzero(fx);
	fx.dwSize = sizeof(fx);
	fx.dwFlags = 18;
	fx.dwGain = 10000;
	fx.cAxes = 1;
	fx.rgdwAxes = axes;
	fx.rglDirection = dir;
	fx.cbTypeSpecificParams = sizeof(cond);
	fx.lpvTypeSpecificParams = &cond;

    if (isDamperStarted)
	{
		auto hr = fxDamper->SetParameters(&fx, DIEP_TYPESPECIFICPARAMS);
		GUARD_FATAL(hr == S_OK);
	}
	else if (fabsf(damper) > 0.0001f)
	{
		auto hr = fxDamper->SetParameters(&fx, DIEP_START | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS);
		GUARD_FATAL(hr == S_OK);
		isDamperStarted = true;
	}

	lastDamper = damper;
}

}
