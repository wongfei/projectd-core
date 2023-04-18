#pragma once

#include "Renderer/GLTexture.h"
#include "Renderer/ChartSurface.h"

namespace D {

struct GLChart
{
	void init(int w, int h);
	void clear();
	void plot(ChartSeries& series, const ChartColor& color);
	void present(float x, float y, float w, float h);
	void present(float x, float y, float scale = 1.0f);

	GLTexture texture;
	ChartSurface<ChartRgb8> chart;
	ChartColor bg_color;
	ChartColor border_color;
};

}
