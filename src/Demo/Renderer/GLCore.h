#pragma once

#include "Core/Common.h"
#include "Core/Math.h"
#include <SDL_opengl.h>

#define GL_GUARD_FATAL do { const auto err = glGetError(); if (err != GL_NO_ERROR) { D::gl_fatal(err, X_FILE, X_LINE); THROW_FATAL; } } while (0);

namespace D {

inline void gl_fatal(GLenum err, const char* file, int line)
{
	log_printf(L"GL_GUARD at %S (line %d) [code 0x%X]", file, line, (unsigned int)err);
}

template<typename T>
struct GLHandle : NonCopyable
{
	inline GLHandle() {}
	inline explicit GLHandle(T _id) : id(_id) {}
	inline bool valid() const { return id != T(0); }

	T id = T(0);

protected:
	inline ~GLHandle() {}
};

struct GLDisplayList : public GLHandle<GLuint>
{
	inline ~GLDisplayList() { release(); }
	inline void gen() { if (!id) { id = glGenLists(1); GL_GUARD_FATAL; } }
	inline void release() { if (id) { glDeleteLists(id, 1); id = 0; } }
	inline void beginCompile(GLenum mode) { gen(); glNewList(id, mode); GL_GUARD_FATAL; }
	inline void endCompile() { glEndList(); GL_GUARD_FATAL; }
	inline void draw() { DEBUG_GUARD_FATAL(valid()); glCallList(id); GL_GUARD_FATAL; }
};

struct GLCompileScoped
{
	inline GLCompileScoped(GLDisplayList& _list) : list(_list) { list.gen(); list.beginCompile(GL_COMPILE); }
	inline ~GLCompileScoped() { list.endCompile(); }
	GLDisplayList& list;
};

}
