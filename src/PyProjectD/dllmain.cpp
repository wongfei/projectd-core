#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Core/Diag.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			D::log_printf(L"DLL_PROCESS_ATTACH");
			//srand(666);
			//D::log_init(L"pyprojectd.log");
			break;

		case DLL_PROCESS_DETACH:
			D::log_printf(L"DLL_PROCESS_DETACH");
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}

	return TRUE;
}
