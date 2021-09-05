#include "Renderer/GLChart.h"

namespace D {

void GLChart::init(int w, int h)
{
	GUARD_FATAL(w > 0 && h > 0);

	texture.resize(w, h, GL_RGB8);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GL_GUARD_FATAL;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); GL_GUARD_FATAL;

	chart.resize(w, h);
	bg_color = ChartColor(0, 0, 0, 255);
	border_color = ChartColor(128, 128, 128, 255);
}

void GLChart::clear()
{
	chart.fill(bg_color);
	chart.draw_borders(border_color);
}

void GLChart::plot(ChartSeries& series, const ChartColor& color)
{
	chart.draw_series(series, color);
}

void GLChart::present(float x, float y, float w, float h)
{
	if (w == 0.0f)
		w = (float)chart.get_w();
	if (h == 0.0f)
		h = (float)chart.get_h();

	texture.update(chart.get_data());

	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1); glVertex2f(x, y);
	glTexCoord2f(1, 1); glVertex2f(x + w, y);
	glTexCoord2f(1, 0); glVertex2f(x + w, y + h);
	glTexCoord2f(0, 0); glVertex2f(x, y + h);
	glEnd();
}

void GLChart::present(float x, float y, float scale)
{
	present(x, y, chart.get_w() * scale, chart.get_h() * scale);
}

}
