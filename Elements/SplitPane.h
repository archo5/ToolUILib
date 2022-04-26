
#pragma once
#include "../Model/Objects.h"


namespace ui {

struct SeparatorLineStyle
{
	static constexpr const char* NAME = "SeparatorLineStyle";

	float size = 8;
	PainterHandle backgroundPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct SplitPane : UIElement
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
	void OnLayout(const UIRect& rect) override;
	void SlotIterator_Init(UIObjectIteratorData& data) override;
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	void CustomAppendChild(UIObject* obj) override;
	UIObject* FindLastChildContainingPos(Point2f pos) const override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	SplitPane* SetSplits(std::initializer_list<float> splits, bool firstTimeOnly = true);
	SplitPane* SetDirection(bool vertical);

	SeparatorLineStyle vertSepStyle; // for horizontal splitting
	SeparatorLineStyle horSepStyle; // for vertical splitting
	std::vector<UIObject*> _children;
	std::vector<float> _splits;
	bool _splitsSet = false;
	bool _verticalSplit = false;
	SubUI<uint16_t> _splitUI;
	float _dragOff = 0;
};

} // ui
