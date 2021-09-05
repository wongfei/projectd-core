#pragma once

#include "Renderer/GLTexture.h"

namespace D {

struct GLSkyBox
{
	void load(float dim, const char* path, const char* name, const char* ext);
	void draw();

	GLDisplayList list;
	GLTexture tex[6]; // front, back, left, right, top, bottom
};

}
