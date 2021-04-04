
#pragma once

#include "Platform.h"
#include "Common.h"
#include "ObjectIteration.h"

#include <math.h>


namespace ui {

constexpr float PI = 3.14159f;
constexpr float PI2 = 3.14159f * 2;
constexpr float DEG2RAD = 3.14159f / 180;
constexpr float RAD2DEG = 180 * 3.14159f;


inline float lerp(float a, float b, float s) { return a + (b - a) * s; }
inline float invlerp(float a, float b, float x) { return (x - a) / (b - a); }


template <class T> struct Vec2
{
	T x, y;

	UI_FORCEINLINE Vec2 operator + (const Vec2& o) const { return { x + o.x, y + o.y }; }
	UI_FORCEINLINE Vec2 operator - (const Vec2& o) const { return { x - o.x, y - o.y }; }
	UI_FORCEINLINE Vec2 operator * (T f) const { return { x * f, y * f }; }
	UI_FORCEINLINE Vec2 operator / (T f) const { return { x / f, y / f }; }
	UI_FORCEINLINE Vec2& operator += (const Vec2& o) { x += o.x; y += o.y; return *this; }
	UI_FORCEINLINE Vec2& operator -= (const Vec2& o) { x -= o.x; y -= o.y; return *this; }
	UI_FORCEINLINE Vec2& operator *= (T f) { x *= f; y *= f; return *this; }
	UI_FORCEINLINE Vec2& operator /= (T f) { x /= f; y /= f; return *this; }

	UI_FORCEINLINE bool operator == (const Vec2& o) const { return x == o.x && y == o.y; }
	UI_FORCEINLINE bool operator != (const Vec2& o) const { return x != o.x || y != o.y; }

	UI_FORCEINLINE T LengthSq() const { return x * x + y * y; }
	UI_FORCEINLINE float Length() const { return sqrtf(x * x + y * y); }
	Vec2 Normalized() const
	{
		T lsq = LengthSq();
		if (!lsq)
			return { 0, 0 };
		float l = sqrtf(lsq);
		return { x / l, y / l };
	}
	UI_FORCEINLINE Vec2 Perp() const { return { -y, x }; }
	UI_FORCEINLINE Vec2 Perp2() const { return { y, -x }; }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "Vec2");
		OnField(oi, "x", x);
		OnField(oi, "y", y);
		oi.EndObject();
	}

	template <class U> UI_FORCEINLINE Vec2<U> Cast() const { return { U(x), U(y) }; }
};

using Vec2f = Vec2<float>;
using Point2f = Vec2<float>;
using Point2i = Vec2<int>;

template <class T> UI_FORCEINLINE T Vec2Dot(const Vec2<T>& a, const Vec2<T>& b) { return a.x * b.x + a.y * b.y; }

template <class T> struct Size2
{
	T x, y;

	template <class U> UI_FORCEINLINE Size2<U> Cast() const { return { U(x), U(y) }; }
};

using Size2f = Size2<float>;
using Size2i = Size2<int>;

template <class T> struct Range
{
	Range(T _min = std::numeric_limits<T>::lowest(), T _max = std::numeric_limits<T>::max()) : min(_min), max(_max) {}

	T min, max;
};

using Range2f = Range<float>;


template<class T> struct AABB2
{
	T x0, y0, x1, y1;

	UI_FORCEINLINE static AABB2 UniformBorder(T v) { return { v, v, v, v }; }
	UI_FORCEINLINE static AABB2 FromPoint(T x, T y) { return { x, y, x, y }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(T x, T y, T e) { return { x - e, y - e, x + e, y + e }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(T x, T y, T ex, T ey) { return { x - ex, y - ey, x + ex, y + ey }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(Vec2<T> c, T e) { return { c.x - e, c.y - e, c.x + e, c.y + e }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(Vec2<T> c, Vec2<T> e) { return { c.x - e.x, c.y - e.y, c.x + e.x, c.y + e.y }; }
	UI_FORCEINLINE T GetWidth() const { return x1 - x0; }
	UI_FORCEINLINE T GetHeight() const { return y1 - y0; }
	UI_FORCEINLINE T GetAspectRatio() const { return y0 == y1 ? 1 : GetWidth() / GetHeight(); }
	UI_FORCEINLINE Size2<T> GetSize() const { return { GetWidth(), GetHeight() }; }
	UI_FORCEINLINE Vec2<T> GetMin() const { return { x0, y0 }; }
	UI_FORCEINLINE Vec2<T> GetMax() const { return { x1, y1 }; }
	UI_FORCEINLINE bool Contains(T x, T y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
	UI_FORCEINLINE bool Contains(Vec2<T> p) const { return p.x >= x0 && p.x < x1 && p.y >= y0 && p.y < y1; }
	UI_FORCEINLINE bool Intersects(const AABB2& o) const { return x0 <= o.x1 && o.x0 <= x1 && y0 <= o.y1 && o.y0 <= y1; }
	UI_FORCEINLINE AABB2 ExtendBy(const AABB2& ext) const { return { x0 - ext.x0, y0 - ext.y0, x1 + ext.x1, y1 + ext.y1 }; }
	UI_FORCEINLINE AABB2 ShrinkBy(const AABB2& ext) const { return { x0 + ext.x0, y0 + ext.y0, x1 - ext.x1, y1 - ext.y1 }; }
	UI_FORCEINLINE AABB2 MoveBy(float dx, float dy) const { return { x0 + dx, y0 + dy, x1 + dx, y1 + dy }; }
	UI_FORCEINLINE AABB2 operator * (T f) const { return { x0 * f, y0 * f, x1 * f, y1 * f }; }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "AABB2");
		OnField(oi, "x0", x0);
		OnField(oi, "y0", y0);
		OnField(oi, "x1", x1);
		OnField(oi, "y1", y1);
		oi.EndObject();
	}

	template <class U> UI_FORCEINLINE AABB2<U> Cast() const { return { U(x0), U(y0), U(x1), U(y1) }; }
};
template <class T> UI_FORCEINLINE AABB2<T> operator * (T f, const AABB2<T>& o) { return o * f; }

using AABB2f = AABB2<float>;
using AABB2i = AABB2<int>;

} // ui
