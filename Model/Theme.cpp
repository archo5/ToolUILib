
#include "../Render/Render.h"
#include "Theme.h"

#include "ThemeData.h"


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


Theme* Theme::current;


struct DefaultTheme : Theme
{
	ThemeDataHandle themeData;

	StyleBlock dtObject;
	StyleBlock dtText;

	DefaultTheme()
	{
		themeData = LoadTheme("data/theme_default");

		CreateObject();
		CreateText();
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

	IPainter* GetPainter(const StaticID& id)
	{
		auto it = themeData->painters.find(id._name);
		return it.is_valid() ? it->value : nullptr;
	}
	AABB2i GetIntRect(const StaticID& id)
	{
		// TODO
		return {};
	}
	StyleBlockRef GetStyle(const StaticID& id)
	{
		auto it = themeData->styles.find(id._name);
		if (it.is_valid())
			return it->value;
		return object;
	}

	draw::ImageHandle cache[(int)ThemeImage::_COUNT];
	draw::ImageHandle GetImage(ThemeImage ti)
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
	Theme::current = new DefaultTheme;
}

void FreeTheme()
{
	delete static_cast<DefaultTheme*>(Theme::current);
	Theme::current = nullptr;
}

} // ui
