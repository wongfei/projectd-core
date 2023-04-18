#pragma once

#include "Core/Common.h"
#include "Core/Math.h"
#include <SDL_opengl.h>

#define GL_GUARD D::gl_guard(X_FILE, X_LINE)

namespace D {

void gl_guard(const char* file, int line);

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
	inline void gen() { if (!id) { id = glGenLists(1); GL_GUARD; } }
	inline void release() { if (id) { glDeleteLists(id, 1); id = 0; } }
	inline void beginCompile(GLenum mode) { gen(); glNewList(id, mode); GL_GUARD; }
	inline void endCompile() { glEndList(); GL_GUARD; }
	inline void draw() { DEBUG_GUARD_FATAL(valid()); glCallList(id); GL_GUARD; }
};

struct GLCompileScoped
{
	inline GLCompileScoped(GLDisplayList& _list) : list(_list) { list.gen(); list.beginCompile(GL_COMPILE); }
	inline ~GLCompileScoped() { list.endCompile(); }
	GLDisplayList& list;
};

}
