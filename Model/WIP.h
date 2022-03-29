
#include "Objects.h"
#include <algorithm>


namespace ui {

struct StackExpandLTRLayoutElement : UIElement
{
	struct Slot
	{
		UIWeakPtr<UIObject> element;
	};
	static TempEditable<Slot> GetSlotTemplate()
	{
		return { &_slotTemplate };
	}
	static Slot _slotTemplate;

	std::vector<Slot> _slots;

	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		if (type == EstSizeType::Expanding)
		{
			float size = 0;
			for (auto& slot : _slots)
				if (auto* ch = slot.element.Get())
					size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
			return size;
		}
		else
			return containerSize.x;
	}
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto& slot : _slots)
			if (auto* ch = slot.element.Get())
				size = max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min);
		return size;
	}
	void CalcLayout(const UIRect& inrect, LayoutState& state) override
	{
		float p = inrect.x0;
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
			auto* ch = slot.element.Get();
			if (!ch)
				continue;
			auto s = ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding);
			auto sw = ch->GetStyle().GetWidth();
			float fr = sw.unit == CoordTypeUnit::Fraction ? sw.value : 1;
			items.push_back({ ch, s.min, s.max, s.min, fr });
			sorted.push_back(sorted.size());
			sum += s.min;
			frsum += fr;
		}
		std::sort(sorted.begin(), sorted.end(), [&items](int ia, int ib)
		{
			const auto& a = items[ia];
			const auto& b = items[ib];
			return (a.maxw - a.minw) < (b.maxw - b.minw);
		});
		float leftover = max(inrect.GetWidth() - sum, 0.0f);
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
		}
		for (const auto& item : items)
		{
			item.ch->PerformLayout({ p, inrect.y0, p + item.w, inrect.y1 }, inrect.GetSize());
			p += item.w;
		}
		state.finalContentRect = { inrect.x0, inrect.y0, max(inrect.x1, p), inrect.y1 };
	}
	void OnReset() override
	{
		_slots.clear();
	}
	void CustomAppendChild(UIObject* obj) override
	{
		AppendChild(obj);
		Slot slot = _slotTemplate;
		slot.element = obj;
		_slots.push_back(slot);
	}
};

} // ui
