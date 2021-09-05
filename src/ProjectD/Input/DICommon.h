#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "Core/Common.h"
#include "Core/Math.h"

#define HR_GUARD_WARN(hr, action)\
	if (FAILED(hr)) { SHOULD_NOT_REACH_WARN; D::log_printf(L"hr=0x%X", hr); action; }

#define DI_MAKE_PROP(type, name)\
	type name;\
	memzero(name.diph);\
	name.diph.dwSize = sizeof(type);\
	name.diph.dwHeaderSize = sizeof(DIPROPHEADER)

namespace D {

struct ComDtor
{
	void operator()(IUnknown* obj) const { obj->Release(); }
};

inline std::wstring strw(const GUID& guid)
{
	const int len = 256;
	TCHAR str[len]; str[0] = 0;
	StringFromGUID2(guid, str, len);
	return std::wstring(str);
}

}
