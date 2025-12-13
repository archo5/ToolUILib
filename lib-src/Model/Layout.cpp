
#include "Layout.h"

#include "System.h" // TODO: only for FrameContents->EventSystem->size & Push/Pop


namespace ui {

extern uint32_t g_curLayoutFrame;


EdgeSliceLayoutElement::Slot EdgeSliceLayoutElement::_slotTemplate;

EstSizeRange EdgeSliceLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return EstSizeRange::SoftAtLeast(containerSize.x);
}

EstSizeRange EdgeSliceLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return EstSizeRange::SoftAtLeast(containerSize.y);
}

void EdgeSliceLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto subr = rect;
	for (const auto& slot : _slots)
	{
		auto* ch = slot._obj;
		if (!ch->_NeedsLayout())
			continue;

		auto e = slot.edge;
		EstSizeRange r;//(DoNotInitialize{});
		float d;
		LayoutInfo cinfo = info;
		switch (e)
		{
		case Edge::Top:
			r = ch->CalcEstimatedHeight(subr.GetSize(), EstSizeType::Expanding);
			if (&slot == &_slots.Last())
			{
				d = min(r.ExpandToFill(subr.GetHeight()), subr.GetHeight());
			}
			else
			{
				d = r.GetMin();
				cinfo = info.WithoutFillV();
			}
			ch->PerformLayout({ subr.x0, subr.y0, subr.x1, subr.y0 + d }, cinfo);
			subr.y0 += d;
			break;
		case Edge::Bottom:
			d = ch->CalcEstimatedHeight(subr.GetSize(), EstSizeType::Expanding).GetMin();
			ch->PerformLayout({ subr.x0, subr.y1 - d, subr.x1, subr.y1 }, info.WithoutFillV());
			subr.y1 -= d;
			break;
		case Edge::Left:
			d = ch->CalcEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).GetMin();
			ch->PerformLayout({ subr.x0, subr.y0, subr.x0 + d, subr.y1 }, info.WithoutFillH());
			subr.x0 += d;
			break;
		case Edge::Right:
			d = ch->CalcEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).GetMin();
			ch->PerformLayout({ subr.x1 - d, subr.y0, subr.x1, subr.y1 }, info.WithoutFillH());
			subr.x1 -= d;
			break;
		}
	}
	_finalRect = rect;
}


EstSizeRange LayerLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	EstSizeRange r;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		auto cr = slot._obj->CalcEstimatedWidth(containerSize, type);
		r = r.LimitTo(cr);
	}
	return r;
}

EstSizeRange LayerLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	EstSizeRange r;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		auto cr = slot._obj->CalcEstimatedHeight(containerSize, type);
		r = r.LimitTo(cr);
	}
	return r;
}

void LayerLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		slot._obj->PerformLayout(rect, { LayoutInfo::FillH | LayoutInfo::FillV });
	}
	_finalRect = rect;
}


PlacementLayoutElement::Slot PlacementLayoutElement::_slotTemplate;

EstSizeRange PlacementLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	EstSizeRange r;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		if (slot.measure)
		{
			auto cr = slot._obj->CalcEstimatedWidth(containerSize, type);
			r = r.LimitTo(cr);
		}
	}

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = r;
	return r;
}

EstSizeRange PlacementLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	EstSizeRange r;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		if (slot.measure)
		{
			auto cr = slot._obj->CalcEstimatedHeight(containerSize, type);
			r = r.LimitTo(cr);
		}
	}

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = r;
	return r;
}

void PlacementLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	UIRect contRect = rect;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		UIRect r = rect;
		if (slot.placement)
		{
			if (slot.placement->fullScreenRelative)
				r = { 0, 0, system->eventSystem.width, system->eventSystem.height };
			slot.placement->OnApplyPlacement(slot._obj, r);
		}
		contRect.Include(r);
		r = r.CastRounded<float>();
		slot._obj->PerformLayout(r, { LayoutInfo::FillH | LayoutInfo::FillV });
	}
	_finalRect = contRect;
}


void PointAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect) const
{
	UIRect parentRect = outRect;
	Size2f contSize = parentRect.GetSize();

	float w = curObj->CalcEstimatedWidth(contSize, EstSizeType::Expanding).softMin;
	float h = curObj->CalcEstimatedHeight(contSize, EstSizeType::Expanding).softMin;

	float x = lerp(parentRect.x0, parentRect.x1, anchor.x) - w * pivot.x + bias.x;
	float y = lerp(parentRect.y0, parentRect.y1, anchor.y) - h * pivot.y + bias.y;
	outRect = { x, y, x + w, y + h };
}

void RectAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect) const
{
	UIRect parentRect = outRect;

	outRect.x0 = lerp(parentRect.x0, parentRect.x1, anchor.x0) + bias.x0;
	outRect.x1 = lerp(parentRect.x0, parentRect.x1, anchor.x1) + bias.x1;
	outRect.y0 = lerp(parentRect.y0, parentRect.y1, anchor.y0) + bias.y0;
	outRect.y1 = lerp(parentRect.y0, parentRect.y1, anchor.y1) + bias.y1;
}

} // ui
