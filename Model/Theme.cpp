
#include "../Render/Render.h"
#include "Theme.h"

#include "ThemeData.h"

#include "../Core/Font.h"
static float GetFontHeight() { return 12; } // TODO remove


namespace ui {

static HashMap<StringView, uint32_t> g_staticIDs;

StaticID::StaticID(const char* name) : _name(name)
{
	auto it = g_staticIDs.find(_name);
	if (it.is_valid())
		_id = it->value;
	else
		g_staticIDs.insert(_name, _id = GetCount()++);
}

uint32_t& StaticID::GetCount()
{
	static uint32_t count;
	return count;
}


StaticID sid_panel("panel");
StaticID sid_button("button");
StaticID sid_selectable("selectable");
StaticID sid_listbox("listbox");


Theme* Theme::current;


static unsigned char* LoadTGA(const char* img, int size[2])
{
	FILE* f = fopen(img, "rb");
	char idlen = getc(f);
	assert(idlen == 0); // no id
	char cmtype = getc(f);
	assert(cmtype == 0); // no colormap
	char imgtype = getc(f);
	assert(imgtype == 2); // only uncompressed true color image supported
	fseek(f, 5, SEEK_CUR); // skip colormap

	fseek(f, 4, SEEK_CUR); // skip x/y origin
	short fsize[2];
	fread(&fsize, 2, 2, f); // x/y size
	char bpp = getc(f);
	char imgdesc = getc(f);

	// image id here?
	// colormap here?
	// image data
	size[0] = fsize[0];
	size[1] = fsize[1];
	unsigned char* out = new unsigned char[fsize[0] * fsize[1] * 4];
	for (int i = 0; i < fsize[0] * fsize[1]; i++)
	{
		out[i * 4 + 2] = getc(f);
		out[i * 4 + 1] = getc(f);
		out[i * 4 + 0] = getc(f);
		out[i * 4 + 3] = getc(f);
	}
	return out;
}

ArrayView<char> LoadThemeResource();
static unsigned char* LoadThemeTGASys(int size[2])
{
	ArrayView<char> data = LoadThemeResource();
	if (data.empty())
		return nullptr;
	size_t p = 0;
	char idlen = data[p++];
	assert(idlen == 0); // no id
	char cmtype = data[p++];
	assert(cmtype == 0); // no colormap
	char imgtype = data[p++];
	assert(imgtype == 2); // only uncompressed true color image supported
	p += 5;

	p += 4;
	short fsize[2];
	memcpy(fsize, &data[p], 4);
	p += 4;
	char bpp = data[p++];
	char imgdesc = data[p++];

	size[0] = fsize[0];
	size[1] = fsize[1];
	unsigned char* out = new unsigned char[fsize[0] * fsize[1] * 4];
	for (int i = 0; i < fsize[0] * fsize[1]; i++)
	{
		out[i * 4 + 2] = data[p++];
		out[i * 4 + 1] = data[p++];
		out[i * 4 + 0] = data[p++];
		out[i * 4 + 3] = data[p++];
	}
	return out;
}

Sprite g_themeSprites[TE__COUNT] =
{
#if 1
	// button
	{ 0, 0, 32, 32, 5, 5, 5, 5 },
	{ 32, 0, 64, 32, 5, 5, 5, 5 },
	{ 64, 0, 96, 32, 5, 5, 5, 5 },
	{ 96, 0, 128, 32, 5, 5, 5, 5 },
	{ 128, 0, 160, 32, 5, 5, 5, 5 },

	// panel
	{ 160, 0, 192, 32, 6, 6, 6, 6 },

	// textbox
	{ 192, 0, 224, 32, 5, 5, 5, 5 },
	{ 224, 0, 256, 32, 5, 5, 5, 5 },

	// checkbgr
	{ 0, 32, 32, 64, 5, 5, 5, 5 },
	{ 32, 32, 64, 64, 5, 5, 5, 5 },
	{ 64, 32, 96, 64, 5, 5, 5, 5 },
	{ 96, 32, 128, 64, 5, 5, 5, 5 },

	// checkmarks
	{ 128, 32, 160, 64, 5, 5, 5, 5 },
	{ 160, 32, 192, 64, 5, 5, 5, 5 },
	{ 192, 32, 224, 64, 5, 5, 5, 5 },
	{ 224, 32, 256, 64, 5, 5, 5, 5 },

	// radiobgr
	{ 0, 64, 32, 96, 0, 0, 0, 0 },
	{ 32, 64, 64, 96, 0, 0, 0, 0 },
	{ 64, 64, 96, 96, 0, 0, 0, 0 },
	{ 96, 64, 128, 96, 0, 0, 0, 0 },

	// radiomark
	{ 128, 64, 160, 96, 0, 0, 0, 0 },
	{ 160, 64, 192, 96, 0, 0, 0, 0 },

	// selector
	{ 192, 64, 224, 96, 0, 0, 0, 0 },
	{ 224, 64, 240, 80, 0, 0, 0, 0 },
	{ 240, 64, 252, 76, 0, 0, 0, 0 },
	// outline
	{ 224, 80, 240, 96, 4, 4, 4, 4 },

	// tab
	{ 0, 96, 32, 128, 5, 5, 5, 5 },
	{ 32, 96, 64, 128, 5, 5, 5, 5 },
	{ 64, 96, 96, 128, 5, 5, 5, 5 },
	{ 96, 96, 128, 128, 5, 5, 5, 5 },

	// treetick
	{ 128, 96, 160, 128, 0, 0, 0, 0 },
	{ 160, 96, 192, 128, 0, 0, 0, 0 },
	{ 192, 96, 224, 128, 0, 0, 0, 0 },
	{ 224, 96, 256, 128, 0, 0, 0, 0 },

#else
	{ 0, 0, 64, 64, 5, 5, 5, 5 },
	{ 64, 0, 128, 64, 5, 5, 5, 5 },
	{ 128, 0, 192, 64, 5, 5, 5, 5 },

	{ 192, 0, 224, 32, 5, 5, 5, 5 },
	{ 224, 0, 256, 32, 5, 5, 5, 5 },
	{ 192, 32, 224, 64, 5, 5, 5, 5 },
	{ 224, 32, 256, 64, 5, 5, 5, 5 },
#endif
};

ui::draw::ImageHandle g_themeImages[TE__COUNT];

ui::AABB2f GetThemeElementBorderWidths(EThemeElement e)
{
	auto& s = g_themeSprites[e];
	return { float(s.bx0), float(s.by0), float(s.bx1), float(s.by1) };
}

void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1)
{
	const Sprite& s = g_themeSprites[e];
	ui::AABB2f outer = { x0, y0, x1, y1 };
	ui::AABB2f inner = outer.ShrinkBy({ float(s.bx0), float(s.by0), float(s.bx1), float(s.by1) });
	float ifw = 1.0f / (s.ox1 - s.ox0), ifh = 1.0f / (s.oy1 - s.oy0);
	int s_ix0 = s.bx0, s_iy0 = s.by0, s_ix1 = (s.ox1 - s.ox0) - s.bx1, s_iy1 = (s.oy1 - s.oy0) - s.by1;
	ui::AABB2f texinner = { s_ix0 * ifw, s_iy0 * ifh, s_ix1 * ifw, s_iy1 * ifh };
	ui::draw::RectColTex9Slice(outer, inner, ui::Color4b::White(), g_themeImages[e], { 0, 0, 1, 1 }, texinner);
}


