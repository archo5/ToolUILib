
#pragma once

#include "Curve_QuadSpline.h"

#include "Editors/CurveEditor.h"


namespace ui {


struct Curve_QuadSpline_View : ICurveView
{
	Curve_QuadSpline* curve = nullptr;

	Curve_QuadSpline_View() {}
	Curve_QuadSpline_View(Curve_QuadSpline& c) : curve(&c) {}

	u32 GetFeatures() override { return Tangents | DirOnlyTangents; }

	u32 GetCurveCount() override { return 1; }
	u32 GetPointCount(u32) override { return curve->points.Size(); }

	Vec2f GetPoint(u32, u32 pointid) override
	{
		return { curve->points[pointid].time, curve->points[pointid].value };
	}
	void SetPoint(u32, u32 pointid, Vec2f p) override;
	void SwapPoints(u32 curveid, u32 pointid) override { std::swap(curve->points[pointid], curve->points[pointid + 1]); }

	float SampleCurve(u32, float x) override;

	Vec2f GetTangentDiff(u32 pointid, bool right)
	{
		auto& P = curve->points[pointid];
		Vec2f d = { 1, P.velocity };
		if (!right)
			d = -d;
		return d;
	}

	bool HasLeftTangent(u32, u32 pointid) override { return true; }
	Vec2f GetLeftTangentDiff(u32, u32 pointid) override { return GetTangentDiff(pointid, false); }
	void SetLeftTangentDiff(u32, u32 pointid, Vec2f d) override { curve->points[pointid].velocity = divf_safe(d.y, d.x); }
	bool HasRightTangent(u32, u32 pointid) override { return true; }
	Vec2f GetRightTangentDiff(u32, u32 pointid) override { return GetTangentDiff(pointid, true); }
	void SetRightTangentDiff(u32, u32 pointid, Vec2f d) override { curve->points[pointid].velocity = divf_safe(d.y, d.x); }

	void OnEvent(const CurveEditorInput& input, Event& e) override;

	void DrawAll(const CurveEditorInput& input, const CurveEditorState& state) override;
};


} // ui
