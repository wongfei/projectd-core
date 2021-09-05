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

struct CarDebug
{
	std::vector<ColorPoint> points;
	std::vector<ColorLine> lines;
};

}
