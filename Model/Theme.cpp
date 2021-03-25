
#include "../Render/Render.h"
#include "Theme.h"

namespace ui {

Theme* Theme::current;

static
struct DefaultTheme : Theme
{
	StyleBlock dtObject;
	StyleBlock dtText;
	StyleBlock dtProperty;
	StyleBlock dtPropLabel;
	StyleBlock dtPanel;
	StyleBlock dtHeader;
	StyleBlock dtButton;
	StyleBlock dtCheckbox;
	StyleBlock dtRadioButton;
	StyleBlock dtSelectable;
	StyleBlock dtCollapsibleTreeNode;
	StyleBlock dtTextBoxBase;
	StyleBlock dtListBox;
	StyleBlock dtProgressBarBase;
	StyleBlock dtProgressBarCompletion;
	StyleBlock dtSliderHBase;
	StyleBlock dtSliderHTrack;
	StyleBlock dtSliderHTrackFill;
	StyleBlock dtSliderHThumb;
	StyleBlock dtScrollVTrack;
	StyleBlock dtScrollVThumb;
	StyleBlock dtTabGroup;
	StyleBlock dtTabList;
	StyleBlock dtTabButton;
	StyleBlock dtTabPanel;
	StyleBlock dtTableBase;
	StyleBlock dtTableCell;
	StyleBlock dtTableRowHeader;
	StyleBlock dtTableColHeader;
	StyleBlock dtColorBlock;
	StyleBlock dtColorInspectBlock;
	StyleBlock dtImage;
	StyleBlock dtSelectorContainer;
	StyleBlock dtSelector;

