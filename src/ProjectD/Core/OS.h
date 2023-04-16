#pragma once

#include "Core/Core.h"
#include <string>

namespace D {

void osTraceDebug(const wchar_t* msg);
void osFatalExit(const wchar_t* msg);

void osSetCurrentDir(const std::wstring& path);
void* osLoadLibraryA(const char* path);
void* osLoadLibraryW(const wchar_t* path);
void* osGetProcAddress(void* lib, const char* name);

bool osFileExists(const std::wstring& path);
bool osDirExists(const std::wstring& path);

void osEnsureDirExists(const std::wstring& path);
void osCreateDirectoryTree(const std::wstring& path);

std::wstring osGetDirPath(const std::wstring& path);
std::wstring osGetFileName(const std::wstring& path);

void* osFindWindow(const wchar_t* ClassName, const wchar_t* Title);
void* osFindProcessWindow(unsigned int ProcessId);

struct FileHandle
{
	FILE* fd = nullptr;
	FileHandle() {}
	explicit FileHandle(FILE* _fd) : fd(_fd) {}
	~FileHandle() { close(); }
	bool open(const wchar_t* filename, const wchar_t* mode);
	void close();
	size_t size() const;
};

}
