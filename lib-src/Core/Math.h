
#pragma once

#include "Platform.h"
#include "Common.h"
#include "ObjectIteration.h"

#include <math.h>


namespace ui {

constexpr float E = 2.7182817f;
constexpr float PI = 3.1415927f;
constexpr float PI2 = 3.1415927f * 2;
constexpr float DEG2RAD = PI / 180;
constexpr float RAD2DEG = 180 / PI;


UI_FORCEINLINE float lerp(float a, float b, float s) { return a + (b - a) * s; }
UI_FORCEINLINE float invlerp(float a, float b, float x) { return (x - a) / (b - a); }
UI_FORCEINLINE float invlerpc(float a, float b, float x) { return clamp01((x - a) / (b - a)); }
UI_FORCEINLINE float sign(float x) { return x == 0 ? 0.0f : x > 0 ? 1.0f : -1.0f; }

inline float MoveTowards(float cur, float tgt, float maxStep)
{
	float diff = tgt - cur;
	float dist = fabsf(diff);
	if (dist > maxStep)
	{
		float sgn = sign(diff);
		return cur + maxStep * sgn;
	}
	else
		return tgt;
}


inline float AngleNormalize360(float a)
{
	a = fmodf(a, 360);
	if (a < 0)
		a += 360;
	return a;
}
inline float AngleNormalize180(float a)
{
	a = fmodf(a, 360);
	if (a < -180)
		a += 360;
	else if (a > 180)
		a -= 360;
	return a;
}
inline float AngleDiff(float from, float to)
{
	return AngleNormalize180(to - from);
}
inline float AngleLerp(float a, float b, float s)
{
	return a + AngleDiff(a, b) * s;
}
inline float AngleMoveTowards(float cur, float tgt, float maxdist)
{
	float diff = AngleDiff(cur, tgt);
	float dsign = sign(diff);
	float dabs = fabsf(diff);
	return cur + dsign * min(dabs, maxdist);
}


template <class T> struct Vec2
{
	T x, y;

	UI_FORCEINLINE Vec2(DoNotInitialize) {}
	UI_FORCEINLINE Vec2() : x(0), y(0) {}
	UI_FORCEINLINE Vec2(T v) : x(v), y(v) {}
	UI_FORCEINLINE Vec2(T ax, T ay) : x(ax), y(ay) {}

	UI_FORCEINLINE Vec2 operator + (const Vec2& o) const { return { x + o.x, y + o.y }; }
	UI_FORCEINLINE Vec2 operator - (const Vec2& o) const { return { x - o.x, y - o.y }; }
	UI_FORCEINLINE Vec2 operator * (const Vec2& o) const { return { x * o.x, y * o.y }; }
	UI_FORCEINLINE Vec2 operator / (const Vec2& o) const { return { x / o.x, y / o.y }; }
	UI_FORCEINLINE Vec2 operator * (T f) const { return { x * f, y * f }; }
	UI_FORCEINLINE Vec2 operator / (T f) const { return { x / f, y / f }; }

	UI_FORCEINLINE Vec2& operator += (const Vec2& o) { x += o.x; y += o.y; return *this; }
	UI_FORCEINLINE Vec2& operator -= (const Vec2& o) { x -= o.x; y -= o.y; return *this; }
	UI_FORCEINLINE Vec2& operator *= (const Vec2& o) { x *= o.x; y *= o.y; return *this; }
	UI_FORCEINLINE Vec2& operator /= (const Vec2& o) { x /= o.x; y /= o.y; return *this; }
	UI_FORCEINLINE Vec2& operator *= (T f) { x *= f; y *= f; return *this; }
	UI_FORCEINLINE Vec2& operator /= (T f) { x /= f; y /= f; return *this; }

	UI_FORCEINLINE Vec2 operator + () const { return *this; }
	UI_FORCEINLINE Vec2 operator - () const { return { -x, -y }; }

	UI_FORCEINLINE bool operator == (const Vec2& o) const { return x == o.x && y == o.y; }
	UI_FORCEINLINE bool operator != (const Vec2& o) const { return x != o.x || y != o.y; }

	UI_FORCEINLINE T LengthSq() const { return x * x + y * y; }
	UI_FORCEINLINE float Length() const { return sqrtf(x * x + y * y); }
	UI_FORCEINLINE float Angle() const { return atan2(y, x) * RAD2DEG; }
	UI_FORCEINLINE float Angle2() const { return atan2(x, y) * RAD2DEG; }
	Vec2<float> Normalized() const
	{
		T lsq = LengthSq();
		if (!lsq)
			return { 0, 0 };
		float l = sqrtf(lsq);
		return { x / l, y / l };
	}
	UI_FORCEINLINE Vec2 Abs() const { return { abs(x), abs(y) }; }
	UI_FORCEINLINE Vec2 Perp() const { return { -y, x }; }
	UI_FORCEINLINE Vec2 Perp2() const { return { y, -x }; }
	Vec2 Rotated(float angle) const
	{
		float c = cosf(angle * DEG2RAD);
		float s = sinf(angle * DEG2RAD);
		return
		{
			T(x * c - y * s),
			T(x * s + y * c),
		};
	}

	bool OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 2, &x))
			return true;
		bool ret = oi.BeginObject(FI, "Vec2");
		ret &= OnField(oi, "x", x);
		ret &= OnField(oi, "y", y);
		oi.EndObject();
		return ret;
	}

	template <class U> UI_FORCEINLINE Vec2<U> Cast() const { return { U(x), U(y) }; }
	template <class U> UI_FORCEINLINE Vec2<U> CastRounded() const { return { U(round(x)), U(round(y)) }; }
	template <class U> UI_FORCEINLINE Vec2<U> CastFloored() const { return { U(floor(x)), U(floor(y)) }; }
};
template <class T>
inline size_t HashValue(Vec2<T> v)
{
	size_t h = HashValue(v.x);
	h *= 121;
	h ^= HashValue(v.y);
	return h;
}

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Point2f = Vec2<float>;
using Point2i = Vec2<int>;

