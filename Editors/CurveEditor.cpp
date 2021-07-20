
#include "CurveEditor.h"

namespace ui {

Range<uint32_t> ICurves::ExpandForCurves(Range<uint32_t> src, uint32_t max)
{
	if (src.min > 0)
		src.min--;
	if (src.max < max)
		src.max++;
	// exclude last point (for which there is no matching curve)
	if (src.max == max)
		src.max--;
	return src;
}

AABB2f ICurves::GetPreferredViewport(bool includeTangents)
{
	AABB2f bbox = AABB2f::Empty();
	for (uint32_t cid = 0, numcurves = GetCurveCount(); cid < numcurves; cid++)
	{
		bbox = bbox.Include(GetPreferredCurveViewport(includeTangents, cid, { 0, GetPointCount(cid) }));
	}
	return bbox;
}

AABB2f ICurves::GetPreferredCurveViewport(bool includeTangents, uint32_t curveid, Range<uint32_t> pointRange)
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

Range<uint32_t> ICurves::ExpandForTangents(uint32_t curveid, Range<uint32_t> src)
{
	if (src.min > 0)
		src.min--;
	if (src.max < GetPointCount(curveid))
		src.max++;
	return src;
}

int ICurves::GetCurvePointsForRange(uint32_t curveid, uint32_t firstpointid, Rangef qrange, Vec2f* out, int maxOut)
{
	if (maxOut < 2)
		return 0;
	for (int i = 0; i < maxOut; i++)
	{
		float f = float(i) / float(maxOut - 1);
		float q = lerp(qrange.min, qrange.max, f);
		out[i] = GetInterpolatedPoint(curveid, firstpointid, q);
	}
	return maxOut;
}

int ICurves::GetCurvePointsForViewport(uint32_t curveid, uint32_t firstpointid, AABB2f vp, float winWidth, Vec2f* out, int maxOut)
{
	if (winWidth <= 0 || !vp.IsValid())
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

Vec2f ICurves::GetScreenPoint(const CurveEditorInput& input, CurvePointID cpid)
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

void ICurves::SetScreenPoint(const CurveEditorInput& input, CurvePointID cpid, Vec2f sp)
{
	Vec2f p = input.viewport.LerpFlipY(input.winRect.InverseLerp(sp));
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

uint32_t ICurves::_FixPointOrder(uint32_t curveid, uint32_t pointid)
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

void ICurves::_SwapPoints(uint32_t curveid, uint32_t p0, uint32_t p1)
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

CurvePointID ICurves::HitTest(const CurveEditorInput& input, Vec2f cursorPos)
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
					Vec2f p = GetSliceMidpoint(cid, pid);
					p = GetInterpolatedPoint(cid, pid, p.x);
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

void ICurves::DrawCurve(const CurveEditorInput& input, uint32_t curveid)
{
	auto vp = input.viewport;
	auto col = GetCurveColor(curveid);
	auto pointRange = GetLeastPointRange(curveid, { vp.x0, vp.x1 });

	// include adjacent points for edge curves
	pointRange = ExpandForCurves(pointRange, GetPointCount(curveid));
	if (pointRange.max == GetPointCount(curveid))
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

void ICurves::DrawCurvePointsType(const CurveEditorInput& input, const CurveEditorState& state, uint32_t curveid, CurvePointType type, Range<uint32_t> pointRange)
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
			p = GetSliceMidpoint(curveid, pid);
			p = GetInterpolatedPoint(curveid, pid, p.x);
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

void ICurves::DrawAllPoints(const CurveEditorInput& input, const CurveEditorState& state)
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

void ICurves::DrawAllTangentLines(const CurveEditorInput& input, const CurveEditorState& state)
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

void ICurves::DrawAll(const CurveEditorInput& input, const CurveEditorState& state)
{
	uint32_t numCurves = GetCurveCount();
	auto vp = input.viewport;

	if (draw::PushScissorRect(input.winRect.ExtendBy(AABB2f::UniformBorder(1)).Cast<int>()))
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


bool CurveEditorUI::OnEvent(const CurveEditorInput& input, ICurves* curves, Event& e)
{
	uiState.InitOnEvent(e);
	auto ret = uiState.GlobalDragOnEvent(curves->HitTest(input, e.position), e);
	//printf("CEUI: ret=%d hover=%d press=%d\n", ret, uiState._hovered.pointID, uiState._pressed.pointID);
	switch (ret)
	{
	case SubUIDragState::Start:
		dragOff = curves->GetScreenPoint(input, uiState._pressed) - e.position;
		break;
	case SubUIDragState::Move:
		curves->SetScreenPoint(input, uiState._pressed, e.position + dragOff);
		if (uiState._pressed.pointType == CPT_Point)
			uiState._pressed.pointID = curves->_FixPointOrder(uiState._pressed.curveID, uiState._pressed.pointID);
		break;
	}
	return false;
}

void CurveEditorUI::Render(const CurveEditorInput& input, ICurves* curves)
{
	if (!curves)
		return;
	curves->DrawAll(input, *this);
}


void CurveEditorElement::OnInit()
{
	GetStyle().SetPadding(4);
	SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
}

void CurveEditorElement::OnEvent(Event& e)
{
	if (_ui.OnEvent({ viewport, GetContentRect(), &settings }, curves, e))
		e.StopPropagation();
}

void CurveEditorElement::OnPaint()
{
	_ui.Render({ viewport, GetContentRect(), &settings }, curves);
}


void Sequence01Curve::SetPoint(uint32_t, uint32_t pointid, Vec2f p)
{
	float newX = p.x;
	float newY = clamp(p.y, 0.0f, 1.0f);

	auto& dp = points[pointid];
	dp.posY = newY;

	float prevX = pointid > 0 ? points[pointid - 1].posX : 0;
	dp.deltaX = newX - prevX;
	if (dp.deltaX < 0)
		dp.deltaX = 0;
	dp.posX = prevX + dp.deltaX;

	for (uint32_t i = pointid + 1; i < uint32_t(points.size()); i++)
	{
		points[i].posX = points[i - 1].posX + points[i].deltaX;
	}
}

static float DoPowerCurve(float q, float tweak)
{
	return tweak >= 0
		? 1 - powf(1 - q, exp2(tweak))
		: powf(q, exp2(-tweak));
}

Vec2f Sequence01Curve::GetInterpolatedPoint(uint32_t, uint32_t firstpointid, float q)
{
	auto& p0 = points[firstpointid];
	auto& p1 = points[firstpointid + 1];
	float retX = lerp(p0.posX, p1.posX, q);
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
	}
	float retY = lerp(p0.posY, p1.posY, q);
	return { retX, retY };
}

} // ui
