
#pragma once

#include "Curve_CubicNrmRemap.h"

#include "Editors/CurveEditor.h"


namespace ui {


struct Curve_CubicNormalizedRemap_View : ICurveView
{
	Curve_CubicNormalizedRemap* curve = nullptr;

	uint32_t GetFeatures() override { return Tangents; }

	uint32_t GetCurveCount() override { return 1; }
	uint32_t GetPointCount(uint32_t) override { return 2; }

	Vec2f GetPoint(uint32_t, uint32_t pointid) override { return pointid == 0 ? Vec2f(0, 0) : Vec2f(1, 1); }
	void SetPoint(uint32_t, uint32_t pointid, Vec2f p) override {}

	bool HasLeftTangent(uint32_t, uint32_t pointid) override { return pointid == 1; }
	Vec2f GetLeftTangentDiff(uint32_t, uint32_t) override { return -curve->t1l; }
	void SetLeftTangentDiff(uint32_t, uint32_t, Vec2f d) override { curve->t1l = -Vec2f(clamp01(d.x), d.y); }
	bool HasRightTangent(uint32_t, uint32_t pointid) override { return pointid == 0; }
	Vec2f GetRightTangentDiff(uint32_t, uint32_t) override { return curve->t0r; }
	void SetRightTangentDiff(uint32_t, uint32_t, Vec2f d) override { curve->t0r = { clamp01(d.x), d.y }; }

	Vec2f GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q) override
	{
		return { q, curve->InterpolateSlow(q) };
	}

	void OnEvent(const CurveEditorInput& input, Event& e) override;
};


} // ui
