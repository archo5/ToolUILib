
#pragma once

#include "Math.h"

#include <math.h>
#include <float.h>


namespace ui {

enum DoNotInitializeTag
{
	DoNotInitialize
};


struct Vec3f
{
	float x, y, z;

	UI_FORCEINLINE Vec3f(DoNotInitializeTag) {}
	UI_FORCEINLINE Vec3f() : x(0), y(0), z(0) {}
	UI_FORCEINLINE Vec3f(float f) : x(f), y(f), z(f) {}
	UI_FORCEINLINE Vec3f(float ax, float ay) : x(ax), y(ay), z(0) {}
	UI_FORCEINLINE Vec3f(float ax, float ay, float az) : x(ax), y(ay), z(az) {}

	UI_FORCEINLINE Vec3f operator + (const Vec3f& o) const { return { x + o.x, y + o.y, z + o.z }; }
	UI_FORCEINLINE Vec3f operator - (const Vec3f& o) const { return { x - o.x, y - o.y, z - o.z }; }
	UI_FORCEINLINE Vec3f operator * (const Vec3f& o) const { return { x * o.x, y * o.y, z * o.z }; }
	UI_FORCEINLINE Vec3f operator / (const Vec3f& o) const { return { x / o.x, y / o.y, z / o.z }; }

	UI_FORCEINLINE Vec3f& operator += (const Vec3f& o) { x += o.x; y += o.y; z += o.z; return *this; }
	UI_FORCEINLINE Vec3f& operator -= (const Vec3f& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	UI_FORCEINLINE Vec3f& operator *= (const Vec3f& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	UI_FORCEINLINE Vec3f& operator /= (const Vec3f& o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	UI_FORCEINLINE Vec3f& operator *= (float f) { x *= f; y *= f; z *= f; return *this; }
	UI_FORCEINLINE Vec3f& operator /= (float f) { x /= f; y /= f; z /= f; return *this; }

	UI_FORCEINLINE Vec3f operator * (float f) const { return { x * f, y * f, z * f }; }

	UI_FORCEINLINE Vec3f operator + () const { return *this; }
	UI_FORCEINLINE Vec3f operator - () const { return { -x, -y, -z }; }

	UI_FORCEINLINE bool operator == (const Vec3f& o) const { return x == o.x && y == o.y && z == o.z; }

	UI_FORCEINLINE float LengthSq() const { return x * x + y * y + z * z; }
	UI_FORCEINLINE float Length() const { return sqrtf(LengthSq()); }
	Vec3f Normalized() const
	{
		float lsq = LengthSq();
		if (lsq == 0)
			return { 0, 0, 0 };
		float q = 1.0f / sqrtf(lsq);
		return { x * q, y * q, z * q };
	}

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "Vec3");
		OnField(oi, "x", x);
		OnField(oi, "y", y);
		OnField(oi, "z", z);
		oi.EndObject();
	}
};

inline Vec3f Vec3Lerp(const Vec3f& a, const Vec3f& b, float s) { return { lerp(a.x, b.x, s), lerp(a.y, b.y, s), lerp(a.z, b.z, s) }; }
inline Vec3f Vec3Lerp(const Vec3f& a, const Vec3f& b, const Vec3f& s) { return { lerp(a.x, b.x, s.x), lerp(a.y, b.y, s.y), lerp(a.z, b.z, s.z) }; }
UI_FORCEINLINE float Vec3Dot(const Vec3f& a, const Vec3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3f Vec3Cross(const Vec3f& a, const Vec3f& b)
{
	return
	{
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y,
	};
}


struct Vec4f
{
	float x, y, z, w;

	UI_FORCEINLINE Vec3f GetVec3() const { return { x, y, z }; }
	UI_FORCEINLINE Vec3f WDivide() const { return { x / w, y / w, z / w }; }
};
UI_FORCEINLINE Vec4f V4(const Vec3f& v, float w) { return { v.x, v.y, v.z, w }; }


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

	UI_FORCEINLINE static Mat4f Translate(const Vec3f& v) { return Translate(v.x, v.y, v.z); }
	static Mat4f Translate(float x, float y, float z);

	UI_FORCEINLINE static Mat4f Scale(float q) { return Scale(q, q, q); }
	UI_FORCEINLINE static Mat4f Scale(const Vec3f& v) { return Scale(v.x, v.y, v.z); }
	static Mat4f Scale(float x, float y, float z);

	static Mat4f RotateX(float a);
	static Mat4f RotateY(float a);
	static Mat4f RotateZ(float a);
	static Mat4f RotateAxisAngle(const Vec3f& axis, float a);

	UI_FORCEINLINE static Mat4f LookAtLH(const Vec3f& pos, const Vec3f& tgt, const Vec3f& up) { return LookAtDirLH(pos, tgt - pos, up); }
	static Mat4f LookAtDirLH(const Vec3f& pos, const Vec3f& dir, const Vec3f& up);
	UI_FORCEINLINE static Mat4f LookAtRH(const Vec3f& pos, const Vec3f& tgt, const Vec3f& up) { return LookAtDirRH(pos, tgt - pos, up); }
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

	UI_FORCEINLINE Mat4f Transposed() const
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

	bool InvertTo(Mat4f& inv) const;
	Mat4f Inverted() const
	{
		Mat4f ret;
		if (InvertTo(ret))
			return ret;
		return Mat4f::Identity();
	}

	Vec4f TransformPointNoDivide(const Vec3f& p) const
	{
		return
		{
			p.x * v00 + p.y * v01 + p.z * v02 + v03,
			p.x * v10 + p.y * v11 + p.z * v12 + v13,
			p.x * v20 + p.y * v21 + p.z * v22 + v23,
			p.x * v30 + p.y * v31 + p.z * v32 + v33,
		};
	}
	Vec3f TransformPoint(const Vec3f& p) const
	{
		auto t = TransformPointNoDivide(p);
		return { t.x / t.w, t.y / t.w, t.z / t.w };
	}
	Vec3f TransformDirection(const Vec3f& d) const
	{
		return
		{
			d.x * v00 + d.y * v01 + d.z * v02,
			d.x * v10 + d.y * v11 + d.z * v12,
			d.x * v20 + d.y * v21 + d.z * v22,
		};
	}

	Vec3f GetTranslation() const { return { v03, v13, v23 }; }
	Vec3f GetScale() const;

	Mat4f GetRotationMatrix() const;
	Mat4f RemoveTranslation() const;
	Mat4f RemoveScale() const;
};


struct Ray3f
{
	Vec3f origin;
	Vec3f direction;

