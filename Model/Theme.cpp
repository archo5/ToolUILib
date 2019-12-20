
#include "../Render/Theme.h"
#include "Theme.h"

namespace ui {

Theme* Theme::current;

static
struct ThemeInit
{
	Theme* t;
	ThemeInit()
	{
		t = new Theme;
		CreateButton();
		Theme::current = t;
	}
	~ThemeInit()
	{
		delete t;
	}
	void CreateButton()
	{
		auto* b = new style::Block();
		style::Accessor a(b);
		a.SetLayout(style::Layout::InlineBlock);
		a.SetBoxSizing(style::BoxSizing::BorderBox);
		a.SetPadding(5);
		b->paint_func = [](const style::PaintInfo& info)
		{
			auto r = info.rect;
			DrawThemeElement(
				info.IsDisabled() ? TE_ButtonDisabled :
				info.IsDown() ? TE_ButtonPressed :
				info.IsHovered() ? TE_ButtonHover : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
		};
		t->button = b;
	}
}
init;

} // ui
