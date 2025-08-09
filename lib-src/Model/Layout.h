
#pragma once
#include "../Core/Array.h"
#include "Objects.h"


namespace ui {

struct PaddingElement : UIObjectSingleChild
{
	UIRect padding;

	void OnReset() override;
	Size2f GetReducedContainerSize(Size2f size);
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

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
struct LayoutElement : UIObject
{
	using Slot = SlotT;

	static TempEditable<Slot> GetSlotTemplate()
	{
		return { &_slotTemplate };
	}
	static Slot _slotTemplate;

	Array<Slot> _slots;

	// size cache
	uint32_t _cacheFrameWidth = {};
	uint32_t _cacheFrameHeight = {};
	Rangef _cacheValueWidth = { 0, 0 };
	Rangef _cacheValueHeight = { 0, 0 };

	void OnReset() override
	{
		UIObject::OnReset();

		_slots.Clear();
	}

	void SlotIterator_Init(UIObjectIteratorData& data) override
	{
		data.data0 = 0;
	}

	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override
	{
		if (data.data0 >= _slots.Size())
			return nullptr;
		return _slots[data.data0++]._obj;
	}

	void RemoveChildImpl(UIObject* ch) override
	{
		for (size_t i = 0; i < _slots.Size(); i++)
		{
			if (_slots[i]._obj == ch)
			{
				_slots.RemoveAt(i);
				break;
			}
		}
	}

	void DetachChildren(bool recursive) override
	{
		for (size_t i = 0; i < _slots.Size(); i++)
		{
			auto* ch = _slots[i]._obj;

			if (recursive)
				ch->DetachChildren(true);

			// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
			if (system)
				ch->_DetachFromTree();

			ch->parent = nullptr;
		}
		_slots.Clear();
	}

	void AppendChild(UIObject* obj) override
	{
		obj->DetachParent();

		obj->parent = this;
		Slot slot = _slotTemplate;
		slot._obj = obj;
		_slots.Append(slot);
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

	UIObject* FindObjectAtPoint(Point2f pos) override
	{
		for (size_t i = _slots.Size(); i > 0; )
		{
			i--;
			if (_slots[i]._obj->Contains(pos))
				if (auto* o = _slots[i]._obj->FindObjectAtPoint(pos))
					return o;
		}
		return nullptr;
	}

	void _AttachToFrameContents(FrameContents* owner) override
	{
		UIObject::_AttachToFrameContents(owner);

		for (auto& slot : _slots)
			slot._obj->_AttachToFrameContents(owner);
	}

	void _DetachFromFrameContents() override
	{
		for (auto& slot : _slots)
			slot._obj->_DetachFromFrameContents();

		UIObject::_DetachFromFrameContents();
	}

	void _DetachFromTree() override
	{
		if (!(flags & UIObject_IsInTree))
			return;

		for (auto& slot : _slots)
			slot._obj->_DetachFromTree();

		UIObject::_DetachFromTree();
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
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

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
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
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
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	StackExpandLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }
};


namespace _ {
struct WrapperLTRLayoutElement_Slot
{
	UIObject* _obj = nullptr;
};
} // _

struct WrapperLTRLayoutElement : LayoutElement<_::WrapperLTRLayoutElement_Slot>
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


enum class Edge : uint8_t
{
	Left,
	Top,
	Right,
	Bottom,
};

namespace _ {
struct EdgeSliceLayoutElement_Slot
{
	UIObject* _obj = nullptr;
	Edge edge = Edge::Top;
};
} // _

struct EdgeSliceLayoutElement : LayoutElement<_::EdgeSliceLayoutElement_Slot>
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


namespace _ {
struct LayerLayoutElement_Slot
{
	UIObject* _obj = nullptr;
};
}

struct LayerLayoutElement : LayoutElement<_::LayerLayoutElement_Slot>
{
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
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
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

struct PointAnchoredPlacement : IPlacement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const override;

	void SetAnchorAndPivot(Point2f p)
	{
		anchor = p;
		pivot = p;
	}

	Point2f anchor = { 0, 0 };
	Point2f pivot = { 0, 0 };
	Point2f bias = { 0, 0 };
};

struct RectAnchoredPlacement : IPlacement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const override;

	UIRect anchor = { 0, 0, 1, 1 };
	UIRect bias = { 0, 0, 0, 0 };
};

} // ui
