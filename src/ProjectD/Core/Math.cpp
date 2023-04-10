#include "Core/Math.h"
#include <DirectXMath.h>
#include <cstring>

namespace D {

inline DirectX::XMMATRIX xmload(const mat44f& m) { return DirectX::XMMATRIX(&m.M11); }
inline mat44f xmstore(const DirectX::XMMATRIX& m) { DirectX::XMFLOAT4X4 f44; DirectX::XMStoreFloat4x4(&f44, m); return *(mat44f*)&f44._11; }

plane4f::plane4f(const vec3f& p1, const vec3f& p2, const vec3f& p3)
{
	auto a = p3 - p1;
	auto b = p2 - p1;
	normal = a.cross(b);
	d = -(p1 * normal);
}

mat44f::mat44f()
{
	M11 = 1;
	M12 = 0;
	M13 = 0;
	M14 = 0;

	M21 = 0;
	M22 = 1;
	M23 = 0;
	M24 = 0;

	M31 = 0;
	M32 = 0;
	M33 = 1;
	M34 = 0;

	M41 = 0;
	M42 = 0;
	M43 = 0;
	M44 = 1;
}

mat44f::mat44f(const float* ptr)
{
	memcpy(&M11, ptr, 16 * sizeof(float));
}

void mat44f::copyTo(float* ptr) const
{
	memcpy(ptr, &M11, 16 * sizeof(float));
}

mat44f mat44f::getRotator() const
{
	mat44f res = *this;
	res.M41 = 0, res.M42 = 0, res.M43 = 0, res.M44 = 1;
	return res;
}

vec3f mat44f::getEulerAngles() const // TODO: this is joke
{
	vec3f result;
	result.x = atan2f(-M31, M33);

	float v7 = 1;
	float v8 = M32;
	if (v8 > 1.0 || (v7 = -1, v8 < -1.0))
		v8 = v7;
	result.y = asinf(v8);

	float v10, v11;
	if (M12 == 0.0f && M22 == 0.0f)
	{
		v11 = M21;
		v10 = M11;
		result.x = 0.0f;
	}
	else
	{
		v11 = -M12;
		v10 = M22;
	}
	result.z = atan2f(v11, v10);

	return vec3f(result.y, result.x, result.z) * -57.295779513082323f;
}

mat44f mat44f::createFromAxisAngle(const vec3f& axis, float angle)
{
	mat44f res;

	float sin_a = sinf(angle);
	float cos_a = cosf(angle);
	float om_cos_a = 1.0f - cos_a;

	res.M11 = ((axis.x * axis.x) * om_cos_a) + cos_a;
	res.M22 = ((axis.y * axis.y) * om_cos_a) + cos_a;
	res.M33 = ((axis.z * axis.z) * om_cos_a) + cos_a;

	res.M12 = (axis.z * sin_a) + (axis.y * axis.x) * om_cos_a;
	res.M23 = (axis.x * sin_a) + (axis.z * axis.y) * om_cos_a;
	res.M31 = (axis.y * sin_a) + (axis.z * axis.x) * om_cos_a;

	res.M13 = (axis.z * axis.x) * om_cos_a - (axis.y * sin_a);
	res.M21 = (axis.y * axis.x) * om_cos_a - (axis.z * sin_a);
	res.M32 = (axis.z * axis.y) * om_cos_a - (axis.x * sin_a);
	
	res.M14 = 0;
	res.M24 = 0;
	res.M34 = 0;
	
	res.M41 = 0;
	res.M42 = 0;
	res.M43 = 0;
	res.M44 = 1;

	return res;
}

mat44f mat44f::mult(const mat44f& a, const mat44f& b)
{
	auto res = DirectX::XMMatrixMultiply(xmload(a), xmload(b));
	return xmstore(res);
}

mat44f mat44f::rotate(const mat44f& m, const vec3f& axis, float angle)
{
	mat44f rotator = mat44f::createFromAxisAngle(axis, angle);
	//return mat44f::mult(m, rotator);
	return mat44f::mult(rotator, m);
}

}
