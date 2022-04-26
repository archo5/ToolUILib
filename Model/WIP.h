
#include "Objects.h"
#include <algorithm>


namespace ui {

struct PaddingElement : UIObjectSingleChild
{
	UIRect padding;

	void OnReset() override;
	Size2f GetReducedContainerSize(Size2f size);
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

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
		// reset the template
		_slotTemplate = {};

		if (system)
			obj->_AttachToFrameContents(system);
	}

	void OnPaint(const UIPaintContext& ctx) override
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

namespace _ {
struct StackLTRLayoutElement_Slot
{
	UIObject* _obj = nullptr;
};
} // _

struct StackLTRLayoutElement : LayoutElement<_::StackLTRLayoutElement_Slot>
{
	float paddingBetweenElements = 0;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

	StackLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }
};

namespace _ {
struct StackTopDownLayoutElement_Slot
{
	UIObject* _obj = nullptr;
};
} // _

struct StackTopDownLayoutElement : LayoutElement<_::StackTopDownLayoutElement_Slot>
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;
};

namespace _ {
struct StackExpandLTRLayoutElement_Slot
{
	using T = StackExpandLTRLayoutElement_Slot;

	UIObject* _obj = nullptr;
	float fraction = 1;

	T& DisableScaling() { fraction = 0; return *this; }
	T& SetMaximumScaling() { fraction = FLT_MAX; return *this; }
	T& SetScaleWeight(float f) { fraction = f; return *this; }
};
} // _

struct StackExpandLTRLayoutElement : LayoutElement<_::StackExpandLTRLayoutElement_Slot>
{
	float paddingBetweenElements = 0;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

	StackExpandLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }
};

struct WrapperLTRLayoutElement : UIElement
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
			size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
		return Rangef::AtLeast(size);
	}
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		float size = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
			size = max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min);
		return Rangef::AtLeast(size);
	}
	void OnLayout(const UIRect& rect) override
	{
		auto contSize = rect.GetSize();
		float p = rect.x0;
		float y0 = rect.y0;
		float maxH = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			float w = ch->GetFullEstimatedWidth(contSize, EstSizeType::Expanding).min;
			float h = ch->GetFullEstimatedHeight(contSize, EstSizeType::Expanding).min;
			ch->PerformLayout({ p, y0, p + w, y0 + h });
			p += w;
			maxH = max(maxH, h);
		}
		_finalRect = rect;
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
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;
};

struct TabbedPanelStyle
{
	static constexpr const char* NAME = "TabbedPanelStyle";

	//float tabHeight = 22; // TODO autodetect?
	float tabButtonOverlap = 2;
	float tabButtonYOffsetInactive = 1;
	float tabInnerButtonMargin = 4;
	AABB2f tabButtonPadding = {};
	PainterHandle tabButtonPainter;
	FontSettings tabButtonFont;
	AABB2f tabPanelPadding = {};
	PainterHandle tabPanelPainter;
	Size2f tabCloseButtonSize = {};
	PainterHandle tabCloseButtonPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct TabbedPanel : UIObjectSingleChild
{
	struct Tab
	{
		std::string text;
		UIObject* obj = nullptr;
		uintptr_t uid = UINTPTR_MAX;

		float _contentWidth = 0;
	};

	std::vector<Tab> _tabs;
	size_t _curTabNum = 0;
	SubUI<uint32_t> _tabUI;

	TabbedPanelStyle style;
	float tabHeight = 22;

	UIObject* _tabBarExtension = nullptr;
	bool showCloseButton = false;
	std::function<void(size_t, uintptr_t)> onClose;
	bool rebuildOnChange = true;

	void SetTabBarExtension(UIObject* o);
	void AddTextTab(StringView text, uintptr_t uid = UINTPTR_MAX);
	void AddUITab(UIObject* obj, uintptr_t uid = UINTPTR_MAX);
	uintptr_t GetCurrentTabUID(uintptr_t def = UINTPTR_MAX) const;
	bool SetActiveTabByUID(uintptr_t uid);

	template <class E> void AddEnumTab(StringView name, E value)
	{
		AddTextTab(name, uintptr_t(value));
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
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	UIObject* FindLastChildContainingPos(Point2f pos) const override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	void _DetachFromTree() override;
};

} // ui
