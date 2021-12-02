
#include "Theme.h"


namespace ui {

Theme* Theme::current;


struct DefaultTheme : Theme
{
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
