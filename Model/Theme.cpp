
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
	style::Block dtListBox;
	style::Block dtProgressBarBase;
	style::Block dtProgressBarCompletion;
	style::Block dtSliderHBase;
	style::Block dtSliderHTrack;
	style::Block dtSliderHTrackFill;
	style::Block dtSliderHThumb;
	style::Block dtScrollVTrack;
	style::Block dtScrollVThumb;
	style::Block dtTabGroup;
	style::Block dtTabList;
	style::Block dtTabButton;
	style::Block dtTabPanel;
	style::Block dtTableBase;
	style::Block dtTableCell;
	style::Block dtTableRowHeader;
	style::Block dtTableColHeader;
	style::Block dtImage;

	ThemeInit()
	{
		CreateObject();
		CreatePanel();
		CreateButton();
		CreateCheckbox();
		CreateRadioButton();
		CreateCollapsibleTreeNode();
		CreateTextBoxBase();
		CreateListBox();
		CreateProgressBarBase();
		CreateProgressBarCompletion();
		CreateSliderHBase();
		CreateSliderHTrack();
		CreateSliderHTrackFill();
		CreateSliderHThumb();
		CreateScrollVTrack();
		CreateScrollVThumb();
		CreateTabGroup();
		CreateTabList();
		CreateTabButton();
		CreateTabPanel();
		CreateTableBase();
		CreateTableCell();
		CreateTableRowHeader();
		CreateTableColHeader();
		CreateImage();
		Theme::current = &defaultTheme;
	}
	void PreventHeapDelete(style::Accessor& a)
	{
		a.block->_refCount++;
	}
	void CreateObject()
	{
		style::Accessor a(&dtObject);
		PreventHeapDelete(a);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
		};
		defaultTheme.object = a.block;
	}
	void CreatePanel()
	{
		style::Accessor a(&dtPanel);
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
	void CreateListBox()
	{
		style::Accessor a(&dtListBox);
		PreventHeapDelete(a);
		a.SetLayout(style::Layout::Stack);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetPadding(5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.listBox = a.block;
	}
	void CreateProgressBarBase()
	{
		style::Accessor a(&dtProgressBarBase);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetWidth(style::Coord::Percent(100));
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.progressBarBase = a.block;
	}
	void CreateProgressBarCompletion()
	{
		style::Accessor a(&dtProgressBarCompletion);
		PreventHeapDelete(a);
		a.SetLayout(style::Layout::InlineBlock);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetMargin(2);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.progressBarCompletion = a.block;
	}
	void CreateSliderHBase()
	{
		style::Accessor a(&dtSliderHBase);
		PreventHeapDelete(a);
		a.SetPaddingTop(20);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetWidth(style::Coord::Percent(100));
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
		};
		defaultTheme.sliderHBase = a.block;
	}
	void CreateSliderHTrack()
	{
		style::Accessor a(&dtSliderHTrack);
		PreventHeapDelete(a);
		a.SetMargin(5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.sliderHTrack = a.block;
	}
	void CreateSliderHTrackFill()
	{
		style::Accessor a(&dtSliderHTrackFill);
		PreventHeapDelete(a);
		a.SetMargin(5);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.sliderHTrackFill = a.block;
	}
	void CreateSliderHThumb()
	{
		style::Accessor a(&dtSliderHThumb);
		PreventHeapDelete(a);
		a.SetPadding(4, 4);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover :
				info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal,
				r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.sliderHThumb = a.block;
	}
	void CreateScrollVTrack()
	{
		style::Accessor a(&dtScrollVTrack);
		PreventHeapDelete(a);
		a.SetWidth(20);
		a.SetPadding(2);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.scrollVTrack = a.block;
	}
	void CreateScrollVThumb()
	{
		style::Accessor a(&dtScrollVThumb);
		PreventHeapDelete(a);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover :
				info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal,
				r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.scrollVThumb = a.block;
	}
	void CreateTabGroup()
	{
		style::Accessor a(&dtTabGroup);
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
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
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetWidth(style::Coord::Percent(100));
		a.SetMarginTop(-2);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_TabPanel, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.tabPanel = a.block;
	}
	void CreateTableBase()
	{
		style::Accessor a(&dtTableBase);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetWidth(style::Coord::Percent(100));
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		};
		defaultTheme.tableBase = a.block;
	}
	void CreateTableCell()
	{
		style::Accessor a(&dtTableCell);
		PreventHeapDelete(a);
		a.SetPadding(1);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			GL::BatchRenderer br;
			GL::SetTexture(0);
			br.Begin();
			br.SetColor(0.05f, 0.05f, 0.05f);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.SetColor(0.15f, 0.15f, 0.15f);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y0 + 1, 0, 0, 1, 1);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x0 + 1, info.rect.y1, 0, 0, 1, 1);
			br.Quad(info.rect.x0, info.rect.y1 - 1, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.Quad(info.rect.x1 - 1, info.rect.y0, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.End();
		};
		defaultTheme.tableCell = a.block;
	}
	void CreateTableRowHeader()
	{
		style::Accessor a(&dtTableRowHeader);
		PreventHeapDelete(a);
		a.SetPadding(1);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			GL::BatchRenderer br;
			GL::SetTexture(0);
			br.Begin();
			br.SetColor(0.1f, 0.1f, 0.1f);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.SetColor(0.2f, 0.2f, 0.2f);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y0 + 1, 0, 0, 1, 1);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x0 + 1, info.rect.y1, 0, 0, 1, 1);
			br.Quad(info.rect.x0, info.rect.y1 - 1, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.Quad(info.rect.x1 - 1, info.rect.y0, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.End();
		};
		defaultTheme.tableRowHeader = a.block;
	}
	void CreateTableColHeader()
	{
		style::Accessor a(&dtTableColHeader);
		PreventHeapDelete(a);
		a.SetPadding(1);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
			GL::BatchRenderer br;
			GL::SetTexture(0);
			br.Begin();
			br.SetColor(0.1f, 0.1f, 0.1f);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.SetColor(0.2f, 0.2f, 0.2f);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y0 + 1, 0, 0, 1, 1);
			br.Quad(info.rect.x0, info.rect.y0, info.rect.x0 + 1, info.rect.y1, 0, 0, 1, 1);
			br.Quad(info.rect.x0, info.rect.y1 - 1, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.Quad(info.rect.x1 - 1, info.rect.y0, info.rect.x1, info.rect.y1, 0, 0, 1, 1);
			br.End();
		};
		defaultTheme.tableColHeader = a.block;
	}
	void CreateImage()
	{
		style::Accessor a(&dtImage);
		PreventHeapDelete(a);
		a.SetLayout(style::Layout::InlineBlock);
		a.MutablePaintFunc() = [](const style::PaintInfo& info)
		{
		};
		defaultTheme.image = a.block;
	}
}
init;

} // ui
