#include "Core/OS.h"

#ifdef WIN32

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace D {

void osTraceDebug(const wchar_t* msg)
{
	OutputDebugStringW(msg);
	OutputDebugStringW(L"\n");
}

void osFatalExit(const wchar_t* msg)
{
	MessageBoxW(NULL, msg, L"ERROR", MB_OK | MB_ICONERROR);
	ExitProcess(666);
}

void* osLoadLibraryA(const char* path)
{
	return LoadLibraryA(path);
}

void* osLoadLibraryW(const wchar_t* path)
{
	return LoadLibraryW(path);
}

void* osGetProcAddress(void* lib, const char* name)
{
	return GetProcAddress((HMODULE)lib, name);
}

bool osFileExists(const std::wstring& path)
{
	DWORD dwAttrib = GetFileAttributesW(path.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool osDirExists(const std::wstring& path)
{
	DWORD dwAttrib = GetFileAttributesW(path.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void osEnsureDirExists(const std::wstring& path)
{
	if (!osDirExists(path.c_str()))
	{
		CreateDirectoryW(path.c_str(), nullptr);
	}
}

void osCreateDirectoryTree(const std::wstring& path)
{
	std::wstring::size_type pos = 0;
	do
	{
		pos = path.find_first_of(L"\\/", pos + 1);
		osEnsureDirExists(path.substr(0, pos).c_str());
	}
	while (pos != std::wstring::npos);
}

std::wstring osGetDirPath(const std::wstring& path)
{
	std::wstring res;
	const int len = (int)path.length();
	for (int i = len - 1; i >= 0; --i)
	{
		if (path[i] == L'\\' || path[i] == L'/')
		{
			res = path.substr(0, (size_t)i + 1);
			break;
		}
	}
	return std::move(res);
}

std::wstring osGetFileName(const std::wstring& path)
{
	std::wstring res;
	const int len = (int)path.length();
	for (int i = len - 1; i >= 0; --i)
	{
		if (path[i] == L'\\' || path[i] == L'/')
		{
			res = path.substr((size_t)i + 1);
			break;
		}
	}
	return std::move(res);
}

struct OSFindWindowData
{
	LPCWSTR ClassName = nullptr;
	LPCWSTR Title = nullptr;
	DWORD ProcessId = 0;
	HWND OutWindow = nullptr;
};

static BOOL osIsMainWindow(HWND Window)
{   
	return ((GetWindow(Window, GW_OWNER) == (HWND)nullptr) && IsWindowVisible(Window));
}

static BOOL CALLBACK osEnumWindowsCallback(HWND Window, OSFindWindowData* Param)
{
	wchar_t Name[1024];

	if (Param->ClassName)
	{
		GetClassNameW(Window, Name, _countof(Name) - 1);
		if (wcscmp(Name, Param->ClassName) != 0) return TRUE; // continue search
	}

	if (Param->Title)
	{
		GetWindowTextW(Window, Name, _countof(Name) - 1);
		if (wcscmp(Name, Param->Title) != 0) return TRUE; // continue search
	}

	if (Param->ProcessId)
	{
		DWORD WinProcessId = 0;
		GetWindowThreadProcessId(Window, &WinProcessId);
		if (Param->ProcessId != WinProcessId || !osIsMainWindow(Window)) return TRUE; // continue search
	}

	Param->OutWindow = Window;
	return FALSE; // done
}

void* osFindWindow(const wchar_t* ClassName, const wchar_t* Title)
{
	OSFindWindowData Param;
	Param.ClassName = ClassName;
	Param.Title = Title;

	EnumWindows((WNDENUMPROC)osEnumWindowsCallback, (LPARAM)&Param);
	return Param.OutWindow;
}

void* osFindProcessWindow(unsigned int ProcessId)
{
	if (!ProcessId)
		ProcessId = GetCurrentProcessId();

	OSFindWindowData Param;
	Param.ProcessId = ProcessId;

	EnumWindows((WNDENUMPROC)osEnumWindowsCallback, (LPARAM)&Param);
	return Param.OutWindow;
}

bool FileHandle::open(const wchar_t* filename, const wchar_t* mode)
{
	close();
	_wfopen_s(&fd, filename, mode);
	return (fd != nullptr);
}

void FileHandle::close()
{
	if (fd)
	{
		fclose(fd);
		fd = nullptr;
	}
}

}

#else // NOT WIN32

#error "Platform not supported"

#endif // WIN32
