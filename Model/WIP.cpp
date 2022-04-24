
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

Rangef PaddingElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = padding.x0 + padding.x1;
	return (_child ? _child->GetFullEstimatedWidth(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

Rangef PaddingElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad = padding.y0 + padding.y1;
	return (_child ? _child->GetFullEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
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


StackLTRLayoutElement::Slot StackLTRLayoutElement::_slotTemplate;

float StackLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float size = 0;
	bool first = true;
	for (auto& slot : _slots)
	{
		if (!first)
			size += paddingBetweenElements;
		else
			first = false;
		size += slot._obj->GetFullEstimatedWidth(containerSize, EstSizeType::Exact).min;
	}
	return size;
}

float StackLTRLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float size = 0;
	for (auto& slot : _slots)
		size = max(size, slot._obj->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min);
	return size;
}

void StackLTRLayoutElement::CalcLayout(const UIRect& inrect, LayoutState& state)
{
	// put items one after another in the indicated direction
	// container size adapts to child elements in stacking direction, and to parent in the other
	float p = inrect.x0;
	for (auto& slot : _slots)
	{
		float w = slot._obj->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Exact).min;
		Rangef hr = slot._obj->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Expanding);
		float h = clamp(inrect.y1 - inrect.y0, hr.min, hr.max);
		slot._obj->PerformLayout({ p, inrect.y0, p + w, inrect.y0 + h }, inrect.GetSize());
		p += w + paddingBetweenElements;
	}
}


StackTopDownLayoutElement::Slot StackTopDownLayoutElement::_slotTemplate;

float StackTopDownLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float size = 0;
	for (auto& slot : _slots)
		size = max(size, slot._obj->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min);
	return size;
}

float StackTopDownLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float size = 0;
	for (auto& slot : _slots)
		size += slot._obj->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min;
	return size;
}

void StackTopDownLayoutElement::CalcLayout(const UIRect& inrect, LayoutState& state)
{
	// put items one after another in the indicated direction
	// container size adapts to child elements in stacking direction, and to parent in the other
	float p = inrect.y0;
	for (auto& slot : _slots)
	{
		Rangef wr = slot._obj->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding);
		float w = clamp(inrect.x1 - inrect.x0, wr.min, wr.max);
		float h = slot._obj->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Exact).min;
		slot._obj->PerformLayout({ inrect.x0, p, inrect.x0 + w, p + h }, inrect.GetSize());
		p += h;
	}
}


StackExpandLTRLayoutElement::Slot StackExpandLTRLayoutElement::_slotTemplate;

float StackExpandLTRLayoutElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (type == EstSizeType::Expanding)
	{
		float size = 0;
		for (auto& slot : _slots)
			size += slot._obj->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
		if (_slots.size() >= 2)
			size += paddingBetweenElements * (_slots.size() - 1);
		return size;
	}
	else
		return containerSize.x;
}

float StackExpandLTRLayoutElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float size = 0;
	for (auto& slot : _slots)
		size = max(size, slot._obj->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min);
	return size;
}

void StackExpandLTRLayoutElement::CalcLayout(const UIRect& inrect, LayoutState& state)
{
	float p = inrect.x0;
	float sum = 0, frsum = 0;
	struct Item
	{
		UIObject* ch;
		float minw;
		float maxw;
		float w;
		float fr;
	};
	std::vector<Item> items;
	std::vector<int> sorted;
	for (auto& slot : _slots)
	{
		auto s = slot._obj->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding);
		items.push_back({ slot._obj, s.min, s.max, s.min, slot.fraction });
		sorted.push_back(sorted.size());
		sum += s.min;
		frsum += slot.fraction;
	}
	if (_slots.size() >= 2)
		sum += paddingBetweenElements * (_slots.size() - 1);
	if (frsum > 0)
	{
		std::sort(sorted.begin(), sorted.end(), [&items](int ia, int ib)
		{
			const auto& a = items[ia];
			const auto& b = items[ib];
			return (a.maxw - a.minw) < (b.maxw - b.minw);
		});
		float leftover = max(inrect.GetWidth() - sum, 0.0f);
		for (auto idx : sorted)
		{
			auto& item = items[idx];
			float mylo = leftover * item.fr / frsum;
			float w = item.minw + mylo;
			if (w > item.maxw)
				w = item.maxw;
			w = roundf(w);
			float actual_lo = w - item.minw;
			leftover -= actual_lo;
			frsum -= item.fr;
			item.w = w;

			if (frsum <= 0)
				break;
		}
	}
	for (const auto& item : items)
	{
		item.ch->PerformLayout({ p, inrect.y0, p + item.w, inrect.y1 }, inrect.GetSize());
		p += item.w + paddingBetweenElements;
	}
}


