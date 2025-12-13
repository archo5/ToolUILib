
#include "Layout.h"

#include "System.h" // TODO: only for FrameContents->EventSystem->size & Push/Pop

#include <algorithm> // TODO: only for std::sort


namespace ui {

extern uint32_t g_curLayoutFrame;


StackLTRLayoutElement::Slot StackLTRLayoutElement::_slotTemplate;

EstSizeRange StackLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	EstSizeRange r;
	bool first = true;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		if (!first)
			r.Add(paddingBetweenElements);
		else
			first = false;
		r = r.AddMins(slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Exact));
	}

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = r;
	return r;
}

Rangef StackLTRLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	float size = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		size = max(size, slot._obj->CalcEstimatedHeight(containerSize, EstSizeType::Expanding).min);
	}
	Rangef r = Rangef::AtLeast(size);

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = r;
	return r;
}

void StackLTRLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	// put items one after another in the indicated direction
	// container size adapts to child elements in stacking direction, and to parent in the other
	float p = rect.x0;
	auto rectSize = rect.GetSize();
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		float w = slot._obj->CalcEstimatedWidth(rectSize, EstSizeType::Exact).GetMin();
		Rangef hr = slot._obj->CalcEstimatedHeight(rectSize, EstSizeType::Expanding);
		float h = clamp(rect.y1 - rect.y0, hr.min, hr.max);
		slot._obj->PerformLayout({ p, rect.y0, p + w, rect.y0 + h }, info.WithoutFillH());
		p += w + paddingBetweenElements;
	}
	_finalRect = rect;
}


StackTopDownLayoutElement::Slot StackTopDownLayoutElement::_slotTemplate;

EstSizeRange StackTopDownLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	EstSizeRange r;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		r = r.WithMins(slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Expanding));
	}

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = r;
	return r;
}

Rangef StackTopDownLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	float size = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		size += slot._obj->CalcEstimatedHeight(containerSize, EstSizeType::Exact).min;
	}
	Rangef r = Rangef::AtLeast(size);

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = r;
	return r;
}

void StackTopDownLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	// put items one after another in the indicated direction
	// container size adapts to child elements in stacking direction, and to parent in the other
	float p = rect.y0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		EstSizeRange wr = slot._obj->CalcEstimatedWidth(rect.GetSize(), EstSizeType::Expanding);
		float w = wr.ExpandToFill(rect.GetWidth());
		float h = slot._obj->CalcEstimatedHeight(rect.GetSize(), EstSizeType::Exact).min;
		slot._obj->PerformLayout({ rect.x0, p, rect.x0 + w, p + h }, info.WithoutFillV());
		p += h;
	}
	_finalRect = rect;
}


StackExpandLTRLayoutElement::Slot StackExpandLTRLayoutElement::_slotTemplate;

EstSizeRange StackExpandLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	EstSizeRange r;
	if (type == EstSizeType::Expanding)
	{
		for (auto& slot : _slots)
		{
			if (!slot._obj->_NeedsLayout())
				continue;

			r = r.AddMins(slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Expanding));
		}
		if (_slots.size() >= 2)
			r = r.Add(paddingBetweenElements * (_slots.size() - 1));
	}
	else
		r = EstSizeRange::SoftAtLeast(containerSize.x);

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = r;
	return r;
}

Rangef StackExpandLTRLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	float size = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		size = max(size, slot._obj->CalcEstimatedHeight(containerSize, EstSizeType::Exact).min);
	}
	Rangef r = Rangef::AtLeast(size);

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = r;
	return r;
}

void StackExpandLTRLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	float p = rect.x0;
	float sum = 0, frsum = 0;
	struct Item
	{
		UIObject* ch;
		float hardMin;
		float softMin;
		float hardMax;
		float w;
		float fr;
	};
	Array<Item> items;
	Array<int> sorted;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		auto s = slot._obj->CalcEstimatedWidth(rect.GetSize(), EstSizeType::Expanding);
		items.Append({ slot._obj, s.hardMin, s.GetMin(), s.hardMax, s.GetMin(), slot.fraction });
		sorted.Append(sorted.size());
		sum += s.GetMin();
		frsum += slot.fraction;
	}
	if (_slots.size() >= 2)
		sum += paddingBetweenElements * (_slots.size() - 1);
	if (frsum > 0)
	{
		std::sort(sorted.begin(), sorted.end(), [&items](int ia, int ib)
		{
			const auto& a = items[ia];
			const auto& b = items[ib];
			return (a.hardMax - a.softMin) < (b.hardMax - b.softMin);
		});
		float rectW = rect.GetWidth();
		if (rectW > sum)
		{
			float leftover = rectW - sum;
			for (auto idx : sorted)
			{
				auto& item = items[idx];
				float mylo = leftover * item.fr / frsum;
				float w = item.w + mylo;
				if (w > item.hardMax)
					w = item.hardMax;
				w = roundf(w);
				float actual_lo = w - item.softMin;
				leftover -= actual_lo;
				frsum -= item.fr;
				item.w = w;

				if (frsum <= 0)
					break;
			}
		}
		else
		{
			for (auto idx : sorted)
			{
				auto& item = items[idx];
				item.w = max(item.hardMin, roundf(item.w * rectW / sum));
				rectW -= item.w;
				sum -= item.softMin;
				if (sum <= 0)
					break;
			}
		}
	}
	for (const auto& item : items)
	{
		item.ch->PerformLayout({ p, rect.y0, p + item.w, rect.y1 }, info.WithoutFillH());
		p += item.w + paddingBetweenElements;
	}
	_finalRect = rect;
}


WrapperLTRLayoutElement::Slot WrapperLTRLayoutElement::_slotTemplate;

EstSizeRange WrapperLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	EstSizeRange r;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		r = r.AddMins(slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Expanding));
	}

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = r;
	return r;
}
Rangef WrapperLTRLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	float size = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		size = max(size, slot._obj->CalcEstimatedHeight(containerSize, EstSizeType::Expanding).min);
	}
	Rangef r = Rangef::AtLeast(size);

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = r;
	return r;
}
void WrapperLTRLayoutElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto contSize = rect.GetSize();
	float p = rect.x0;
	float y0 = rect.y0;
	float maxH = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		float w = slot._obj->CalcEstimatedWidth(contSize, EstSizeType::Expanding).GetMin();
		float h = slot._obj->CalcEstimatedHeight(contSize, EstSizeType::Expanding).min;
		slot._obj->PerformLayout({ p, y0, p + w, y0 + h }, info.WithoutAnyFill());
		p += w;
		maxH = max(maxH, h);
	}
	_finalRect = rect;
}


EdgeSliceLayoutElement::Slot EdgeSliceLayoutElement::_slotTemplate;

EstSizeRange EdgeSliceLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return EstSizeRange::SoftAtLeast(containerSize.x);
}

Rangef EdgeSliceLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(containerSize.y);
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
		Rangef r(DoNotInitialize{});
		float d;
		LayoutInfo cinfo = info;
		switch (e)
		{
		case Edge::Top:
			r = ch->CalcEstimatedHeight(subr.GetSize(), EstSizeType::Expanding);
			if (&slot == &_slots.Last())
			{
				d = min(r.Clamp(subr.GetHeight()), subr.GetHeight());
			}
			else
			{
				d = r.min;
				cinfo = info.WithoutFillV();
			}
			ch->PerformLayout({ subr.x0, subr.y0, subr.x1, subr.y0 + d }, cinfo);
			subr.y0 += d;
			break;
		case Edge::Bottom:
			d = ch->CalcEstimatedHeight(subr.GetSize(), EstSizeType::Expanding).min;
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


LayerLayoutElement::Slot LayerLayoutElement::_slotTemplate;

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

Rangef LayerLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		auto cr = slot._obj->CalcEstimatedHeight(containerSize, type);
		r = r.Intersect(cr);
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

Rangef PlacementLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		if (slot.measure)
		{
			auto cr = slot._obj->CalcEstimatedHeight(containerSize, type);
			r = r.Intersect(cr);
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
	float h = curObj->CalcEstimatedHeight(contSize, EstSizeType::Expanding).min;

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
