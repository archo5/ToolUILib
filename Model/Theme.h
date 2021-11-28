
#pragma once
#include "Layout.h"

#include "../Core/StaticID.h"
#include "../Render/Render.h"


namespace ui {

void InitTheme();
void FreeTheme();

enum class ThemeImage
{
	Unknown = 0,
	CheckerboardBackground,

	_COUNT,
};

using StaticID_Painter = StaticID<IPainter>;
using StaticID_IntRect = StaticID<AABB2i>;
using StaticID_Style = StaticID<StyleBlock>;

struct Theme
{
	// core controls
	// TODO move out
	StyleBlockRef object;
	StyleBlockRef text;

	virtual IPainter* GetPainter(const StaticID_Painter& id) = 0;
	virtual AABB2i GetIntRect(const StaticID_IntRect& id) = 0;
	virtual StyleBlockRef GetStyle(const StaticID_Style& id) = 0;
	virtual draw::ImageHandle GetImage(ThemeImage ti) = 0;

	static Theme* current;
};

} // ui
