
#pragma once

#include <algorithm>


template <class T> struct Point
{
	T x, y;
};


template<class T> struct AABB
{
	T x0, y0, x1, y1;

	T GetWidth() const { return x1 - x0; }
	T GetHeight() const { return y1 - y0; }
	bool Contains(T x, T y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
	bool Contains(Point<T> p) const { return p.x >= x0 && p.x < x1 && p.y >= y0 && p.y < y1; }
	AABB ExtendBy(const AABB& ext) const { return { x0 - ext.x0, y0 - ext.y0, x1 + ext.x1, y1 + ext.y1 }; }
	AABB ShrinkBy(const AABB& ext) const { return { x0 + ext.x0, y0 + ext.y0, x1 - ext.x1, y1 - ext.y1 }; }
	AABB operator * (T f) const { return { x0 * f, y0 * f, x1 * f, y1 * f }; }
};
template <class T> AABB<T> operator * (T f, const AABB<T>& o) { return o * f; }


inline float lerp(float a, float b, float s) { return a + (b - a) * s; }
inline float invlerp(float a, float b, float x) { return (x - a) / (b - a); }
