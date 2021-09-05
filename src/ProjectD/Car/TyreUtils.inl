#pragma once

#include "Car/CarCommon.h"

namespace D {

inline float calcSlipAngleRAD(float vy, float vx)
{
	if (vx != 0.0f)
		return atanf(-(vy / fabsf(vx)));
	return 0;
}

inline float calcCamberRAD(const vec3f& roadNormal, const mat44f& susMatrix)
{
	float fTmp = ((susMatrix.M12 * roadNormal.y) + (susMatrix.M11 * roadNormal.x)) + (susMatrix.M13 * roadNormal.z);
	if (fTmp <= -1.0f || fTmp >= 1.0f)
		return -1.5707964f;
	else
		return -asinf(fTmp);
}

inline float calcContactPatchLength(float radius, float deflection)
{
	float v = radius - deflection;
	if (v <= 0.0f || radius <= v)
		return 0.0f;
	else
		return sqrtf((radius * radius) - (v * v)) * 2.0f;
}

inline float calcLoadSensMult(float targetD, float targetLoad, float sensExp)
{
	return (targetD * targetLoad) / powf(targetLoad, sensExp);
}

inline float loadSensLinearD(float d0, float d1, float load)
{
	return ((load * 0.0005f) * d1) + d0;
}

inline float loadSensExpD(float exp, float mult, float load)
{
	if (load != 0.0f)
		return (powf(load, exp) * mult) / load;
	return 0.0f;
}

}