struct ThemeElementPainter : IPainter
{
	static constexpr int MAX_ELEMENTS = 16;
	EThemeElement elements[MAX_ELEMENTS] = {};

	ThemeElementPainter() {}
	ThemeElementPainter(EThemeElement e) { SetAll(e); }
	ThemeElementPainter* SetAll(EThemeElement e)
	{
		for (int i = 0; i < MAX_ELEMENTS; i++)
			elements[i] = e;
		return this;
	}
	ThemeElementPainter* SetNormalDisabled(EThemeElement normal, EThemeElement disabled)
	{
		for (int i = 0; i < MAX_ELEMENTS; i++)
			elements[i] = i & PS_Disabled ? disabled : normal;
		return this;
	}
	ThemeElementPainter* SetNormalHoverPressedDisabled(EThemeElement normal, EThemeElement hover, EThemeElement pressed, EThemeElement disabled)
	{
		for (int i = 0; i < MAX_ELEMENTS; i++)
		{
			elements[i] =
				i & PS_Disabled ? disabled : 
				i & PS_Down ? pressed :
				i & PS_Hover ? hover :
				normal;
		}
		return this;
	}
	ContentPaintAdvice Paint(const PaintInfo& info) override
	{
		auto r = info.rect;
		DrawThemeElement(elements[info.state & (MAX_ELEMENTS - 1)], r.x0, r.y0, r.x1, r.y1);
		return {};
	}
};

