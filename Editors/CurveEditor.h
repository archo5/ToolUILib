
#pragma once

#include "../Model/Objects.h"


namespace ui {

struct CurveEditorSettings
{
	// to prevent init lists silently zeroing future members
	CurveEditorSettings() {}

	float pointRadius = 5;
	float tangentLineThickness = 1;
};

struct CurveEditorInput
{
	AABB2f viewport;
	AABB2f winRect;
	CurveEditorSettings* settings;
};

enum CurvePointType
{
	CPT_Point = 0,
	CPT_LTangent = 1,
	CPT_RTangent = 2,
	CPT_Midpoint = 3,
};

struct CurvePointID
{
	uint32_t curveID : 30;
	uint32_t pointType : 2; // CurvePointType
	uint32_t pointID;

	constexpr CurvePointID(uint32_t cid, CurvePointType t, uint32_t pid)
		: curveID(cid), pointType(t), pointID(pid)
	{
	}

	bool operator == (const CurvePointID& o) const
	{
		return curveID == o.curveID && pointType == o.pointType && pointID == o.pointID;
	}
	bool operator != (const CurvePointID& o) const { return !(*this == o); }
};

template<> struct SubUIValueHelper<CurvePointID>
{
	static constexpr CurvePointID GetNullValue()
	{
		return { UINT32_MAX, CPT_Midpoint, UINT32_MAX };
	}
};

struct CurveEditorState
{
	SubUI<CurvePointID> uiState;
	Vec2f dragOff;
};

struct ICurves
{
	static Range<uint32_t> ExpandForCurves(Range<uint32_t> src, uint32_t max);

	enum Feature
	{
		SliceMidpoints = 1 << 0,
		Tangents = 1 << 1,
	};
	virtual uint32_t GetFeatures() = 0;

	virtual AABB2f GetPreferredViewport(bool includeTangents);
	virtual AABB2f GetPreferredCurveViewport(bool includeTangents, uint32_t curveid, Range<uint32_t> pointRange);

	virtual uint32_t GetCurveCount() { return 1; }
	virtual uint32_t GetPointCount(uint32_t curveid) = 0;

	// only guarantees that at least the points you need will be in the range, as an optimization
	// does not guarantee that the smallest set of points is there
	virtual Range<uint32_t> GetLeastPointRange(uint32_t curveid, Rangef xrange) { return { 0, GetPointCount(curveid) }; }
	virtual Range<uint32_t> ExpandForTangents(uint32_t curveid, Range<uint32_t> src);

	virtual bool IsCurveVisible(uint32_t curveid) { return true; }
	virtual Color4b GetCurveColor(uint32_t curveid) { return { 127, 255 }; }

	virtual Vec2f GetPoint(uint32_t curveid, uint32_t pointid) = 0;
	virtual void SetPoint(uint32_t curveid, uint32_t pointid, Vec2f p) = 0;

	virtual Vec2f GetInterpolatedPoint(uint32_t curveid, uint32_t firstpointid, float q) = 0;
	virtual int GetCurvePointsForRange(uint32_t curveid, uint32_t firstpointid, Rangef qrange, Vec2f* out, int maxOut);
	virtual int GetCurvePointsForViewport(uint32_t curveid, uint32_t firstpointid, AABB2f vp, float winWidth, Vec2f* out, int maxOut);

	virtual bool HasLeftTangent(uint32_t curveid, uint32_t pointid) { return false; }
	virtual Vec2f GetLeftTangentPoint(uint32_t curveid, uint32_t pointid) { return {}; }
	virtual void SetLeftTangentPoint(uint32_t curveid, uint32_t pointid, Vec2f p) {}
	virtual bool HasRightTangent(uint32_t curveid, uint32_t pointid) { return false; }
	virtual Vec2f GetRightTangentPoint(uint32_t curveid, uint32_t pointid) { return {}; }
	virtual void SetRightTangentPoint(uint32_t curveid, uint32_t pointid, Vec2f p) {}

	enum class SliceMode
	{
		Empty, // no additional modification
		Modifier, // option to modify a parameter of the slice via vertical dragging
		ModMoveX, // same as before + ability to move on the X axis
	};
	virtual bool HasSliceMidpoint(uint32_t curveid, uint32_t sliceid) { return false; }
	// slice midpoint coordinates: x = lerp factor, y = modifier (screen pixel units)
	virtual Vec2f GetSliceMidpoint(uint32_t curveid, uint32_t sliceid) { return { 0.5f, 0 }; }
	virtual void SetSliceMidpoint(uint32_t curveid, uint32_t sliceid, Vec2f p) {}

	Vec2f GetScreenPoint(const CurveEditorInput& input, CurvePointID cpid);
	void SetScreenPoint(const CurveEditorInput& input, CurvePointID cpid, Vec2f sp);
	uint32_t _FixPointOrder(uint32_t curveid, uint32_t pointid);
	void _SwapPoints(uint32_t curveid, uint32_t p0, uint32_t p1);

	virtual CurvePointID HitTest(const CurveEditorInput& input, Vec2f cursorPos);
	virtual void DrawCurve(const CurveEditorInput& input, uint32_t curveid);
	virtual void DrawCurvePointsType(
		const CurveEditorInput& input,
		const CurveEditorState& state,
		uint32_t curveid,
		CurvePointType type,
		Range<uint32_t> pointRange);

