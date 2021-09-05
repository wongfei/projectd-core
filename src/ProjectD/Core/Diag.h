#pragma once

#include "Core/Core.h"
#include <stdexcept>

#define X_DEBUG_BREAK __debugbreak()
#define X_FILE D::get_filename(__FILE__)
#define X_LINE __LINE__

#define THROW_FATAL do { X_DEBUG_BREAK; throw D::Exception("FATAL_ERROR", X_FILE, X_LINE); } while (0)

#define SHOULD_NOT_REACH_WARN do { D::guard_warn(X_FILE, X_LINE); } while (0)
#define SHOULD_NOT_REACH_FATAL do { D::guard_fatal(X_FILE, X_LINE); THROW_FATAL; } while (0)
#define TODO_NOT_IMPLEMENTED_FATAL SHOULD_NOT_REACH_FATAL

#define GUARD_WARN(expr) if (!(expr)) { SHOULD_NOT_REACH_WARN; }
#define GUARD_FATAL(expr) if (!(expr)) { SHOULD_NOT_REACH_FATAL; }

#if defined(DEBUG)
	#define DEBUG_GUARD_WARN GUARD_WARN
	#define DEBUG_GUARD_FATAL GUARD_FATAL
    #define TRACE_CTOR(name) log_printf(L"+" STRINGIZE_W(name) L" %p", this)
    #define TRACE_DTOR(name) log_printf(L"~" STRINGIZE_W(name) L" %p", this)
#else
	#define DEBUG_GUARD_WARN(expr)
	#define DEBUG_GUARD_FATAL(expr)
    #define TRACE_CTOR(name)
    #define TRACE_DTOR(name)
#endif

namespace D {

class Exception : public std::exception
{
public:
    explicit Exception(std::string&& _msg, const char* _file, int _line) : msg(_msg), file(_file), line(_line) {}
    virtual ~Exception() noexcept {}
    virtual const char* what() const noexcept { return msg.c_str(); }

protected:
    std::string msg;
    const char* file;
    int line;
};

void log_init(const wchar_t* filename);
void log_close();
void log_printf(const wchar_t* format, ...);

void guard_warn(const char* file, int line);
void guard_fatal(const char* file, int line);

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