PlacementLayoutElement::Slot PlacementLayoutElement::_slotTemplate;

Rangef PlacementLayoutElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (slot.measure)
		{
			auto cr = slot._obj->GetFullEstimatedWidth(containerSize, type);
			r = r.Intersect(cr);
		}
	}
	return r;
}

Rangef PlacementLayoutElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	Rangef r = Rangef::AtLeast(0);
	for (auto& slot : _slots)
	{
		if (slot.measure)
		{
			auto cr = slot._obj->GetFullEstimatedHeight(containerSize, type);
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
			slot.placement->OnApplyPlacement(slot._obj, r);
		}
		//contRect = contRect.Include(r);
		r.x0 = roundf(r.x0);
		r.y0 = roundf(r.y0);
		r.x1 = roundf(r.x1);
		r.y1 = roundf(r.y1);
		slot._obj->PerformLayout(r, containerSize);
	}
	finalRectC = finalRectCP = contRect;
}


void TabbedPanelStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	//OnField(oi, "tabHeight", tabHeight);
	OnField(oi, "tabButtonOverlap", tabButtonOverlap);
	OnField(oi, "tabButtonYOffsetInactive", tabButtonYOffsetInactive);
	OnField(oi, "tabInnerButtonMargin", tabInnerButtonMargin);
	OnFieldBorderBox(oi, "tabButtonPadding", tabButtonPadding);
	OnFieldPainter(oi, td, "tabButtonPainter", tabButtonPainter);
	OnFieldFontSettings(oi, td, "tabButtonFont", tabButtonFont);
	OnFieldBorderBox(oi, "tabPanelPadding", tabPanelPadding);
	OnFieldPainter(oi, td, "tabPanelPainter", tabPanelPainter);
	OnField(oi, "tabCloseButtonWidth", tabCloseButtonSize.x);
	OnField(oi, "tabCloseButtonHeight", tabCloseButtonSize.y);
	OnFieldPainter(oi, td, "tabCloseButtonPainter", tabCloseButtonPainter);
}


void TabbedPanel::SetTabBarExtension(UIObject* obj)
{
	if (_tabBarExtension == obj)
		return;

	if (_tabBarExtension)
		_tabBarExtension->DetachParent();

	_tabBarExtension = obj;

	if (obj)
	{
		obj->DetachParent();
		obj->parent = this;
		if (system)
			obj->_AttachToFrameContents(system);
	}
}

void TabbedPanel::AddTextTab(StringView text, uintptr_t uid)
{
	Tab tab;
	tab.text = to_string(text);
	tab.uid = uid;
	_tabs.push_back(tab);
}

void TabbedPanel::AddUITab(UIObject* obj, uintptr_t uid)
{
	Tab tab;
	tab.obj = obj;
	tab.uid = uid;
	_tabs.push_back(tab);

	obj->DetachParent();
	obj->parent = this;
	if (system)
		obj->_AttachToFrameContents(system);
}

uintptr_t TabbedPanel::GetCurrentTabUID(uintptr_t def) const
{
	return _curTabNum < _tabs.size() ? _tabs[_curTabNum].uid : def;
}

bool TabbedPanel::SetActiveTabByUID(uintptr_t uid)
{
	for (auto& tab : _tabs)
	{
		if (tab.uid == uid)
		{
			_curTabNum = &tab - &_tabs.front();
			return true;
		}
	}
	return false;
}

static StaticID<TabbedPanelStyle> sid_tabbed_panel("tabbed_panel");

void TabbedPanel::OnReset()
{
	UIObjectSingleChild::OnReset();

	tabHeight = 22;
	style = *GetCurrentTheme()->GetStruct(sid_tabbed_panel);

	_tabs.clear();
	_tabBarExtension = nullptr;
	//_curTabNum = 0;
	rebuildOnChange = true;
}

