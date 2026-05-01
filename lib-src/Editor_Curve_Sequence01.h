
#pragma once

#include "Curve_Sequence01.h"

#include "Editors/CurveEditor.h"


namespace ui {


struct Curve_Sequence01_View : ICurveView
{
	Curve_Sequence01* curve = nullptr;

	u32 GetFeatures() override { return SliceMidpoints; }

	u32 GetCurveCount() override { return 1; }
	u32 GetPointCount(u32) override { return curve->points.size(); }

	Vec2f GetPoint(u32, u32 pointid) override
	{
		return { curve->points[pointid].posX, curve->points[pointid].posY };
	}
	void SetPoint(u32, u32 pointid, Vec2f p) override;
	void SwapPoints(u32 curveid, u32 pointid) override { std::swap(curve->points[pointid], curve->points[pointid + 1]); }

	float SampleCurve(u32 curveid, float x) override;
	void GetScreenCurvePoints(const CurveEditorInput& input, u32 curveid, Array<Vec2f>& curvepoints) override;

	bool HasSliceMidpoint(u32 curveid, u32 sliceid) override { return true; }
	Vec2f GetSliceMidpoint(u32 curveid, u32 sliceid) override
	{
		return { 0.5f, curve->points[sliceid + 1].tweak };
	}
	void SetSliceMidpoint(u32 curveid, u32 sliceid, Vec2f p) override
	{
		curve->points[sliceid + 1].tweak = p.y;
	}
	float GetSliceMidpointVertDragFactor(u32 curveid, u32 sliceid) override;
	Vec2f GetSliceMidpointPosition(u32 curveid, u32 sliceid) override;

	void OnEvent(const CurveEditorInput& input, Event& e) override;
};


} // ui
