
#include "CurveEditor.h"

#include "../Model/Menu.h"
#include "../Model/Native.h"

#include "../Editor_Curve_Sequence01.h"
#include "../Editor_Curve_CubicNrmRemap.h"
#include "../Editor_Curve_QuadSpline.h"


namespace ui {

void GridAxisSettings::Draw(AABB2f viewport, AABB2f winRect, bool vertical)
{
	if (baseUnit == 0)
		return;

	float winRange = vertical ? winRect.GetHeight() : winRect.GetWidth();
	float viewMin = vertical ? viewport.y0 : viewport.x0;
	float viewMax = vertical ? viewport.y1 : viewport.x1;
	float viewRange = viewMax - viewMin;
	if (winRange <= 0 || viewRange <= 0)
		return;

	float unitWin = baseUnit * winRange / viewRange;
	bool enableLine1 = unitWin >= minPixelDist;
	bool enableLine2 = unitWin * line2Period >= minPixelDist;
	int actualLine3Period = line3Period;
	while (unitWin * actualLine3Period < minPixelDist)
		actualLine3Period *= 2;

	int firstLine, lastLine, step;
	if (!enableLine1 && !enableLine2)
	{
		// all are line3
		firstLine = ceil(viewMin / (baseUnit * actualLine3Period)) * actualLine3Period;
		lastLine = floor(viewMax / (baseUnit * actualLine3Period)) * actualLine3Period;
		step = actualLine3Period;
	}
	else
	{
		firstLine = ceil(viewMin / baseUnit);
		lastLine = floor(viewMax / baseUnit);
		step = 1;
	}

	for (int i = firstLine; i <= lastLine; i += step)
	{
		Color4b col;
		if (line3Color.a && (i % actualLine3Period) == 0)
			col = line3Color;
		else if (enableLine2 && line2Color.a && (i % line2Period) == 0)
			col = line2Color;
		else if (enableLine1 && line1Color.a)
			col = line1Color;
		else
			col = { 0 };

		if (col.a)
		{
			if (vertical)
			{
				float y = lerp(winRect.y0, winRect.y1, invlerp(viewport.y1, viewport.y0, i));
				draw::AALineCol(winRect.x0, y, winRect.x1, y, 1, col);
			}
			else
			{
				float x = lerp(winRect.x0, winRect.x1, invlerp(viewport.x0, viewport.x1, i));
				draw::AALineCol(x, winRect.y0, x, winRect.y1, 1, col);
			}
		}
	}
}

void GridSettings::Draw(AABB2f viewport, AABB2f winRect)
{
	x.Draw(viewport, winRect, false);
	y.Draw(viewport, winRect, true);
}


Vec2f CurveEditorInput::ScreenFromCurve(Vec2f p) const
{
	return winRect.Lerp(viewport.InverseLerpFlipY(p));
}

Vec2f CurveEditorInput::CurveFromScreen(Vec2f p) const
{
	return viewport.LerpFlipY(winRect.InverseLerp(p));
}


Range<u32> ICurveView::ExpandForCurves(Range<u32> src, u32 max)
{
	if (src.min > 0)
		src.min--;
	if (src.max < max)
		src.max++;
	// exclude last point (for which there is no matching curve)
	if (src.max == max && max)
		src.max--;
	return src;
}

AABB2f ICurveView::GetPreferredViewport(bool includeTangents)
{
	AABB2f bbox = AABB2f::Empty();
	for (u32 cid = 0, numcurves = GetCurveCount(); cid < numcurves; cid++)
	{
		bbox.Include(GetPreferredCurveViewport(includeTangents, cid, { 0, GetPointCount(cid) }));
	}
	return bbox;
}

AABB2f ICurveView::GetPreferredCurveViewport(bool includeTangents, u32 curveid, Range<u32> pointRange)
{
	auto features = GetFeatures();
	if (!(features & Tangents) || (features & DirOnlyTangents))
		includeTangents = false;
	AABB2f bbox = AABB2f::Empty();
	for (u32 pid = pointRange.min; pid < pointRange.max; pid++)
	{
		Vec2f pt = GetPoint(curveid, pid);
		bbox.Include(pt);
		if (includeTangents)
		{
			bbox.Include(pt + GetLeftTangentDiff(curveid, pid));
			bbox.Include(pt + GetRightTangentDiff(curveid, pid));
		}
	}
	return bbox;
}

Range<u32> ICurveView::ExpandForTangents(u32 curveid, Range<u32> src)
{
	if (src.min > 0)
		src.min--;
	if (src.max < GetPointCount(curveid))
		src.max++;
	return src;
}

void ICurveView::GetScreenCurvePoints(const CurveEditorInput& input, u32 curveid, Array<Vec2f>& curvepoints)
{
	for (float sx = input.winRect.x0; sx <= input.winRect.x1; sx++)
	{
		float vx = lerp(input.viewport.x0, input.viewport.x1, invlerp(input.winRect.x0, input.winRect.x1, sx));
		float vy = SampleCurve(curveid, vx);
		float sy = lerp(input.winRect.y1, input.winRect.y0, invlerp(input.viewport.y0, input.viewport.y1, vy));
		curvepoints.Append({ sx, sy });
	}
}

Vec2f ICurveView::GetSliceMidpointPosition(u32 curveid, u32 sliceid)
{
	auto p = GetSliceMidpoint(curveid, sliceid);
	float x = lerp(GetPoint(curveid, sliceid).x, GetPoint(curveid, sliceid + 1).x, p.x);
	return { x, SampleCurve(curveid, x) };
}

Vec2f ICurveView::GetScreenPoint(const CurveEditorInput& input, CurvePointID cpid)
{
	if (cpid.pointType == CPT_LTangent || cpid.pointType == CPT_RTangent)
	{
		Vec2f pt = GetPoint(cpid.curveID, cpid.pointID);
		Vec2f td = cpid.pointType == CPT_RTangent
			? GetRightTangentDiff(cpid.curveID, cpid.pointID)
			: GetLeftTangentDiff(cpid.curveID, cpid.pointID);

		if (GetFeatures() & DirOnlyTangents)
		{
			pt = input.ScreenFromCurve(pt);
			td.y = -td.y;
			td *= input.winRect.GetSize().ToVec2() / input.viewport.GetSize().ToVec2();
			td = td.Normalized();
			pt += td * input.settings->pointRadius * 3; // TODO separate setting
			return pt;
		}
		else
		{
			return input.ScreenFromCurve(pt + td);
		}
	}
	Vec2f p = {};
	switch (cpid.pointType)
	{
	case CPT_Point:
		p = GetPoint(cpid.curveID, cpid.pointID);
		break;
	case CPT_Midpoint:
		p = GetSliceMidpointPosition(cpid.curveID, cpid.pointID);
		break;
	}
	return input.ScreenFromCurve(p);
}

void ICurveView::SetScreenPoint(const CurveEditorInput& input, CurvePointID cpid, Vec2f sp)
{
	Vec2f p = input.CurveFromScreen(sp);
	if (float snapX = input.settings->snapX)
		p.x = roundf(p.x / snapX) * snapX;
	switch (cpid.pointType)
	{
	case CPT_Point:
		SetPoint(cpid.curveID, cpid.pointID, p);
		break;
	case CPT_LTangent:
		p -= GetPoint(cpid.curveID, cpid.pointID);
		SetLeftTangentDiff(cpid.curveID, cpid.pointID, p);
		break;
	case CPT_RTangent:
		p -= GetPoint(cpid.curveID, cpid.pointID);
		SetRightTangentDiff(cpid.curveID, cpid.pointID, p);
		break;
	case CPT_Midpoint:
		SetSliceMidpoint(cpid.curveID, cpid.pointID, p);
		break;
	}
}

u32 ICurveView::_FixPointOrder(u32 curveid, u32 pointid)
{
	while (pointid > 0 && GetPoint(curveid, pointid).x < GetPoint(curveid, pointid - 1).x)
	{
		SwapPoints(curveid, pointid - 1);
		pointid--;
	}
	auto end = GetPointCount(curveid);
	while (pointid + 1 < end && GetPoint(curveid, pointid).x > GetPoint(curveid, pointid + 1).x)
	{
		SwapPoints(curveid, pointid);
		pointid++;
	}
	return pointid;
}

CurvePointID ICurveView::HitTest(const CurveEditorInput& input, Vec2f cursorPos)
{
	u32 numCurves = GetCurveCount();
	auto vp = input.viewport;

	float pradsq = input.settings->pointRadius * input.settings->pointRadius;

	for (u32 cid = 0; cid < numCurves; cid++)
	{
		auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

		for (u32 pid = pointRange.max; pid > pointRange.min; )
		{
			pid--;
			Vec2f p = input.ScreenFromCurve(GetPoint(cid, pid));
			if ((p - cursorPos).LengthSq() <= pradsq)
				return { cid, CPT_Point, pid };
		}
	}

	if (GetFeatures() & SliceMidpoints)
	{
		for (u32 cid = 0; cid < numCurves; cid++)
		{
			auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });
			auto midptRange = ExpandForCurves(pointRange, GetPointCount(cid));

			for (u32 pid = midptRange.max; pid > midptRange.min; )
			{
				pid--;

				if (HasSliceMidpoint(cid, pid))
				{
					Vec2f p = GetSliceMidpointPosition(cid, pid);
					p = input.ScreenFromCurve(p);
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_Midpoint, pid };
				}
				if (HasRightTangent(cid, pid))
				{
					Vec2f p = GetScreenPoint(input, CurvePointID(cid, CPT_RTangent, pid));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_RTangent, pid };
				}
				if (HasLeftTangent(cid, pid))
				{
					Vec2f p = GetScreenPoint(input, CurvePointID(cid, CPT_LTangent, pid));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_LTangent, pid };
				}
			}
		}
	}

	if (GetFeatures() & Tangents)
	{
		for (u32 cid = 0; cid < numCurves; cid++)
		{
			auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

			for (u32 pid = pointRange.max; pid > pointRange.min; )
			{
				pid--;

				if (HasRightTangent(cid, pid))
				{
					Vec2f p = GetScreenPoint(input, CurvePointID(cid, CPT_RTangent, pid));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_RTangent, pid };
				}
				if (HasLeftTangent(cid, pid))
				{
					Vec2f p = GetScreenPoint(input, CurvePointID(cid, CPT_LTangent, pid));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_LTangent, pid };
				}
			}
		}
	}

	return SubUIValueHelper<CurvePointID>::GetNullValue();
}

