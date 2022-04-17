
#include "WIP.h"
#include "Theme.h"
#include "System.h"


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

Rangef PaddingElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	float pad = padding.x0 + padding.x1;
	return (_child ? _child->GetFullEstimatedWidth(GetReducedContainerSize(containerSize), type, forParentLayout) : Rangef::AtLeast(0)).Add(pad);
}

Rangef PaddingElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	float pad = padding.y0 + padding.y1;
	return (_child ? _child->GetFullEstimatedHeight(GetReducedContainerSize(containerSize), type, forParentLayout) : Rangef::AtLeast(0)).Add(pad);
}

void PaddingElement::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	auto padsub = rect.ShrinkBy(padding);
	if (_child)
	{
		_child->PerformLayout(padsub, GetReducedContainerSize(containerSize));
		finalRectC = _child->finalRectCP;
		finalRectCP = finalRectC.ExtendBy(padding);
	}
	else
	{
		finalRectCP = rect;
		finalRectC = padsub;
	}
}


StackExpandLTRLayoutElement::Slot StackExpandLTRLayoutElement::_slotTemplate;


PlacementLayoutElement::Slot PlacementLayoutElement::_slotTemplate;

Rangef PlacementLayoutElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (slot.measure)
		{
			auto cr = slot._element->GetFullEstimatedWidth(containerSize, type, forParentLayout);
			r = r.Intersect(cr);
		}
	}
	return r;
}

Rangef PlacementLayoutElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (slot.measure)
		{
			auto cr = slot._element->GetFullEstimatedHeight(containerSize, type, forParentLayout);
			r = r.Intersect(cr);
		}
	}
	return r;
}

void PlacementLayoutElement::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	UIRect contRect = rect;
	for (auto& slot : _slots)
	{
		UIRect r = rect;
		if (slot.placement)
		{
			if (slot.placement->fullScreenRelative)
				r = { 0, 0, system->eventSystem.width, system->eventSystem.height };
			slot.placement->OnApplyPlacement(slot._element, r);
		}
		//contRect = contRect.Include(r);
		slot._element->PerformLayout(r, containerSize);
	}
	finalRectC = finalRectCP = contRect;
}

void PlacementLayoutElement::OnReset()
{
	UIElement::OnReset();

	_slots.clear();
}

void PlacementLayoutElement::SlotIterator_Init(UIObjectIteratorData& data)
{
	data.data0 = 0;
}

UIObject* PlacementLayoutElement::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	if (data.data0 >= _slots.size())
		return nullptr;
	return _slots[data.data0++]._element;
}

void PlacementLayoutElement::RemoveChildImpl(UIObject* ch)
{
	for (size_t i = 0; i < _slots.size(); i++)
	{
		if (_slots[i]._element == ch)
		{
			_slots.erase(_slots.begin() + i);
			break;
		}
	}
}

void PlacementLayoutElement::DetachChildren(bool recursive)
{
	for (size_t i = 0; i < _slots.size(); i++)
	{
		auto* ch = _slots[i]._element;

		if (recursive)
			ch->DetachChildren(true);

		// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
		if (system)
			ch->_DetachFromTree();

		ch->parent = nullptr;
	}
	_slots.clear();
}

void PlacementLayoutElement::CustomAppendChild(UIObject* obj)
{
	obj->DetachParent();

	obj->parent = this;
	Slot slot = _slotTemplate;
	slot._element = obj;
	_slots.push_back(slot);

	if (system)
		obj->_AttachToFrameContents(system);
}

void PlacementLayoutElement::PaintChildrenImpl(const UIPaintContext& ctx)
{
	for (auto& slot : _slots)
		slot._element->Paint(ctx);
}

UIObject* PlacementLayoutElement::FindLastChildContainingPos(Point2f pos) const
{
	for (size_t i = _slots.size(); i > 0; )
	{
		i--;
		if (_slots[i]._element->Contains(pos))
			return _slots[i]._element;
	}
	return nullptr;
}

