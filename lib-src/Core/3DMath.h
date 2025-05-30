
#pragma once

#include "Math.h"

#include <math.h>
#include <float.h>


namespace ui {


template <class T> struct Vec3
{
	T x, y, z;

	UI_FORCEINLINE Vec3(DoNotInitialize) {}
	UI_FORCEINLINE Vec3() : x(0), y(0), z(0) {}
	UI_FORCEINLINE Vec3(T f) : x(f), y(f), z(f) {}
	UI_FORCEINLINE Vec3(T ax, T ay) : x(ax), y(ay), z(0) {}
	UI_FORCEINLINE Vec3(T ax, T ay, T az) : x(ax), y(ay), z(az) {}
	UI_FORCEINLINE Vec3(Vec2<T> v, T az = 0) : x(v.x), y(v.y), z(az) {}

	UI_FORCEINLINE Vec3 operator + (const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
	UI_FORCEINLINE Vec3 operator - (const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
	UI_FORCEINLINE Vec3 operator * (const Vec3& o) const { return { x * o.x, y * o.y, z * o.z }; }
	UI_FORCEINLINE Vec3 operator / (const Vec3& o) const { return { x / o.x, y / o.y, z / o.z }; }

	UI_FORCEINLINE Vec3& operator += (const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
	UI_FORCEINLINE Vec3& operator -= (const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
	UI_FORCEINLINE Vec3& operator *= (const Vec3& o) { x *= o.x; y *= o.y; z *= o.z; return *this; }
	UI_FORCEINLINE Vec3& operator /= (const Vec3& o) { x /= o.x; y /= o.y; z /= o.z; return *this; }

	UI_FORCEINLINE Vec3& operator *= (T f) { x *= f; y *= f; z *= f; return *this; }
	UI_FORCEINLINE Vec3& operator /= (T f) { x /= f; y /= f; z /= f; return *this; }

	UI_FORCEINLINE Vec3 operator * (T f) const { return { x * f, y * f, z * f }; }

	UI_FORCEINLINE Vec3 operator + () const { return *this; }
	UI_FORCEINLINE Vec3 operator - () const { return { -x, -y, -z }; }

	UI_FORCEINLINE bool operator == (const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
	UI_FORCEINLINE bool operator != (const Vec3& o) const { return x != o.x || y != o.y || z != o.z; }

	UI_FORCEINLINE T LengthSq() const { return x * x + y * y + z * z; }
	UI_FORCEINLINE float Length() const { return sqrtf(LengthSq()); }
	Vec3<float> Normalized() const
	{
		float lsq = LengthSq();
		if (lsq == 0)
			return { 0, 0, 0 };
		float q = 1.0f / sqrtf(lsq);
		return { x * q, y * q, z * q };
	}

	UI_FORCEINLINE Vec2<T> ToVec2() const { return { x, y }; }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 3, &x))
			return;
		oi.BeginObject(FI, "Vec3");
		OnField(oi, "x", x);
		OnField(oi, "y", y);
		OnField(oi, "z", z);
		oi.EndObject();
	}

	template <class U> UI_FORCEINLINE Vec3<U> Cast() const { return { U(x), U(y), U(z) }; }
};
template <class T>
inline size_t HashValue(Vec3<T> v)
{
	size_t h = HashValue(v.x);
	h *= 121;
	h ^= HashValue(v.y);
	h *= 121;
	h ^= HashValue(v.z);
	return h;
}
template <class T>
UI_FORCEINLINE Vec3<T> operator * (T f, Vec3<T> v) { return { f * v.x, f * v.y, f * v.z }; }
template <class T>
UI_FORCEINLINE float Vec3Dot(const Vec3<T>& a, const Vec3<T>& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
template <class T>
inline Vec3<T> Vec3Cross(const Vec3<T>& a, const Vec3<T>& b)
{
	return
	{
		a.y * b.z - b.y * a.z,
		a.z * b.x - b.z * a.x,
		a.x * b.y - b.x * a.y,
	};
}

using Vec3i = Vec3<int>;
using Vec3f = Vec3<float>;

inline Vec3f Vec3Lerp(const Vec3f& a, const Vec3f& b, float s) { return { lerp(a.x, b.x, s), lerp(a.y, b.y, s), lerp(a.z, b.z, s) }; }
inline Vec3f Vec3Lerp(const Vec3f& a, const Vec3f& b, const Vec3f& s) { return { lerp(a.x, b.x, s.x), lerp(a.y, b.y, s.y), lerp(a.z, b.z, s.z) }; }


template <class T>
struct Vec4
{
	T x, y, z, w;

	UI_FORCEINLINE Vec4(DoNotInitialize) {}
	UI_FORCEINLINE Vec4() : x(0), y(0), z(0), w(0) {}
	UI_FORCEINLINE Vec4(T f) : x(f), y(f), z(f), w(f) {}
	UI_FORCEINLINE Vec4(T ax, T ay, T az, T aw) : x(ax), y(ay), z(az), w(aw) {}
	UI_FORCEINLINE Vec4(Vec3<T> v, T aw = 0) : x(v.x), y(v.y), z(v.z), w(aw) {}

	UI_FORCEINLINE Vec4 operator + () const { return *this; }
	UI_FORCEINLINE Vec4 operator - () const { return { -x, -y, -z, -w }; }

	UI_FORCEINLINE bool operator == (const Vec4& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	UI_FORCEINLINE bool operator != (const Vec4& o) const { return x != o.x || y != o.y || z != o.z || w != o.w; }

	UI_FORCEINLINE Vec3<T> GetVec3() const { return { x, y, z }; }
	UI_FORCEINLINE Vec3f WDivide() const { return { x / w, y / w, z / w }; }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 4, &x))
			return;
		oi.BeginObject(FI, "Vec4");
		OnField(oi, "x", x);
		OnField(oi, "y", y);
		OnField(oi, "z", z);
		OnField(oi, "w", w);
		oi.EndObject();
	}

	template <class U> UI_FORCEINLINE Vec4<U> Cast() const { return { U(x), U(y), U(z), U(w) }; }
};
using Vec4f = Vec4<float>;

inline Vec4f Vec4Lerp(const Vec4f& a, const Vec4f& b, float s) { return { lerp(a.x, b.x, s), lerp(a.y, b.y, s), lerp(a.z, b.z, s), lerp(a.w, b.w, s) }; }
inline Vec4f Vec4Lerp(const Vec4f& a, const Vec4f& b, const Vec4f& s) { return { lerp(a.x, b.x, s.x), lerp(a.y, b.y, s.y), lerp(a.z, b.z, s.z), lerp(a.w, b.w, s.w) }; }


struct Quat
{
	float x, y, z, w;

	Quat(DoNotInitialize) {}
	Quat() : x(0), y(0), z(0), w(1) {}
	UI_FORCEINLINE Quat(float ax, float ay, float az, float aw) : x(ax), y(ay), z(az), w(aw) {}

	Quat Normalized() const;
	UI_FORCEINLINE Quat Inverted() const { return { -x, -y, -z, w }; }
	UI_FORCEINLINE Quat operator - () const { return { -x, -y, -z, -w }; }

	Vec3f Axis() const;
	float Angle() const;

	Quat operator * (const Quat& o) const;
	Vec4f ToAxisAngle() const;
	Vec3f ToDirAxisLenAngle() const;
	Vec3f ToEulerAnglesXYZ() const;
	Vec3f ToEulerAnglesZYX() const;
	Vec3f Rotate(Vec3f v) const;

	UI_FORCEINLINE bool operator == (const Quat& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
	UI_FORCEINLINE bool operator != (const Quat& o) const { return x != o.x || y != o.y || z != o.z || w != o.w; }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 4, &x))
			return;
		oi.BeginObject(FI, "Quat");
		OnField(oi, "x", x);
		OnField(oi, "y", y);
		OnField(oi, "z", z);
		OnField(oi, "w", w);
		oi.EndObject();
	}