void ICurveView::DrawCurve(const CurveEditorInput& input, u32 curveid)
{
	if (!input.viewport.IsValid() || !input.winRect.IsValid() || GetPointCount(curveid) < 2)
		return;

	auto col = GetCurveColor(curveid);

	Array<Vec2f> curvepoints;
	GetScreenCurvePoints(input, curveid, curvepoints);
	draw::AALineCol(curvepoints, 1.0f, col, false);
}

void ICurveView::DrawCurvePointsType(const CurveEditorInput& input, const CurveEditorState& state, u32 curveid, CurvePointType type, Range<u32> pointRange)
{
	auto col = GetCurveColor(curveid);
	for (u32 pid = pointRange.min; pid < pointRange.max; pid++)
	{
		switch (type)
		{
		case CPT_Midpoint:
			if (!HasSliceMidpoint(curveid, pid))
				continue;
			break;
		case CPT_LTangent:
			if (!HasLeftTangent(curveid, pid))
				continue;
			break;
		case CPT_RTangent:
			if (!HasRightTangent(curveid, pid))
				continue;
			break;
		}

		Color4b pcol = col;
		CurvePointID cpid = { curveid, type, pid };
		if (state.uiState.IsPressed(cpid))
			pcol = Color4fLerp(col, Color4f::Black(), 0.2f);
		else if (state.uiState.IsHovered(cpid))
			pcol = Color4fLerp(col, Color4f::White(), 0.2f);

		Vec2f p = GetScreenPoint(input, cpid);
		draw::AACircleLineCol(p, input.settings->pointRadius - 0.5f, 1, pcol);
	}
}

