
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

bool CQS_FindCurveMidpoint(Curve_QuadSpline::Point pa, Curve_QuadSpline::Point pb, Vec2f& outcmp)
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

static float invqslerp(float p0, float p1, float p2, float v)
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

static float qslerp(float a, float b, float c, float q)
{
	float ab = lerp(a, b, q);
	float bc = lerp(b, c, q);
	return lerp(ab, bc, q);
}

static float curvesample(ArrayView<Curve_QuadSpline::Point> curve, Rangef range, float sx, bool accelsmoothing)
{
	size_t pos = curve.Size() - 1;
	for (size_t i = 0; i < curve.Size(); i++)
	{
		if (sx > curve[i].time)
		{
			pos = i;
		}
	}
	auto pa = curve[pos];
	if (pa.time > sx)
		pa.time -= range.GetWidth();
	auto pb = curve.NextWrap(pos);
	if (pb.time < sx)
		pb.time += range.GetWidth();

	Vec2f cmp;
	if (CQS_FindCurveMidpoint(pa, pb, cmp))
	{
		float q = invqslerp(pa.time, cmp.x, pb.time, sx);
		float cv = qslerp(pa.value, cmp.y, pb.value, q);
		if (!accelsmoothing)
			return cv;

		float pwlv = sx <= cmp.x
			? lerp(pa.value, cmp.y, invlerpc(pa.time, cmp.x, sx))
			: lerp(cmp.y, pb.value, invlerpc(cmp.x, pb.time, sx));

		float qq = sinf(acosf(q * 2 - 1));
		return lerp(pwlv, cv, qq);
	}
	else
	{
		float q = invlerpc(pa.time, pb.time, sx);
		return lerp(pa.value, pb.value, q);
	}
}

float Curve_QuadSpline::Sample(float t)
{
	if (flags & Loop)
		t = curvewrap(timeRange, t);
	else
		t = timeRange.Clamp(t);
	return curvesample(points, timeRange, t, flags & AccelSmoothing);
}

Rangef Curve_QuadSpline::CalcHeightRange()
{
	Rangef range = Rangef::Empty();
	for (size_t i = 0; i < points.Size(); i++)
	{
		range.Include(points[i].value);

		if ((flags & Loop) || i + 1 < points.Size())
		{
			auto pa = points[i];
			auto pb = points.NextWrap(i);
			Vec2f cmp;
			if (CQS_FindCurveMidpoint(pa, pb, cmp))
			{
				// excessive
				//range.Include(cmp.y);

				float da = 2 * (cmp.y - pa.value);
				float db = 2 * (pb.value - cmp.y);
				float root_t = invlerp(da, db, 0);
				if (root_t >= 0 && root_t <= 1)
				{
					range.Include(qslerp(pa.value, cmp.y, pb.value, root_t));
				}
			}
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
