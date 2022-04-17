
#include "CurveEditor.h"

#include "../Model/Menu.h"


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


Range<uint32_t> ICurveView::ExpandForCurves(Range<uint32_t> src, uint32_t max)
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
	for (uint32_t cid = 0, numcurves = GetCurveCount(); cid < numcurves; cid++)
	{
		bbox = bbox.Include(GetPreferredCurveViewport(includeTangents, cid, { 0, GetPointCount(cid) }));
	}
	return bbox;
}

AABB2f ICurveView::GetPreferredCurveViewport(bool includeTangents, uint32_t curveid, Range<uint32_t> pointRange)
{
	if (!(GetFeatures() & Tangents))
		includeTangents = false;
	AABB2f bbox = AABB2f::Empty();
	for (uint32_t pid = pointRange.min; pid < pointRange.max; pid++)
	{
		bbox = bbox.Include(GetPoint(curveid, pid));
		if (includeTangents)
		{
			bbox = bbox.Include(GetLeftTangentPoint(curveid, pid));
			bbox = bbox.Include(GetRightTangentPoint(curveid, pid));
		}
	}
	return bbox;
}

Range<uint32_t> ICurveView::ExpandForTangents(uint32_t curveid, Range<uint32_t> src)
{
	if (src.min > 0)
		src.min--;
	if (src.max < GetPointCount(curveid))
		src.max++;
	return src;
}

int ICurveView::GetCurvePointsForRange(uint32_t curveid, uint32_t firstpointid, Rangef qrange, Vec2f* out, int maxOut)
{
	if (maxOut < 2)
		return 0;
	int imin = 0;
	int imax = maxOut;
	if (qrange.min == 0)
	{
		out[imin++] = GetPoint(curveid, firstpointid);
	}
	if (qrange.max == 1)
	{
		out[--imax] = GetPoint(curveid, firstpointid + 1);
	}
	for (int i = imin; i < imax; i++)
	{
		float f = float(i) / float(maxOut - 1);
		float q = lerp(qrange.min, qrange.max, f);
		out[i] = GetInterpolatedPoint(curveid, firstpointid, q);
	}
	return maxOut;
}

int ICurveView::GetCurvePointsForViewport(uint32_t curveid, uint32_t firstpointid, AABB2f vp, float winWidth, Vec2f* out, int maxOut)
{
	if (winWidth <= 0 || !vp.IsValid() || GetPointCount(curveid) < 2)
		return 0;
	Rangef vpRange = { vp.x0, vp.x1 };
	auto p0 = GetPoint(curveid, firstpointid);
	auto p1 = GetPoint(curveid, firstpointid + 1);
	auto intersection = vpRange.Intersect({ p0.x, p1.x });
	float issw = winWidth * intersection.GetWidth() / vp.GetWidth();
	if (maxOut > issw)
		maxOut = issw;
	float qmin = clamp(invlerp(p0.x, p1.x, intersection.min), 0.0f, 1.0f);
	float qmax = clamp(invlerp(p0.x, p1.x, intersection.max), 0.0f, 1.0f);
	return GetCurvePointsForRange(curveid, firstpointid, { qmin, qmax }, out, maxOut);
}

Vec2f ICurveView::GetSliceMidpointPosition(uint32_t curveid, uint32_t sliceid)
{
	auto p = GetSliceMidpoint(curveid, sliceid);
	return GetInterpolatedPoint(curveid, sliceid, p.x);
}

Vec2f ICurveView::GetScreenPoint(const CurveEditorInput& input, CurvePointID cpid)
{
	Vec2f p = {};
	switch (cpid.pointType)
	{
	case CPT_Point:
		p = GetPoint(cpid.curveID, cpid.pointID);
		break;
	case CPT_LTangent:
		p = GetLeftTangentPoint(cpid.curveID, cpid.pointID);
		break;
	case CPT_RTangent:
		p = GetRightTangentPoint(cpid.curveID, cpid.pointID);
		break;
	case CPT_Midpoint:
		p = GetSliceMidpoint(cpid.curveID, cpid.pointID);
		break;
	}
	return input.winRect.Lerp(input.viewport.InverseLerpFlipY(p));
}