void ICurveView::DrawAllPoints(const CurveEditorInput& input, const CurveEditorState& state)
{
	u32 numCurves = GetCurveCount();
	auto vp = input.viewport;

	for (u32 cid = 0; cid < numCurves; cid++)
	{
		if (!IsCurveVisible(cid))
			continue;

		auto pointCount = GetPointCount(cid);
		auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

		if (GetFeatures() & SliceMidpoints)
		{
			// include adjacent points for edge curve midpoints
			auto midptRange = ExpandForCurves(pointRange, pointCount);

			DrawCurvePointsType(input, state, cid, CPT_Midpoint, midptRange);
		}

		if (GetFeatures() & Tangents)
		{
			// include nearby tangents
			auto tangentRange = ExpandForTangents(cid, pointRange);

			DrawCurvePointsType(input, state, cid, CPT_LTangent, tangentRange);
			DrawCurvePointsType(input, state, cid, CPT_RTangent, tangentRange);
		}
		DrawCurvePointsType(input, state, cid, CPT_Point, pointRange);
	}
}

void ICurveView::DrawAllTangentLines(const CurveEditorInput& input, const CurveEditorState& state)
{
	u32 numCurves = GetCurveCount();
	auto vp = input.viewport;

	for (u32 cid = 0; cid < numCurves; cid++)
	{
		if (!IsCurveVisible(cid))
			continue;

		auto pointCount = GetPointCount(cid);
		auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

		// to include tangent lines for points just out of view
		auto tangentRange = ExpandForTangents(cid, pointRange);

		auto col = GetCurveColor(cid);
		for (u32 pid = tangentRange.min; pid < tangentRange.max; pid++)
		{
			auto pt = GetPoint(cid, pid);
			pt = input.ScreenFromCurve(pt);

			if (HasLeftTangent(cid, pid))
			{
				auto tanpt = GetScreenPoint(input, CurvePointID(cid, CPT_LTangent, pid));

				draw::AALineCol({ pt, tanpt }, input.settings->tangentLineThickness, col, false);
			}
			if (HasRightTangent(cid, pid))
			{
				auto tanpt = GetScreenPoint(input, CurvePointID(cid, CPT_RTangent, pid));

				draw::AALineCol({ pt, tanpt }, input.settings->tangentLineThickness, col, false);
			}
		}
	}
}