	UI_FORCEINLINE Ray3f() {}
	UI_FORCEINLINE Ray3f(const Vec3f& p, const Vec3f& d) : origin(p), direction(d) {}

	Vec3f GetPoint(float dist) const { return origin + direction * dist; }
};


UI_FORCEINLINE float PlanePointSignedDistance(const Vec4f& plane, const Vec3f& point)
{
	return Vec3Dot(plane.GetVec3(), point) - plane.w;
}

struct RayPlaneIntersectResult
{
	// distance to intersection (positive if in front, negative if behind)
	float dist;
	// plane orientation wrt ray direction (negative if ray is oriented towards front, positive if towards back, 0 if no intersection)
	float angcos;
};
inline RayPlaneIntersectResult RayPlaneIntersect(const Vec3f& pos, const Vec3f& dir, const Vec4f& plane)
{
	float diralign = -Vec3Dot(dir, plane.GetVec3());
	if (diralign == 0)
		return { 0, diralign };
	float posdist = PlanePointSignedDistance(plane, pos);
	return { posdist / diralign, diralign };
}

inline Ray3f GetCameraRay(const Mat4f& invVP, float x, float y)
{
	Vec3f p0 = { x, y, 0 };
	Vec3f p1 = { x, y, 1 };
	p0 = invVP.TransformPoint(p0);
	p1 = invVP.TransformPoint(p1);
	return { p0, (p1 - p0).Normalized() };
}

} // ui
