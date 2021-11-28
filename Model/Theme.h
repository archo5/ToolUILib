
#pragma once
#include "Layout.h"

#include "../Render/Render.h"


namespace ui {

// TODO refactor

struct StaticID
{
	const char* _name;
	uint32_t _id;

	StaticID(const char* name);
	UI_FORCEINLINE bool operator == (const StaticID& o) const { return _id == o._id; }
	UI_FORCEINLINE bool operator != (const StaticID& o) const { return _id != o._id; }

	static uint32_t& GetCount();
};

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
	// core controls
	// TODO move out
	StyleBlockRef object;
	StyleBlockRef text;
	StyleBlockRef property;
	StyleBlockRef propLabel;
	StyleBlockRef image;

	virtual IPainter* GetPainter(const StaticID& id) = 0;
	virtual AABB2i GetIntRect(const StaticID& id) = 0;
	virtual StyleBlockRef GetStyle(const StaticID& id) = 0;
	virtual draw::ImageHandle GetImage(ThemeImage ti) = 0;

	static Theme* current;
};

} // ui
