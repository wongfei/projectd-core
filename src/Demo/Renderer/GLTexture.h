#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct GLTexture : public GLHandle<GLuint>
{
	GLTexture();
	~GLTexture();
	void gen();
	void release();
	void bind();
	void resize(GLsizei width, GLsizei height, GLint internalFormat);
	void load(const char* filename);
	void update(const void* data);

	GLsizei width = 0;
	GLsizei height = 0;
	GLint internalFormat = 0;
	GLenum texFormat = 0;
	GLenum texType = 0;
	bool allocated = false;
};

}
