#include "Sim/SlipStream.h"

namespace D {

SlipStream::SlipStream()
{
	//TRACE_CTOR(SlipStream);
}

SlipStream::~SlipStream()
{
	//TRACE_DTOR(SlipStream);
}

float SlipStream::getSlipEffect(const vec3f& p)
{
	float fSlip = 0;

	vec3f vDelta = (p - triangle.points[0]);
	float fDeltaLen = vDelta.len();

	if (fDeltaLen < length)
	{
		vDelta.norm(fDeltaLen);
		float fDot = vDelta * dir;

		if (fDot <= 0.7f)
			fSlip = 0;
		else
			fSlip = (((1.0f - (fDeltaLen / length)) * (fDot - 0.7f)) * 3.3333333f) * effectGainMult;
	}

	return fSlip;
}

void SlipStream::setPosition(const vec3f& pos, const vec3f& vel) // TODO: check
{
	triangle.points[0] = pos;

	float fVelLen = vel.len();
	dir = vel.get_norm(fVelLen) * -1.0f;

	float fLen = (fVelLen * speedFactor) * speedFactorMult;
	length = fLen;

	vec2f vNormXZ(vel.x, vel.z);
	vNormXZ.norm();

	/*
	fOffX = (((fZero * 0.0) - vn_z) * fLength) * 0.25;
	fOffY = (((vn_z * 0.0) - (vn_x * 0.0)) * fLength) * 0.25;
	fOffZ = ((vn_x - (fZero * 0.0)) * fLength) * 0.25;
  	*/
	vec3f vOff1(-vNormXZ[1], 0, vNormXZ[0]);
	vOff1 *= (fLen * 0.25f);

	/*
	triangle.points[1]
	v21 = ((vn_x * fLengthNeg) + pos->x) + fOffX;
	v22 = ((fZero * fLengthNeg) + pos->y) + fOffY;
	v24 = (vn_z * fLengthNeg + pos->z) + fOffZ;

	triangle.points[2]
	v25 = ((vn_x * fLengthNeg) + pos->x) - fOffX;
	v25y = ((fZero * fLengthNeg) + pos->y) - fOffY;
	v26 = (vn_z * fLengthNeg + pos->z) - fOffZ;
	*/
	vec3f vOff2((vNormXZ[0] * -fLen), 0, (vNormXZ[1] * -fLen));

	triangle.points[1] = pos + vOff2 + vOff1;
	triangle.points[2] = pos + vOff2 - vOff1;
}

}