void ICurveView::SetScreenPoint(const CurveEditorInput& input, CurvePointID cpid, Vec2f sp)
{
	Vec2f p = input.viewport.LerpFlipY(input.winRect.InverseLerp(sp));
	if (float snapX = input.settings->snapX)
		p.x = roundf(p.x / snapX) * snapX;
	switch (cpid.pointType)
	{
	case CPT_Point:
		SetPoint(cpid.curveID, cpid.pointID, p);
		break;
	case CPT_LTangent:
		SetLeftTangentPoint(cpid.curveID, cpid.pointID, p);
		break;
	case CPT_RTangent:
		SetRightTangentPoint(cpid.curveID, cpid.pointID, p);
		break;
	case CPT_Midpoint:
		SetSliceMidpoint(cpid.curveID, cpid.pointID, p);
		break;
	}
}

uint32_t ICurveView::_FixPointOrder(uint32_t curveid, uint32_t pointid)
{
	while (pointid > 0 && GetPoint(curveid, pointid).x < GetPoint(curveid, pointid - 1).x)
	{
		_SwapPoints(curveid, pointid - 1, pointid);
		pointid--;
	}
	auto end = GetPointCount(curveid);
	while (pointid + 1 < end && GetPoint(curveid, pointid).x > GetPoint(curveid, pointid + 1).x)
	{
		_SwapPoints(curveid, pointid, pointid + 1);
		pointid++;
	}
	return pointid;
}

void ICurveView::_SwapPoints(uint32_t curveid, uint32_t p0, uint32_t p1)
{
	auto tmp = GetPoint(curveid, p0);
	SetPoint(curveid, p0, GetPoint(curveid, p1));
	SetPoint(curveid, p1, tmp);

	if (GetFeatures() & Tangents)
	{
		tmp = GetLeftTangentPoint(curveid, p0);
		SetLeftTangentPoint(curveid, p0, GetLeftTangentPoint(curveid, p1));
		SetLeftTangentPoint(curveid, p1, tmp);

		tmp = GetRightTangentPoint(curveid, p0);
		SetRightTangentPoint(curveid, p0, GetRightTangentPoint(curveid, p1));
		SetRightTangentPoint(curveid, p1, tmp);
	}
}

CurvePointID ICurveView::HitTest(const CurveEditorInput& input, Vec2f cursorPos)
{
	uint32_t numCurves = GetCurveCount();
	auto vp = input.viewport;

	float pradsq = input.settings->pointRadius * input.settings->pointRadius;

	for (uint32_t cid = 0; cid < numCurves; cid++)
	{
		auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

		for (uint32_t pid = pointRange.max; pid > pointRange.min; )
		{
			pid--;
			Vec2f p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(GetPoint(cid, pid)));
			if ((p - cursorPos).LengthSq() <= pradsq)
				return { cid, CPT_Point, pid };
		}
	}

	if (GetFeatures() & SliceMidpoints)
	{
		for (uint32_t cid = 0; cid < numCurves; cid++)
		{
			auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });
			auto midptRange = ExpandForCurves(pointRange, GetPointCount(cid));

			for (uint32_t pid = midptRange.max; pid > midptRange.min; )
			{
				pid--;

				if (HasSliceMidpoint(cid, pid))
				{
					Vec2f p = GetSliceMidpointPosition(cid, pid);
					p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(p));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_Midpoint, pid };
				}
				if (HasRightTangent(cid, pid))
				{
					Vec2f p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(GetRightTangentPoint(cid, pid)));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_RTangent, pid };
				}
				if (HasLeftTangent(cid, pid))
				{
					Vec2f p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(GetLeftTangentPoint(cid, pid)));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_LTangent, pid };
				}
			}
		}
	}

	if (GetFeatures() & Tangents)
	{
		for (uint32_t cid = 0; cid < numCurves; cid++)
		{
			auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

			for (uint32_t pid = pointRange.max; pid > pointRange.min; )
			{
				pid--;

				if (HasRightTangent(cid, pid))
				{
					Vec2f p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(GetRightTangentPoint(cid, pid)));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_RTangent, pid };
				}
				if (HasLeftTangent(cid, pid))
				{
					Vec2f p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(GetLeftTangentPoint(cid, pid)));
					if ((p - cursorPos).LengthSq() <= pradsq)
						return { cid, CPT_LTangent, pid };
				}
			}
		}
	}

	return SubUIValueHelper<CurvePointID>::GetNullValue();
}

