
#include "WIP.h"
#include "Theme.h"
#include "System.h"


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

void PaddingElement::OnLayout(const UIRect& rect)
{
	auto padsub = rect.ShrinkBy(padding);
	if (_child && _child->_NeedsLayout())
	{
		_child->PerformLayout(padsub);
		_finalRect = _child->GetFinalRect().ExtendBy(padding);
	}
	else
	{
		_finalRect = rect;
	}
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

void StackLTRLayoutElement::OnLayout(const UIRect& rect)
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
		slot._obj->PerformLayout({ p, rect.y0, p + w, rect.y0 + h });
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

void StackTopDownLayoutElement::OnLayout(const UIRect& rect)
{
	// put items one after another in the indicated direction
	// container size adapts to child elements in stacking direction, and to parent in the other
	float p = rect.y0;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		Rangef wr = slot._obj->CalcEstimatedWidth(rect.GetSize(), EstSizeType::Expanding);
		float w = clamp(rect.x1 - rect.x0, wr.min, wr.max);
		float h = slot._obj->CalcEstimatedHeight(rect.GetSize(), EstSizeType::Exact).min;
		slot._obj->PerformLayout({ rect.x0, p, rect.x0 + w, p + h });
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

void StackExpandLTRLayoutElement::OnLayout(const UIRect& rect)
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
	std::vector<Item> items;
	std::vector<int> sorted;
	for (auto& slot : _slots)
	{
		if (!slot._obj->_NeedsLayout())
			continue;

		auto s = slot._obj->CalcEstimatedWidth(rect.GetSize(), EstSizeType::Expanding);
		items.push_back({ slot._obj, s.min, s.max, s.min, slot.fraction });
		sorted.push_back(sorted.size());
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
		float leftover = max(rect.GetWidth() - sum, 0.0f);
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
	for (const auto& item : items)
	{
		item.ch->PerformLayout({ p, rect.y0, p + item.w, rect.y1 });
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
void WrapperLTRLayoutElement::OnLayout(const UIRect& rect)
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
		slot._obj->PerformLayout({ p, y0, p + w, y0 + h });
		p += w;
		maxH = max(maxH, h);
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

void PlacementLayoutElement::OnLayout(const UIRect& rect)
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
		//contRect = contRect.Include(r);
		r.x0 = roundf(r.x0);
		r.y0 = roundf(r.y0);
		r.x1 = roundf(r.x1);
		r.y1 = roundf(r.y1);
		slot._obj->PerformLayout(r);
	}
	_finalRect = contRect;
}

} // ui