	// overriding the order of point types to match hit test
	virtual void DrawAllPoints(const CurveEditorInput& input, const CurveEditorState& state);

	virtual void DrawAllTangentLines(const CurveEditorInput& input, const CurveEditorState& state);

	virtual void DrawAll(const CurveEditorInput& input, const CurveEditorState& state);
};


struct CurveEditorUI : CurveEditorState
{
	bool OnEvent(const CurveEditorInput& input, ICurves* curves, Event& e);
	void Render(const CurveEditorInput& input, ICurves* curves);
};

struct CurveEditorElement : UIElement
{
	void OnInit() override;
	void OnEvent(Event& e) override;
	void OnPaint() override;

	CurveEditorUI _ui;
	ICurves* curves = nullptr;
	AABB2f viewport = { 0, 0, 1, 1 };
	CurveEditorSettings settings;
};


struct BasicLinear01Curve : ICurves
{
	std::vector<Vec2f> points;

	uint32_t GetFeatures() override { return 0; }

	uint32_t GetCurveCount() override { return 1; }
	uint32_t GetPointCount(uint32_t) override { return points.size(); }

	Vec2f GetPoint(uint32_t, uint32_t pointid) override { return points[pointid]; }
	void SetPoint(uint32_t, uint32_t pointid, Vec2f p) override { points[pointid] = { p.x, clamp(p.y, 0.0f, 1.0f) }; }
	Vec2f GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q) override
	{
		return Vec2fLerp(points[firstpointid], points[firstpointid + 1], q);
	}
	int GetCurvePointsForRange(uint32_t curveid, uint32_t firstpointid, Rangef qrange, Vec2f* out, int maxOut) override
	{
		if (maxOut < 2)
			return 0;
		out[0] = GetInterpolatedPoint(curveid, firstpointid, qrange.min);
		out[1] = GetInterpolatedPoint(curveid, firstpointid + 1, qrange.min);
		return 2;
	}
};

struct Sequence01Curve : ICurves
{
	enum class Mode
	{
		Hold,
		SinglePowerCurve,
		DoublePowerCurve,
		SawWave,
	};
	struct Point
	{
		float deltaX;
		float posX; // computed
		float posY;
		Mode mode = Mode::SinglePowerCurve;
		float tweak = 0;
	};

	std::vector<Point> points;

	uint32_t GetFeatures() override { return SliceMidpoints; }

	uint32_t GetCurveCount() override { return 1; }
	uint32_t GetPointCount(uint32_t) override { return points.size(); }

	Vec2f GetPoint(uint32_t, uint32_t pointid) override
	{
		return { points[pointid].posX, points[pointid].posY };
	}
	void SetPoint(uint32_t, uint32_t pointid, Vec2f p) override;
	Vec2f GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q) override;

	bool HasSliceMidpoint(uint32_t curveid, uint32_t sliceid) override { return true; }
	Vec2f GetSliceMidpoint(uint32_t curveid, uint32_t sliceid) override
	{
		return { 0.5f, points[sliceid + 1].tweak };
	}
	void SetSliceMidpoint(uint32_t curveid, uint32_t sliceid, Vec2f p) override
	{
		points[sliceid + 1].tweak = p.y;
	}
};

struct CubicNormalizedRemapCurve : ICurves
{
	Vec2f t0r = { 1.0f / 3.0f, 0.0f / 3.0f };
	Vec2f t1l = { 1.0f / 3.0f, 0.0f / 3.0f };

	uint32_t GetFeatures() override { return Tangents; }

	uint32_t GetCurveCount() override { return 1; }
	uint32_t GetPointCount(uint32_t) override { return 2; }

	Vec2f GetPoint(uint32_t, uint32_t pointid) override { return pointid == 0 ? Vec2f(0, 0) : Vec2f(1, 1); }
	void SetPoint(uint32_t, uint32_t pointid, Vec2f p) override {}

	virtual bool HasLeftTangent(uint32_t, uint32_t pointid) { return pointid == 1; }
	virtual Vec2f GetLeftTangentPoint(uint32_t, uint32_t) { return Vec2f(1, 1) - t1l; }
	virtual void SetLeftTangentPoint(uint32_t, uint32_t, Vec2f p) { t1l = Vec2f(1 - clamp(p.x, 0.0f, 1.0f), 1 - p.y); }
	virtual bool HasRightTangent(uint32_t, uint32_t pointid) { return pointid == 0; }
	virtual Vec2f GetRightTangentPoint(uint32_t, uint32_t) { return t0r; }
	virtual void SetRightTangentPoint(uint32_t, uint32_t, Vec2f p) { t0r = { clamp(p.x, 0.0f, 1.0f), p.y }; }

	static float cubiclerp(float x0, float x1, float x2, float x3, float q)
	{
		float x01 = lerp(x0, x1, q);
		float x12 = lerp(x1, x2, q);
		float x23 = lerp(x2, x3, q);
		float x012 = lerp(x01, x12, q);
		float x123 = lerp(x12, x23, q);
		return lerp(x012, x123, q);
	}
	Vec2f GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q) override
	{
		float oq = q;

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

		return { oq, cubiclerp(0, y1, y2, 1, q) };
	}
};

} // ui
