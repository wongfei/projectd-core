#include "Input/DIContext.h"
#include "Input/DIDevice.h"

namespace D {

DIContext::DIContext()
{
	TRACE_CTOR(DIContext);
}

DIContext::~DIContext()
{
	TRACE_DTOR(DIContext);

	release();
}

static BOOL DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	const auto type = (unsigned int)GET_DIDEVICE_TYPE(lpddi->dwDevType);
	const auto subtype = (unsigned int)GET_DIDEVICE_SUBTYPE(lpddi->dwDevType);
	const auto guid = strw(lpddi->guidInstance);

	log_printf(L"DEV: type=0x%X subtype=0x%X %s \"%s\"", type, subtype, guid.c_str(), lpddi->tszInstanceName);

	auto context = (DIContext*)pvRef;

	switch (type)
	{
		case DI8DEVTYPE_JOYSTICK:
		case DI8DEVTYPE_GAMEPAD:
		case DI8DEVTYPE_DRIVING:
		case DI8DEVTYPE_FLIGHT:
		case DI8DEVTYPE_1STPERSON:
		{
			context->availableDevices.emplace_back(*lpddi);
			break;
		}
	}

	return TRUE;
}

bool DIContext::init(HWND _window)
{
	release();

	window = _window;
	log_printf(L"DIContext: init: window=%p", window);

	do
	{
		log_printf(L"DIContext: DirectInput8Create");
		IDirectInput8* _dinput = nullptr;
		auto hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID*)&_dinput, nullptr);
		HR_GUARD_WARN(hr, break);
		dinput.reset(_dinput);

		log_printf(L"DIContext: EnumDevices");
		hr = dinput->EnumDevices(DI8DEVCLASS_GAMECTRL, DIEnumDevicesCallback, this, DIEDFL_ATTACHEDONLY);
		HR_GUARD_WARN(hr, break);

		for (auto& info : availableDevices)
		{
			auto dev(std::make_unique<DIDevice>());
			if (dev->init(window, dinput.get(), info))
			{
				devices.emplace_back(std::move(dev));
			}
		}

		// SUCCESS
		return true;
	}
	while (0);

	// ERROR
	release();
	return false;
}

void DIContext::release()
{
	availableDevices.clear();
	devices.clear();
	dinput.reset();
}

void DIContext::stopFF()
{
	log_printf(L"DIContext: stopFF");
	for (auto& dev : devices)
	{
		if (dev->constantForceStarted || dev->isDamperStarted)
			dev->sendFF(0.0f, 0.0f);
	}
}

DIDevice* DIContext::findDeviceByName(const std::wstring& name)
{
	for (auto& dev : devices)
	{
		if (dev->name == name)
			return dev.get();
	}

	return nullptr;
}

DIDevice* DIContext::findDeviceByGuid(const std::wstring& guid)
{
	for (auto& dev : devices)
	{
		if (dev->guid == guid)
			return dev.get();
	}

	return nullptr;
}

DIDevice* DIContext::findDeviceBySubstring(const std::wstring& str)
{
	for (auto& dev : devices)
	{
		if (dev->guid.find(str) == 0 || dev->name.find(str) == 0)
			return dev.get();
	}

	return nullptr;
}

DIDevice* DIContext::findDeviceByType(unsigned int type, unsigned int subtype)
{
	for (auto& dev : devices)
	{
		const auto dtype = (unsigned int)GET_DIDEVICE_TYPE(dev->info.dwDevType);
		const auto dsubtype = (unsigned int)GET_DIDEVICE_SUBTYPE(dev->info.dwDevType);

		if ((!type || type == dtype) && (!subtype || subtype == dsubtype))
			return dev.get();
	}

	return nullptr;
}

}
