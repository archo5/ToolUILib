
#pragma once

#include "Core/Math.h"
#include "Core/ObjectIterationCore.h"


namespace ui {


float invqslerp(float p0, float p1, float p2, float v);
float qslerp(float a, float b, float c, float q);
size_t FindCurveSection(const float* timevalues, size_t stride, size_t count, float x);
void CurveSectionToPoints(size_t cs, size_t numcs, bool loop, size_t& p0, size_t& p1);


struct Curve_QuadSpline
{
	Rangef timeRange = { 0, 1 };
	struct Point
	{
		float time;
		float value;
		float velocity;

		void OnSerialize(IObjectIterator& oi, const FieldInfo& fi)
		{
			oi.OnFieldManyF32(fi, 3, &time);
		}
	};
	Array<Point> points;
	enum
	{
		Loop = 1 << 0,
		AccelSmoothing = 1 << 1,
		// if not looping while sampling outside of the curve, ..
		// .. extend the value along velocity instead of just reusing the value of the closest point
		Extrapolate = 1 << 2,
	};
	u8 flags = Loop | AccelSmoothing | Extrapolate;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& fi)
	{
		oi.BeginObject(fi, "Curve_QuadSpline");
		ui::OnField(oi, "timeRange", timeRange);
		ui::OnField(oi, "points", points);
		ui::OnField(oi, "flags", flags);
		oi.EndObject();
	}
	float Sample(float t);
	Rangef CalcHeightRange();
	void InsertPoint(float time, float value, float velocity);
};

bool CQS_FindCurveMidpoint(Curve_QuadSpline::Point pa, Curve_QuadSpline::Point pb, Vec2f& outcmp);
float CQS_Interpolate(const Curve_QuadSpline::Point& pa, const Curve_QuadSpline::Point& pb, float sx, bool accelsmoothing);


} // ui