void ICurveView::DrawAll(const CurveEditorInput& input, const CurveEditorState& state)
{
	// curve lines
	for (u32 cid = 0, numCurves = GetCurveCount(); cid < numCurves; cid++)
	{
		if (!IsCurveVisible(cid))
			continue;

		DrawCurve(input, cid);
	}

	if (GetFeatures() & Tangents)
		DrawAllTangentLines(input, state);

	DrawAllPoints(input, state);
}


bool CurveEditorUI::OnEvent(const CurveEditorInput& input, ICurveView* curves, Event& e)
{
	auto& pid = uiState._pressed;
	uiState.InitOnEvent(e);
	auto ret = uiState.GlobalDragOnEvent(curves->HitTest(input, e.position), e);
	//printf("CEUI: ret=%d hover=%d press=%d\n", ret, uiState._hovered.pointID, uiState._pressed.pointID);
	switch (ret)
	{
	case SubUIDragState::Start:
		dragPointStart = pid.pointType == CPT_Midpoint
			? curves->GetSliceMidpoint(pid.curveID, pid.pointID)
			: curves->GetScreenPoint(input, pid);
		dragCursorStart = e.position;
		e.context->CaptureMouse(e.current);
		break;
	case SubUIDragState::Move:
		if (pid.pointType == CPT_Midpoint)
		{
			float q = -curves->GetSliceMidpointVertDragFactor(pid.curveID, pid.pointID);
			curves->SetSliceMidpoint(pid.curveID, pid.pointID, dragPointStart + (e.position - dragCursorStart) * q);
		}
		else
		{
			curves->SetScreenPoint(input, pid, dragPointStart + e.position - dragCursorStart);

			if (pid.pointType == CPT_Point)
				pid.pointID = curves->_FixPointOrder(pid.curveID, pid.pointID);
		}
		e.context->OnChange(e.current);
		break;
	case SubUIDragState::Stop:
		e.context->ReleaseMouse();
		break;
	}
	uiState.FinalizeOnEvent(e);
	curves->OnEvent(input, e);
	return false;
}

void CurveEditorUI::Render(const CurveEditorInput& input, ICurveView* curves)
{
	if (!curves)
		return;

	if (draw::PushScissorRect(input.winRect.ExtendBy(1)))
	{
		curves->DrawAll(input, *this);
	}
	draw::PopScissorRect();
}


void CurveEditorElement::OnReset()
{
	FrameElement::OnReset();

	curveView = nullptr;
	viewport = { 0, 0, 1, 1 };
	settings = {};
	gridSettings = {};

	SetDefaultFrameStyle(DefaultFrameStyle::ListBox); // TODO custom style
}

void CurveEditorElement::OnEvent(Event& e)
{
	if (_ui.OnEvent({ viewport, GetContentRect(), &settings }, curveView, e))
		e.StopPropagation();
}

void CurveEditorElement::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();

	gridSettings.Draw(viewport, GetContentRect());
	_ui.Render({ viewport, GetContentRect(), &settings }, curveView);

	PaintChildren(ctx, cpa);
}


struct CurveProxyEditorElement : FrameElement
{
	Curve_RTEditButton* _editor = nullptr;
	CurveEditorUI _ui;

	void OnReset() override
	{
		FrameElement::OnReset();

		_editor = nullptr;

		SetDefaultFrameStyle(DefaultFrameStyle::ListBox); // TODO custom style
	}
	void OnEvent(Event& e) override
	{
		if (_ui.OnEvent({ _editor->viewport, GetContentRect(), &_editor->settings }, _editor->curveView, e))
			e.StopPropagation();
	}
	void OnPaint(const UIPaintContext& ctx) override
	{
		auto cpa = PaintFrame();

		_editor->gridSettings.Draw(_editor->viewport, GetContentRect());
		_ui.Render({ _editor->viewport, GetContentRect(), &_editor->settings }, _editor->curveView);

		PaintChildren(ctx, cpa);
	}
};


