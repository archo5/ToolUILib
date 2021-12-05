
#pragma once

#include "Layout.h"
#include "../Render/Render.h"
#include "../Core/HashTable.h"
#include "../Core/StaticID.h"


namespace ui {

struct IThemeLoader
{
	virtual PainterHandle LoadPainter() = 0;
	virtual Color4b LoadColor(const FieldInfo& FI) = 0;
	virtual draw::ImageSetHandle FindImageSet(const std::string& name) = 0;
};

using StaticID_ImageSet = StaticID<draw::ImageSet>;
using StaticID_Painter = StaticID<IPainter>;
using StaticID_Style = StaticID<StyleBlock>;

struct ThemeData : RefCountedST
{
	HashMap<std::string, draw::ImageSetHandle> imageSets;
	HashMap<std::string, PainterHandle> painters;
	HashMap<std::string, StyleBlockRef> styles;

	std::vector<draw::ImageSetHandle> _cachedImageSets;
	std::vector<PainterHandle> _cachedPainters;
	std::vector<StyleBlockRef> _cachedStyles;

	draw::ImageSetHandle FindImageSetByName(const std::string& name);
	PainterHandle FindPainterByName(const std::string& name);
	StyleBlockRef FindStyleByName(const std::string& name);

	draw::ImageSetHandle GetImageSet(const StaticID_ImageSet& id);
	PainterHandle GetPainter(const StaticID_Painter& id, bool returnDefaultIfMissing = true);
	StyleBlockRef GetStyle(const StaticID_Style& id, bool returnDefaultIfMissing = true);

	void LoadTheme(StringView folder);
};
using ThemeDataHandle = RCHandle<ThemeData>;

ThemeDataHandle LoadTheme(StringView folder);

ThemeData* GetCurrentTheme();
void SetCurrentTheme(ThemeData* theme);


typedef IPainter* PainterCreateFunc(IThemeLoader*, IObjectIterator&);

void RegisterPainter(const char* type, PainterCreateFunc* createFunc);

} // ui
