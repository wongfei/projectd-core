#pragma once

#include "Renderer/GLTexture.h"
#include <vector>

namespace D {

struct GlyphSize
{
	float w = 0;
	float h = 0;
};

struct GLFont
{
	void init(const char* filename, const short* coords, int numCoords, float symHeight);
	void initDefault();
	void setHeight(float size);
	void bind();
	void draw(float left, float top, const char* str, ...);
	GlyphSize getGlyphSize(char c) const;

	GLTexture texture;
	std::vector<short> coords;
	float texHeight = 0;
	float symHeight = 0;
	float scale = 1.0f;
};

}