	static Quat Identity() { return { 0, 0, 0, 1 }; }
	static Quat RotateAxisAngle(const Vec3f& axis, float angle);
	static Quat RotateDirAxisLenAngle(const Vec3f& v);
	static Quat RotateBetweenNormalDirections(const Vec3f& a, const Vec3f& b);
	static Quat RotateBetweenDirections(const Vec3f& a, const Vec3f& b);
	static Quat RotateX(float angle);
	static Quat RotateY(float angle);
	static Quat RotateZ(float angle);
	static Quat RotateEulerAnglesXYZ(const Vec3f& angles);
	static Quat RotateEulerAnglesZYX(const Vec3f& angles);
};

inline float QuatDot(const Quat& a, const Quat& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
inline Quat QuatNLerp(const Quat& a, const Quat& b1, float s)
{
	Quat b = QuatDot(a, b1) >= 0 ? b1 : -b1;
	return Quat
	{
		lerp(a.x, b.x, s),
		lerp(a.y, b.y, s),
		lerp(a.z, b.z, s),
		lerp(a.w, b.w, s),
	}
	.Normalized();
}
inline Quat QuatSLerp(const Quat& a, const Quat& b1, float s)
{
	float dot = QuatDot(a, b1);
	Quat b = dot >= 0 ? b1 : -b1;
	float fl, fr;
	float absdot = fabsf(dot);
	if (absdot < 0.99999f)
	{
		float a = acosf(absdot);
		float rsa = 1.0f / sinf(a);
		fl = sinf((1 - s) * a) * rsa;
		fr = sinf(s * a) * rsa;
	}
	else
	{
		fl = 1 - s;
		fr = s;
	}
	return Quat
	{
		a.x * fl + b.x * fr,
		a.y * fl + b.y * fr,
		a.z * fl + b.z * fr,
		a.w * fl + b.w * fr,
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

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		OnFieldMany(oi, FI, 16, a);
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
	static Mat4f Rotate(const Quat& q);

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

	static Mat4f OrthoLH(float width, float height, float znear, float zfar);
	static Mat4f OrthoRH(float width, float height, float znear, float zfar);
	static Mat4f OrthoOffCenterLH(float left, float right, float top, float bottom, float znear, float zfar);
	static Mat4f OrthoOffCenterRH(float left, float right, float top, float bottom, float znear, float zfar);

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

	Vec4f Transform(const Vec4f& p) const
	{
		return
		{
			p.x * v00 + p.y * v01 + p.z * v02 + p.w * v03,
			p.x * v10 + p.y * v11 + p.z * v12 + p.w * v13,
			p.x * v20 + p.y * v21 + p.z * v22 + p.w * v23,
			p.x * v30 + p.y * v31 + p.z * v32 + p.w * v33,
		};
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
	Quat GetRotationQuaternion() const;
	Mat4f RemoveTranslation() const;
	Mat4f RemoveScale() const;
};

// a fast definition that works best in MSVC debug builds
static constexpr const Mat4f Mat4fIdentity = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 };


struct Transform3Df
{
	Vec3f position;
	Quat rotation;

	UI_FORCEINLINE Transform3Df(DoNotInitialize) : position(DoNotInitialize{}), rotation(DoNotInitialize{}) {}
	UI_FORCEINLINE Transform3Df() {}
	UI_FORCEINLINE Transform3Df(Vec3f p) : position(p) {}
	UI_FORCEINLINE Transform3Df(Quat r) : rotation(r) {}
	UI_FORCEINLINE Transform3Df(Vec3f p, Quat r) : position(p), rotation(r) {}
	static UI_FORCEINLINE Transform3Df Identity() { return {}; }
	static Transform3Df FromMatrixLossy(const Mat4f& m);

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);

	inline Transform3Df Inverted() const
	{
		Quat ir = rotation.Inverted();
		return { ir.Rotate(-position), ir };
	}
	inline Vec3f TransformPoint(Vec3f p) const { return rotation.Rotate(p) + position; }
	inline Vec3f TransformDir(Vec3f d) const { return rotation.Rotate(d); }
	inline Mat4f ToMatrix() const
	{
		return Mat4f::Rotate(rotation) * Mat4f::Translate(position);
	}
};
inline Transform3Df operator * (const Transform3Df& a, const Transform3Df& b)
{
	return { b.TransformPoint(a.position), a.rotation * b.rotation };
}
inline Transform3Df operator * (const Transform3Df& a, const Mat4f& b)
{
	return { b.TransformPoint(a.position), a.rotation * b.GetRotationQuaternion() };
}

Transform3Df TransformLerp(const Transform3Df& a, const Transform3Df& b, float q);


struct TransformScale3Df
{
	Vec3f position;
	Quat rotation;
	Vec3f scale = { 1, 1, 1 };

	UI_FORCEINLINE TransformScale3Df(DoNotInitialize) :
		position(DoNotInitialize{}),
		rotation(DoNotInitialize{}),
		scale(DoNotInitialize{})
	{}
	UI_FORCEINLINE TransformScale3Df() {}
	UI_FORCEINLINE TransformScale3Df(Vec3f p) : position(p) {}
	UI_FORCEINLINE TransformScale3Df(Vec3f p, Quat r) : position(p), rotation(r) {}
	UI_FORCEINLINE TransformScale3Df(Vec3f p, Quat r, Vec3f s) : position(p), rotation(r), scale(s) {}
	static UI_FORCEINLINE TransformScale3Df Identity() { return {}; }
	static TransformScale3Df FromMatrixLossy(const Mat4f& m);

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);

	Transform3Df WithoutScale() const { return { position, rotation }; }
	Mat4f ToMatrix() const;
	TransformScale3Df InvertedLossy() const;
	TransformScale3Df TransformLossy(const Mat4f& other) const;
	TransformScale3Df TransformLossy(const TransformScale3Df& other) const;

	inline Vec3f TransformPoint(Vec3f p) const { return rotation.Rotate(p * scale) + position; }
	inline Vec3f TransformDir(Vec3f d) const { return rotation.Rotate(d * scale); }
	inline Vec3f TransformNormalDir(Vec3f d) const { return rotation.Rotate(d / scale); }
	inline Vec3f TransformDirNoScale(Vec3f d) const { return rotation.Rotate(d); }
};

TransformScale3Df TransformLerp(const TransformScale3Df& a, const TransformScale3Df& b, float q);


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

Ray3f GetCameraRay(const Mat4f& invVP, float x, float y);

} // ui
