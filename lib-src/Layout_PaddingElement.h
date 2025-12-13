
#pragma once
#include "Model/Objects.h"


namespace ui {


struct PaddingElement : UIObjectSingleChild
{
	UIRect padding;

	void OnReset() override;
	Size2f GetReducedContainerSize(Size2f size);
	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
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


} // ui
