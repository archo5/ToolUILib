
#pragma once
#include "../Core/Array.h"
#include "../Layout_ListLayoutElementBase.h"
#include "../Layout_Stack.h" // legacy ref, TODO remove


namespace ui {

namespace _ {
struct EdgeSliceLayoutElement_Slot
{
	UIObject* _obj = nullptr;
	Edge edge = Edge::Top;
};
} // _

struct EdgeSliceLayoutElement : ListLayoutElementBase<_::EdgeSliceLayoutElement_Slot>
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


namespace _ {
struct LayerLayoutElement_Slot
{
	UIObject* _obj = nullptr;
};
}

struct LayerLayoutElement : ListLayoutElementBase<_::LayerLayoutElement_Slot>
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
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

struct PlacementLayoutElement : ListLayoutElementBase<_::PlacementLayoutElement_Slot>
{
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
