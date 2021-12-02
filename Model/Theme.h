
#pragma once
#include "ThemeData.h"


namespace ui {

void InitTheme();
void FreeTheme();

enum class ThemeImage
{
	Unknown = 0,
	CheckerboardBackground,

	_COUNT,
};

struct Theme
{
	virtual draw::ImageHandle GetImage(ThemeImage ti) = 0;

	static Theme* current;
};

} // ui