struct Curve_RTEditorWindowContents : Buildable
{
	Curve_RTEditButton* _editor = nullptr;

	void Build() override
	{
		auto& cee = Make<CurveProxyEditorElement>();
		cee._editor = _editor;
		cee.HandleEvent(EventType::Change) = [this, &cee](Event& e)
		{
			e.context->OnChange(_editor);
			e.context->OnCommit(_editor);
		};
	}
};


struct Curve_RTEditorWindow : NativeWindowBase
{
	Curve_RTEditorWindow(Curve_RTEditButton* editor)
	{
		SetTitle("Curve editor");
		SetInnerSize(700, 300);

		auto* B = CreateUIObject<Curve_RTEditorWindowContents>();
		B->_editor = editor;
		SetContents(B, true);
	}
};


Curve_RTEditButton& Curve_RTEditButton::SetCurveView(ICurveView* cv)
{
	curveView = cv;
	if (_rtWindow)
		_rtWindow->RebuildRoot();
	return *this;
}

void Curve_RTEditButton::OnDisable()
{
	delete _rtWindow;
	_rtWindow = nullptr;
}

void Curve_RTEditButton::OnReset()
{
	FrameElement::OnReset();

	SetDefaultFrameStyle(DefaultFrameStyle::Button);
	SetFlag(UIObject_DB_Button, true);

	curveView = nullptr;
}

void Curve_RTEditButton::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();

	auto r = GetFinalRect().ShrinkBy(frameStyle.padding);

	CurveEditorInput cei;
	cei.viewport = curveView->GetPreferredViewport(false);
	cei.winRect = r;
	for (u32 cid = 0, cnum = curveView->GetCurveCount(); cid < cnum; cid++)
		curveView->DrawCurve(cei, cid);
}

void Curve_RTEditButton::OnEvent(Event& e)
{
	if (e.type == EventType::Activate)
	{
		if (!_rtWindow)
			_rtWindow = new Curve_RTEditorWindow(this);
		_rtWindow->SetVisible(true);
	}
}


namespace imm {

imCtrlInfoT<Curve_RTEditButton> imEditCurveRT(ICurveView* curveView)
{
	auto& ceb = Make<Curve_RTEditButton>();
	ceb.Init(curveView);

	if (ceb.flags & UIObject_AfterIMEdit)
	{
		ceb._OnIMChange();
		ceb.flags &= ~UIObject_AfterIMEdit;
	}
	bool edited = false;
	if (ceb.flags & UIObject_IsEdited)
	{
		ceb.flags &= ~UIObject_IsEdited;
		ceb.flags |= UIObject_AfterIMEdit;
		ceb.RebuildContainer();
		edited = true;
	}

	ceb.HandleEvent(&ceb) = [&ceb](Event& e)
	{
		if (e.type == EventType::Change)
			e.StopPropagation();
		if (e.type == EventType::Commit)
		{
			e.StopPropagation();
			e.current->flags |= UIObject_IsEdited;
			e.current->RebuildContainer();
		}
	};

	return { edited, &ceb };
}

} // imm


void Curve_Sequence01::Point::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Curve_Sequence01::Point");

	OnField(oi, "deltaX", deltaX);
	OnField(oi, "posX", posX);
	OnField(oi, "posY", posY);
	OnFieldEnumInt(oi, "mode", mode);
	OnField(oi, "tweak", tweak);

	oi.EndObject();
}

void Curve_Sequence01::SerializeData(IObjectIterator& oi)
{
	OnField(oi, "points", points);
}

void Curve_Sequence01::GenSawWaveMiddlePoints(float tweak, const std::function<void(Vec2f)>& outpts)
{
	int count = int(floorf(fabsf(tweak)));
	if (tweak >= 0)
	{
		for (int i = 0; i < count; i++)
		{
			float q = (i + 1.f) / (count + 1.f);
			outpts({ q, 1 });
			outpts({ q, 0 });
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			outpts({ i / float(count), 1 });
			outpts({ (i + 1) / float(count), 0 });
		}
	}
}

static float DoPowerCurve(float q, float tweak)
{
	return tweak >= 0
		? 1 - powf(1 - q, exp2(tweak))
		: powf(q, exp2(-tweak));
}

