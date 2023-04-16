#include "Core/SharedMemory.h"
#include "Core/Diag.h"

#ifdef WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace D {

SharedMemory::SharedMemory()
{
}

SharedMemory::~SharedMemory()
{
	close();
}

void SharedMemory::allocate(const wchar_t* name, size_t size)
{
	close();

	mappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, name);
	if (!mappingHandle)
	{
		log_printf(L"ERROR: CreateFileMapping failed: code=0x%X", GetLastError());
	}
	else
	{
		dataPtr = MapViewOfFile(mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, (DWORD)size);
		if (!dataPtr)
		{
			log_printf(L"ERROR: MapViewOfFile failed: code=0x%X", GetLastError());
		}
		else
		{
			memset(dataPtr, 0, size);
		}
	}
}

void SharedMemory::open(const wchar_t* name, size_t size)
{
	close();

	mappingHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (!mappingHandle)
	{
		log_printf(L"ERROR: CreateFileMapping failed: code=0x%X", GetLastError());
	}
	else
	{
		dataPtr = MapViewOfFile(mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, (DWORD)size);
		if (!dataPtr)
		{
			log_printf(L"ERROR: MapViewOfFile failed: code=0x%X", GetLastError());
		}
	}
}

void SharedMemory::close()
{
	if (dataPtr)
	{
		UnmapViewOfFile(dataPtr);
		dataPtr = nullptr;
	}

	if (mappingHandle)
	{
		CloseHandle(mappingHandle);
		mappingHandle = nullptr;
	}
}

}

#else // NOT WIN32

#error "Platform not supported"

#endif // WIN32
