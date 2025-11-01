
#pragma once

#include "../Model/Objects.h"
#include "SeparatorLineStyle.h"


namespace ui {

struct SplitPane : UIObject
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	void SlotIterator_Init(UIObjectIteratorData& data) override;
	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	void AppendChild(UIObject* obj) override;
	UIObject* FindObjectAtPoint(Point2f pos) override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;

	SplitPane& SetDirection(Direction d);
	UI_FORCEINLINE Direction GetDirection() const { return _verticalSplit ? Direction::Vertical : Direction::Horizontal; }

	SplitPane& SetSplitPos(int which, float pos);
	float GetSplitPos(int which) const;
	UI_FORCEINLINE SplitPane& SetSplits(std::initializer_list<float> splits) { return SetSplits(splits.begin(), splits.size()); }
	SplitPane& SetSplits(const float* splits, size_t numSplits);

	SplitPane& Init(Direction d, float* splits, size_t numSplits);
	template <size_t N>
	UI_FORCEINLINE SplitPane& Init(Direction d, float(&splits)[N]) { return Init(d, splits, N); }

	SeparatorLineStyle vertSepStyle; // for horizontal splitting
	SeparatorLineStyle horSepStyle; // for vertical splitting
	Array<UIObject*> _children;
	Array<float> _splits;
	bool _splitsSet = false;
	bool _verticalSplit = false;
	SubUI<uint16_t> _splitUI;
	float _dragOff = 0;
};

} // ui