struct BorderRectanglePainter : IPainter
{
	Color4b backgroundColor;
	Color4b borderColor;

	BorderRectanglePainter(Color4b bgcol, Color4b bordercol) : backgroundColor(bgcol), borderColor(bordercol) {}
	ContentPaintAdvice Paint(const PaintInfo& info) override
	{
		auto r = info.rect;
		draw::RectCol(r.x0, r.y0, r.x1, r.y1, backgroundColor);
		draw::RectCol(r.x0, r.y0, r.x1, r.y0 + 1, borderColor);
		draw::RectCol(r.x0, r.y0, r.x0 + 1, r.y1, borderColor);
		draw::RectCol(r.x0, r.y1 - 1, r.x1, r.y1, borderColor);
		draw::RectCol(r.x1 - 1, r.y0, r.x1, r.y1, borderColor);
		return {};
	}
};


struct DefaultTheme : Theme
{
	ThemeDataHandle themeData;

	StyleBlock dtObject;
	StyleBlock dtText;
	StyleBlock dtProperty;
	StyleBlock dtPropLabel;
	StyleBlock dtHeader;
	StyleBlock dtCollapsibleTreeNode;
	StyleBlock dtTextBoxBase;
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
	StyleBlock dtImage;
	StyleBlock dtSelectorContainer;
	StyleBlock dtSelector;

