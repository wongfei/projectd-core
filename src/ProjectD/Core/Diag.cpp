#include "Core/Diag.h"
#include "Core/String.h"
#include "Core/OS.h"

namespace D {

struct LogTerminator
{
	LogTerminator() {}
	~LogTerminator() { log_close(); }
};

static FILE* _logFile = nullptr;
static LogTerminator _logTerminator;

void log_init(const wchar_t* filename)
{
	log_close();
	if (_wfopen_s(&_logFile, filename, L"wt") != 0)
	{
		_logFile = nullptr;
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
		osTraceDebug(str.c_str());

		if (_logFile)
		{
			fputws(str.c_str(), _logFile);
			fputwc(L'\n', _logFile);
		}
	}
}

void log_close()
{
	if (_logFile) 
	{
		fclose(_logFile);
		_logFile = nullptr;
	}
}

void guard_warn(const char* file, int line)
{
	log_printf(L"GUARD_WARN at %S (line %d)", file, line);
}

void guard_fatal(const char* file, int line)
{
	log_printf(L"GUARD_FATAL at %S (line %d)", file, line);
}

}
