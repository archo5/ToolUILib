
#pragma once

#include "Curve_QuadSpline.h"

#include "Editors/CurveEditor.h"


namespace ui {


struct Curve_QuadSpline_View : ICurveView
{
	Curve_QuadSpline* curve = nullptr;

	Curve_QuadSpline_View() {}
	Curve_QuadSpline_View(Curve_QuadSpline& c) : curve(&c) {}

	u32 GetFeatures() override { return SliceMidpoints | Tangents | DirOnlyTangents | MovableSliceMidpoints; }

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

	bool HasSliceMidpoint(u32, u32 sliceid) override
	{
		if (sliceid + 1 == curve->points.Size() && !(curve->flags & curve->Loop))
			return false;
		return curve->points[sliceid].enableTransitionPoint;
	}
	// slice midpoint coordinates: x = lerp factor, y = modifier (screen pixel units)
	Vec2f GetSliceMidpoint(u32, u32 sliceid) override
	{
		return { curve->points[sliceid].qx, curve->points[sliceid].qy };
	}
	void SetSliceMidpoint(u32, u32 sliceid, Vec2f p) override
	{
		//curve->points[sliceid].qx = clamp01(p.x);
		//curve->points[sliceid].qy = clamp01(p.y);
		curve->points[sliceid].qs = clamp01(p.y);
	}
	Vec2f GetSliceMidpointDragFactor(u32, u32 sliceid) override { return 0.01f; }
	Vec2f GetSliceMidpointPosition(u32, u32 sliceid) override
	{
		auto itp = QLIFSpline_InstantiateTransitionPoint(
			curve->points[sliceid],
			curve->points.NextWrap(sliceid),
			curve->points[sliceid]);
		return { itp.time, itp.value };
	}
	void SetSliceMidpointPosition(u32, u32 sliceid, Vec2f p) override
	{
		auto q = QLIFSpline_TransitionPointFromPosition(
			curve->points[sliceid],
			curve->points.NextWrap(sliceid),
			p);
		auto& pt = curve->points[sliceid];
		pt.qx = q.x;
		pt.qy = q.y;
	}

	void OnEvent(const CurveEditorInput& input, Event& e) override;

	void DrawAll(const CurveEditorInput& input, const CurveEditorState& state) override;
};


} // ui