template <class T> UI_FORCEINLINE T Vec2Dot(const Vec2<T>& a, const Vec2<T>& b) { return a.x * b.x + a.y * b.y; }
template <class T> UI_FORCEINLINE T Vec2Cross(const Vec2<T>& a, const Vec2<T>& b) { return a.x * b.y - a.y * b.x; }
UI_FORCEINLINE Vec2f Vec2fLerp(Vec2f a, Vec2f b, float q) { return a * (1 - q) + b * q; }
inline Vec2f Vec2fFromAngle(float angle) { angle *= DEG2RAD; return { cosf(angle), sinf(angle) }; }
inline Vec2f Vec2fFromAngle2(float angle) { angle *= DEG2RAD; return { sinf(angle), cosf(angle) }; }

template <class T> struct Size2
{
	T x, y;

	UI_FORCEINLINE Size2(DoNotInitialize) {}
	UI_FORCEINLINE Size2() : x(0), y(0) {}
	UI_FORCEINLINE Size2(T ax, T ay) : x(ax), y(ay) {}

	UI_FORCEINLINE Size2 operator * (T f) const { return { x * f, y * f }; }
	UI_FORCEINLINE Size2 operator / (T f) const { return { x / f, y / f }; }

	UI_FORCEINLINE T Min() const { return min(x, y); }
	UI_FORCEINLINE T Max() const { return max(x, y); }
	float GetAspectRatio() const { return y == 0 ? 1 : float(x) / float(y); }

	UI_FORCEINLINE bool operator == (const Size2& o) const { return x == o.x && y == o.y; }
	UI_FORCEINLINE bool operator != (const Size2& o) const { return x != o.x || y != o.y; }

	template <class U> UI_FORCEINLINE Size2<U> Cast() const { return { U(x), U(y) }; }
	UI_FORCEINLINE Vec2<T> ToVec2() const { return { x, y }; }

	bool OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 2, &x))
			return true;
		bool ret = oi.BeginObject(FI, "Size2");
		ret &= OnField(oi, "x", x);
		ret &= OnField(oi, "y", y);
		oi.EndObject();
		return ret;
	}
};

using Size2f = Size2<float>;
using Size2i = Size2<int>;

struct All {};

