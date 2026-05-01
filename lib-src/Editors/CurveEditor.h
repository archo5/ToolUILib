
#pragma once

#include "../Model/Objects.h"

#include "../Model/Controls.h" // TODO: only for FrameElement


namespace ui {

// TODO move Grid
struct GridAxisSettings
{
	float baseUnit = 1; // 0 disables the grid
	float minPixelDist = 10;
	// line1Period = 1
	uint16_t line2Period = 2; // period = 0 disables the line
	uint16_t line3Period = 5;
	Color4b line1Color = Color4b(100, 100); // alpha=0 disables rendering
	Color4b line2Color = Color4b(120, 100);
	Color4b line3Color = Color4b(200, 100);

	void Draw(AABB2f viewport, AABB2f winRect, bool vertical);
};

struct GridSettings
{
	GridAxisSettings x;
	GridAxisSettings y;

	void Draw(AABB2f viewport, AABB2f winRect);
};

struct CurveEditorSettings
{
	// to prevent init lists silently zeroing future members
	CurveEditorSettings() {}

	float snapX = 0;
	float pointRadius = 5;
	float tangentLineThickness = 1;
};

struct CurveEditorInput
{
	AABB2f viewport;
	AABB2f winRect;
	CurveEditorSettings* settings;

	Vec2f ScreenFromCurve(Vec2f p) const;
	Vec2f CurveFromScreen(Vec2f p) const;
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
	u32 curveID : 30;
	u32 pointType : 2; // CurvePointType
	u32 pointID;

	constexpr CurvePointID(u32 cid, CurvePointType t, u32 pid)
		: curveID(cid), pointType(t), pointID(pid)
	{
	}

	bool operator == (const CurvePointID& o) const
	{
		return curveID == o.curveID && pointType == o.pointType && pointID == o.pointID;
	}
	bool operator != (const CurvePointID& o) const { return !(*this == o); }

	bool IsValid() const
	{
		return !(curveID == 0x3fffffff && pointType == CPT_Midpoint && pointID == UINT32_MAX);
	}
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
	Vec2f dragPointStart;
	Vec2f dragCursorStart;
};

struct ICurveView
{
	static Range<u32> ExpandForCurves(Range<u32> src, u32 max);

	enum Feature
	{
		SliceMidpoints = 1 << 0,
		Tangents = 1 << 1,
		DirOnlyTangents = 1 << 2,
	};
	virtual u32 GetFeatures() = 0;

	virtual AABB2f GetPreferredViewport(bool includeTangents);
	virtual AABB2f GetPreferredCurveViewport(bool includeTangents, u32 curveid, Range<u32> pointRange);

	virtual u32 GetCurveCount() { return 1; }
	virtual u32 GetPointCount(u32 curveid) = 0;

	// only guarantees that at least the points you need will be in the range, as an optimization
	// does not guarantee that the smallest set of points is there
	virtual Range<u32> GetLeastPointRange(u32 curveid, Rangef xrange) { return { 0, GetPointCount(curveid) }; }
	virtual Range<u32> ExpandForTangents(u32 curveid, Range<u32> src);

	virtual bool IsCurveVisible(u32 curveid) { return true; }
	virtual Color4b GetCurveColor(u32 curveid) { return { 191, 255 }; }

	virtual Vec2f GetPoint(u32 curveid, u32 pointid) = 0;
	virtual void SetPoint(u32 curveid, u32 pointid, Vec2f p) = 0;
	virtual void SwapPoints(u32 curveid, u32 pointid) = 0;

	virtual float SampleCurve(u32 curveid, float x) = 0;
	virtual void GetScreenCurvePoints(const CurveEditorInput& input, u32 curveid, Array<Vec2f>& curvepoints);

	virtual bool HasLeftTangent(u32 curveid, u32 pointid) { return false; }
	virtual Vec2f GetLeftTangentDiff(u32 curveid, u32 pointid) { return {}; }
	virtual void SetLeftTangentDiff(u32 curveid, u32 pointid, Vec2f d) {}
	virtual bool HasRightTangent(u32 curveid, u32 pointid) { return false; }
	virtual Vec2f GetRightTangentDiff(u32 curveid, u32 pointid) { return {}; }
	virtual void SetRightTangentDiff(u32 curveid, u32 pointid, Vec2f d) {}

	virtual bool HasSliceMidpoint(u32 curveid, u32 sliceid) { return false; }
	// slice midpoint coordinates: x = lerp factor, y = modifier (screen pixel units)
	virtual Vec2f GetSliceMidpoint(u32 curveid, u32 sliceid) { return { 0.5f, 0 }; }
	virtual void SetSliceMidpoint(u32 curveid, u32 sliceid, Vec2f p) {}
	virtual float GetSliceMidpointVertDragFactor(u32 curveid, u32 sliceid) { return 1; }
	virtual Vec2f GetSliceMidpointPosition(u32 curveid, u32 sliceid);

	Vec2f GetScreenPoint(const CurveEditorInput& input, CurvePointID cpid);
	void SetScreenPoint(const CurveEditorInput& input, CurvePointID cpid, Vec2f sp);
	u32 _FixPointOrder(u32 curveid, u32 pointid);

	virtual CurvePointID HitTest(const CurveEditorInput& input, Vec2f cursorPos);
	virtual void OnEvent(const CurveEditorInput& input, Event& e) {}

	virtual void DrawCurve(const CurveEditorInput& input, u32 curveid);
	virtual void DrawCurvePointsType(
		const CurveEditorInput& input,
		const CurveEditorState& state,
		u32 curveid,
		CurvePointType type,
		Range<u32> pointRange);

	// overriding the order of point types to match hit test
	virtual void DrawAllPoints(const CurveEditorInput& input, const CurveEditorState& state);

	virtual void DrawAllTangentLines(const CurveEditorInput& input, const CurveEditorState& state);

	virtual void DrawAll(const CurveEditorInput& input, const CurveEditorState& state);
};


struct CurveEditorUI : CurveEditorState
{
	bool OnEvent(const CurveEditorInput& input, ICurveView* curves, Event& e);
	void Render(const CurveEditorInput& input, ICurveView* curves);
};

struct CurveEditorElement : FrameElement
{
	CurveEditorUI _ui;
	ICurveView* curveView = nullptr;
	AABB2f viewport = { 0, 0, 1, 1 };
	CurveEditorSettings settings;
	GridSettings gridSettings;

	void OnReset() override;
	void OnEvent(Event& e) override;
	void OnPaint(const UIPaintContext& ctx) override;
};


struct BasicLinear01Curve : ICurveView
{
	Array<Vec2f> points;

	u32 GetFeatures() override { return 0; }

	u32 GetCurveCount() override { return 1; }
	u32 GetPointCount(u32) override { return points.size(); }

	Vec2f GetPoint(u32, u32 pointid) override { return points[pointid]; }
	void SetPoint(u32, u32 pointid, Vec2f p) override { points[pointid] = { p.x, clamp(p.y, 0.0f, 1.0f) }; }
	void SwapPoints(u32 curveid, u32 pointid) override { std::swap(points[pointid], points[pointid + 1]); }
	float SampleCurve(u32, float x) override
	{
		if (points.IsEmpty())
			return 0;
		if (points.Size() == 1)
			return points[0].y;
		size_t pos = 0;
		for (size_t i = 0; i < points.Size(); i++)
		{
			if (x > points[i].x)
			{
				pos = i;
			}
		}
		if (pos + 1 == points.Size())
			return points.Last().y;
		float q = invlerpc(points[pos].x, points[pos + 1].x, x);
		return lerp(points[pos].y, points[pos + 1].y, q);
	}
};

} // ui
