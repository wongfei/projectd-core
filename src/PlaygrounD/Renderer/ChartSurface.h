#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace D {

struct ChartPoint {
	float x, y;
	inline ChartPoint() {}
	inline ChartPoint(float _x, float _y) : x(_x), y(_y) {}
};

struct ChartColor {
	uint8_t r, g, b, a;
	inline ChartColor() {}
	inline ChartColor(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}
};

struct ChartRgb8 {
	uint8_t r, g, b;
	inline ChartRgb8() {}
	inline ChartRgb8(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b) {}
	inline ChartRgb8(const ChartColor& col) : r(col.r), g(col.g), b(col.b) {}
};

struct ChartRgba8 {
	uint8_t r, g, b, a;
	inline ChartRgba8() {}
	inline ChartRgba8(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}
	inline ChartRgba8(const ChartColor& col) : r(col.r), g(col.g), b(col.b), a(col.a) {}
};

class ChartSeries {
public:
	inline ChartSeries() {}
	inline ChartSeries(size_t nval, float y0, float y1) { init(nval, y0, y1); }

	inline void init(size_t nval, float y0, float y1) {
		resize(nval);
		set_range_y(y0, y1);
	}

	inline void add_value(float x, float y) {
		points_[pos_++] = ChartPoint(x, y);
		if (pos_ >= nval_) {
			pos_ = 0;
		}
	}

	inline void resize(size_t nval) {
		points_.resize(nval);
		nval_ = nval;
		reset();
	}

	inline void reset() {
		memset(&points_[0], 0, sizeof(points_[0]) * nval_);
		pos_ = 0;
	}

	inline void set_range_y(float y0, float y1) {
		y0_ = y0;
		y1_ = y1;
	}

	inline void set_ay(float ay0, float ay1) {
		ay0_ = ay0;
		ay1_ = ay1;
	}

	inline void set_autoscale(bool autoscale) {
		autoscale_ = autoscale;
	}

	inline const std::vector<ChartPoint>& get_points() const { return points_; }
	inline size_t get_nval() const { return nval_; }
	inline size_t get_pos() const { return pos_; }
	inline float get_y0() const { return y0_; }
	inline float get_y1() const { return y1_; }
	inline float get_ay0() const { return ay0_; }
	inline float get_ay1() const { return ay1_; }
	inline bool is_autoscaled() const { return autoscale_; }

private:
	std::vector<ChartPoint> points_;
	size_t nval_ = 0;
	size_t pos_ = 0;
	float y0_ = 0;
	float y1_ = 1;
	float ay0_ = 0;
	float ay1_ = 0;
	bool autoscale_ = false;
};

template<typename TPixelFormat>
class ChartSurface {
public:
	inline ChartSurface() {}
	inline ChartSurface(size_t width, size_t height) { resize(width, height); }

	inline void resize(size_t width, size_t height) {
		width_ = width;
		height_ = height;
		pixels_.resize(width * height);
	}

	inline void setpix(int x, int y, const TPixelFormat& color) {
		if (x >= 0 && x < width_ && y >= 0 && y < height_) {
			pixels_[(size_t)y * width_ + (size_t)x] = color;
		}
	}

	inline void fill(const TPixelFormat& color) {
		auto* pix = &pixels_[0];
		for (size_t y = 0; y < height_; ++y) {
			for (size_t x = 0; x < width_; ++x) {
				*pix++ = color;
			}
		}
	}

	inline void draw_borders(const TPixelFormat& color) {
		const int w = (int)width_;
		const int h = (int)height_;
		for (int x = 0; x < w; ++x) {
			setpix(x, 0, color);
			setpix(x, h - 1, color);
		}
		for (int y = 0; y < h; ++y) {
			setpix(0, y, color);
			setpix(w - 1, y, color);
		}
	}

	inline void draw_series(ChartSeries& series, const TPixelFormat& color) {
		const auto& points = series.get_points();
		const auto nval = series.get_nval();
		const auto scale_x = (float)width_ / (float)nval;
		const auto height = (float)height_;
		auto y0 = series.get_y0();
		auto y1 = series.get_y1();

		if (series.is_autoscaled()) {
			//y0 = FLT_MAX;
			//y1 = -FLT_MAX;
			for (size_t pos = series.get_pos() + 1, id = 0; id < nval; ++id, ++pos) {
				if (pos >= nval) {
					pos = 0;
				}
				const float vy = points[pos].y;
				if (y0 > vy) y0 = vy;
				if (y1 < vy) y1 = vy;
			}
			series.set_ay(y0, y1);
		}

		for (size_t pos = series.get_pos() + 1, id = 0; id < nval; ++id, ++pos) {
			if (pos >= nval) {
				pos = 0;
			}
			const float x = id * scale_x;
			//const float y = height - linear_scale(points[pos].y, y0, y1, 0, height);
			const float y = linear_scale(points[pos].y, y0, y1, 0, height);
			setpix((int)x, (int)y, color);
		}
	}

	inline float linear_scale(float x, float min, float max, float a, float b) {
		return ((b - a) * (x - min)) / (max - min) + a;
	}

	inline const void* get_data() const { return &pixels_[0]; }
	inline size_t get_w() const { return width_; }
	inline size_t get_h() const { return height_; }

private:
	std::vector<TPixelFormat> pixels_;
	size_t width_ = 0;
	size_t height_ = 0;
};

}
