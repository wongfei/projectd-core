#include "CubicSpline.h"

namespace D {

void CubicSpline::setPoints(const std::vector<float>& refs, const std::vector<float>& vals)
{
	spline.set_points(refs, vals);
}

float CubicSpline::getValue(float ref) const
{
	return spline(ref);
}

}
