
#pragma once

#include "Core/Math.h"
#include "Core/ObjectIterationCore.h"


namespace ui {


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
	};
	u8 flags = Loop | AccelSmoothing;

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
};


} // ui
