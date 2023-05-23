#pragma once

#include "Core/Core.h"
#include <cmath>
#include <intrin.h>

#if !defined(M_PI)
#define M_PI 3.1415926535897932384626433832795f
#endif

#define M_DEG2RAD (float)0.01745329251994329576923690768489
#define M_RAD2DEG (float)57.295779513082320876798154814105

namespace D {

// thanks UE!
#if 1
inline int truncToInt(float F) { return _mm_cvtt_ss2si(_mm_set_ss(F)); }
inline int floorToInt(float F) { return _mm_cvt_ss2si(_mm_set_ss(F + F - 0.5f)) >> 1; }
inline int roundToInt(float F) { return _mm_cvt_ss2si(_mm_set_ss(F + F + 0.5f)) >> 1; }
inline int ceilToInt(float F) { return -(_mm_cvt_ss2si(_mm_set_ss(-0.5f - (F + F))) >> 1); }
#else
inline int truncToInt(float F) { return (int)F; }
inline int floorToInt(float F) { return truncToInt(floorf(F)); }
inline int roundToInt(float F) { return floorToInt(F + 0.5f); }
inline int ceilToInt(float F) { return truncToInt(ceilf(F)); }
#endif

template<typename T>
inline T tmin(const T& a, const T& b) { return (a < b ? a : b); }

template<typename T>
inline T tmax(const T& a, const T& b) { return (a > b ? a : b); }

template<typename T>
inline T tclamp(const T& x, const T& a, const T& b) { return (x < a ? a : (x > b ? b : x)); }

inline float signf(const float x) { return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); }

inline float linscalef(float x, float x0, float x1, float r0, float r1) {
	x = tclamp(x, x0, x1);
	return ((r1 - r0) * (x - x0)) / (x1 - x0) + r0;
}

inline float sawToothWave(float v, float p) {
	return ((v - ((v / p) * p)) - (p * 0.5f)) / (p * 0.5f);
}

inline float randR(float min, float max) {
	float r = (float)rand() / (float)RAND_MAX;
	return min + r * (max - min);
}

struct vec2f
{
	float x = 0;
	float y = 0;

	inline vec2f() {}
	inline vec2f(const vec2f& v) : x(v.x), y(v.y) {}
	inline vec2f(float ix, float iy) : x(ix), y(iy) {}
	inline explicit vec2f(const float* v) : x(v[0]), y(v[1]) {}

	inline vec2f clone() const { return *this; }

	inline vec2f& operator=(const vec2f& v) { x = v.x, y = v.y; return *this; }
	inline float& operator[](const int id) { return (&x)[id]; }
	inline const float& operator[](const int id) const { return (&x)[id]; }

	inline vec2f operator*(const float f) const { return vec2f(x * f, y * f); }
	inline vec2f operator/(const float f) const { return vec2f(x / f, y / f); }
	inline vec2f operator+(const vec2f& v) const { return vec2f(x + v.x, y + v.y); }
	inline vec2f operator-(const vec2f& v) const { return vec2f(x - v.x, y - v.y); }
	inline float operator*(const vec2f& v) const { return (x * v.x + y * v.y); }

	inline vec2f& operator*=(const float f) { x *= f, y *= f; return *this; }
	inline vec2f& operator/=(const float f) { x /= f, y /= f; return *this; }
	inline vec2f& operator+=(const vec2f& v) { x += v.x, y += v.y; return *this; }
	inline vec2f& operator-=(const vec2f& v) { x -= v.x, y -= v.y; return *this; }

	inline float sqlen() const { return (x * x + y * y); }
	inline float len() const { return sqrtf(sqlen()); }

	inline vec2f& norm(float l) { if (l != 0.0f) { (*this) *= (1.0f / l); } return *this; }
	inline vec2f& norm() { return norm(len()); }

	inline vec2f get_norm(float l) const { return clone().norm(l); }
	inline vec2f get_norm() const { return clone().norm(); }
};

struct vec3f
{
	float x = 0;
	float y = 0;
	float z = 0;