float Curve_Sequence01::EvaluateSegment(const Point& p0, const Point& p1, float q)
{
	switch (p1.mode)
	{
	case Mode::Hold:
		q = 0;
		break;
	case Mode::SinglePowerCurve:
		q = DoPowerCurve(q, p1.tweak);
		break;
	case Mode::DoublePowerCurve:
		q = DoPowerCurve(fabsf(q * 2 - 1), p1.tweak) * sign(q * 2 - 1) * 0.5f + 0.5f;
		break;
	case Mode::SawWave:
		q = fmodf(q * (1.0f + floorf(fabsf(p1.tweak))) * 0.99999f, 1.0f);
		if (p1.tweak < 0)
			q = 1 - q;
		break;
	case Mode::PulseWave:
		q = fmodf(q * (1.0f + floorf(fabsf(p1.tweak))) * 0.99999f, 1.0f);
		if (p1.tweak < 0)
			q = 1 - q;
		q = q > 0.5f ? 1 : 0;
	case Mode::Steps: {
		float count = (1.0f + floorf(fabsf(p1.tweak))) * 0.99999f;
		if (p1.tweak >= 0)
			q = floorf(q * (count + 1)) / count;
		else
			q = (floorf(q * count) + 1.0f) / (count + 1.0f);
		break; }
	}
	return lerp(p0.posY, p1.posY, q);
}

float Curve_Sequence01::Evaluate(float t)
{
	if (points.IsEmpty())
		return 0;

	if (t <= points[0].posX)
		return points[0].posY;

	// TODO optimize
	for (size_t i = 1; i < points.size(); i++)
	{
		if (points[i].posX >= t)
		{
			auto& p0 = points[i - 1];
			auto& p1 = points[i];
			return EvaluateSegment(p0, p1, invlerp(p0.posX, p1.posX, t));
		}
	}
	return points.Last().posY;
}

void Curve_Sequence01::RemovePoint(size_t i)
{
	points.RemoveAt(i);
	if (i > 0)
		points[i].deltaX = points[i].posX - points[i - 1].posX;
	else
		points[i].deltaX = points[i].posX;
}

void Curve_Sequence01::AddPoint(Vec2f pos)
{
	size_t before = 0;
	while (before < points.size() && points[before].posX <= pos.x)
		before++;

	Curve_Sequence01::Point p = {};
	if (points.NotEmpty())
		p = before < points.size() ? points[before] : points.Last();
	p.posX = pos.x;
	p.posY = clamp(pos.y, 0.0f, 1.0f);

	points.InsertAt(before, p);
	if (before > 0)
		points[before].deltaX = points[before].posX - points[before - 1].posX;
	if (before + 1 < points.size())
		points[before + 1].deltaX = points[before + 1].posX - points[before].posX;
	if (before == 0 && points.size() > 1)
		points[before + 1].mode = Mode::SinglePowerCurve;
}


void Curve_Sequence01_View::SetPoint(u32, u32 pointid, Vec2f p)
{
	float newX = p.x;
	float newY = clamp(p.y, 0.0f, 1.0f);

	auto& dp = curve->points[pointid];
	dp.posY = newY;

	float prevX = pointid > 0 ? curve->points[pointid - 1].posX : 0;
	dp.deltaX = newX - prevX;
	if (dp.deltaX < 0)
		dp.deltaX = 0;
	dp.posX = prevX + dp.deltaX;

	for (u32 i = pointid + 1; i < u32(curve->points.size()); i++)
	{
		curve->points[i].posX = curve->points[i - 1].posX + curve->points[i].deltaX;
	}
}

float Curve_Sequence01_View::SampleCurve(u32, float x)
{
	return curve->Evaluate(x);
}

