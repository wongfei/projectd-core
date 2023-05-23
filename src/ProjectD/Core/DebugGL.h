#pragma once

#include "Core/Math.h"
#include <vector>

namespace D {

struct ColorPoint
{
	vec3f p;
	vec3f color;
};

struct ColorLine
{
	vec3f p1;
	vec3f p2;
	vec3f color;
};

struct DebugGL
{
	std::vector<ColorPoint> points;
	std::vector<ColorLine> lines;
	
	DebugGL() { reserve(1024); }

	void point(const vec3f& p, const vec3f& color, float size = 1) { points.emplace_back(ColorPoint{ p, color }); }
	void line(const vec3f& p1, const vec3f& p2, const vec3f& color, float width = 1) { lines.emplace_back(ColorLine{ p1, p2, color }); }
	void clear() { points.clear(); lines.clear(); }
	void reserve(size_t n) { points.reserve(n); lines.reserve(n); }

	static DebugGL& get() { static DebugGL inst; return inst; }
};
	
}
