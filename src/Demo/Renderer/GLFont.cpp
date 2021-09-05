#include "Renderer/GLFont.h"
#include "Renderer/DefaultFont.h"

namespace D {

void GLFont::initDefault()
{
	init(DefaultFontTextureFile, DefaultFontCoords, sizeof(DefaultFontCoords) / sizeof(DefaultFontCoords[0]), DefaultFontHeight);
}

void GLFont::init(const char* filename, const short* _coords, int numCoords, float _symHeight)
{
	texture.load(filename);

	GLint h = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	texHeight = (float)h;
	symHeight = _symHeight;

	coords.assign(_coords, _coords + numCoords);
}

void GLFont::setHeight(float size)
{
	if (symHeight != 0.0f)
		scale = size / symHeight;
	else
		scale = 0;
}

void GLFont::bind()
{
	texture.bind();
}

inline bool isPrintable(char c) { return (c >= '!' && c <= '~'); }

void GLFont::draw(float left, float top, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	auto str = strafv(format, args);
	va_end(args);

	if (str.empty())
		return;

	const auto* buf = str.c_str();
	const size_t n = str.length();

	const float norm = 1.0f / texHeight;
	float x = (float)left;
	float y = (float)top;

	glBegin(GL_QUADS);
	for (size_t i = 0; i < n; ++i)
	{
		char c = buf[i];
		if (c == '\0') { break; }
		else if (c == ' ') { x += getGlyphSize('_').w; continue; }

		if (!isPrintable(c))
			continue;

		c -= '!';
		int off = (int)c * 4;

		short c0 = coords[off + 0];
		short c1 = coords[off + 1];
		short c2 = coords[off + 2];
		short c3 = coords[off + 3];

		float w = (c2 - c0) * scale;
		float h = (c3 - c1) * scale;

		float t0 = c0 * norm;
		float t2 = c2 * norm;
		float t1 = (c1 + 2) * norm;
		float t3 = (c3 - 2) * norm;

		glTexCoord2f(t0, t1); glVertex2f(x,     y);
		glTexCoord2f(t2, t1); glVertex2f(x + w, y);
		glTexCoord2f(t2, t3); glVertex2f(x + w, y + h);
		glTexCoord2f(t0, t3); glVertex2f(x,     y + h);

		x += (w + 1);
	}
	glEnd();
}

GlyphSize GLFont::getGlyphSize(char c) const
{
	if (!isPrintable(c))
		return GlyphSize{0, 0};

	c -= '!';
	int off = (int)c * 4;

	short c0 = coords[off + 0];
	short c1 = coords[off + 1];
	short c2 = coords[off + 2];
	short c3 = coords[off + 3];

	float w = (c2 - c0) * scale;
	float h = (c3 - c1) * scale;

	return GlyphSize{w, h};
}

}
