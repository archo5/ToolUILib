
#include "TabbedPanel.h"

#include "../Model/Theme.h"
#include "../Model/System.h" // only for EventSystem


namespace ui {

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
	float x0 = GetFinalRect().x0 + 4;
	float y0 = GetFinalRect().y0;
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

	ContentPaintAdvice cpa;
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
			cpa = style.tabPanelPainter->Paint(pi);
			draw::PopScissorRect();

			draw::PushScissorRect(UIRect{ btnRect.x1, panelRect.y0, panelRect.x1, panelRect.y1 });
			style.tabPanelPainter->Paint(pi);
			draw::PopScissorRect();
		}
		else
			cpa = style.tabPanelPainter->Paint(pi);
	}

	if (_child)
		_child->Paint(ctx.WithAdvice(cpa));
}

void TabbedPanel::OnEvent(Event& e)
{
	_tabUI.InitOnEvent(e);

	float x0 = GetFinalRect().x0 + 4;
	float y0 = GetFinalRect().y0;
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

Rangef TabbedPanel::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = 
		+ style.tabPanelPadding.x0
		+ style.tabPanelPadding.x1
		+ (showCloseButton ? style.tabInnerButtonMargin + style.tabCloseButtonSize.x : 0);
	return (_child ? _child->CalcEstimatedWidth(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

Rangef TabbedPanel::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad =
		+ style.tabPanelPadding.y0
		+ style.tabPanelPadding.y1
		+ tabHeight
		- style.tabButtonOverlap;
	return (_child ? _child->CalcEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

void TabbedPanel::OnLayout(const UIRect& rect)
{
	auto btnRectSize = rect.GetSize(); // TODO use only the part that will be used for buttons?
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
			tab._contentWidth = tab.obj->CalcEstimatedWidth(btnRectSize, EstSizeType::Exact).min;
		}
		else
		{
			tab._contentWidth = GetTextWidth(tbFont, tbFontSize, tab.text);
		}
		if (tab.obj)
		{
			float xbase = x0 + style.tabButtonPadding.x0;
			UIRect trect = { xbase, y0, xbase + tab._contentWidth, y1 };
			tab.obj->PerformLayout(trect);
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
		_tabBarExtension->PerformLayout(xrect);
	}

	if (_child)
	{
		_child->PerformLayout(subr);
	}
	_finalRect = rect;
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
