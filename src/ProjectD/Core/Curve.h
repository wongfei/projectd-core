#pragma once

#include "Core/CubicSpline.h"
#include <string>
#include <vector>

namespace D {

struct Curve
{
	Curve();
	~Curve();

	Curve(const Curve& other);
	Curve& operator=(const Curve& other);

	Curve(Curve&& other) noexcept;
	Curve& operator=(Curve&& other) noexcept;

	void init(const Curve& other);
	void init(Curve&& other) noexcept;
	void reset();

	void addValue(float ref, float val);
	void scale(float scale);

	int getCount() const;
	float getMaxReference() const;
	std::pair<float, float> getPairAtIndex(int index) const;

	float getValue(float ref);
	float getCubicSplineValue(float ref);

	bool load(const std::wstring& filename);
	bool parseInline(const std::wstring& str);

	std::vector<float> references;
	std::vector<float> values;
	CubicSpline spline;
	bool splineReady = false;
};

}