void ICurveView::DrawCurve(const CurveEditorInput& input, uint32_t curveid)
{
	auto vp = input.viewport;
	auto col = GetCurveColor(curveid);
	auto pointRange = GetLeastPointRange(curveid, { vp.x0, vp.x1 });

	// include adjacent points for edge curves
	pointRange = ExpandForCurves(pointRange, GetPointCount(curveid));
	if (pointRange.max == GetPointCount(curveid) && pointRange.max)
		pointRange.max--;

	constexpr int MAX_CURVE_POINTS = 1024;
	Vec2f curvePoints[MAX_CURVE_POINTS];

	for (uint32_t pid = pointRange.min; pid < pointRange.max; pid++)
	{
		int numPoints = GetCurvePointsForViewport(curveid, pid, vp, input.winRect.GetWidth(), curvePoints, MAX_CURVE_POINTS);
		for (int i = 0; i < numPoints; i++)
			curvePoints[i] = input.winRect.Lerp(input.viewport.InverseLerpFlipY(curvePoints[i]));
		draw::AALineCol({ curvePoints, size_t(numPoints) }, 1.0f, col, false);
	}
}

void ICurveView::DrawCurvePointsType(const CurveEditorInput& input, const CurveEditorState& state, uint32_t curveid, CurvePointType type, Range<uint32_t> pointRange)
{
	auto col = GetCurveColor(curveid);
	for (uint32_t pid = pointRange.min; pid < pointRange.max; pid++)
	{
		Vec2f p;
		switch (type)
		{
		case CPT_Midpoint:
			if (!HasSliceMidpoint(curveid, pid))
				continue;
			p = GetSliceMidpointPosition(curveid, pid);
			break;
		case CPT_LTangent:
			if (!HasLeftTangent(curveid, pid))
				continue;
			p = GetLeftTangentPoint(curveid, pid);
			break;
		case CPT_RTangent:
			if (!HasRightTangent(curveid, pid))
				continue;
			p = GetRightTangentPoint(curveid, pid);
			break;
		case CPT_Point:
		default:
			p = GetPoint(curveid, pid);
			break;
		}

		Color4b pcol = col;
		CurvePointID cpid = { curveid, type, pid };
		if (state.uiState.IsPressed(cpid))
			pcol = Color4fLerp(col, Color4f::Black(), 0.2f);
		else if (state.uiState.IsHovered(cpid))
			pcol = Color4fLerp(col, Color4f::White(), 0.2f);

		p = input.winRect.Lerp(input.viewport.InverseLerpFlipY(p));
		draw::AACircleLineCol(p, input.settings->pointRadius - 0.5f, 1, pcol);
	}
}

void ICurveView::DrawAllPoints(const CurveEditorInput& input, const CurveEditorState& state)
{
	uint32_t numCurves = GetCurveCount();
	auto vp = input.viewport;

	for (uint32_t cid = 0; cid < numCurves; cid++)
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
	uint32_t numCurves = GetCurveCount();
	auto vp = input.viewport;

	for (uint32_t cid = 0; cid < numCurves; cid++)
	{
		if (!IsCurveVisible(cid))
			continue;

		auto pointCount = GetPointCount(cid);
		auto pointRange = GetLeastPointRange(cid, { vp.x0, vp.x1 });

		// to include tangent lines for points just out of view
		auto tangentRange = ExpandForTangents(cid, pointRange);

		auto col = GetCurveColor(cid);
		for (uint32_t pid = tangentRange.min; pid < tangentRange.max; pid++)
		{
			auto pt = GetPoint(cid, pid);
			pt = input.winRect.Lerp(input.viewport.InverseLerpFlipY(pt));

			if (HasLeftTangent(cid, pid))
			{
				auto tanpt = GetLeftTangentPoint(cid, pid);
				tanpt = input.winRect.Lerp(input.viewport.InverseLerpFlipY(tanpt));

				draw::AALineCol({ pt, tanpt }, input.settings->tangentLineThickness, col, false);
			}
			if (HasRightTangent(cid, pid))
			{
				auto tanpt = GetRightTangentPoint(cid, pid);
				tanpt = input.winRect.Lerp(input.viewport.InverseLerpFlipY(tanpt));

				draw::AALineCol({ pt, tanpt }, input.settings->tangentLineThickness, col, false);
			}
		}
	}
}

