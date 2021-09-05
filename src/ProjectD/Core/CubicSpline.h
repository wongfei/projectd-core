#pragma once

#include "Core/Core.h"

#pragma warning(push)
#pragma warning(disable: 4244) // conversion from 'double' to 'float'
#include "tkspline.h"
#pragma warning(pop)

namespace D {

struct CubicSpline
{
	void setPoints(const std::vector<float>& refs, const std::vector<float>& vals);
	float getValue(float ref) const;

	tk::spline spline;
};

}