void PlacementLayoutElement::_AttachToFrameContents(FrameContents* owner)
{
	UIElement::_AttachToFrameContents(owner);

	for (auto& slot : _slots)
		slot._element->_AttachToFrameContents(owner);
}

void PlacementLayoutElement::_DetachFromFrameContents()
{
	for (auto& slot : _slots)
		slot._element->_DetachFromFrameContents();

	UIElement::_DetachFromFrameContents();
}


static StaticID_Style sid_tab_panel("tab_panel");
static StaticID_Style sid_tab_button("tab_button");
static StaticID_Style sid_tab_close_button("tab_close_button");

void TabbedPanel::OnReset()
{
	UIObjectSingleChild::OnReset();

	_tabs.clear();
	//_curTabNum = 0;
	panelStyle = GetCurrentTheme()->GetStyle(sid_tab_panel);
	tabButtonStyle = GetCurrentTheme()->GetStyle(sid_tab_button);
	tabCloseButtonStyle = GetCurrentTheme()->GetStyle(sid_tab_close_button);
	tabHeight = 22;
	tabButtonOverlap = 2;
	rebuildOnChange = true;
}

void TabbedPanel::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	float x0 = GetContentRect().x0 + 4;
	float y0 = GetContentRect().y0;
	float y1 = y0 + tabHeight;
	auto* tbFont = tabButtonStyle->GetFont();
	int tbFontSize = tabButtonStyle->font_size;
	UIRect btnRect = { x0, y0, x0, y1 };
	for (auto& tab : _tabs)
	{
		size_t id = &tab - &_tabs.front();

		float tw = GetTextWidth(tbFont, tbFontSize, tab.text);
		float x1 = x0 + tw + tabButtonStyle->padding_left + tabButtonStyle->padding_right;

		if (showCloseButton)
		{
			float cx0 = x0 + tabButtonStyle->padding_left + tw + tabInnerButtonMargin;
			float cy0 = y0 + tabButtonStyle->padding_top;
			if (id != _curTabNum)
				cy0 += tabButtonYOffsetInactive;
			x1 += tabInnerButtonMargin + tabCloseButtonStyle->width.value;
			PaintInfo pi(this);
			pi.rect =
			{
				cx0,
				cy0,
				cx0 + tabCloseButtonStyle->width.value,
				cy0 + tabCloseButtonStyle->height.value
			};
			pi.state = 0;
			pi.checkState = 0;
			if (_tabUI.IsHovered(id | (1 << 31)))
				pi.state |= PS_Hover;
			if (_tabUI.IsPressed(id | (1 << 31)))
				pi.state |= PS_Down;
			tabCloseButtonStyle->background_painter->Paint(pi);
		}

		UIRect tabButtonRect = { x0, y0, x1, y1 };

		PaintInfo pi(this);
		pi.rect = tabButtonRect;
		pi.state = 0;
		if (_tabUI.IsHovered(id))
			pi.state |= PS_Hover;
		if (_tabUI.IsPressed(id))
			pi.state |= PS_Down;
		if (id == _curTabNum)
			pi.SetChecked(true);
		auto cpa = tabButtonStyle->background_painter->Paint(pi);

		draw::TextLine(tbFont, tbFontSize,
			x0 + tabButtonStyle->padding_left + cpa.offset.x,
			y0 + tabButtonStyle->padding_top + tbFontSize + cpa.offset.y,
			tab.text,
			cpa.HasTextColor() ? cpa.GetTextColor() : tabButtonStyle->text_color);

		x0 = x1;
	}
	btnRect.x1 = x0;

	PaintInfo pi(this);
	pi.rect.y0 += tabHeight - tabButtonOverlap;
	auto panelRect = pi.rect;
	if (tabButtonOverlap > 0 && btnRect.x1 < panelRect.x1)
	{
		draw::PushScissorRect(UIRect{ panelRect.x0, panelRect.y0, btnRect.x0, panelRect.y1 });
		panelStyle->background_painter->Paint(pi);
		draw::PopScissorRect();

		draw::PushScissorRect(UIRect{ btnRect.x0, btnRect.y1, btnRect.x1, panelRect.y1 });
		panelStyle->background_painter->Paint(pi);
		draw::PopScissorRect();

		draw::PushScissorRect(UIRect{ btnRect.x1, panelRect.y0, panelRect.x1, panelRect.y1 });
		panelStyle->background_painter->Paint(pi);
		draw::PopScissorRect();
	}
	else
		panelStyle->background_painter->Paint(pi);

	ph.PaintChildren(this, ctx);
}

