
#pragma once
#include "../Core/Array.h"
#include "../Layout_ListLayoutElementBase.h"
#include "../Layout_Stack.h" // legacy ref, TODO remove


namespace ui {

namespace _ {
struct EdgeSliceLayoutElement_Slot : ListLayoutSlotBase
{
	Edge edge = Edge::Top;
};
} // _

struct EdgeSliceLayoutElement : ListLayoutElementBase<_::EdgeSliceLayoutElement_Slot>
{
	static Slot _slotTemplate;
	static TempEditable<Slot> GetSlotTemplate() { return { &_slotTemplate }; }
	Slot _CopyAndResetSlotTemplate() override { Slot ret = _slotTemplate; _slotTemplate = {}; return ret; }

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


struct LayerLayoutElement : ListLayoutElementBase<ListLayoutSlotBase>
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


namespace _ {
struct PlacementLayoutElement_Slot : ListLayoutSlotBase
{
	const IPlacement* placement = nullptr;
	bool measure = true;
};
} // _

struct PlacementLayoutElement : ListLayoutElementBase<_::PlacementLayoutElement_Slot>
{
	static Slot _slotTemplate;
	static TempEditable<Slot> GetSlotTemplate() { return { &_slotTemplate }; }
	Slot _CopyAndResetSlotTemplate() override { Slot ret = _slotTemplate; _slotTemplate = {}; return ret; }

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
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