void ICurveView::DrawAll(const CurveEditorInput& input, const CurveEditorState& state)
{
	uint32_t numCurves = GetCurveCount();
	auto vp = input.viewport;

	if (draw::PushScissorRect(input.winRect.ExtendBy(AABB2f::UniformBorder(1))))
	{
		// curve lines
		for (uint32_t cid = 0; cid < numCurves; cid++)
		{
			if (!IsCurveVisible(cid))
				continue;

			DrawCurve(input, cid);
		}
	}
	draw::PopScissorRect();

	if (GetFeatures() & Tangents)
		DrawAllTangentLines(input, state);

	DrawAllPoints(input, state);
}


bool CurveEditorUI::OnEvent(const CurveEditorInput& input, ICurveView* curves, Event& e)
{
	uiState.InitOnEvent(e);
	auto ret = uiState.GlobalDragOnEvent(curves->HitTest(input, e.position), e);
	//printf("CEUI: ret=%d hover=%d press=%d\n", ret, uiState._hovered.pointID, uiState._pressed.pointID);
	switch (ret)
	{
	case SubUIDragState::Start:
		dragPointStart = curves->GetScreenPoint(input, uiState._pressed);
		dragCursorStart = e.position;
		break;
	case SubUIDragState::Move: {
		auto pid = uiState._pressed;

		float q = 1;
		if (pid.pointType == CPT_Midpoint)
			q = curves->GetSliceMidpointVertDragFactor(pid.curveID, pid.pointID);

		curves->SetScreenPoint(input, pid, dragPointStart + (e.position - dragCursorStart) * q);

		if (pid.pointType == CPT_Point)
			pid.pointID = curves->_FixPointOrder(pid.curveID, pid.pointID);

		break; }
	}
	curves->OnEvent(input, e);
	return false;
}

void CurveEditorUI::Render(const CurveEditorInput& input, ICurveView* curves)
{
	if (!curves)
		return;
	curves->DrawAll(input, *this);
}


void CurveEditorElement::OnReset()
{
	FillerElement::OnReset();

	curveView = nullptr;
	viewport = { 0, 0, 1, 1 };
	settings = {};
	gridSettings = {};

	GetStyle().SetPadding(4);
	flags |= UIObject_DB_CaptureMouseOnLeftClick | UIObject_SetsChildTextStyle;
}

void CurveEditorElement::OnEvent(Event& e)
{
	if (_ui.OnEvent({ viewport, GetContentRect(), &settings }, curveView, e))
		e.StopPropagation();
}

void CurveEditorElement::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	gridSettings.Draw(viewport, GetContentRect());
	_ui.Render({ viewport, GetContentRect(), &settings }, curveView);

	ph.PaintChildren(this, ctx);
}


void Sequence01Curve::Point::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Sequence01Curve::Point");

	OnField(oi, "deltaX", deltaX);
	OnField(oi, "posX", posX);
	OnField(oi, "posY", posY);
	OnFieldEnumInt(oi, "mode", mode);
	OnField(oi, "tweak", tweak);

	oi.EndObject();
}

void Sequence01Curve::SerializeData(IObjectIterator& oi)
{
	OnField(oi, "points", points);
}

