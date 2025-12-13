
#pragma once
#include "Layout_ListLayoutElementBase.h"


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


} // ui

