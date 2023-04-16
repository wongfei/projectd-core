#pragma once

#include "GLCore.h"

namespace D {

void gl_guard(const char* file, int line)
{
	const auto err = glGetError();
	if (err != GL_NO_ERROR)
	{
		log_printf(L"GL_ERROR -> 0x%X [FILE: %S LINE: %d]", (unsigned int)err, file, line);
		log_flush();
		throw D::Exception("GL_ERROR", file, line);
	}
}

}
