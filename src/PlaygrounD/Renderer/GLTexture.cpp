#include "Renderer/GLTexture.h"
#include <SDL2/SDL_image.h>
#include <gl/GLU.h>

namespace D {

GLTexture::GLTexture()
{
}

GLTexture::~GLTexture()
{
	release();
}

void GLTexture::gen()
{
	if (!id)
	{
		glGenTextures(1, &id); GL_GUARD;
	}
}

void GLTexture::release()
{
	if (id)
	{
		glDeleteTextures(1, &id);
		id = 0;
	}
	internalFormat = 0;
	allocated = false;
}

void GLTexture::bind()
{
	DEBUG_GUARD_FATAL(valid());
	glBindTexture(GL_TEXTURE_2D, id); GL_GUARD;
}

void GLTexture::resize(GLsizei _width, GLsizei _height, GLint _internalFormat)
{
	if (valid() && width == _width && height == _height && internalFormat == _internalFormat)
		return;

	release();

	width = _width;
	height = _height;
	internalFormat = _internalFormat;

	GLsizei pixelSize = 0;
	switch (internalFormat)
	{
		case GL_RGB8:
			texFormat = GL_RGB;
			texType = GL_UNSIGNED_BYTE;
			pixelSize = 3;
			break;

		case GL_RGBA8:
			texFormat = GL_RGBA;
			texType = GL_UNSIGNED_BYTE;
			pixelSize = 4;
			break;

		case GL_LUMINANCE8:
			texFormat = GL_LUMINANCE;
			texType = GL_UNSIGNED_BYTE;
			pixelSize = 1;
			break;

		case GL_LUMINANCE16:
			texFormat = GL_LUMINANCE;
			texType = GL_UNSIGNED_SHORT;
			pixelSize = 2;
			break;

		default:
			SHOULD_NOT_REACH_FATAL;
			return;
	}

	gen();
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); GL_GUARD;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); GL_GUARD;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_GUARD;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GL_GUARD;
}

void GLTexture::load(const std::wstring& filename)
{
	release();

	log_printf(L"GLTexture: load: %s", filename.c_str());

	struct SurfaceDtor { void operator()(SDL_Surface* ptr) { SDL_FreeSurface(ptr); } };
	std::unique_ptr<SDL_Surface, SurfaceDtor> image(IMG_Load(stra(filename).c_str()));
	GUARD_FATAL(image);

	GLint format;
	switch (image->format->BitsPerPixel)
	{
		case 24:
			format = GL_RGB8;
			break;

		case 32:
			format = GL_RGBA8;
			break;

		default:
			log_printf(L"GLTexture: unsupported bpp: %d", (int)image->format->BitsPerPixel);
			SHOULD_NOT_REACH_FATAL;
	}

	resize(image->w, image->h, format);
	gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat, image->w, image->h, texFormat, texType, image->pixels); GL_GUARD;
}

void GLTexture::update(const void* data)
{
	bind();
	if (allocated)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, texFormat, texType, data); GL_GUARD;
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, texFormat, texType, data); GL_GUARD;
		allocated = true;
	}
}

}
