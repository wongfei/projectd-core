#pragma once

#include "Core/Core.h"
#include <stdexcept>

#define X_FILE D::get_filename(__FILE__)
#define X_LINE __LINE__

#if defined(DEBUG)
    #define X_DEBUG_BREAK __debugbreak()
    #define DEBUG_GUARD_WARN GUARD_WARN
	#define DEBUG_GUARD_FATAL GUARD_FATAL
#else
    #define X_DEBUG_BREAK
    #define DEBUG_GUARD_WARN(expr)
	#define DEBUG_GUARD_FATAL(expr)
#endif

#define GUARD_WARN(expr) if (!(expr)) { D::trace_warn(L"GUARD_WARN", X_FILE, X_LINE); }
#define GUARD_FATAL(expr) if (!(expr)) { D::trace_error(L"GUARD_FATAL", X_FILE, X_LINE); throw D::Exception("GUARD_FATAL", X_FILE, X_LINE); }

#define SHOULD_NOT_REACH_WARN do { D::trace_warn(L"SHOULD_NOT_REACH", X_FILE, X_LINE); } while (0)
#define SHOULD_NOT_REACH_FATAL do { D::trace_error(L"SHOULD_NOT_REACH", X_FILE, X_LINE); throw D::Exception("SHOULD_NOT_REACH", X_FILE, X_LINE); } while (0)

#define TODO_NOT_IMPLEMENTED_FATAL SHOULD_NOT_REACH_FATAL

#define TRACE_CTOR(name) log_printf(L"+" STRINGIZE_W(name) L" %p", this)
#define TRACE_DTOR(name) log_printf(L"~" STRINGIZE_W(name) L" %p", this)

namespace D {

class Exception : public std::exception
{
public:
    explicit Exception(const std::string& _msg, const char* _file, int _line) : msg(_msg), file(_file), line(_line) {}
    virtual ~Exception() noexcept {}
    virtual const char* what() const noexcept { return msg.c_str(); }

protected:
    std::string msg;
    const char* file;
    int line;
};

void log_init(const wchar_t* filename);
void log_flush();
void log_close();
void log_printf(const wchar_t* format, ...);

void trace_warn(const wchar_t* msg, const char* file, int line);
void trace_error(const wchar_t* msg, const char* file, int line);

constexpr const char* get_filename(const char* path)
{
    const char* result = path;
    while (*path) {
        if (*path == '/' || *path == '\\') {
            result = path + 1;
        }
		path++;
    }
    return result;
}

}
