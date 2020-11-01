
#pragma once
#include <math.h>
#include "Math.h"


constexpr float DEG2RAD = 3.14159f / 180;
constexpr float RAD2DEG = 180 * 3.14159f;


struct Vec3f
{
	float x, y, z;

	Vec3f operator + (const Vec3f& o) const { return { x + o.x, y + o.y, z + o.z }; }
	Vec3f operator - (const Vec3f& o) const { return { x - o.x, y - o.y, z - o.z }; }
	Vec3f operator * (const Vec3f& o) const { return { x * o.x, y * o.y, z * o.z }; }
	Vec3f operator / (const Vec3f& o) const { return { x / o.x, y / o.y, z / o.z }; }

	Vec3f& operator += (const Vec3f& o) { x += o.x; y += o.y; z += o.z; return *this; }
	Vec3f& operator -= (const Vec3f& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	Vec3f& operator *= (const Vec3f& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	Vec3f& operator /= (const Vec3f& o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	Vec3f operator * (float f) const { return { x * f, y * f, z * f }; }

	Vec3f operator + () const { return *this; }
	Vec3f operator - () const { return { -x, -y, -z }; }

	bool operator == (const Vec3f& o) const { return x == o.x && y == o.y && z == o.z; }

	float LengthSq() const { return x * x + y * y + z * z; }
	float Length() const { return sqrtf(LengthSq()); }
	Vec3f Normalized() const
	{
		float lsq = LengthSq();
		if (lsq == 0)
			return { 0, 0, 0 };
		float q = 1.0f / sqrtf(lsq);
		return { x * q, y * q, z * q };
	}
};

inline Vec3f Vec3Lerp(const Vec3f& a, const Vec3f& b, float s) { return { lerp(a.x, b.x, s), lerp(a.y, b.y, s), lerp(a.z, b.z, s) }; }
inline float Vec3Dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3f Vec3Cross(const Vec3f& a, const Vec3f& b)
{
	return
	{
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y,
	};
}


struct Mat4f
{
	union
	{
		struct
		{
			float v00, v10, v20, v30;
			float v01, v11, v21, v31;
			float v02, v12, v22, v32;
			float v03, v13, v23, v33;
		};
		float a[16];
		float m[4][4];
	};

	static Mat4f Identity()
	{
		return
		{
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1,
		};
	}

	static Mat4f Translate(const Vec3f& v) { return Translate(v.x, v.y, v.z); }
	static Mat4f Translate(float x, float y, float z);

	static Mat4f Scale(const Vec3f& v) { return Scale(v.x, v.y, v.z); }
	static Mat4f Scale(float x, float y, float z);

	static Mat4f RotateX(float a);
	static Mat4f RotateY(float a);
	static Mat4f RotateZ(float a);

	static Mat4f LookAtLH(const Vec3f& pos, const Vec3f& tgt, const Vec3f& up) { return LookAtDirLH(pos, tgt - pos, up); }
	static Mat4f LookAtDirLH(const Vec3f& pos, const Vec3f& dir, const Vec3f& up);
	static Mat4f LookAtRH(const Vec3f& pos, const Vec3f& tgt, const Vec3f& up) { return LookAtDirRH(pos, tgt - pos, up); }
	static Mat4f LookAtDirRH(const Vec3f& pos, const Vec3f& dir, const Vec3f& up);

	static Mat4f PerspectiveFOVLH(float fovY, float aspect, float znear, float zfar)
	{
		float yscale = 1.0f / tanf(fovY / 2 * 3.14159f / 180);
		return PerspectiveExtLH(yscale / aspect, yscale, znear, zfar);
	}
	static Mat4f PerspectiveExtLH(float xscale, float yscale, float znear, float zfar);
	static Mat4f PerspectiveFOVRH(float fovY, float aspect, float znear, float zfar)
	{
		float yscale = 1.0f / tanf(fovY / 2 * 3.14159f / 180);
		return PerspectiveExtRH(yscale / aspect, yscale, znear, zfar);
	}
	static Mat4f PerspectiveExtRH(float xscale, float yscale, float znear, float zfar);

	Mat4f Transposed() const
	{
		return
		{
			v00, v01, v02, v03,
			v10, v11, v12, v13,
			v20, v21, v22, v23,
			v30, v31, v32, v33,
		};
	}

	Mat4f operator * (const Mat4f& o) const
	{
		Mat4f ret;
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				float sum = 0;
				for (int i = 0; i < 4; i++)
				{
					sum += m[x][i] * o.m[i][y];
				}
				ret.m[x][y] = sum;
			}
		}
		return ret;
	}
};
