
#pragma once
#include "../Core/Array.h"
#include "../Layout_ListLayoutElementBase.h"


namespace ui {

namespace _ {
struct StackLTRLayoutElement_Slot
{
	UIObject* _obj = nullptr;
};
} // _

struct StackLTRLayoutElement : ListLayoutElementBase<_::StackLTRLayoutElement_Slot>
{
	float paddingBetweenElements = 0;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
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

struct StackTopDownLayoutElement : ListLayoutElementBase<_::StackTopDownLayoutElement_Slot>
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
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

struct StackExpandLTRLayoutElement : ListLayoutElementBase<_::StackExpandLTRLayoutElement_Slot>
{
	float paddingBetweenElements = 0;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
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

struct WrapperLTRLayoutElement : ListLayoutElementBase<_::WrapperLTRLayoutElement_Slot>
{
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


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