template <class T> struct Range
{
	using Limits = std::numeric_limits<T>;

	T min, max;

	UI_FORCEINLINE Range(DoNotInitialize) {}
	UI_FORCEINLINE Range(All) : min(Limits::lowest()), max(Limits::max()) {}
	UI_FORCEINLINE Range(T _min, T _max) : min(_min), max(_max) {}
	UI_FORCEINLINE static Range AtLeast(T v) { return { v, Limits::max() }; }
	UI_FORCEINLINE static Range AtMost(T v) { return { Limits::lowest(), v }; }
	UI_FORCEINLINE static Range Exact(T v) { return { v, v }; }
	UI_FORCEINLINE static Range All() { return { Limits::lowest(), Limits::max() }; }
	UI_FORCEINLINE static Range Empty() { return { Limits::max(), Limits::lowest() }; }

	UI_FORCEINLINE bool operator == (const Range& o) const { return min == o.min && max == o.max; }
	UI_FORCEINLINE bool operator != (const Range& o) const { return min != o.min || max != o.max; }

	bool OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 2, &min))
			return true;
		bool ret = oi.BeginObject(FI, "Range");
		ret &= OnField(oi, "min", min);
		ret &= OnField(oi, "max", max);
		oi.EndObject();
		return ret;
	}

	UI_FORCEINLINE T GetWidth() const { return max - min; }
	UI_FORCEINLINE bool IsValid() const { return min <= max; }
	UI_FORCEINLINE bool Contains(T v) const { return v >= min && v < max; }
	UI_FORCEINLINE bool Overlaps(const Range& o) const { return min < o.max && o.min < max; }
	inline Range Intersect(const Range& o) const { return { ::ui::max(min, o.min), ::ui::min(max, o.max) }; }
	inline Range LimitTo(const Range& limit) const { return { limit.Clamp(min), limit.Clamp(max) }; }
	Range Add(T o) const
	{
		Range r = *this;
		if (r.min < Limits::max())
			r.min += o;
		if (r.max < Limits::max())
			r.max += o;
		return r;
	}
	UI_FORCEINLINE void Include(T o)
	{
		min = ui::min(min, o);
		max = ui::max(max, o);
	}
	UI_FORCEINLINE void Include(Range o)
	{
		min = ui::min(min, o.min);
		max = ui::max(max, o.max);
	}
	Range With(T o) const
	{
		Range r = *this;
		r.Include(o);
		return r;
	}
	Range With(Range o) const
	{
		Range r = *this;
		r.Include(o);
		return r;
	}
	UI_FORCEINLINE T Clamp(T val) const { return clamp(val, min, max); }

	template <class U> static U _CastSingle(T val)
	{
		if (val == Limits::lowest())
			return Range<U>::Limits::lowest();
		if (val == Limits::max())
			return Range<U>::Limits::max();
		if (!Limits::is_integer && Range<U>::Limits::is_integer)
			return U(round(val));
		return U(val);
	}
	template <class U> Range<U> Cast() const
	{
		Range<U> ret = DoNotInitialize{};
		ret.min = _CastSingle<U>(min);
		ret.max = _CastSingle<U>(max);
		return ret;
	}
};

using Rangei = Range<int>;
using Rangef = Range<float>;


