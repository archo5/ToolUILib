
#include "Layout_PaddingElement.h"


namespace ui {


void PaddingElement::OnReset()
{
	UIObjectSingleChild::OnReset();
	padding = {};
}

Size2f PaddingElement::GetReducedContainerSize(Size2f size)
{
	size.x -= padding.x0 + padding.x1;
	size.y -= padding.y0 + padding.y1;
	return size;
}

EstSizeRange PaddingElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = padding.x0 + padding.x1;
	return (_child && _child->_NeedsLayout() ? _child->CalcEstimatedWidth(GetReducedContainerSize(containerSize), type) : EstSizeRange()).Add(pad);
}

EstSizeRange PaddingElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad = padding.y0 + padding.y1;
	return (_child && _child->_NeedsLayout() ? _child->CalcEstimatedHeight(GetReducedContainerSize(containerSize), type) : EstSizeRange()).Add(pad);
}

void PaddingElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto padsub = rect.ShrinkBy(padding);
	if (_child && _child->_NeedsLayout())
	{
		_child->PerformLayout(padsub, info);
		ApplyLayoutInfo(_child->GetFinalRect().ExtendBy(padding), rect, info);
	}
	else
	{
		_finalRect = rect;
	}
}


} // ui
