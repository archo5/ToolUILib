
#include "Objects.h"
#include <algorithm>


namespace ui {

struct PaddingElement : UIObjectSingleChild
{
	UIRect padding;

	void OnReset() override;
	Size2f GetReducedContainerSize(Size2f size);
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true) override;
	void OnLayout(const UIRect& rect, const Size2f& containerSize) override;

	PaddingElement& SetPadding(float w) { padding = UIRect::UniformBorder(w); return *this; }
	PaddingElement& SetPadding(float x, float y) { padding = { x, y, x, y }; return *this; }
	PaddingElement& SetPadding(float l, float t, float r, float b) { padding = { l, t, r, b }; return *this; }
	PaddingElement& SetPadding(const UIRect& r) { padding = r; return *this; }

	PaddingElement& SetPaddingLeft(float p) { padding.x0 = p; return *this; }
	PaddingElement& SetPaddingTop(float p) { padding.y0 = p; return *this; }
	PaddingElement& SetPaddingRight(float p) { padding.x1 = p; return *this; }
	PaddingElement& SetPaddingBottom(float p) { padding.y1 = p; return *this; }
};

template <class SlotT>
struct LayoutElement : UIElement
{
	using Slot = SlotT;

	static TempEditable<Slot> GetSlotTemplate()
	{
		return { &_slotTemplate };
	}
	static Slot _slotTemplate;

	std::vector<Slot> _slots;

	void OnReset() override
	{
		UIElement::OnReset();

		_slots.clear();
	}

	void SlotIterator_Init(UIObjectIteratorData& data) override
	{
		data.data0 = 0;
	}

	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override
	{
		if (data.data0 >= _slots.size())
			return nullptr;
		return _slots[data.data0++]._obj;
	}

	void RemoveChildImpl(UIObject* ch) override
	{
		for (size_t i = 0; i < _slots.size(); i++)
		{
			if (_slots[i]._obj == ch)
			{
				_slots.erase(_slots.begin() + i);
				break;
			}
		}
	}

	void DetachChildren(bool recursive) override
	{
		for (size_t i = 0; i < _slots.size(); i++)
		{
			auto* ch = _slots[i]._obj;

			if (recursive)
				ch->DetachChildren(true);

			// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
			if (system)
				ch->_DetachFromTree();

			ch->parent = nullptr;
		}
		_slots.clear();
	}

	void CustomAppendChild(UIObject* obj) override
	{
		obj->DetachParent();

		obj->parent = this;
		Slot slot = _slotTemplate;
		slot._obj = obj;
		_slots.push_back(slot);

		if (system)
			obj->_AttachToFrameContents(system);
	}

	void PaintChildrenImpl(const UIPaintContext& ctx) override
	{
		for (auto& slot : _slots)
			slot._obj->Paint(ctx);
	}

	UIObject* FindLastChildContainingPos(Point2f pos) const override
	{
		for (size_t i = _slots.size(); i > 0; )
		{
			i--;
			if (_slots[i]._obj->Contains(pos))
				return _slots[i]._obj;
		}
		return nullptr;
	}

	void _AttachToFrameContents(FrameContents* owner) override
	{
		UIElement::_AttachToFrameContents(owner);

		for (auto& slot : _slots)
			slot._obj->_AttachToFrameContents(owner);
	}

	void _DetachFromFrameContents() override
	{
		for (auto& slot : _slots)
			slot._obj->_DetachFromFrameContents();

		UIElement::_DetachFromFrameContents();
	}
};

struct StackLTRLayoutElement : UIElement
{
	float paddingBetweenElements = 0;

	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			if (ch != firstChild)
				size += paddingBetweenElements;
			size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Exact).min;
		}
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
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		float p = inrect.x0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			float w = ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Exact).min;
			Rangef hr = ch->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Expanding);
			float h = clamp(inrect.y1 - inrect.y0, hr.min, hr.max);
			ch->PerformLayout({ p, inrect.y0, p + w, inrect.y0 + h }, inrect.GetSize());
			p += w + paddingBetweenElements;
		}
	}

	StackLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }
};

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
			size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min;
		return size;
	}
	void CalcLayout(const UIRect& inrect, LayoutState& state) override
	{
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		float p = inrect.y0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			Rangef wr = ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding);
			float w = clamp(inrect.x1 - inrect.x0, wr.min, wr.max);
			float h = ch->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Exact).min;
			ch->PerformLayout({ inrect.x0, p, inrect.x0 + w, p + h }, inrect.GetSize());
			p += h;
		}
	}
};

namespace _ {
struct StackExpandLTRLayoutElement_Slot
{
	UIObject* _obj = nullptr;
	const IPlacement* placement = nullptr;
	bool measure = true;
};
} // _

struct StackExpandLTRLayoutElement : LayoutElement<_::StackExpandLTRLayoutElement_Slot>
{
	float paddingBetweenElements = 0;

	float CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	float CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void CalcLayout(const UIRect& inrect, LayoutState& state) override;

	StackExpandLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }
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
	}
};

namespace _ {
struct PlacementLayoutElement_Slot
{
	UIObject* _obj = nullptr;
	const IPlacement* placement = nullptr;
	bool measure = true;
};
} // _

struct PlacementLayoutElement : LayoutElement<_::PlacementLayoutElement_Slot>
{
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true) override;
	void OnLayout(const UIRect& rect, const Size2f& containerSize) override;
};

struct TabbedPanel : UIObjectSingleChild
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
	StyleBlockRef tabCloseButtonStyle;
	float tabHeight = 22;
	float tabButtonOverlap = 2;
	float tabButtonYOffsetInactive = 1;
	float tabInnerButtonMargin = 4;

	bool showCloseButton = false;
	std::function<void(size_t, uintptr_t)> onClose;
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

	template <class E> void AddEnumTab(StringView name, E value)
	{
		_tabs.push_back({ to_string(name), uintptr_t(value) });
	}
	template <class E> E GetCurrentTabEnumValue(E def = E(0)) const
	{
		return E(GetCurrentTabUID(uintptr_t(def)));
	}
	template <class E> bool SetActiveTabByEnumValue(E value)
	{
		return SetActiveTabByUID(uintptr_t(value));
	}

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	Size2f GetReducedContainerSize(Size2f size);
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout = true) override;
	void OnLayout(const UIRect& rect, const Size2f& containerSize) override;
};

} // ui
