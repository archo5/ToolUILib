
#pragma once

#include "Curve_Sequence01.h"

#include "Editors/CurveEditor.h"


namespace ui {


struct Curve_Sequence01_View : ICurveView
{
	Curve_Sequence01* curve = nullptr;

	uint32_t GetFeatures() override { return SliceMidpoints; }

	uint32_t GetCurveCount() override { return 1; }
	uint32_t GetPointCount(uint32_t) override { return curve->points.size(); }

	Vec2f GetPoint(uint32_t, uint32_t pointid) override
	{
		return { curve->points[pointid].posX, curve->points[pointid].posY };
	}
	void SetPoint(uint32_t, uint32_t pointid, Vec2f p) override;
	void SwapPoints(u32 curveid, u32 pointid) override { std::swap(curve->points[pointid], curve->points[pointid + 1]); }
	Vec2f GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q) override;

	bool HasSliceMidpoint(uint32_t curveid, uint32_t sliceid) override { return true; }
	Vec2f GetSliceMidpoint(uint32_t curveid, uint32_t sliceid) override
	{
		return { 0.5f, curve->points[sliceid + 1].tweak };
	}
	void SetSliceMidpoint(uint32_t curveid, uint32_t sliceid, Vec2f p) override
	{
		curve->points[sliceid + 1].tweak = p.y;
	}
	float GetSliceMidpointVertDragFactor(uint32_t curveid, uint32_t sliceid) override;
	Vec2f GetSliceMidpointPosition(uint32_t curveid, uint32_t sliceid) override;

	void OnEvent(const CurveEditorInput& input, Event& e) override;
};


} // ui
