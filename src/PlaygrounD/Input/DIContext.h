#pragma once

#include "Input/DICommon.h"
#include <vector>

namespace D {

struct DIDevice;

struct DIContext : public virtual IObject
{
	DIContext();
	~DIContext();

	bool init(HWND window);
	void release();
	void stopFF();
	DIDevice* findDeviceByName(const std::wstring& name);
	DIDevice* findDeviceByGuid(const std::wstring& guid);
	DIDevice* findDeviceBySubstring(const std::wstring& str);
	DIDevice* findDeviceByType(unsigned int type = 0, unsigned int subtype = 0);

	HWND window = nullptr;
	std::unique_ptr<IDirectInput8, ComDtor> dinput;
	std::vector<DIDEVICEINSTANCE> availableDevices;
	std::vector<std::unique_ptr<DIDevice>> devices;
};

}
