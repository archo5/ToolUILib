
#include "../Render/Render.h"
#include "Theme.h"

#include "ThemeData.h"


namespace ui {

Theme* Theme::current;


struct DefaultTheme : Theme
{
	ThemeDataHandle themeData;

	DefaultTheme()
	{
		themeData = LoadTheme("data/theme_default");
	}

	IPainter* GetPainter(const StaticID_Painter& id)
	{
		auto it = themeData->painters.find(id._name);
		return it.is_valid() ? it->value : nullptr;
	}
	AABB2i GetIntRect(const StaticID_IntRect& id)
	{
		// TODO
		return {};
	}
	StyleBlockRef GetStyle(const StaticID_Style& id)
	{
		auto it = themeData->styles.find(id._name);
		if (it.is_valid())
			return it->value;
		return GetObjectStyle();
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