	inline vec3f() {}
	inline vec3f(const vec3f& v) : x(v.x), y(v.y), z(v.z) {}
	inline vec3f(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
	inline explicit vec3f(const float* v) : x(v[0]), y(v[1]), z(v[2]) {}

	inline vec3f clone() const { return *this; }

	inline vec3f& operator=(const vec3f& v) { x = v.x, y = v.y, z = v.z; return *this; }
	inline float& operator[](const int id) { return (&x)[id]; }
	inline const float& operator[](const int id) const { return (&x)[id]; }

	//inline vec3f operator*(const float f) const { return vec3f(x * f, y * f, z * f); }
	//inline vec3f operator/(const float f) const { return vec3f(x / f, y / f, z / f); }
	//inline vec3f operator+(const vec3f& v) const { return vec3f(x + v.x, y + v.y, z + v.z); }
	//inline vec3f operator-(const vec3f& v) const { return vec3f(x - v.x, y - v.y, z - v.z); }
	//inline float operator*(const vec3f& v) const { return (x * v.x + y * v.y + z * v.z); }

	inline vec3f& operator*=(const float f) { x *= f, y *= f, z *= f; return *this; }
	inline vec3f& operator/=(const float f) { x /= f, y /= f, z /= f; return *this; }
	inline vec3f& operator+=(const vec3f& v) { x += v.x, y += v.y, z += v.z; return *this; }
	inline vec3f& operator-=(const vec3f& v) { x -= v.x, y -= v.y, z -= v.z; return *this; }

	inline float sqlen() const { return (x * x + y * y + z * z); }
	inline float len() const { return sqrtf(sqlen()); }

	inline vec3f& norm(float l) { if (l != 0.0f) { (*this) *= (1.0f / l); } return *this; }
	inline vec3f& norm() { return norm(len()); }

	inline vec3f get_norm(float l) const { return clone().norm(l); }
	inline vec3f get_norm() const { return clone().norm(); }

	inline vec3f cross(const vec3f& v) const { return vec3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

	vec3f rotateAxisAngle(const vec3f& axis, float angle);
};

inline vec3f operator*(const vec3f& v, const float f) { return vec3f(v.x * f, v.y * f, v.z * f); }
inline vec3f operator/(const vec3f& v, const float f) { return vec3f(v.x / f, v.y / f, v.z / f); }
inline vec3f operator*(const float f, const vec3f& v) { return vec3f(v.x * f, v.y * f, v.z * f); }
inline vec3f operator/(const float f, const vec3f& v) { return vec3f(v.x / f, v.y / f, v.z / f); }
inline vec3f operator+(const vec3f& a, const vec3f& b) { return vec3f(a.x + b.x, a.y + b.y, a.z + b.z); }
inline vec3f operator-(const vec3f& a, const vec3f& b) { return vec3f(a.x - b.x, a.y - b.y, a.z - b.z); }
inline float operator*(const vec3f& a, const vec3f& b) { return (a.x * b.x + a.y * b.y + a.z * b.z); }

struct vec4f
{
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 0;
};

struct plane4f
{
	vec3f normal;
	float d = 0;

	plane4f() {}
	plane4f(const vec3f& p1, const vec3f& p2, const vec3f& p3);
};

struct mat44f
{
	#pragma warning(push)
	#pragma warning(disable: 4201)
	union
	{
		float dim1[16];
		float dim2[4][4];
		struct
		{
			float M11;
			float M12;
			float M13;
			float M14;
			float M21;
			float M22;
			float M23;
			float M24;
			float M31;
			float M32;
			float M33;
			float M34;
			float M41;
			float M42;
			float M43;
			float M44;
		};
	};
	#pragma warning(pop)

	mat44f();
	explicit mat44f(const float* ptr);
	void copyTo(float* ptr) const;

	mat44f getRotator() const;
	vec3f getEulerAngles() const;

	static mat44f createFromAxisAngle(const vec3f& axis, float angle);
	static mat44f mult(const mat44f& a, const mat44f& b);
	static mat44f rotate(const mat44f& m, const vec3f& axis, float angle);
};

inline vec3f operator*(const vec3f& v, const mat44f& m)
{
	return vec3f(
		m.M41 + v * vec3f(m.M11, m.M21, m.M31),
		m.M42 + v * vec3f(m.M12, m.M22, m.M32),
		m.M43 + v * vec3f(m.M13, m.M23, m.M33)
	);
}

inline vec3f randV(float min, float max) {
	return vec3f(randR(min, max), randR(min, max), randR(min, max));
}

inline vec3f rgb(int r, int g, int b) {
	return vec3f((float)r, (float)g, (float)b) / 255.0f;
}

struct ray3f
{
	vec3f pos;
	vec3f dir;
	float length = 0;

	inline ray3f() {}
	inline ray3f(const vec3f& _pos, const vec3f& _dir, float _length) : pos(_pos), dir(_dir), length(_length) {}

	static ray3f fromTwoPoints(const vec3f& a, const vec3f& b) { const auto ba = b - a; const float len = ba.len(); return ray3f(a, ba / len, len); }
};

}
