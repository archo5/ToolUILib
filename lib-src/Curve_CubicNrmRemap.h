
#pragma once

#include "Core/Math.h"


namespace ui {


struct Curve_CubicNormalizedRemap
{
	Vec2f t0r = { 1.0f / 3.0f, 0.0f / 3.0f };
	Vec2f t1l = { 1.0f / 3.0f, 0.0f / 3.0f };

	static float cubiclerp(float x0, float x1, float x2, float x3, float q)
	{
		float x01 = lerp(x0, x1, q);
		float x12 = lerp(x1, x2, q);
		float x23 = lerp(x2, x3, q);
		float x012 = lerp(x01, x12, q);
		float x123 = lerp(x12, x23, q);
		return lerp(x012, x123, q);
	}
	float InterpolateSlow(float q)
	{
		float x1 = t0r.x;
		float x2 = 1 - t1l.x;
		float y1 = t0r.y;
		float y2 = 1 - t1l.y;

		// suboptimal but might be OK for UI and is far simpler
		Rangef search = { 0, 1 };
		for (int i = 0; i < 10; i++)
		{
			float testpos = (search.min + search.max) * 0.5f;
			float testresult = cubiclerp(0, x1, x2, 1, testpos);
			if (testresult < q)
				search.min = testpos;
			else
				search.max = testpos;
		}
		q = (search.min + search.max) * 0.5f;

		return cubiclerp(0, y1, y2, 1, q);
	}
};


} // ui
