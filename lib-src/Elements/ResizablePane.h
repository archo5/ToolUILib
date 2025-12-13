
#pragma once

#include "../Model/Objects.h"
#include "SeparatorLineStyle.h"


namespace ui {

struct ResizablePane : UIObjectSingleChild
{
	SeparatorLineStyle vertSepStyle; // for horizontal resizing
	SeparatorLineStyle horSepStyle; // for vertical resizing
	Edge resizableEdge = Edge::Left;
	bool isRelSplit = false;
	SubUI<u8> _splitUI;
	float splitPos = 100;
	float edgeQ = 0; // 0: splitPos defines the inside, 1: outside
	float _dragOCP = 0;
	float _dragOSP = 0;
	Size2f _contSize;

	ResizablePane& SetResizableEdge(Edge e) { resizableEdge = e; return *this; }
	ResizablePane& SetEdgePos(bool rel, float pos, float eq = 0)
	{
		isRelSplit = rel;
		splitPos = pos;
		edgeQ = eq;
		return *this;
	}
	ResizablePane& Init(Edge e, bool rel, float pos, float eq = 0)
	{
		resizableEdge = e;
		isRelSplit = rel;
		splitPos = pos;
		edgeQ = eq;
		return *this;
	}
	ResizablePane& InitEditable(Edge e, bool rel, float& pos, float eq = 0)
	{
		Init(e, rel, pos, eq);
		HandleEvent(EventType::Resize) = [this, &pos](Event&)
		{
			pos = splitPos;
		};
		return *this;
	}

	AABB2f GetSplitRect();

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};

} // ui
