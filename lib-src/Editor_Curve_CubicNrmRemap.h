
#pragma once

#include "Curve_CubicNrmRemap.h"

#include "Editors/CurveEditor.h"


namespace ui {


struct Curve_CubicNormalizedRemap_View : ICurveView
{
	Curve_CubicNormalizedRemap* curve = nullptr;

	Curve_CubicNormalizedRemap_View() {}
	Curve_CubicNormalizedRemap_View(Curve_CubicNormalizedRemap& c) : curve(&c) {}

	u32 GetFeatures() override { return Tangents; }

	u32 GetCurveCount() override { return 1; }
	u32 GetPointCount(u32) override { return 2; }

	Vec2f GetPoint(u32, u32 pointid) override { return pointid == 0 ? Vec2f(0, 0) : Vec2f(1, 1); }
	void SetPoint(u32, u32 pointid, Vec2f p) override {}
	void SwapPoints(u32 curveid, u32 pointid) override {}

	float SampleCurve(u32 curveid, float x) override { return curve->InterpolateSlow(x); }

	bool HasLeftTangent(u32, u32 pointid) override { return pointid == 1; }
	Vec2f GetLeftTangentDiff(u32, u32) override { return -curve->t1l; }
	void SetLeftTangentDiff(u32, u32, Vec2f d) override { curve->t1l = { clamp01(-d.x), -d.y }; }
	bool HasRightTangent(u32, u32 pointid) override { return pointid == 0; }
	Vec2f GetRightTangentDiff(u32, u32) override { return curve->t0r; }
	void SetRightTangentDiff(u32, u32, Vec2f d) override { curve->t0r = { clamp01(d.x), d.y }; }

	void OnEvent(const CurveEditorInput& input, Event& e) override;
};


} // ui