static float DoPowerCurve(float q, float tweak)
{
	return tweak >= 0
		? 1 - powf(1 - q, exp2(tweak))
		: powf(q, exp2(-tweak));
}

float Sequence01Curve::EvaluateSegment(const Point& p0, const Point& p1, float q)
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

float Sequence01Curve::Evaluate(float t)
{
	if (points.empty())
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
	return points.back().posY;
}

void Sequence01Curve::RemovePoint(size_t i)
{
	points.erase(points.begin() + i);
	if (i > 0)
		points[i].deltaX = points[i].posX - points[i - 1].posX;
	else
		points[i].deltaX = points[i].posX;
}

void Sequence01Curve::AddPoint(Vec2f pos)
{
	size_t before = 0;
	while (before < points.size() && points[before].posX <= pos.x)
		before++;

	Sequence01Curve::Point p = {};
	if (!points.empty())
		p = before < points.size() ? points[before] : points.back();
	p.posX = pos.x;
	p.posY = clamp(pos.y, 0.0f, 1.0f);

	points.insert(points.begin() + before, p);
	if (before > 0)
		points[before].deltaX = points[before].posX - points[before - 1].posX;
	if (before + 1 < points.size())
		points[before + 1].deltaX = points[before + 1].posX - points[before].posX;
	if (before == 0 && points.size() > 1)
		points[before + 1].mode = Mode::SinglePowerCurve;
}


void Sequence01CurveView::SetPoint(uint32_t, uint32_t pointid, Vec2f p)
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

	for (uint32_t i = pointid + 1; i < uint32_t(curve->points.size()); i++)
	{
		curve->points[i].posX = curve->points[i - 1].posX + curve->points[i].deltaX;
	}
}

Vec2f Sequence01CurveView::GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q)
{
	auto& p0 = curve->points[firstpointid];
	auto& p1 = curve->points[firstpointid + 1];
	float retX = lerp(p0.posX, p1.posX, q);
	float retY = curve->EvaluateSegment(p0, p1, q);
	return { retX, retY };
}

float Sequence01CurveView::GetSliceMidpointVertDragFactor(uint32_t curveid, uint32_t sliceid)
{
	auto& p0 = curve->points[sliceid];
	auto& p1 = curve->points[sliceid + 1];
	if (p1.mode == Sequence01Curve::Mode::SinglePowerCurve)
		return p0.posY < p1.posY ? 1 : -1;
	return 1;
}

Vec2f Sequence01CurveView::GetSliceMidpointPosition(uint32_t curveid, uint32_t sliceid)
{
	auto& p0 = curve->points[sliceid];
	auto& p1 = curve->points[sliceid + 1];
	switch (p1.mode)
	{
	case Sequence01Curve::Mode::SawWave:
	case Sequence01Curve::Mode::PulseWave:
	case Sequence01Curve::Mode::Steps:
		return { (p0.posX + p1.posX) * 0.5f, (p0.posY + p1.posY) * 0.5f };
	default:
		return ICurveView::GetSliceMidpointPosition(curveid, sliceid);
	}
}

void Sequence01CurveView::OnEvent(const CurveEditorInput& input, Event& e)
{
	using Mode = Sequence01Curve::Mode;
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
				cm.basePriority += MenuItemCollection::SEPARATOR_THRESHOLD;

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
				cm.basePriority += MenuItemCollection::SEPARATOR_THRESHOLD;

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
			Vec2f cwp = input.viewport.LerpFlipY(input.winRect.InverseLerp(e.position));
			if (float s = input.settings->snapX)
				cwp.x = roundf(cwp.x / s) * s;
			cm.AddNext("Add") = [this, cwp]()
			{
				curve->AddPoint(cwp);
			};
		}
	}
}


void CubicNormalizedRemapCurveView::OnEvent(const CurveEditorInput& input, Event& e)
{
	if (e.type == EventType::ContextMenu)
	{
		auto& cm = ContextMenu::Get();
		cm.Add("Reset") = [this]()
		{
			*curve = CubicNormalizedRemapCurve();
		};
	}
}

} // ui
