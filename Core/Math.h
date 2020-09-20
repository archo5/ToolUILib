
#pragma once

#include <math.h>


template <class T> struct Point
{
	T x, y;

	Point<T> operator + (const Point<T>& o) const { return { x + o.x, y + o.y }; }
	Point<T> operator - (const Point<T>& o) const { return { x - o.x, y - o.y }; }
	Point<T>& operator += (const Point<T>& o) { x += o.x; y += o.y; return *this; }
	Point<T>& operator -= (const Point<T>& o) { x -= o.x; y -= o.y; return *this; }
	Point<T>& operator *= (float f) { x *= f; y *= f; return *this; }

	template <class U> Point<U> Cast() const { return { U(x), U(y) }; }
};

template <class T> struct Size
{
	T x, y;

	template <class U> Size<U> Cast() const { return { U(x), U(y) }; }
};

template <class T> struct Range
{
	T min, max;
};


template<class T> struct AABB
{
	T x0, y0, x1, y1;

	static AABB UniformBorder(T v) { return { v, v, v, v }; }
	static AABB FromPoint(T x, T y) { return { x, y, x, y }; }
	static AABB FromCenterExtents(T x, T y, T e) { return { x - e, y - e, x + e, y + e }; }
	static AABB FromCenterExtents(T x, T y, T ex, T ey) { return { x - ex, y - ey, x + ex, y + ey }; }
	T GetWidth() const { return x1 - x0; }
	T GetHeight() const { return y1 - y0; }
	Size<T> GetSize() const { return { GetWidth(), GetHeight() }; }
	bool Contains(T x, T y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
	bool Contains(Point<T> p) const { return p.x >= x0 && p.x < x1 && p.y >= y0 && p.y < y1; }
	bool Intersects(const AABB& o) const { return x0 <= o.x1 && o.x0 <= x1 && y0 <= o.y1 && o.y0 <= y1; }
	AABB ExtendBy(const AABB& ext) const { return { x0 - ext.x0, y0 - ext.y0, x1 + ext.x1, y1 + ext.y1 }; }
	AABB ShrinkBy(const AABB& ext) const { return { x0 + ext.x0, y0 + ext.y0, x1 - ext.x1, y1 - ext.y1 }; }
	AABB MoveBy(float dx, float dy) const { return { x0 + dx, y0 + dy, x1 + dx, y1 + dy }; }
	AABB operator * (T f) const { return { x0 * f, y0 * f, x1 * f, y1 * f }; }

	template <class U> AABB<U> Cast() const { return { U(x0), U(y0), U(x1), U(y1) }; }
};
template <class T> AABB<T> operator * (T f, const AABB<T>& o) { return o * f; }


inline float lerp(float a, float b, float s) { return a + (b - a) * s; }
inline float invlerp(float a, float b, float x) { return (x - a) / (b - a); }
template <class T> __forceinline T min(T a, T b) { return a < b ? a : b; }
template <class T> __forceinline T max(T a, T b) { return a > b ? a : b; }
template <class T> __forceinline T clamp(T x, T vmin, T vmax) { return x < vmin ? vmin : x > vmax ? vmax : x; }
