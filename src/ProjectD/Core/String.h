#pragma once

#include "Core/Core.h"
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>

namespace D {

inline std::string stra(const char* str) { return std::string(str); }
inline std::wstring strw(const wchar_t* str) { return std::wstring(str); }

std::string stra(const wchar_t* str);
std::string stra(const std::wstring& str);

std::wstring strw(const char* str);
std::wstring strw(const std::string& str);

std::string strafv(const char* format, va_list args);
std::string straf(const char* format, ...);

std::wstring strwfv(const wchar_t* format, va_list args);
std::wstring strwf(const wchar_t* format, ...);

std::vector<std::wstring> split(const std::wstring& s, const std::wstring& delim);
std::vector<std::string> split(const std::string& s, const std::string& delim);

}
