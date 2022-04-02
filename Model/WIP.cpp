
#include "WIP.h"
#include "Theme.h"
#include "System.h"


namespace ui {

StackExpandLTRLayoutElement::Slot StackExpandLTRLayoutElement::_slotTemplate;


static StaticID_Style sid_tab_panel("tab_panel");
static StaticID_Style sid_tab_button("tab_button");

void TabbedPanel::OnReset()
{
	StackTopDownLayoutElement::OnReset();

	// TODO fix SubUI to work without the button flag?
	flags |= UIObject_DB_CaptureMouseOnLeftClick | UIObject_DB_FocusOnLeftClick | UIObject_DB_Button;

	_tabs.clear();
	//_curTabNum = 0;
	panelStyle = GetCurrentTheme()->GetStyle(sid_tab_panel);
	tabButtonStyle = GetCurrentTheme()->GetStyle(sid_tab_button);
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
		float tw = GetTextWidth(tbFont, tbFontSize, tab.text);
		float x1 = x0 + tw + tabButtonStyle->padding_left + tabButtonStyle->padding_right;
		UIRect tabButtonRect = { x0, y0, x1, y1 };

		size_t id = &tab - &_tabs.front();
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

		x0 = x1;
	}

	_tabUI.FinalizeOnEvent(e);
}

} // ui