void TabbedPanel::OnPaint(const UIPaintContext& ctx)
{
	float x0 = GetContentRect().x0 + 4;
	float y0 = GetContentRect().y0;
	float y1 = y0 + tabHeight;
	auto* tbFont = style.tabButtonFont.GetFont();
	int tbFontSize = style.tabButtonFont.size;
	UIRect btnRect = { x0, y0, x0, y1 };
	for (auto& tab : _tabs)
	{
		size_t id = &tab - &_tabs.front();

		float tw = tab._contentWidth;
		float x1 = x0 + tw + style.tabButtonPadding.x0 + style.tabButtonPadding.x1;

		if (showCloseButton)
		{
			float cx0 = x0 + style.tabButtonPadding.x0 + tw + style.tabInnerButtonMargin;
			float cy0 = y0 + style.tabButtonPadding.y0;
			if (id != _curTabNum)
				cy0 += style.tabButtonYOffsetInactive;
			x1 += style.tabInnerButtonMargin + style.tabCloseButtonSize.x;
			PaintInfo pi(this);
			pi.rect =
			{
				cx0,
				cy0,
				cx0 + style.tabCloseButtonSize.x,
				cy0 + style.tabCloseButtonSize.y
			};
			pi.state = 0;
			pi.checkState = 0;
			if (_tabUI.IsHovered(id | (1 << 31)))
				pi.state |= PS_Hover;
			if (_tabUI.IsPressed(id | (1 << 31)))
				pi.state |= PS_Down;
			if (style.tabCloseButtonPainter)
				style.tabCloseButtonPainter->Paint(pi);
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
		ContentPaintAdvice cpa;
		if (style.tabButtonPainter)
			cpa = style.tabButtonPainter->Paint(pi);

		if (tab.obj)
		{
			tab.obj->Paint(ctx);
		}
		else
		{
			draw::TextLine(tbFont, tbFontSize,
				x0 + style.tabButtonPadding.x0 + cpa.offset.x,
				y0 + style.tabButtonPadding.y0 + tbFontSize + cpa.offset.y,
				tab.text,
				cpa.HasTextColor() ? cpa.GetTextColor() : ctx.textColor);
		}

		x0 = x1;
	}
	btnRect.x1 = x0;

	if (_tabBarExtension)
		_tabBarExtension->Paint(ctx);

	if (style.tabPanelPainter)
	{
		PaintInfo pi(this);
		pi.rect.y0 += tabHeight - style.tabButtonOverlap;
		auto panelRect = pi.rect;
		if (style.tabButtonOverlap > 0 && btnRect.x1 < panelRect.x1)
		{
			draw::PushScissorRect(UIRect{ panelRect.x0, panelRect.y0, btnRect.x0, panelRect.y1 });
			style.tabPanelPainter->Paint(pi);
			draw::PopScissorRect();

			draw::PushScissorRect(UIRect{ btnRect.x0, btnRect.y1, btnRect.x1, panelRect.y1 });
			style.tabPanelPainter->Paint(pi);
			draw::PopScissorRect();

			draw::PushScissorRect(UIRect{ btnRect.x1, panelRect.y0, panelRect.x1, panelRect.y1 });
			style.tabPanelPainter->Paint(pi);
			draw::PopScissorRect();
		}
		else
			style.tabPanelPainter->Paint(pi);
	}

	PaintChildren(ctx, {});
}

void TabbedPanel::OnEvent(Event& e)
{
	_tabUI.InitOnEvent(e);

	float x0 = GetContentRect().x0 + 4;
	float y0 = GetContentRect().y0;
	float y1 = y0 + tabHeight;
	auto* tbFont = style.tabButtonFont.GetFont();
	int tbFontSize = style.tabButtonFont.size;
	for (auto& tab : _tabs)
	{
		float tw = tab._contentWidth;
		float x1 = x0 + tw + style.tabButtonPadding.x0 + style.tabButtonPadding.x1;
		if (showCloseButton)
		{
			x1 += style.tabInnerButtonMargin + style.tabCloseButtonSize.x;
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
			float cx0 = x0 + style.tabButtonPadding.x0 + tw + style.tabInnerButtonMargin;
			float cy0 = y0 + style.tabButtonPadding.y0;
			if (id != _curTabNum)
				cy0 += style.tabButtonYOffsetInactive;
			PaintInfo pi(this);
			UIRect tabCloseButtonRect =
			{
				cx0,
				cy0,
				cx0 + style.tabCloseButtonSize.x,
				cy0 + style.tabCloseButtonSize.y
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
	size.x -= style.tabPanelPadding.x0 + style.tabPanelPadding.x1;
	size.y -= style.tabPanelPadding.y0 + style.tabPanelPadding.y1 + tabHeight - style.tabButtonOverlap;
	return size;
}

Rangef TabbedPanel::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = 
		+ style.tabPanelPadding.x0
		+ style.tabPanelPadding.x1
		+ (showCloseButton ? style.tabInnerButtonMargin + style.tabCloseButtonSize.x : 0);
	return (_child ? _child->GetFullEstimatedWidth(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

Rangef TabbedPanel::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad =
		+ style.tabPanelPadding.y0
		+ style.tabPanelPadding.y1
		+ tabHeight
		- style.tabButtonOverlap;
	return (_child ? _child->GetFullEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

void TabbedPanel::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	auto subr = rect;
	subr.y0 += tabHeight - style.tabButtonOverlap;
	subr = subr.ShrinkBy(style.tabPanelPadding);

	auto* tbFont = style.tabButtonFont.GetFont();
	int tbFontSize = style.tabButtonFont.size;
	auto cr = rect;
	float x0 = cr.x0 + 4;
	float x1 = cr.x1 - 4;
	float y0 = cr.y0;
	float y1 = y0 + tabHeight;
	for (auto& tab : _tabs)
	{
		if (tab.obj)
		{
			tab._contentWidth = tab.obj->GetFullEstimatedWidth(containerSize, EstSizeType::Exact).min;
		}
		else
		{
			tab._contentWidth = GetTextWidth(tbFont, tbFontSize, tab.text);
		}
		if (tab.obj)
		{
			float xbase = x0 + style.tabButtonPadding.x0;
			UIRect trect = { xbase, y0, xbase + tab._contentWidth, y1 };
			tab.obj->PerformLayout(trect, trect.GetSize());
		}

		float tw = tab._contentWidth + style.tabButtonPadding.x0 + style.tabButtonPadding.x1;
		if (showCloseButton)
		{
			tw += style.tabInnerButtonMargin + style.tabCloseButtonSize.x;
		}
		x0 += tw;
	}
	if (_tabBarExtension)
	{
		UIRect xrect = { x0, y0, x1, y1 };
		_tabBarExtension->PerformLayout(xrect, xrect.GetSize());
	}

	if (_child)
	{
		_child->PerformLayout(subr, subr.GetSize());
	}
	finalRectC = finalRectCP = rect;
}

UIObject* TabbedPanel::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	auto id = data.data0++;
	for (auto& tab : _tabs)
		if (tab.obj && id-- == 0)
			return tab.obj;
	if (_tabBarExtension && id-- == 0)
		return _tabBarExtension;
	if (_child && id-- == 0)
		return _child;
	return nullptr;
}

void TabbedPanel::RemoveChildImpl(UIObject* ch)
{
	if (_tabBarExtension == ch)
	{
		_tabBarExtension = nullptr;
		return;
	}

	for (auto& tab : _tabs)
	{
		if (tab.obj == ch)
		{
			tab.obj = nullptr;
			return;
		}
	}

	UIObjectSingleChild::RemoveChildImpl(ch);
}

void TabbedPanel::DetachChildren(bool recursive)
{
	if (_tabBarExtension)
	{
		if (recursive)
			_tabBarExtension->DetachChildren(true);
		if (system)
			_tabBarExtension->_DetachFromTree();
		_tabBarExtension->parent = nullptr;
	}
	for (auto& tab : _tabs)
	{
		if (tab.obj)
		{
			if (recursive)
				tab.obj->DetachChildren(true);
			if (system)
				tab.obj->_DetachFromTree();
			tab.obj->parent = nullptr;
		}
	}

	UIObjectSingleChild::DetachChildren(recursive);
}

UIObject* TabbedPanel::FindLastChildContainingPos(Point2f pos) const
{
	if (_tabBarExtension)
		if (_tabBarExtension->Contains(pos))
			return _tabBarExtension;

	for (auto& tab : _tabs)
		if (tab.obj && tab.obj->Contains(pos))
			return tab.obj;

	return UIObjectSingleChild::FindLastChildContainingPos(pos);
}

void TabbedPanel::_AttachToFrameContents(FrameContents* owner)
{
	UIObjectSingleChild::_AttachToFrameContents(owner);

	for (auto& tab : _tabs)
		if (tab.obj)
			tab.obj->_AttachToFrameContents(owner);

	if (_tabBarExtension)
		_tabBarExtension->_AttachToFrameContents(owner);
}

void TabbedPanel::_DetachFromFrameContents()
{
	if (_tabBarExtension)
		_tabBarExtension->_DetachFromFrameContents();

	for (auto& tab : _tabs)
		if (tab.obj)
			tab.obj->_DetachFromFrameContents();

	UIObjectSingleChild::_DetachFromFrameContents();
}

void TabbedPanel::_DetachFromTree()
{
	if (!(flags & UIObject_IsInTree))
		return;

	if (_tabBarExtension)
		_tabBarExtension->_DetachFromTree();

	for (auto& tab : _tabs)
		if (tab.obj)
			tab.obj->_DetachFromTree();

	UIObjectSingleChild::_DetachFromTree();
}

} // ui
