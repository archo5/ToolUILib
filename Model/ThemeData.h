
#pragma once

#include "Layout.h"
#include "../Render/Render.h"
#include "../Core/HashTable.h"


namespace ui {

struct IThemeLoader
{
	virtual PainterHandle LoadPainter() = 0;
	virtual draw::ImageSetHandle FindImageSet(const std::string& name) = 0;
};

struct ThemeData : RefCountedST
{
	HashMap<std::string, draw::ImageSetHandle> imageSets;
	HashMap<std::string, PainterHandle> painters;
	HashMap<std::string, StyleBlockRef> styles;
};
using ThemeDataHandle = RCHandle<ThemeData>;

typedef IPainter* PainterCreateFunc(IThemeLoader*, IObjectIterator&);

void RegisterPainter(const char* type, PainterCreateFunc* createFunc);

ThemeDataHandle LoadTheme(StringView folder);

} // ui
