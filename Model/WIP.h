
#include "Objects.h"
#include <algorithm>


namespace ui {

struct StackTopDownLayoutElement : UIElement
{
	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
			size = max(size, ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min);
		return size;
	}
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
			size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min;
		return size;
	}
	void CalcLayout(const UIRect& inrect, LayoutState& state) override
	{
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		float p = inrect.y0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			float h = ch->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Expanding).min;
			ch->PerformLayout({ inrect.x0, p, inrect.x1, p + h }, inrect.GetSize());
			p += h;
		}
		state.finalContentRect = { inrect.x0, inrect.y0, inrect.x1, p };
	}
};

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

struct WrapperLTRLayoutElement : UIElement
{
	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
			size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
		return size;
	}
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
			size = max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min);
		return size;
	}
	void CalcLayout(const UIRect& inrect, LayoutState& state) override
	{
		auto contSize = inrect.GetSize();
		float p = inrect.x0;
		float y0 = inrect.y0;
		float maxH = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			float w = ch->GetFullEstimatedWidth(contSize, EstSizeType::Expanding).min;
			float h = ch->GetFullEstimatedHeight(contSize, EstSizeType::Expanding).min;
			ch->PerformLayout({ p, y0, p + w, y0 + h }, contSize);
			p += w;
			maxH = max(maxH, h);
		}
		state.finalContentRect = { inrect.x0, inrect.y0, p, y0 + maxH };
	}
};

struct TabbedPanel : StackTopDownLayoutElement
{
	struct Tab
	{
		std::string text;
		uintptr_t uid = UINTPTR_MAX;
	};

	std::vector<Tab> _tabs;
	size_t _curTabNum = 0;
	SubUI<uint32_t> _tabUI;

	StyleBlockRef panelStyle;
	StyleBlockRef tabButtonStyle;
	float tabHeight = 22;
	float tabButtonOverlap = 2;
	float tabButtonYOffsetInactive = 1;
	bool rebuildOnChange = true;

	void AddTab(const Tab& tab)
	{
		_tabs.push_back(tab);
	}
	uintptr_t GetCurrentTabUID(uintptr_t def = UINTPTR_MAX) const
	{
		return _curTabNum < _tabs.size() ? _tabs[_curTabNum].uid : def;
	}
	bool SetActiveTabByUID(uintptr_t uid)
	{
		for (auto& tab : _tabs)
		{
			if (tab.uid == uid)
			{
				_curTabNum = &tab - &_tabs.front();
				return true;
			}
		}
		return false;
	}

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		return StackTopDownLayoutElement::CalcEstimatedWidth(containerSize, type)
			+ panelStyle->padding_left
			+ panelStyle->padding_right;
	}
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		return StackTopDownLayoutElement::CalcEstimatedHeight(containerSize, type)
			+ panelStyle->padding_top
			+ panelStyle->padding_bottom
			+ tabHeight
			- tabButtonOverlap;
	}
	void CalcLayout(const UIRect& inrect, LayoutState& state) override
	{
		auto subr = inrect;
		subr.y0 += tabHeight - tabButtonOverlap;
		subr = subr.ShrinkBy(panelStyle->GetPaddingRect());
		StackTopDownLayoutElement::CalcLayout(subr, state);
		state.finalContentRect = inrect;
	}
};

} // ui
