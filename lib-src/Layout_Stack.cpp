
#include "Layout_Stack.h"

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


} // ui
