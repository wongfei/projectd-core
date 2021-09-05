#include "Core/String.h"
#include <codecvt>

namespace D {

typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter_t;

std::string stra(const wchar_t* str)
{
	converter_t converter;
	return std::move(converter.to_bytes(str));
}

std::string stra(const std::wstring& str)
{
	converter_t converter;
	return std::move(converter.to_bytes(str));
}

std::wstring strw(const char* str)
{
	converter_t converter;
	return std::move(converter.from_bytes(str));
}

std::wstring strw(const std::string& str)
{
	converter_t converter;
	return std::move(converter.from_bytes(str));
}

std::string strafv(const char* format, va_list args)
{
	const size_t bufCount = 1024;
	char buf[bufCount];

	#ifdef WIN32
		int n = _vsnprintf_s(buf, bufCount, _TRUNCATE, format, args);
	#else
		int n = vsnprintf(buf, bufCount, format, args);
	#endif

	std::string str;
	if (n > 0)
		str.assign(buf, n);

	return std::move(str);
}

std::string straf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	auto str = strafv(format, args);
	va_end(args);

	return std::move(str);
}

std::wstring strwfv(const wchar_t* format, va_list args)
{
	const size_t bufCount = 1024;
	wchar_t buf[bufCount];

	#ifdef WIN32
		int n = _vsnwprintf_s(buf, bufCount, _TRUNCATE, format, args);
	#else
		int n = vswprintf(buf, bufCount, format, args);
	#endif

	std::wstring str;
	if (n > 0)
		str.assign(buf, n);

	return std::move(str);
}

std::wstring strwf(const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);
	auto str = strwfv(format, args);
	va_end(args);

	return std::move(str);
}

std::vector<std::wstring> split(const std::wstring& s, const std::wstring& delim)
{
	std::vector<std::wstring> res;
	size_t start = 0;
	for (;;)
	{
		size_t end = s.find(delim, start);
		if (end != std::string::npos)
		{
			res.emplace_back(s.substr(start, end - start));
			start = end + delim.length();
		}
		else
		{
			res.emplace_back(s.substr(start));
			break;
		}
	}
	return std::move(res);
}

}
