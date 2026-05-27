
#include "Curve_QuadSpline.h"


namespace ui {


static bool StraightLineIntersection(Vec2f a0, Vec2f a1, Vec2f b0, Vec2f b1, Vec2f& outISP)
{
	float denom = (a0.x - a1.x) * (b0.y - b1.y) - (a0.y - a1.y) * (b0.x - b1.x);
	if (denom == 0)
		return false;
	float qanum = (a0.x - b0.x) * (b0.y - b1.y) - (a0.y - b0.y) * (b0.x - b1.x);
	float qbnum = (a0.x - b0.x) * (a0.y - a1.y) - (a0.y - b0.y) * (a0.x - a1.x);
	if (denom < 0)
	{
		qanum = -qanum;
		qbnum = -qbnum;
		denom = -denom;
	}
	outISP = b0 + (b1 - b0) * (qbnum / denom);
	return true;
}

bool QLIFSpline_FindCurveMidpoint(const QLIFSplinePoint& pa, const QLIFSplinePoint& pb, Vec2f& outcmp)
{
	Vec2f pa2 = { pa.time, pa.value };
	Vec2f pb2 = { pb.time, pb.value };
	Vec2f pa2d = Vec2f(1, pa.velocity);
	Vec2f pb2d = -Vec2f(1, pb.velocity);
	Vec2f isp;
	if (!StraightLineIntersection(pa2, pa2 + pa2d, pb2, pb2 + pb2d, isp))
		return false;

	if (Vec2Dot(pa2d, isp - pa2) <= 0)
		return false;

	if (Vec2Dot(pb2d, isp - pb2) <= 0)
		return false;

	outcmp = isp;
	return true;
}

Vec2f QLIFSpline_TransitionPointFromPosition(const QLIFSplinePoint& pa, const QLIFSplinePoint& pb, Vec2f pos)
{
	float tdiff = pb.time - pa.time;

	QLIFSplinePoint x0a = pa;
	QLIFSplinePoint x0b = { pa.time, pb.value - tdiff * pb.velocity, pb.velocity };
	QLIFSplinePoint x1a = { pb.time, pa.value + tdiff * pa.velocity, pa.velocity };
	QLIFSplinePoint x1b = pb;

	float qx = invlerpc(pa.time, pb.time, pos.x);

	Vec2f isp;
	bool flipy = false;
	if (QLIFSpline_FindCurveMidpoint(pa, pb, isp))
	{
		// tangents are intersecting - the interpolation space is one of two quads between tangent and lerp
		if (pos.x < isp.x) // TODO assumes that pa.time <= pb.time
		{
			x1a = x1b;
		}
		else
		{
			x0b = x0a;
			flipy = true;
		}
	}
	else
	{
		// tangents are not intersecting - the interpolation space is a quad
	}
	float y0 = lerp(x0a.value, x1a.value, qx);
	float y1 = lerp(x0b.value, x1b.value, qx);
	float qy = invlerpc(y0, y1, pos.y);
	if (flipy)
		qy = 1 - qy;

	return { qx, qy };
}

QLIFSplinePoint QLIFSpline_InstantiateTransitionPoint(const QLIFSplinePoint& pa, const QLIFSplinePoint& pb, const QLIFSplineTransitionPoint& xp)
{
	float tdiff = pb.time - pa.time;

	QLIFSplinePoint x0a = pa;
	QLIFSplinePoint x0b = { pa.time, pb.value - tdiff * pb.velocity, pb.velocity };
	QLIFSplinePoint x1a = { pb.time, pa.value + tdiff * pa.velocity, pa.velocity };
	QLIFSplinePoint x1b = pb;

	QLIFSplinePoint itrp;
	itrp.time = lerp(pa.time, pb.time, xp.qx);

	bool midp;
	Vec2f isp;
	float qy = xp.qy;
	if (midp = QLIFSpline_FindCurveMidpoint(pa, pb, isp))
	{
		// tangents are intersecting - the interpolation space is one of two quads between tangent and lerp
		if (itrp.time < isp.x) // TODO assumes that pa.time <= pb.time
		{
			x1a = x1b;
		}
		else
		{
			x0b = x0a;
			qy = 1 - qy;
		}
	}
	else
	{
		// tangents are not intersecting - the interpolation space is a quad
	}
	itrp.value = lerp(
		lerp(x0a.value, x1a.value, xp.qx),
		lerp(x0b.value, x1b.value, xp.qx), qy);

	// an acceptable velocity is one that causes the line to hit both tangents
	// the ranges can be calculated for each tangent line individually and then intersected
	itrp.velocity = 0;

	const float BIG = 10000;
	Rangef avr = { -BIG, BIG };
	Rangef bvr = { -BIG, BIG };
	float avel = divf_safe(itrp.value - pa.value, itrp.time - pa.time);
	float bvel = divf_safe(pb.value - itrp.value, pb.time - itrp.time);
	if (itrp.value > pa.value + pa.velocity * (itrp.time - pa.time)) // point above tangent
	{
		avr.min = avel;
	}
	else // point below tangent
	{
		avr.max = avel;
	}
	if (itrp.value > pb.value + pb.velocity * (itrp.time - pb.time)) // point above tangent
	{
		bvr.max = bvel;
	}
	else // point below tangent
	{
		bvr.min = bvel;
	}
	Rangef svr = avr.Intersect(bvr);
	if (svr.IsValid())
	{
		itrp.velocity = tanf(lerp(atanf(svr.min), atanf(svr.max), xp.qs));
	}

	return itrp;
}

static float curvewrap(Rangef range, float sx)
{
	if (!range.Contains(sx))
	{
		sx -= range.min;
		sx = fmodf(sx, range.GetWidth());
		if (sx < 0)
			sx += range.GetWidth();
		sx += range.min;
	}
	return sx;
}

float invqslerp(float p0, float p1, float p2, float v)
{
	// convert quadratic bezier to polynomial factors
	// y =  (A-2B+C)x^2  +  2(B-A)x  +  A
	float a = p0 - p1 * 2 + p2;
	if (a == 0)
		return 0;

	float b = 2 * (p1 - p0);
	float c = p0 - v;

	float r1 = (-b + sqrtf(b * b - 4 * a * c)) / (2 * a);
	float r2 = (-b - sqrtf(b * b - 4 * a * c)) / (2 * a);
	// r1 or r2 can go slightly outside of the range in edge cases
	float away1 = fabsf(r1 - clamp01(r1));
	float away2 = fabsf(r2 - clamp01(r2));
	return clamp01(away1 < away2 ? r1 : r2);
}

float qslerp(float a, float b, float c, float q)
{
	float ab = lerp(a, b, q);
	float bc = lerp(b, c, q);
	return lerp(ab, bc, q);
}

size_t FindCurveSection(const float* timevalues, size_t stride, size_t count, float x)
{
#if 1 // binary search
	size_t iterpos = 0;
	size_t itercount = count;
	while (count > 0)
	{
		size_t it = iterpos; 
		size_t step = count / 2;
		it += step;

		if (*(const float*)(((const char*)timevalues) + stride * it) < x)
		{
			iterpos = ++it;
			count -= step + 1;
		} 
		else
			count = step;
	}

	return iterpos;
#else
	for (size_t i = 0; i < count; i++)
	{
		if (x < *timevalues)
			return i;
		timevalues = (const float*)(((const char*)timevalues) + stride);
	}
	return count;
#endif
}

void CurveSectionToPoints(size_t cs, size_t numcs, bool loop, size_t& p0, size_t& p1)
{
	if (loop)
	{
		p0 = cs ? cs - 1 : numcs - 1;
		p1 = cs == numcs ? 0 : cs;
	}
	else
	{
		p0 = cs ? cs - 1 : 0;
		p1 = cs == numcs ? cs - 1 : cs;
	}
}

static float curvesample(ArrayView<Curve_QuadSpline::Point> curve, Rangef range, float sx, bool loop, bool accelsmoothing, bool extrapolate)
{
	size_t cs = FindCurveSection(&curve.Data()->time, sizeof(curve[0]), curve.Size(), sx);
	size_t p0, p1;
	CurveSectionToPoints(cs, curve.Size(), loop, p0, p1);
	auto pa = curve[p0];
	auto pb = curve[p1];
	if (loop)
	{
		if (pa.time > sx) pa.time -= range.GetWidth();
		if (pb.time < sx) pb.time += range.GetWidth();
	}
	else if (extrapolate)
	{
		if (sx < pa.time) return pa.value + pa.velocity * (sx - pa.time);
		if (sx > pb.time) return pb.value + pb.velocity * (sx - pb.time);
	}

	if (pa.enableTransitionPoint)
	{
		auto itp = QLIFSpline_InstantiateTransitionPoint(pa, pb, pa);
		if (sx < itp.time)
			return QLIFSpline_Interpolate(pa, itp, sx, accelsmoothing);
		else
			return QLIFSpline_Interpolate(itp, pb, sx, accelsmoothing);
	}

	return QLIFSpline_Interpolate(pa, pb, sx, accelsmoothing);
}

float QLIFSpline_Interpolate(const QLIFSplinePoint& pa, const QLIFSplinePoint& pb, float sx, bool accelsmoothing)
{
	Vec2f cmp;
	if (QLIFSpline_FindCurveMidpoint(pa, pb, cmp))
	{
		float q = invqslerp(pa.time, cmp.x, pb.time, sx);
		float cv = qslerp(pa.value, cmp.y, pb.value, q);
		if (!accelsmoothing)
			return cv;

		float pwlv = sx <= cmp.x
			? lerp(pa.value, cmp.y, invlerpc(pa.time, cmp.x, sx))
			: lerp(cmp.y, pb.value, invlerpc(cmp.x, pb.time, sx));

#if QLIF_V1_ACCEL_SMOOTHING
		float qq = sinf(acosf(q * 2 - 1));
#else // v2
		float qqq = sx <= cmp.x ? invlerpc(cmp.x, pa.time, sx) * -1 : invlerpc(cmp.x, pb.time, sx);
		float qq = qqq * qqq;
		qq *= qq;
		qq = 1 - qq;
#endif
		return lerp(pwlv, cv, qq);
	}
	else
	{
		float q = invlerpc(pa.time, pb.time, sx);
		return lerp(pa.value, pb.value, q);
	}
}

void QLIFSpline_RootVertRange(Rangef& outrange, const QLIFSplinePoint& pa, const QLIFSplinePoint& pb)
{
	Vec2f cmp;
	if (QLIFSpline_FindCurveMidpoint(pa, pb, cmp))
	{
		// excessive
		//range.Include(cmp.y);

		float da = 2 * (cmp.y - pa.value);
		float db = 2 * (pb.value - cmp.y);
		float root_t = invlerp(da, db, 0);
		if (root_t >= 0 && root_t <= 1)
		{
			outrange.Include(qslerp(pa.value, cmp.y, pb.value, root_t));
		}
	}
}

float Curve_QuadSpline::Sample(float t)
{
	if (flags & Loop)
		t = curvewrap(timeRange, t);
	return curvesample(points, timeRange, t, flags & Loop, flags & AccelSmoothing, flags & Extrapolate);
}

Rangef Curve_QuadSpline::CalcHeightRange()
{
	Rangef range = Rangef::Empty();
	for (size_t i = 0; i < points.Size(); i++)
	{
		range.Include(points[i].value);

		if ((flags & Loop) || i + 1 < points.Size())
		{
			QLIFSpline_RootVertRange(range, points[i], points.NextWrap(i));
		}
	}
	return range;
}

void Curve_QuadSpline::InsertPoint(float time, float value, float velocity)
{
	size_t inspos = 0;
	for (size_t i = 0; i < points.Size(); i++)
	{
		if (time > points[i].time)
		{
			inspos = i + 1;
		}
	}

	points.InsertAt(inspos, { time, value, velocity });
}


} // ui
