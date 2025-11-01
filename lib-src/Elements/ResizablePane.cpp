
#include "ResizablePane.h"

#include "../Model/Theme.h"
#include "../Model/System.h"


namespace ui {

AABB2f ResizablePane::GetSplitRect()
{
	AABB2f rect = GetFinalRect();
	switch (resizableEdge)
	{
	default:
	case Edge::Left: rect.x1 = rect.x0 + vertSepStyle.size; break;
	case Edge::Right: rect.x0 = rect.x1 - vertSepStyle.size; break;
	case Edge::Top: rect.y1 = rect.y0 + horSepStyle.size; break;
	case Edge::Bottom: rect.y0 = rect.y1 - horSepStyle.size; break;
	}
	return rect;
}

static StaticID<SeparatorLineStyle> sid_seplinestyle_hor("resizable_pane_separator_horizontal");
static StaticID<SeparatorLineStyle> sid_seplinestyle_vert("resizable_pane_separator_vertical");
void ResizablePane::OnReset()
{
	UIObjectSingleChild::OnReset();

	vertSepStyle = *GetCurrentTheme()->GetStruct(sid_seplinestyle_vert);
	horSepStyle = *GetCurrentTheme()->GetStruct(sid_seplinestyle_hor);

	resizableEdge = Edge::Left;
	isRelSplit = false;
	splitPos = 100;
	edgeQ = 0;
}

void ResizablePane::OnPaint(const UIPaintContext& ctx)
{
	UIObjectSingleChild::OnPaint(ctx);

	PaintInfo info(this);
	auto& style = resizableEdge == Edge::Left || resizableEdge == Edge::Right ? vertSepStyle : horSepStyle;
	if (style.backgroundPainter)
	{
		info.rect = GetSplitRect();
		info.state &= ~(PS_Hover | PS_Down);
		if (_splitUI.IsHovered(0))
			info.state |= PS_Hover;
		if (_splitUI.IsPressed(0))
			info.state |= PS_Down;
		style.backgroundPainter->Paint(info);
	}
}

void ResizablePane::OnEvent(Event& e)
{
	// event while building, understanding of children is incomplete
	if (UIContainer::GetCurrent() &&
		GetCurrentBuildable())
		return;

	AABB2f frect = GetFinalRect();
	AABB2f prect = frect;
	if (parent)
		prect = parent->GetFinalRect();

	_splitUI.InitOnEvent(e);
	if (resizableEdge == Edge::Left || resizableEdge == Edge::Right)
	{
		switch (_splitUI.DragOnEvent(0, GetSplitRect(), e))
		{
		case SubUIDragState::Start:
			e.context->CaptureMouse(this);
			_dragOCP = e.position.x;
			_dragOSP = splitPos;
			if (isRelSplit)
				_dragOSP *= _contSize.x - vertSepStyle.size;
			break;
		case SubUIDragState::Move:
			splitPos = _dragOSP + (e.position.x - _dragOCP) * (resizableEdge == Edge::Left ? -1 : 1);
			splitPos = clamp(splitPos, roundf(vertSepStyle.size * edgeQ), _contSize.x - roundf(vertSepStyle.size * (1 - edgeQ)));
			if (isRelSplit)
				splitPos /= _contSize.x - vertSepStyle.size;
			_OnChangeStyle();
			{
				Event ev(e.context, this, EventType::Resize);
				ev.shortCode = 0;
				e.context->BubblingEvent(ev);
			}
			break;
		case SubUIDragState::Stop:
			if (e.context->GetMouseCapture() == this)
				e.context->ReleaseMouse();
			{
				Event ev(e.context, this, EventType::Resize);
				ev.shortCode = 1;
				e.context->BubblingEvent(ev);
			}
			break;
		}
	}
	else
	{
		switch (_splitUI.DragOnEvent(0, GetSplitRect(), e))
		{
		case SubUIDragState::Start:
			e.context->CaptureMouse(this);
			_dragOCP = e.position.y;
			_dragOSP = splitPos;
			if (isRelSplit)
				_dragOSP *= _contSize.y - horSepStyle.size;
			break;
		case SubUIDragState::Move:
			splitPos = _dragOSP + (e.position.y - _dragOCP) * (resizableEdge == Edge::Top ? -1 : 1);
			splitPos = clamp(splitPos, roundf(horSepStyle.size * edgeQ), _contSize.y - roundf(horSepStyle.size * (1 - edgeQ)));
			if (isRelSplit)
				splitPos /= _contSize.y - horSepStyle.size;
			_OnChangeStyle();
			{
				Event ev(e.context, this, EventType::Resize);
				ev.shortCode = 0;
				e.context->BubblingEvent(ev);
			}
			break;
		case SubUIDragState::Stop:
			if (e.context->GetMouseCapture() == this)
				e.context->ReleaseMouse();
			{
				Event ev(e.context, this, EventType::Resize);
				ev.shortCode = 1;
				e.context->BubblingEvent(ev);
			}
			break;
		}
	}
	if (e.type == EventType::SetCursor && _splitUI.IsAnyHovered())
	{
		e.context->SetDefaultCursor(
			resizableEdge == Edge::Left || resizableEdge == Edge::Right
			? DefaultCursor::ResizeCol
			: DefaultCursor::ResizeRow);
		e.StopPropagation();
	}
	_splitUI.FinalizeOnEvent(e);
}

EstSizeRange ResizablePane::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (resizableEdge == Edge::Left || resizableEdge == Edge::Right)
	{
		_contSize = containerSize;
		float x = (isRelSplit ? splitPos * containerSize.x : splitPos) + roundf((1 - edgeQ) * vertSepStyle.size);
		return EstSizeRange::SoftExact(clamp(x, vertSepStyle.size, containerSize.x));
	}
	return EstSizeRange::SoftExact(containerSize.x);
}

Rangef ResizablePane::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (resizableEdge == Edge::Top || resizableEdge == Edge::Bottom)
	{
		_contSize = containerSize;
		float y = (isRelSplit ? splitPos * containerSize.y : splitPos) + roundf((1 - edgeQ) * horSepStyle.size);
		return Rangef::Exact(clamp(y, horSepStyle.size, containerSize.y));
	}
	return Rangef::Exact(containerSize.y);
}

void ResizablePane::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child && _child->_NeedsLayout())
	{
		UIRect mrect = rect;
		switch (resizableEdge)
		{
		default:
		case Edge::Left: mrect.x0 += vertSepStyle.size; break;
		case Edge::Right: mrect.x1 -= vertSepStyle.size; break;
		case Edge::Top: mrect.y0 += horSepStyle.size; break;
		case Edge::Bottom: mrect.y1 -= horSepStyle.size; break;
		}

		_child->PerformLayout(mrect, info);

		//ApplyLayoutInfo(_child->GetFinalRect(), rect, info);
	}
	_finalRect = rect;
}

} // ui
