#include "Core/Diag.h"
#include "Core/String.h"
#include "Core/OS.h"

namespace D {

static std::wstring _logFileName;

void log_set_file(const wchar_t* filename, bool overwrite)
{
	_logFileName = filename;
	if (overwrite)
	{
		log_clear_file();
	}
}

void log_clear_file()
{
	if (!_logFileName.empty())
	{
		FILE* fd = nullptr;
		if (_wfopen_s(&fd, _logFileName.c_str(), L"wt") == 0 && fd)
		{
			fclose(fd);
		}
	}
}

void log_printf(const wchar_t* format, ...)
{
	va_list args;
	va_start(args, format);
	auto str = strwfv(format, args);
	va_end(args);

	if (!str.empty())
	{
		auto line(strwf(L"[%u P=%u T=%u] %s", osGetCurrentTicks(), osGetCurrentProcessId(), osGetCurrentThreadId(), str.c_str()));

		osTraceDebug(line.c_str());

		// TODO: python console don't like wide chars
		fputs(stra(line).c_str(), stdout);
		fputc('\n', stdout);

		if (!_logFileName.empty())
		{
			FILE* fd = nullptr;
			if (_wfopen_s(&fd, _logFileName.c_str(), L"at") == 0 && fd)
			{
				fputws(line.c_str(), fd);
				fputwc(L'\n', fd);
				fclose(fd);
			}
		}
	}
}

void trace_warn(const wchar_t* msg, const char* file, int line)
{
	log_printf(L"%s [FILE: %S LINE: %d]", msg, file, line);
}

void trace_error(const wchar_t* msg, const char* file, int line)
{
	log_printf(L"%s [FILE: %S LINE: %d]", msg, file, line);
}

}