	DefaultTheme()
	{
		themeData = LoadTheme("data/theme_default");

		CreateObject();
		CreateText();
		CreateProperty();
		CreatePropLabel();
		CreateHeader();
		CreateCollapsibleTreeNode();
		CreateTextBoxBase();
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
		CreateSelectorContainer();
		CreateSelector();
#define defaultTheme (*this) // TODO
	}
	void PreventHeapDelete(StyleAccessor& a)
	{
		a.block->_refCount++;
	}
	void CreateObject()
	{
		StyleAccessor a(&dtObject);
		PreventHeapDelete(a);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.object = a.block;
	}
	void CreateText()
	{
		StyleAccessor a(&dtText);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetBoxSizing(BoxSizing::ContentBox);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.text = a.block;
	}
	void CreateProperty()
	{
		StyleAccessor a(&dtProperty);
		PreventHeapDelete(a);
		a.SetLayout(layouts::StackExpand());
		a.SetStackingDirection(StackingDirection::LeftToRight);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.property = a.block;
	}
	void CreatePropLabel()
	{
		StyleAccessor a(&dtPropLabel);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.propLabel = a.block;
	}
	void CreateHeader()
	{
		StyleAccessor a(&dtHeader);
		PreventHeapDelete(a);
		a.SetFontWeight(FontWeight::Bold);
		a.SetPadding(5);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.header = a.block;
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
		a.SetBackgroundPainter(CreateFunctionPainter([](const PaintInfo& info)
		{
			auto r = info.rect;
			float w = min(r.GetWidth(), r.GetHeight());
			DrawThemeElement(info.IsHovered() ?
				info.IsChecked() ? TE_TreeTickOpenHover : TE_TreeTickClosedHover :
				info.IsChecked() ? TE_TreeTickOpenNormal : TE_TreeTickClosedNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
			return ContentPaintAdvice{};
		}));
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
		a.SetBackgroundPainter(CreateFunctionPainter([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal, r.x0, r.y0, r.x1, r.y1);
			if (info.IsFocused())
				DrawThemeElement(TE_Outline, r.x0 - 1, r.y0 - 1, r.x1 + 1, r.y1 + 1);
			return ContentPaintAdvice{};
		}));
		defaultTheme.textBoxBase = a.block;
	}
	void CreateSliderHBase()
	{
		StyleAccessor a(&dtSliderHBase);
		PreventHeapDelete(a);
		a.SetPaddingTop(20);
		a.SetWidth(Coord::Fraction(1));
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.sliderHBase = a.block;
	}
	void CreateSliderHTrack()
	{
		StyleAccessor a(&dtSliderHTrack);
		PreventHeapDelete(a);
		a.SetMargin(8);
		a.SetBackgroundPainter(CreateFunctionPainter([](const PaintInfo& info)
		{
			auto r = info.rect;
			if (r.GetWidth() > 0)
			{
				auto el = info.IsDisabled() ? TE_TextboxDisabled : TE_TextboxNormal;
				r = r.ExtendBy(UIRect::UniformBorder(3));
				DrawThemeElement(el, r.x0, r.y0, r.x1, r.y1);
			}
			return ContentPaintAdvice{};
		}));
		defaultTheme.sliderHTrack = a.block;
	}
	void CreateSliderHTrackFill()
	{
		StyleAccessor a(&dtSliderHTrackFill);
		PreventHeapDelete(a);
		a.SetMargin(8);
		a.SetBackgroundPainter(CreateFunctionPainter([](const PaintInfo& info)
		{
			auto r = info.rect;
			if (r.GetWidth() > 0)
			{
				auto el = info.IsDisabled() ? TE_ButtonDisabled : TE_ButtonNormal;
				r = r.ExtendBy(UIRect::UniformBorder(3));
				DrawThemeElement(el, r.x0, r.y0, r.x1, r.y1);
			}
			return ContentPaintAdvice{};
		}));
		defaultTheme.sliderHTrackFill = a.block;
	}
	void CreateSliderHThumb()
	{
		StyleAccessor a(&dtSliderHThumb);
		PreventHeapDelete(a);
		a.SetPadding(6, 4);
		a.SetBackgroundPainter((new ThemeElementPainter())->SetNormalHoverPressedDisabled(
			TE_ButtonNormal, TE_ButtonHover, TE_ButtonPressed, TE_ButtonDisabled));
		defaultTheme.sliderHThumb = a.block;
	}
	void CreateScrollVTrack()
	{
		StyleAccessor a(&dtScrollVTrack);
		PreventHeapDelete(a);
		a.SetWidth(20);
		a.SetPadding(2);
		a.SetBackgroundPainter((new ThemeElementPainter())->SetNormalDisabled(TE_TextboxNormal, TE_TextboxDisabled));
		defaultTheme.scrollVTrack = a.block;
	}
	void CreateScrollVThumb()
	{
		StyleAccessor a(&dtScrollVThumb);
		PreventHeapDelete(a);
		a.SetMinHeight(16);
		a.SetBackgroundPainter((new ThemeElementPainter())->SetNormalHoverPressedDisabled(
			TE_ButtonNormal, TE_ButtonHover, TE_ButtonPressed, TE_ButtonDisabled));
		defaultTheme.scrollVThumb = a.block;
	}
	void CreateTabGroup()
	{
		StyleAccessor a(&dtTabGroup);
		PreventHeapDelete(a);
		a.SetMargin(2);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.tabGroup = a.block;
	}
	void CreateTabList()
	{
		StyleAccessor a(&dtTabList);
		PreventHeapDelete(a);
		a.SetPadding(0, 4);
		a.SetLayout(layouts::Stack());
		a.SetStackingDirection(StackingDirection::LeftToRight);
		a.SetBackgroundPainter(EmptyPainter::Get());
		defaultTheme.tabList = a.block;
	}
	void CreateTabButton()
	{
		StyleAccessor a(&dtTabButton);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetPadding(5);
		a.SetBackgroundPainter(CreateFunctionPainter([](const PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDown() || info.IsChecked() ? TE_TabSelected :
				info.IsHovered() ? TE_TabHover : TE_TabNormal, r.x0, r.y0, r.x1, r.y1);
			return ContentPaintAdvice{};
		}));
		defaultTheme.tabButton = a.block;
	}
	void CreateTabPanel()
	{
		StyleAccessor a(&dtTabPanel);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetWidth(Coord::Percent(100));
		a.SetMarginTop(-2);
		a.SetBackgroundPainter(new ThemeElementPainter(TE_TabPanel));
		defaultTheme.tabPanel = a.block;
	}
	void CreateTableBase()
	{
		StyleAccessor a(&dtTableBase);
		PreventHeapDelete(a);
		a.SetPadding(5);
		a.SetWidth(Coord::Percent(100));
		a.SetBackgroundPainter(new ThemeElementPainter(TE_TextboxNormal));
		defaultTheme.tableBase = a.block;
	}
	void CreateTableCell()
	{
		StyleAccessor a(&dtTableCell);
		PreventHeapDelete(a);
		a.SetPadding(1, 2);
		a.SetBackgroundPainter(CreateFunctionPainter([](const PaintInfo& info)
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
			return ContentPaintAdvice{};
		}));
		defaultTheme.tableCell = a.block;
	}
	void CreateTableRowHeader()
	{
		StyleAccessor a(&dtTableRowHeader);
		PreventHeapDelete(a);
		a.SetPadding(1, 2);
		a.SetBackgroundPainter(new BorderRectanglePainter(Color4f(0.1f, 0.1f, 0.1f), Color4f(0.2f, 0.2f, 0.2f)));
		defaultTheme.tableRowHeader = a.block;
	}
	void CreateTableColHeader()
	{
		StyleAccessor a(&dtTableColHeader);
		PreventHeapDelete(a);
		a.SetPadding(1, 2);
		a.SetBackgroundPainter(new BorderRectanglePainter(Color4f(0.1f, 0.1f, 0.1f), Color4f(0.2f, 0.2f, 0.2f)));
		defaultTheme.tableColHeader = a.block;
	}
	void CreateImage()
	{
		StyleAccessor a(&dtImage);
		PreventHeapDelete(a);
		a.SetLayout(layouts::InlineBlock());
		a.SetBackgroundPainter(EmptyPainter::Get());
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
		a.SetBackgroundPainter(new ThemeElementPainter(TE_Selector16));
		defaultTheme.selector = a.block;
	}

	IPainter* GetPainter(const StaticID& id) override
	{
		auto it = themeData->painters.find(id._name);
		return it.is_valid() ? it->value : nullptr;
	}
	AABB2i GetIntRect(const StaticID& id) override
	{
		// TODO
		return {};
	}
	StyleBlockRef GetStyle(const StaticID& id) override
	{
		auto it = themeData->styles.find(id._name);
		if (it.is_valid())
			return it->value;
		return object;
	}

	draw::ImageHandle cache[(int)ThemeImage::_COUNT];
	draw::ImageHandle GetImage(ThemeImage ti) override
	{
		auto& cpos = cache[int(ti)];
		if (cpos)
			return cpos;
		auto img = GetImageUncached(ti);
		cpos = img;
		return img;
	}
	draw::ImageHandle GetImageUncached(ThemeImage ti)
	{
		switch (ti)
		{
		case ThemeImage::CheckerboardBackground: {
			Canvas c(16, 16);
			for (int y = 0; y < 16; y++)
				for (int x = 0; x < 16; x++)
					c.GetPixels()[x + y * 16] = ((x < 8) ^ (y < 8) ? Color4f(0.2f, 1) : Color4f(0.4f, 1)).GetColor32();
			return draw::ImageCreateFromCanvas(c, draw::TexFlags::Repeat); }
		default:
			return nullptr;
		}
	}
};


void InitTheme()
{
	int size[2];
	auto* data = LoadThemeTGASys(size);
	if (!data)
		data = LoadTGA("gui-theme2.tga", size);
	for (int i = 0; i < TE__COUNT; i++)
	{
		auto& s = g_themeSprites[i];
		g_themeImages[i] = ui::draw::ImageCreateRGBA8(
			s.ox1 - s.ox0,
			s.oy1 - s.oy0,
			size[0] * 4,
			&data[(s.ox0 + s.oy0 * size[0]) * 4],
			draw::TexFlags::Packed);
	}
	delete[] data;

	Theme::current = new DefaultTheme;
}

void FreeTheme()
{
	delete static_cast<DefaultTheme*>(Theme::current);
	Theme::current = nullptr;

	for (auto& tex : g_themeImages)
		tex = nullptr;
}

} // ui