template<class T> struct AABB2
{
	T x0, y0, x1, y1;

	static constexpr T MIN_VALUE = std::numeric_limits<T>::lowest();
	static constexpr T MAX_VALUE = std::numeric_limits<T>::max();

	UI_FORCEINLINE AABB2(DoNotInitialize) {}
	UI_FORCEINLINE AABB2() : x0(0), y0(0), x1(0), y1(0) {}
	UI_FORCEINLINE AABB2(T ax0, T ay0, T ax1, T ay1) : x0(ax0), y0(ay0), x1(ax1), y1(ay1) {}
	UI_FORCEINLINE AABB2(Vec2<T> pos) : x0(pos.x), y0(pos.y), x1(pos.x), y1(pos.y) {}
	UI_FORCEINLINE AABB2(Vec2<T> amin, Vec2<T> amax) : x0(amin.x), y0(amin.y), x1(amax.x), y1(amax.y) {}
	UI_FORCEINLINE static AABB2 Zero() { return {}; }
	UI_FORCEINLINE static AABB2 Empty() { return { MAX_VALUE, MAX_VALUE, MIN_VALUE, MIN_VALUE }; }
	UI_FORCEINLINE static AABB2 All() { return { MIN_VALUE, MIN_VALUE, MAX_VALUE, MAX_VALUE }; }
	UI_FORCEINLINE static AABB2 UniformBorder(T v) { return { v, v, v, v }; }
	UI_FORCEINLINE static AABB2 FromPoint(T x, T y) { return { x, y, x, y }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(T x, T y, T e) { return { x - e, y - e, x + e, y + e }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(T x, T y, T ex, T ey) { return { x - ex, y - ey, x + ex, y + ey }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(Vec2<T> c, T e) { return { c.x - e, c.y - e, c.x + e, c.y + e }; }
	UI_FORCEINLINE static AABB2 FromCenterExtents(Vec2<T> c, Vec2<T> e) { return { c.x - e.x, c.y - e.y, c.x + e.x, c.y + e.y }; }
	UI_FORCEINLINE static AABB2 FromTwoPoints(Vec2<T> p0, Vec2<T> p1) { return AABB2(p0).With(p1); }

	UI_FORCEINLINE bool operator == (const AABB2& o) const { return x0 == o.x0 && y0 == o.y0 && x1 == o.x1 && y1 == o.y1; }
	UI_FORCEINLINE bool operator != (const AABB2& o) const { return !(*this == o); }

	UI_FORCEINLINE Vec2<T> GetP00() const { return { x0, y0 }; }
	UI_FORCEINLINE Vec2<T> GetP10() const { return { x1, y0 }; }
	UI_FORCEINLINE Vec2<T> GetP01() const { return { x0, y1 }; }
	UI_FORCEINLINE Vec2<T> GetP11() const { return { x1, y1 }; }
	UI_FORCEINLINE T GetWidth() const { return x1 - x0; }
	UI_FORCEINLINE T GetHeight() const { return y1 - y0; }
	UI_FORCEINLINE T GetAspectRatio() const { return y0 == y1 ? 1 : GetWidth() / GetHeight(); }
	UI_FORCEINLINE Size2<T> GetSize() const { return { GetWidth(), GetHeight() }; }
	UI_FORCEINLINE Vec2<T> GetMin() const { return { x0, y0 }; }
	UI_FORCEINLINE Vec2<T> GetMax() const { return { x1, y1 }; }
	UI_FORCEINLINE T GetCenterX() const { return (x0 + x1) / 2; }
	UI_FORCEINLINE T GetCenterY() const { return (y0 + y1) / 2; }
	UI_FORCEINLINE Vec2<T> GetCenter() const { return { (x0 + x1) / 2, (y0 + y1) / 2 }; }
	UI_FORCEINLINE bool IsValid() const { return x0 <= x1 && y0 <= y1; }
	UI_FORCEINLINE bool Contains(T x, T y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
	UI_FORCEINLINE bool Contains(Vec2<T> p) const { return p.x >= x0 && p.x < x1 && p.y >= y0 && p.y < y1; }
	// assumes both AABBs are valid, may return unexpected results with invalid AABBs
	UI_FORCEINLINE bool Contains(const AABB2& o) const { return o.x0 >= x0 && o.x1 <= x1 && o.y0 >= y0 && o.y1 <= y1; }
	UI_FORCEINLINE bool Overlaps(const AABB2& o) const { return x0 <= o.x1 && o.x0 <= x1 && y0 <= o.y1 && o.y0 <= y1; }
	UI_FORCEINLINE AABB2 Intersect(const AABB2& o) const { return { max(x0, o.x0), max(y0, o.y0), min(x1, o.x1), min(y1, o.y1) }; }
	UI_FORCEINLINE AABB2 ExtendBy(T v) const { return { x0 - v, y0 - v, x1 + v, y1 + v }; }
	UI_FORCEINLINE AABB2 ShrinkBy(T v) const { return { x0 + v, y0 + v, x1 - v, y1 - v }; }
	UI_FORCEINLINE AABB2 ExtendBy(const AABB2& ext) const { return { x0 - ext.x0, y0 - ext.y0, x1 + ext.x1, y1 + ext.y1 }; }
	UI_FORCEINLINE AABB2 ShrinkBy(const AABB2& ext) const { return { x0 + ext.x0, y0 + ext.y0, x1 - ext.x1, y1 - ext.y1 }; }
	UI_FORCEINLINE AABB2 MoveBy(T dx, T dy) const { return { x0 + dx, y0 + dy, x1 + dx, y1 + dy }; }
	UI_FORCEINLINE void Include(const AABB2& o) { x0 = min(x0, o.x0); y0 = min(y0, o.y0); x1 = max(x1, o.x1); y1 = max(y1, o.y1); }
	UI_FORCEINLINE void Include(const Vec2<T>& o) { x0 = min(x0, o.x); y0 = min(y0, o.y); x1 = max(x1, o.x); y1 = max(y1, o.y); }
	UI_FORCEINLINE AABB2 With(const AABB2& o) const { return { min(x0, o.x0), min(y0, o.y0), max(x1, o.x1), max(y1, o.y1) }; }
	UI_FORCEINLINE AABB2 With(const Vec2<T>& o) const { return { min(x0, o.x), min(y0, o.y), max(x1, o.x), max(y1, o.y) }; }
	UI_FORCEINLINE AABB2 operator * (T f) const { return { x0 * f, y0 * f, x1 * f, y1 * f }; }
	UI_FORCEINLINE AABB2 operator + (Vec2<T> v) const { return { x0 + v.x, y0 + v.y, x1 + v.x, y1 + v.y }; }
	UI_FORCEINLINE AABB2 operator - (Vec2<T> v) const { return { x0 - v.x, y0 - v.y, x1 - v.x, y1 - v.y }; }
	UI_FORCEINLINE AABB2& operator += (Vec2<T> v) { x0 += v.x; y0 += v.y; x1 += v.x; y1 += v.y; return *this; }
	UI_FORCEINLINE AABB2& operator -= (Vec2<T> v) { x0 -= v.x; y0 -= v.y; x1 -= v.x; y1 -= v.y; return *this; }
	UI_FORCEINLINE Vec2<T> Lerp(Vec2<T> q) const { return { lerp(x0, x1, q.x), lerp(y0, y1, q.y) }; }
	UI_FORCEINLINE Vec2<T> LerpFlipY(Vec2<T> q) const { return { lerp(x0, x1, q.x), lerp(y1, y0, q.y) }; }
	UI_FORCEINLINE Vec2<T> InverseLerp(Vec2<T> p) const { return { invlerp(x0, x1, p.x), invlerp(y0, y1, p.y) }; }
	UI_FORCEINLINE Vec2<T> InverseLerpFlipY(Vec2<T> p) const { return { invlerp(x0, x1, p.x), invlerp(y1, y0, p.y) }; }

	bool OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		if (OnFieldMany(oi, FI, 4, &x0))
			return true;
		bool ret = oi.BeginObject(FI, "AABB2");
		ret &= OnField(oi, "x0", x0);
		ret &= OnField(oi, "y0", y0);
		ret &= OnField(oi, "x1", x1);
		ret &= OnField(oi, "y1", y1);
		oi.EndObject();
		return ret;
	}

	template <class U> UI_FORCEINLINE AABB2<U> Cast() const { return { U(x0), U(y0), U(x1), U(y1) }; }
	template <class U> UI_FORCEINLINE AABB2<U> CastRounded() const { return { U(round(x0)), U(round(y0)), U(round(x1)), U(round(y1)) }; }
	template <class U> UI_FORCEINLINE AABB2<U> CastFloored() const { return { U(floor(x0)), U(floor(y0)), U(floor(x1)), U(floor(y1)) }; }
};
template <class T> UI_FORCEINLINE AABB2<T> operator * (T f, const AABB2<T>& o) { return o * f; }

using AABB2f = AABB2<float>;
using AABB2i = AABB2<int>;

// TODO to cpp file?
inline AABB2f RectGenCentered(AABB2f space, Size2f size, Vec2f placement = { 0.5f, 0.5f }, bool roundPos = true)
{
	float x = space.x0 + (space.GetWidth() - size.x) * placement.x;
	float y = space.y0 + (space.GetHeight() - size.y) * placement.y;
	if (roundPos)
	{
		x = roundf(x);
		y = roundf(y);
	}
	return { x, y, x + size.x, y + size.y };
}

inline AABB2f RectGenFitAspect(AABB2f space, float iasp, Vec2f placement = { 0.5f, 0.5f }, bool roundPos = true)
{
	Size2f scaledSize = space.GetSize();
	float rasp = space.GetSize().GetAspectRatio();
	if (iasp > rasp) // matched width, adjust height
		scaledSize.y = scaledSize.x / iasp;
	else
		scaledSize.x = scaledSize.y * iasp;
	return RectGenCentered(space, scaledSize, placement, roundPos);
}

inline AABB2f RectGenFitSize(AABB2f space, Size2f size, Vec2f placement = { 0.5f, 0.5f }, bool roundPos = true)
{
	float iasp = size.GetAspectRatio();
	return RectGenFitAspect(space, iasp, placement, roundPos);
}

inline AABB2f RectGenFitAspectRange(AABB2f space, Rangef iaspRange, Vec2f placement = { 0.5f, 0.5f }, bool roundPos = true)
{
	Size2f scaledSize = space.GetSize();
	float rasp = space.GetSize().GetAspectRatio();
	float iasp = iaspRange.Clamp(rasp);
	if (iasp > rasp) // matched width, adjust height
		scaledSize.y = scaledSize.x / iasp;
	else
		scaledSize.x = scaledSize.y * iasp;
	return RectGenCentered(space, scaledSize, placement, roundPos);
}

inline AABB2f RectGenFillAspect(AABB2f space, float iasp, Vec2f placement = { 0.5f, 0.5f }, bool roundPos = true)
{
	Size2f scaledSize = space.GetSize();
	float rasp = space.GetSize().GetAspectRatio();
	if (iasp < rasp) // matched width, adjust height
		scaledSize.y = scaledSize.x / iasp;
	else
		scaledSize.x = scaledSize.y * iasp;
	return RectGenCentered(space, scaledSize, placement, roundPos);
}

inline AABB2f RectGenFillSize(AABB2f space, Size2f size, Vec2f placement = { 0.5f, 0.5f }, bool roundPos = true)
{
	float iasp = size.GetAspectRatio();
	return RectGenFillAspect(space, iasp, placement, roundPos);
}

inline AABB2f RectInvLerp(AABB2f space, AABB2f input)
{
	auto size = space.GetSize();
	return
	{
		(input.x0 - space.x0) / size.x,
		(input.y0 - space.y0) / size.y,
		(input.x1 - space.x0) / size.x,
		(input.y1 - space.y0) / size.y,
	};
}


struct ScaleOffset2D
{
	float scale, x, y;

	UI_FORCEINLINE ScaleOffset2D() : scale(1), x(0), y(0) {}
	UI_FORCEINLINE ScaleOffset2D(DoNotInitialize) {}
	UI_FORCEINLINE ScaleOffset2D(float _scale, float _x = 0, float _y = 0) : scale(_scale), x(_x), y(_y) {}
	UI_FORCEINLINE static ScaleOffset2D OffsetThenScale(float _x, float _y, float _scale) { return ScaleOffset2D(_scale, _x * _scale, _y * _scale); }

	UI_FORCEINLINE ScaleOffset2D GetScaleOnly() { return { scale }; }

	UI_FORCEINLINE float TransformX(float v) const { return v * scale + x; }
	UI_FORCEINLINE float TransformY(float v) const { return v * scale + y; }
	UI_FORCEINLINE Vec2f TransformPoint(Vec2f p) const { return { p.x * scale + x, p.y * scale + y }; }
	UI_FORCEINLINE Vec2f InverseTransformPoint(Vec2f p) const { return { (p.x - x) / scale, (p.y - y) / scale }; }
	UI_FORCEINLINE ScaleOffset2D operator * (const ScaleOffset2D& o) const { return { scale * o.scale, x * o.scale + o.x, y * o.scale + o.y }; }
};
UI_FORCEINLINE AABB2f operator * (const AABB2f& rect, const ScaleOffset2D& xf) { return { xf.TransformX(rect.x0), xf.TransformY(rect.y0), xf.TransformX(rect.x1), xf.TransformY(rect.y1) }; }

} // ui