void Curve_Sequence01_View::GetScreenCurvePoints(const CurveEditorInput& input, u32 curveid, Array<Vec2f>& curvepoints)
{
	auto pointrange = GetLeastPointRange(curveid, { input.viewport.x0, input.viewport.x1 });
	pointrange = ExpandForCurves(pointrange, curve->points.Size());

	if (input.viewport.x0 < curve->points.First().posX)
		curvepoints.Append(input.ScreenFromCurve({ input.viewport.x0, curve->points.First().posY }));

	for (u32 i = pointrange.min; i < pointrange.max; i++)
	{
		auto& p0 = curve->points[i];
		auto& p1 = curve->points[i + 1];

		curvepoints.Append(input.ScreenFromCurve({ p0.posX, p0.posY }));

		if (p1.mode == Curve_Sequence01::Mode::Hold)
		{
			curvepoints.Append(input.ScreenFromCurve({ p1.posX, p0.posY }));
		}
		else if (p1.mode == Curve_Sequence01::Mode::SawWave)
		{
			Curve_Sequence01::GenSawWaveMiddlePoints(p1.tweak, [&](Vec2f q)
			{
				curvepoints.Append(input.ScreenFromCurve({ lerp(p0.posX, p1.posX, q.x), lerp(p0.posY, p1.posY, q.y) }));
			});
		}
		else
		{
			float xstep = 1.f * input.viewport.GetWidth() / input.winRect.GetWidth();
			for (float x = p0.posX + xstep, xend = p1.posX; x < xend; x += xstep)
			{
				curvepoints.Append(input.ScreenFromCurve({ x, SampleCurve(0, x) }));
			}
		}
	}

	{
		auto& pm = curve->points[pointrange.max];
		curvepoints.Append(input.ScreenFromCurve({ pm.posX, pm.posY }));
	}

	if (input.viewport.x1 > curve->points.Last().posX)
		curvepoints.Append(input.ScreenFromCurve({ input.viewport.x1, curve->points.Last().posY }));
}

float Curve_Sequence01_View::GetSliceMidpointVertDragFactor(u32 curveid, u32 sliceid)
{
	auto& p0 = curve->points[sliceid];
	auto& p1 = curve->points[sliceid + 1];
	if (p1.mode == Curve_Sequence01::Mode::SinglePowerCurve)
		return p0.posY < p1.posY ? 0.1f : -0.1f;
	return 0.1f;
}

Vec2f Curve_Sequence01_View::GetSliceMidpointPosition(u32 curveid, u32 sliceid)
{
	auto& p0 = curve->points[sliceid];
	auto& p1 = curve->points[sliceid + 1];
	switch (p1.mode)
	{
	case Curve_Sequence01::Mode::SawWave:
	case Curve_Sequence01::Mode::PulseWave:
	case Curve_Sequence01::Mode::Steps:
		return { (p0.posX + p1.posX) * 0.5f, (p0.posY + p1.posY) * 0.5f };
	default:
		return ICurveView::GetSliceMidpointPosition(curveid, sliceid);
	}
}

void Curve_Sequence01_View::OnEvent(const CurveEditorInput& input, Event& e)
{
	using Mode = Curve_Sequence01::Mode;
	if (e.type == EventType::ContextMenu)
	{
		auto hp = HitTest(input, e.position);
		auto& cm = ContextMenu::Get();
		if (hp.IsValid())
		{
			if (hp.pointType == CPT_Point)
			{
				cm.AddNext("Delete point") = [this, hp]()
				{
					curve->RemovePoint(hp.pointID);
				};
				cm.NewSection();

				auto& p = curve->points[hp.pointID];
				cm.AddNext("Set to 0") = [this, &p]() { p.posY = 0; };
				cm.AddNext("Set to 1") = [this, &p]() { p.posY = 1; };
			}
			if (hp.pointType == CPT_Midpoint)
			{
				cm.AddNext("Reset tweak value") = [this, hp]()
				{
					auto& pts = curve->points;
					pts[hp.pointID + 1].tweak = 0;
				};
				cm.NewSection();

				auto& p = curve->points[hp.pointID + 1];
#define UI_SETMODE(NAME, NEWMODE) cm.AddNext(NAME, false, p.mode == NEWMODE) = [this, &p]() { p.mode = NEWMODE; };
				UI_SETMODE("Hold", Mode::Hold);
				UI_SETMODE("Single power curve", Mode::SinglePowerCurve);
				UI_SETMODE("Double power curve", Mode::DoublePowerCurve);
				UI_SETMODE("Saw wave", Mode::SawWave);
				UI_SETMODE("Pulse wave", Mode::PulseWave);
				UI_SETMODE("Steps", Mode::Steps);
#undef UI_SETMODE
			}
		}
		else
		{
			Vec2f cwp = input.CurveFromScreen(e.position);
			if (float s = input.settings->snapX)
				cwp.x = roundf(cwp.x / s) * s;
			cm.AddNext("Add") = [this, cwp]()
			{
				curve->AddPoint(cwp);
			};
		}
	}
}


void Curve_CubicNormalizedRemap_View::OnEvent(const CurveEditorInput& input, Event& e)
{
	if (e.type == EventType::ContextMenu)
	{
		auto& cm = ContextMenu::Get();
		cm.Add("Reset") = [this]()
		{
			*curve = Curve_CubicNormalizedRemap();
		};
	}
}


void Curve_QuadSpline_View::SetPoint(u32, u32 pointid, Vec2f p)
{
	auto& P = curve->points[pointid];
	P.time = p.x;
	P.value = p.y;
}

