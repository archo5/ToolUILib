
#pragma once
#include "Layout_ListLayoutElementBase.h"


namespace ui {


struct StackLTRLayoutElement : ListLayoutElementBase<ListLayoutSlotBase>
{
	float paddingBetweenElements = 0;
	StackLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


struct StackTopDownLayoutElement : ListLayoutElementBase<ListLayoutSlotBase>
{
	float paddingBetweenElements = 0;
	StackTopDownLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


namespace _ {
struct StackExpandLTRLayoutElement_Slot : ListLayoutSlotBase
{
	using T = StackExpandLTRLayoutElement_Slot;

	float fraction = 1;

	T& DisableScaling() { fraction = 0; return *this; }
	T& SetMaximumScaling() { fraction = FLT_MAX; return *this; }
	T& SetScaleWeight(float f) { fraction = f; return *this; }
};
} // _

struct StackExpandLTRLayoutElement : ListLayoutElementBase<_::StackExpandLTRLayoutElement_Slot>
{
	static Slot _slotTemplate;
	static TempEditable<Slot> GetSlotTemplate() { return { &_slotTemplate }; }
	Slot _CopyAndResetSlotTemplate() override { Slot ret = _slotTemplate; _slotTemplate = {}; return ret; }

	float paddingBetweenElements = 0;
	StackExpandLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


// TODO is this still valuable?
struct WrapperLTRLayoutElement : ListLayoutElementBase<ListLayoutSlotBase>
{
	float paddingBetweenElements = 0;
	WrapperLTRLayoutElement& SetPaddingBetweenElements(float p) { paddingBetweenElements = p; return *this; }

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


} // ui