	DefaultTheme()
	{
		CreateObject();
		CreateText();
		CreateProperty();
		CreatePropLabel();
		CreatePanel();
		CreateHeader();
		CreateButton();
		CreateCheckbox();
		CreateRadioButton();
		CreateSelectable();
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
		CreateColorBlock();
		CreateColorInspectBlock();
		CreateImage();
		CreateSelectorContainer();
		CreateSelector();
#define defaultTheme (*this) // TODO
		Theme::current = &defaultTheme;
	}
	void PreventHeapDelete(StyleAccessor& a)
	{
		a.block->_refCount++;
	}
	void CreateObject()
	{
		StyleAccessor a(&dtObject);
		PreventHeapDelete(a);
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.object = a.block;
	}
	void CreateText()
	{
		StyleAccessor a(&dtText);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetBoxSizing(BoxSizing::ContentBox);
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.text = a.block;
	}
	void CreateProperty()
	{
		StyleAccessor a(&dtProperty);
		PreventHeapDelete(a);
		a.SetLayout(layouts::StackExpand());
		a.SetStackingDirection(StackingDirection::LeftToRight);
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.property = a.block;
	}
	void CreatePropLabel()
	{
		StyleAccessor a(&dtPropLabel);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.propLabel = a.block;
	}
	void CreatePanel()
	{
		StyleAccessor a(&dtPanel);
		PreventHeapDelete(a);
		a.SetMargin(2);
		a.SetPadding(6);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_Panel, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.panel = a.block;
	}
	void CreateHeader()
	{
		StyleAccessor a(&dtHeader);
		PreventHeapDelete(a);
		a.SetFontWeight(FontWeight::Bold);
		a.SetPadding(5);
		a.SetPaintFunc([](const PaintInfo& info) {});
		defaultTheme.header = a.block;
	}
	void CreateButton()
	{
		StyleAccessor a(&dtButton);
		PreventHeapDelete(a);
		//a.SetLayout(style::Layout::InlineBlock);
		a.SetLayout(layouts::Stack());
		a.SetStackingDirection(StackingDirection::LeftToRight);
		a.SetHAlign(HAlign::Center);
		a.SetWidth(Coord::Fraction(1));
		a.SetPadding(5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDisabled() ? TE_ButtonDisabled :
				info.IsDown() || info.IsChecked() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
			if (info.IsFocused())
				DrawThemeElement(TE_Outline, r.x0 - 1, r.y0 - 1, r.x1 + 1, r.y1 + 1);
		});
		defaultTheme.button = a.block;
	}
	void CreateCheckbox()
	{
		StyleAccessor a(&dtCheckbox);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		//a.SetWidth(Coord::Percent(12));
		//a.SetHeight(Coord::Percent(12));
		a.SetWidth(GetFontHeight() + 5 + 5);
		a.SetHeight(GetFontHeight() + 5 + 5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			float w = min(r.GetWidth(), r.GetHeight());
			DrawThemeElement(
				info.IsDisabled() ? TE_CheckBgrDisabled :
				info.IsDown() ? TE_CheckBgrPressed :
				info.IsHovered() ? TE_CheckBgrHover : TE_CheckBgrNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
			if (info.checkState == 1)
				DrawThemeElement(info.IsDisabled() ? TE_CheckMarkDisabled : TE_CheckMark, r.x0, r.y0, r.x0 + w, r.y0 + w);
			else if (info.checkState)
				DrawThemeElement(info.IsDisabled() ? TE_CheckIndDisabled : TE_CheckInd, r.x0, r.y0, r.x0 + w, r.y0 + w);
			if (info.IsFocused())
				DrawThemeElement(TE_Outline, r.x0 - 1, r.y0 - 1, r.x0 + w + 1, r.y0 + w + 1);
		});
		defaultTheme.checkbox = a.block;
	}
	void CreateRadioButton()
	{
		StyleAccessor a(&dtRadioButton);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		//a.SetWidth(Coord::Percent(12));
		//a.SetHeight(Coord::Percent(12));
		a.SetWidth(GetFontHeight() + 5 + 5);
		//a.SetMargin(5);
		//a.SetHeight(GetFontHeight());
		a.SetPadding(5);
		a.SetPaddingLeft(GetFontHeight() + 5 + 5 + 5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			float w = min(r.GetWidth(), r.GetHeight());
			DrawThemeElement(
				info.IsDisabled() ? TE_RadioBgrDisabled :
				info.IsDown() ? TE_RadioBgrPressed :
				info.IsHovered() ? TE_RadioBgrHover : TE_RadioBgrNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
			if (info.IsChecked())
				DrawThemeElement(info.IsDisabled() ? TE_RadioMarkDisabled : TE_RadioMark, r.x0, r.y0, r.x0 + w, r.y0 + w);
			if (info.IsFocused())
				DrawThemeElement(TE_Outline, r.x0 - 1, r.y0 - 1, r.x0 + w + 1, r.y0 + w + 1);
		});
		defaultTheme.radioButton = a.block;
	}
	void CreateSelectable()
	{
		StyleAccessor a(&dtSelectable);
		PreventHeapDelete(a);
		a.SetLayout(layouts::Stack());
		a.SetStackingDirection(StackingDirection::LeftToRight);
		a.SetWidth(Coord::Fraction(1));
		a.SetPadding(5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			if (info.IsChecked() || info.IsDown() || info.IsHovered())
			{
				Color4b col;
				if (info.IsChecked())
					col = Color4f(0.6f, 0.04f, 0.0f);
				else if (info.IsDown())
					col = Color4f(0, 0, 0, 0.5f);
				else if (info.IsHovered())
					col = Color4f(1, 1, 1, 0.2f);
				draw::RectCol(r.x0, r.y0, r.x1, r.y1, col);
			}
			if (info.IsFocused())
				DrawThemeElement(TE_Outline, r.x0 - 1, r.y0 - 1, r.x1 + 1, r.y1 + 1);
		});
		defaultTheme.selectable = a.block;
	}
	void CreateCollapsibleTreeNode()
	{
		StyleAccessor a(&dtCollapsibleTreeNode);
		PreventHeapDelete(a);
		//a.SetLayout(layouts::Stack());
		//a.SetPadding(1);
		//a.SetPaddingLeft(GetFontHeight());
		a.SetLayout(layouts::InlineBlock());
		a.SetWidth(GetFontHeight() + 5 + 5);
		a.SetHeight(GetFontHeight() + 5 + 5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			float w = min(r.GetWidth(), r.GetHeight());
			DrawThemeElement(info.IsHovered() ?
				info.IsChecked() ? TE_TreeTickOpenHover : TE_TreeTickClosedHover :
				info.IsChecked() ? TE_TreeTickOpenNormal : TE_TreeTickClosedNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
		});
		defaultTheme.collapsibleTreeNode = a.block;
	}
	void CreateTextBoxBase()
	{
		StyleAccessor a(&dtTextBoxBase);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetPadding(5);
		a.SetWidth(Coord::Fraction(1));
		a.SetHeight(GetFontHeight() + 5 + 5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
			if (info.IsFocused())
				DrawThemeElement(TE_Outline, r.x0 - 1, r.y0 - 1, r.x1 + 1, r.y1 + 1);
		});
		defaultTheme.textBoxBase = a.block;
	}
	void CreateListBox()
	{
		StyleAccessor a(&dtListBox);
		PreventHeapDelete(a);
		a.SetLayout(layouts::Stack());
		a.SetPadding(5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.listBox = a.block;
	}
	void CreateProgressBarBase()
	{
		StyleAccessor a(&dtProgressBarBase);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetWidth(Coord::Percent(100));
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.progressBarBase = a.block;
	}
	void CreateProgressBarCompletion()
	{
		StyleAccessor a(&dtProgressBarCompletion);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetMargin(2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.progressBarCompletion = a.block;
	}
	void CreateSliderHBase()
	{
		StyleAccessor a(&dtSliderHBase);
		PreventHeapDelete(a);
		a.SetPaddingTop(20);
		a.SetWidth(Coord::Fraction(1));
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.sliderHBase = a.block;
	}
	void CreateSliderHTrack()
	{
		StyleAccessor a(&dtSliderHTrack);
		PreventHeapDelete(a);
		a.SetMargin(8);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			if (r.GetWidth() > 0)
			{
				auto el = info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal;
				r = r.ExtendBy(UIRect::UniformBorder(3));
				DrawThemeElement(el, r.x0, r.y0, r.x1, r.y1);
			}
		});
		defaultTheme.sliderHTrack = a.block;
	}
	void CreateSliderHTrackFill()
	{
		StyleAccessor a(&dtSliderHTrackFill);
		PreventHeapDelete(a);
		a.SetMargin(8);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			if (r.GetWidth() > 0)
			{
				auto el = info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal;
				r = r.ExtendBy(UIRect::UniformBorder(3));
				DrawThemeElement(el, r.x0, r.y0, r.x1, r.y1);
			}
		});
		defaultTheme.sliderHTrackFill = a.block;
	}
	void CreateSliderHThumb()
	{
		StyleAccessor a(&dtSliderHThumb);
		PreventHeapDelete(a);
		a.SetPadding(6, 4);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover :
				info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal,
				r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.sliderHThumb = a.block;
	}
	void CreateScrollVTrack()
	{
		StyleAccessor a(&dtScrollVTrack);
		PreventHeapDelete(a);
		a.SetWidth(20);
		a.SetPadding(2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.scrollVTrack = a.block;
	}
	void CreateScrollVThumb()
	{
		StyleAccessor a(&dtScrollVThumb);
		PreventHeapDelete(a);
		a.SetMinHeight(16);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover :
				info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal,
				r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.scrollVThumb = a.block;
	}
	void CreateTabGroup()
	{
		StyleAccessor a(&dtTabGroup);
		PreventHeapDelete(a);
		a.SetMargin(2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.tabGroup = a.block;
	}
	void CreateTabList()
	{
		StyleAccessor a(&dtTabList);
		PreventHeapDelete(a);
		a.SetPadding(0, 4);
		a.SetLayout(layouts::Stack());
		a.SetStackingDirection(StackingDirection::LeftToRight);
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.tabList = a.block;
	}
	void CreateTabButton()
	{
		StyleAccessor a(&dtTabButton);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetPadding(5);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() || info.IsChecked() ? TE_TabSelected :
				info.IsHovered() ? TE_TabHover : TE_TabNormal, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.tabButton = a.block;
	}
	void CreateTabPanel()
	{
		StyleAccessor a(&dtTabPanel);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetWidth(Coord::Percent(100));
		a.SetMarginTop(-2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_TabPanel, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.tabPanel = a.block;
	}
	void CreateTableBase()
	{
		StyleAccessor a(&dtTableBase);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetWidth(Coord::Percent(100));
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.tableBase = a.block;
	}
	void CreateTableCell()
	{
		StyleAccessor a(&dtTableCell);
		PreventHeapDelete(a);
		a.SetPadding(1, 2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			Color4f colContents;
			if (info.IsChecked())
				colContents = { 0.45f, 0.05f, 0.0f };
			else if (info.IsHovered())
				colContents = { 0.25f, 0.05f, 0.0f };
			else
				colContents = { 0.05f, 0.05f, 0.05f };
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y1, colContents);

			Color4f colEdge;
			if (info.IsChecked())
				colEdge = { 0.45f, 0.1f, 0.0f };
			else if (info.IsHovered())
				colEdge = { 0.25f, 0.1f, 0.05f };
			else
				colEdge = { 0.15f, 0.15f, 0.15f };
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y0 + 1, colEdge);
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x0 + 1, info.rect.y1, colEdge);
			draw::RectCol(info.rect.x0, info.rect.y1 - 1, info.rect.x1, info.rect.y1, colEdge);
			draw::RectCol(info.rect.x1 - 1, info.rect.y0, info.rect.x1, info.rect.y1, colEdge);
		});
		defaultTheme.tableCell = a.block;
	}
	void CreateTableRowHeader()
	{
		StyleAccessor a(&dtTableRowHeader);
		PreventHeapDelete(a);
		a.SetPadding(1, 2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y1, Color4f(0.1f, 0.1f, 0.1f));
			Color4f colEdge(0.2f, 0.2f, 0.2f);
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y0 + 1, colEdge);
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x0 + 1, info.rect.y1, colEdge);
			draw::RectCol(info.rect.x0, info.rect.y1 - 1, info.rect.x1, info.rect.y1, colEdge);
			draw::RectCol(info.rect.x1 - 1, info.rect.y0, info.rect.x1, info.rect.y1, colEdge);
		});
		defaultTheme.tableRowHeader = a.block;
	}
	void CreateTableColHeader()
	{
		StyleAccessor a(&dtTableColHeader);
		PreventHeapDelete(a);
		a.SetPadding(1, 2);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y1, Color4f(0.1f, 0.1f, 0.1f));
			Color4f colEdge(0.2f, 0.2f, 0.2f);
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x1, info.rect.y0 + 1, colEdge);
			draw::RectCol(info.rect.x0, info.rect.y0, info.rect.x0 + 1, info.rect.y1, colEdge);
			draw::RectCol(info.rect.x0, info.rect.y1 - 1, info.rect.x1, info.rect.y1, colEdge);
			draw::RectCol(info.rect.x1 - 1, info.rect.y0, info.rect.x1, info.rect.y1, colEdge);
		});
		defaultTheme.tableColHeader = a.block;
	}
	void CreateColorBlock()
	{
		StyleAccessor a(&dtColorBlock);
		PreventHeapDelete(a);
		a.SetWidth(20);
		a.SetHeight(20);
		a.SetLayout(layouts::InlineBlock());
		a.SetPadding(3);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_Panel, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.colorBlock = a.block;
	}
	void CreateColorInspectBlock()
	{
		StyleAccessor a(&dtColorInspectBlock);
		PreventHeapDelete(a);
		a.SetWidth(Coord::Fraction(1));
		a.SetHeight(20);
		a.SetLayout(layouts::InlineBlock());
		a.SetPadding(3);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_Panel, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.colorInspectBlock = a.block;
	}
	void CreateImage()
	{
		StyleAccessor a(&dtImage);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetPaintFunc([](const PaintInfo& info)
		{
		});
		defaultTheme.image = a.block;
	}
	void CreateSelectorContainer()
	{
		StyleAccessor a(&dtSelectorContainer);
		PreventHeapDelete(a);
		a.SetPadding(8);
		defaultTheme.selectorContainer = a.block;
	}
	void CreateSelector()
	{
		StyleAccessor a(&dtSelector);
		PreventHeapDelete(a);
		a.SetWidth(16);
		a.SetHeight(16);
		a.SetPaintFunc([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(TE_Selector16, r.x0, r.y0, r.x1, r.y1);
		});
		defaultTheme.selector = a.block;
	}

	std::weak_ptr<Image> cache[(int)ThemeImage::_COUNT];
	std::shared_ptr<Image> GetImage(ThemeImage ti) override
	{
		auto& cpos = cache[int(ti)];
		if (!cpos.expired())
			return cpos.lock();
		auto img = GetImageUncached(ti);
		cpos = img;
		return img;
	}
	std::shared_ptr<Image> GetImageUncached(ThemeImage ti)
	{
		switch (ti)
		{
		case ThemeImage::CheckerboardBackground: {
			Canvas c(16, 16);
			for (int y = 0; y < 16; y++)
				for (int x = 0; x < 16; x++)
					c.GetPixels()[x + y * 16] = ((x < 8) ^ (y < 8) ? Color4f(0.2f, 1) : Color4f(0.4f, 1)).GetColor32();
			return std::make_shared<Image>(c, draw::TF_Repeat); }
		default:
			return nullptr;
		}
	}
}
init;

} // ui
