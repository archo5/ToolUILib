
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

	virtual bool HasLeftTangent(uint32_t, uint32_t pointid) { return pointid == 1; }
	virtual Vec2f GetLeftTangentPoint(uint32_t, uint32_t) { return Vec2f(1, 1) - curve->t1l; }
	virtual void SetLeftTangentPoint(uint32_t, uint32_t, Vec2f p) { curve->t1l = Vec2f(1 - clamp(p.x, 0.0f, 1.0f), 1 - p.y); }
	virtual bool HasRightTangent(uint32_t, uint32_t pointid) { return pointid == 0; }
	virtual Vec2f GetRightTangentPoint(uint32_t, uint32_t) { return curve->t0r; }
	virtual void SetRightTangentPoint(uint32_t, uint32_t, Vec2f p) { curve->t0r = { clamp(p.x, 0.0f, 1.0f), p.y }; }

	Vec2f GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q) override
	{
		return { q, curve->InterpolateSlow(q) };
	}

	void OnEvent(const CurveEditorInput& input, Event& e) override;
};


} // ui
