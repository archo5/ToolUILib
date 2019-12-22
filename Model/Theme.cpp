
#include "../Render/Render.h"
#include "Theme.h"

namespace ui {

Theme* Theme::current;

static
struct ThemeInit
{
	Theme defaultTheme;
	style::Block dtObject;
	style::Block dtPanel;
	style::Block dtButton;
	style::Block dtCheckbox;
	style::Block dtRadioButton;
	style::Block dtCollapsibleTreeNode;
	style::Block dtTextBoxBase;
	style::Block dtTabGroup;
	style::Block dtTabList;
	style::Block dtTabButton;
	style::Block dtTabPanel;

	ThemeInit()
	{
		CreateObject();
		CreatePanel();
		CreateButton();
		CreateCheckbox();
		CreateRadioButton();
		CreateCollapsibleTreeNode();
		CreateTextBoxBase();
		CreateTabGroup();
		CreateTabList();
		CreateTabButton();
		CreateTabPanel();
		Theme::current = &defaultTheme;
	}
	void CreateObject()
	{
		style::Accessor a(&dtObject);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
		};
		defaultTheme.object = a.block;
	}
	void CreatePanel()
	{
		style::Accessor a(&dtPanel);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetMargin(2);
		a.SetPadding(6);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_Panel, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.panel = a.block;
	}
	void CreateButton()
	{
		style::Accessor a(&dtButton);
		a.SetLayout(style::Layout::InlineBlock);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetPadding(5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDisabled() ? TE_ButtonDisabled :
				info.IsDown() || info.IsChecked() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.button = a.block;
	}
	void CreateCheckbox()
	{
		style::Accessor a(&dtCheckbox);
		a.SetLayout(style::Layout::InlineBlock);
		//a.SetWidth(style::Coord::Percent(12));
		//a.SetHeight(style::Coord::Percent(12));
		a.SetWidth(GetFontHeight() + 5 + 5);
		a.SetHeight(GetFontHeight() + 5 + 5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			float w = std::min(r.GetWidth(), r.GetHeight());
			DrawThemeElement(
				info.IsDisabled() ? TE_CheckBgrDisabled :
				info.IsDown() ? TE_CheckBgrPressed :
				info.IsHovered() ? TE_CheckBgrHover : TE_CheckBgrNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
			if (info.IsChecked())
				DrawThemeElement(info.IsDisabled() ? TE_CheckMarkDisabled : TE_CheckMark, r.x0, r.y0, r.x0 + w, r.y0 + w);
		};
		defaultTheme.checkbox = a.block;
	}
	void CreateRadioButton()
	{
		style::Accessor a(&dtRadioButton);
		a.SetLayout(style::Layout::InlineBlock);
		//a.SetWidth(style::Coord::Percent(12));
		//a.SetHeight(style::Coord::Percent(12));
		//a.SetWidth(GetFontHeight() + 5 + 5);
		//a.SetMargin(5);
		a.SetHeight(GetFontHeight());
		a.SetPadding(5);
		a.SetPaddingLeft(GetFontHeight() + 5 + 5 + 5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			float w = std::min(r.GetWidth(), r.GetHeight());
			DrawThemeElement(
				info.IsDisabled() ? TE_RadioBgrDisabled :
				info.IsDown() ? TE_RadioBgrPressed :
				info.IsHovered() ? TE_RadioBgrHover : TE_RadioBgrNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
			if (info.IsChecked())
				DrawThemeElement(info.IsDisabled() ? TE_RadioMarkDisabled : TE_RadioMark, r.x0, r.y0, r.x0 + w, r.y0 + w);
		};
		defaultTheme.radioButton = a.block;
	}
	void CreateCollapsibleTreeNode()
	{
		style::Accessor a(&dtCollapsibleTreeNode);
		a.SetLayout(style::Layout::Stack);
		//a.SetPadding(1);
		a.SetPaddingLeft(GetFontHeight());
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsHovered() ?
				info.IsChecked() ? TE_TreeTickOpenHover : TE_TreeTickClosedHover :
				info.IsChecked() ? TE_TreeTickOpenNormal : TE_TreeTickClosedNormal, r.x0, r.y0, r.x0 + GetFontHeight(), r.y0 + GetFontHeight());
		};
		defaultTheme.collapsibleTreeNode = a.block;
	}
	void CreateTextBoxBase()
	{
		style::Accessor a(&dtTextBoxBase);
		a.SetLayout(style::Layout::InlineBlock);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetPadding(5);
		a.SetWidth(120);
		a.SetHeight(GetFontHeight() + 5 + 5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.textBoxBase = a.block;
	}
	void CreateTabGroup()
	{
		style::Accessor a(&dtTabGroup);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetMargin(2);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
		};
		defaultTheme.tabGroup = a.block;
	}
	void CreateTabList()
	{
		style::Accessor a(&dtTabList);
		a.SetPadding(0, 4);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetLayout(style::Layout::Stack);
		a.SetStackingDirection(style::StackingDirection::LeftToRight);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
		};
		defaultTheme.tabList = a.block;
	}
	void CreateTabButton()
	{
		style::Accessor a(&dtTabButton);
		a.SetLayout(style::Layout::InlineBlock);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetPadding(5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() || info.IsChecked() ? TE_TabSelected :
				info.IsHovered() ? TE_TabHover : TE_TabNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.tabButton = a.block;
	}
	void CreateTabPanel()
	{
		style::Accessor a(&dtTabPanel);
		a.SetPadding(5);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetWidth(style::Coord(100, style::CoordTypeUnit::Percent));
		a.SetMarginTop(-2);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_TabPanel, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.tabPanel = a.block;
	}
}
init;

} // ui