float Curve_QuadSpline_View::SampleCurve(u32, float x)
{
	return curve->Sample(x);
}

void Curve_QuadSpline_View::OnEvent(const CurveEditorInput& input, Event& e)
{
	if (e.type == EventType::ContextMenu)
	{
		Vec2f cwp = input.CurveFromScreen(e.position);
		if (float s = input.settings->snapX)
			cwp.x = roundf(cwp.x / s) * s;
		auto hp = HitTest(input, e.position);
		auto& cm = ContextMenu::Get();
		if (hp.IsValid())
		{
			if (hp.pointType == CPT_Point)
			{
				cm.AddNext("Delete point") = [this, hp]()
				{
					curve->points.RemoveAt(hp.pointID);
				};
				cm.NewSection();
			}
		}
		else
		{
			cm.AddNext("Add point") = [this, cwp]()
			{
				curve->InsertPoint(cwp.x, cwp.y, 0);
			};
		}
		cm.NewSection();
		cm.AddNext("Set loop start") = [this, cwp]()
		{
			curve->timeRange.min = cwp.x;
		};
		cm.AddNext("Set loop end") = [this, cwp]()
		{
			curve->timeRange.max = cwp.x;
		};
		cm.NewSection();
		cm.AddNext("Loop", false, curve->flags & Curve_QuadSpline::Loop) = [this]()
		{
			curve->flags ^= Curve_QuadSpline::Loop;
		};
		cm.AddNext("Acceleration smoothing", false, curve->flags & Curve_QuadSpline::AccelSmoothing) = [this]()
		{
			curve->flags ^= Curve_QuadSpline::AccelSmoothing;
		};
	}
}

// Curve_QuadSpline.cpp
bool CQS_FindCurveMidpoint(Curve_QuadSpline::Point pa, Curve_QuadSpline::Point pb, Vec2f& outcmp);

static void DrawTangentSwordfight(const CurveEditorInput& input, const Curve_QuadSpline::Point& pa, const Curve_QuadSpline::Point& pb)
{
	Vec2f cmp;
	if (CQS_FindCurveMidpoint(pa, pb, cmp))
	{
		Vec2f pts[3] =
		{
			input.ScreenFromCurve({ pa.time, pa.value }),
			input.ScreenFromCurve(cmp),
			input.ScreenFromCurve({ pb.time, pb.value }),
		};
		draw::AALineCol(pts, 1, { 255, 127, 0, 63 }, false);
	}
	else
	{
		auto pa0 = input.ScreenFromCurve({ pa.time, pa.value });
		auto pb0 = input.ScreenFromCurve({ pb.time, pb.value });
		float w = fabsf(pb.time - pa.time);
		auto pa1 = input.ScreenFromCurve({ pb.time, pa.value + pa.velocity * w });
		auto pb1 = input.ScreenFromCurve({ pa.time, pb.value - pb.velocity * w });
		draw::AALineCol(pa0.x, pa0.y, pa1.x, pa1.y, 1, { 255, 0, 0, 63 });
		draw::AALineCol(pb0.x, pb0.y, pb1.x, pb1.y, 1, { 255, 0, 0, 63 });
	}
}

void Curve_QuadSpline_View::DrawAll(const CurveEditorInput& input, const CurveEditorState& state)
{
	if (curve->points.NotEmpty() && (curve->flags & Curve_QuadSpline::Loop))
	{
		auto p0 = curve->points.Last();
		p0.time -= curve->timeRange.GetWidth();
		DrawTangentSwordfight(input, p0, curve->points.First());
	}
	for (size_t i = 0; i + 1 < curve->points.Size(); i++)
	{
		DrawTangentSwordfight(input, curve->points[i], curve->points[i + 1]);
	}
	if (curve->points.NotEmpty() && (curve->flags & Curve_QuadSpline::Loop))
	{
		auto p1 = curve->points.First();
		p1.time += curve->timeRange.GetWidth();
		DrawTangentSwordfight(input, curve->points.Last(), p1);
	}

	// loop range
	float strmin = input.ScreenFromCurve({ curve->timeRange.min, 0 }).x;
	float strmax = input.ScreenFromCurve({ curve->timeRange.max, 0 }).x;
	draw::AALineCol(strmin, input.winRect.y0, strmin, input.winRect.y1, 1, { 255, 32, 0 });
	draw::AALineCol(strmax, input.winRect.y0, strmax, input.winRect.y1, 1, { 255, 0, 63 });

	ICurveView::DrawAll(input, state);
}


} // ui