void TabbedPanel::OnEvent(Event& e)
{
	_tabUI.InitOnEvent(e);

	float x0 = GetContentRect().x0 + 4;
	float y0 = GetContentRect().y0;
	float y1 = y0 + tabHeight;
	auto* tbFont = tabButtonStyle->GetFont();
	int tbFontSize = tabButtonStyle->font_size;
	for (auto& tab : _tabs)
	{
		float tw = GetTextWidth(tbFont, tbFontSize, tab.text);
		float x1 = x0 + tw + tabButtonStyle->padding_left + tabButtonStyle->padding_right;
		if (showCloseButton)
		{
			x1 += tabInnerButtonMargin + tabCloseButtonStyle->width.value;
		}
		UIRect tabButtonRect = { x0, y0, x1, y1 };

		size_t id = &tab - &_tabs.front();
		bool wasPressed = _tabUI.IsPressed(id);
		if (_tabUI.ButtonOnEvent(id, tabButtonRect, e) || (_tabUI.IsPressed(id) && !wasPressed))
		{
			_curTabNum = id;
			e.context->OnChange(this);
			e.context->OnCommit(this);
			if (rebuildOnChange)
				RebuildContainer();
		}

		if (showCloseButton)
		{
			float cx0 = x0 + tabButtonStyle->padding_left + tw + tabInnerButtonMargin;
			float cy0 = y0 + tabButtonStyle->padding_top;
			if (id != _curTabNum)
				cy0 += tabButtonYOffsetInactive;
			PaintInfo pi(this);
			UIRect tabCloseButtonRect =
			{
				cx0,
				cy0,
				cx0 + tabCloseButtonStyle->width.value,
				cy0 + tabCloseButtonStyle->height.value
			};
			if (_tabUI.ButtonOnEvent(id | (1 << 31), tabCloseButtonRect, e))
			{
				if (onClose)
					onClose(id, tab.uid);
			}
		}

		x0 = x1;
	}

	_tabUI.FinalizeOnEvent(e);
}

Size2f TabbedPanel::GetReducedContainerSize(Size2f size)
{
	size.x -= panelStyle->padding_left + panelStyle->padding_right;
	size.y -= panelStyle->padding_top + panelStyle->padding_bottom + tabHeight - tabButtonOverlap;
	return size;
}

Rangef TabbedPanel::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	float pad = 
		+ panelStyle->padding_left
		+ panelStyle->padding_right
		+ (showCloseButton ? tabInnerButtonMargin + tabCloseButtonStyle->width.value : 0);
	return (_child ? _child->GetFullEstimatedWidth(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

Rangef TabbedPanel::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	float pad =
		+ panelStyle->padding_top
		+ panelStyle->padding_bottom
		+ tabHeight
		- tabButtonOverlap;
	return (_child ? _child->GetFullEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

void TabbedPanel::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	auto subr = rect;
	subr.y0 += tabHeight - tabButtonOverlap;
	subr = subr.ShrinkBy(panelStyle->GetPaddingRect());
	if (_child)
	{
		_child->PerformLayout(subr, subr.GetSize());
	}
	finalRectC = finalRectCP = rect;
}

} // ui
