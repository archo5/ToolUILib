
#include "Layout.h"

#include "System.h" // TODO: only for FrameContents->EventSystem->size & Push/Pop

#include <algorithm> // TODO: only for std::sort


namespace ui {

extern uint32_t g_curLayoutFrame;


void PaddingElement::OnReset()
{
	UIObjectSingleChild::OnReset();
	padding = {};
}

Size2f PaddingElement::GetReducedContainerSize(Size2f size)
{
	size.x -= padding.x0 + padding.x1;
	size.y -= padding.y0 + padding.y1;
	return size;
}

Rangef PaddingElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = padding.x0 + padding.x1;
	return (_child && _child->_NeedsLayout() ? _child->CalcEstimatedWidth(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

Rangef PaddingElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad = padding.y0 + padding.y1;
	return (_child && _child->_NeedsLayout() ? _child->CalcEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

void PaddingElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto padsub = rect.ShrinkBy(padding);
	if (_child && _child->_NeedsLayout())
	{
		_child->PerformLayout(padsub, info);
		ApplyLayoutInfo(_child->GetFinalRect().ExtendBy(padding), rect, info);
	}
	else
	{
		_finalRect = rect;
	}
}

void AddContentPadding::OnBeforeContent() const
{
	Push<PaddingElement>().SetPadding(padding);
}

void AddContentPadding::OnAfterContent() const
{
	Pop();
}


StackLTRLayoutElement::Slot StackLTRLayoutElement::_slotTemplate;

Rangef StackLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	float size = 0;
	bool first = true;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		if (!first)
			size += paddingBetweenElements;
		else
			first = false;
		size += slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Exact).min;
	}
	Rangef r = Rangef::AtLeast(size);

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

		float w = slot._obj->CalcEstimatedWidth(rectSize, EstSizeType::Exact).min;
		Rangef hr = slot._obj->CalcEstimatedHeight(rectSize, EstSizeType::Expanding);
		float h = clamp(rect.y1 - rect.y0, hr.min, hr.max);
		slot._obj->PerformLayout({ p, rect.y0, p + w, rect.y0 + h }, info.WithoutFillH());
		p += w + paddingBetweenElements;
	}
	_finalRect = rect;
}


StackTopDownLayoutElement::Slot StackTopDownLayoutElement::_slotTemplate;

Rangef StackTopDownLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	float size = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		size = max(size, slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Expanding).min);
	}
	Rangef r = Rangef::AtLeast(size);

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

		Rangef wr = slot._obj->CalcEstimatedWidth(rect.GetSize(), EstSizeType::Expanding);
		float w = clamp(wr.max, wr.min, rect.x1 - rect.x0);
		float h = slot._obj->CalcEstimatedHeight(rect.GetSize(), EstSizeType::Exact).min;
		slot._obj->PerformLayout({ rect.x0, p, rect.x0 + w, p + h }, info.WithoutFillV());
		p += h;
	}
	_finalRect = rect;
}


StackExpandLTRLayoutElement::Slot StackExpandLTRLayoutElement::_slotTemplate;

Rangef StackExpandLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	Rangef r(DoNotInitialize{});
	if (type == EstSizeType::Expanding)
	{
		float size = 0;
		for (auto& slot : _slots)
		{
			if (!slot._obj->_NeedsLayout())
				continue;

			size += slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Expanding).min;
		}
		if (_slots.size() >= 2)
			size += paddingBetweenElements * (_slots.size() - 1);
		r = Rangef::AtLeast(size);
	}
	else
		r = Rangef::AtLeast(containerSize.x);

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
		float minw;
		float maxw;
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
		items.Append({ slot._obj, s.min, s.max, s.min, slot.fraction });
		sorted.Append(sorted.size());
		sum += s.min;
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
			return (a.maxw - a.minw) < (b.maxw - b.minw);
		});
		float rectW = rect.GetWidth();
		if (rectW > sum)
		{
			float leftover = rectW - sum;
			for (auto idx : sorted)
			{
				auto& item = items[idx];
				float mylo = leftover * item.fr / frsum;
				float w = item.minw + mylo;
				if (w > item.maxw)
					w = item.maxw;
				w = roundf(w);
				float actual_lo = w - item.minw;
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
				item.w = roundf(item.w * rectW / sum);
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

Rangef WrapperLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	float size = 0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		size += slot._obj->CalcEstimatedWidth(containerSize, EstSizeType::Expanding).min;
	}
	Rangef r = Rangef::AtLeast(size);

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

		float w = slot._obj->CalcEstimatedWidth(contSize, EstSizeType::Expanding).min;
		float h = slot._obj->CalcEstimatedHeight(contSize, EstSizeType::Expanding).min;
		slot._obj->PerformLayout({ p, y0, p + w, y0 + h }, info.WithoutAnyFill());
		p += w;
		maxH = max(maxH, h);
	}
	_finalRect = rect;
}


EdgeSliceLayoutElement::Slot EdgeSliceLayoutElement::_slotTemplate;

Rangef EdgeSliceLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(containerSize.x);
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
			d = ch->CalcEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).min;
			ch->PerformLayout({ subr.x0, subr.y0, subr.x0 + d, subr.y1 }, info.WithoutFillH());
			subr.x0 += d;
			break;
		case Edge::Right:
			d = ch->CalcEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).min;
			ch->PerformLayout({ subr.x1 - d, subr.y0, subr.x1, subr.y1 }, info.WithoutFillH());
			subr.x1 -= d;
			break;
		}
	}
	_finalRect = rect;
}


LayerLayoutElement::Slot LayerLayoutElement::_slotTemplate;

Rangef LayerLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		auto cr = slot._obj->CalcEstimatedWidth(containerSize, type);
		r = r.Intersect(cr);
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

Rangef PlacementLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		if (slot.measure)
		{
			auto cr = slot._obj->CalcEstimatedWidth(containerSize, type);
			r = r.Intersect(cr);
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
		contRect = contRect.Include(r);
		r.x0 = roundf(r.x0);
		r.y0 = roundf(r.y0);
		r.x1 = roundf(r.x1);
		r.y1 = roundf(r.y1);
		slot._obj->PerformLayout(r, { LayoutInfo::FillH | LayoutInfo::FillV });
	}
	_finalRect = contRect;
}


void PointAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect) const
{
	UIRect parentRect = outRect;
	Size2f contSize = parentRect.GetSize();

	float w = curObj->CalcEstimatedWidth(contSize, EstSizeType::Expanding).min;
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
