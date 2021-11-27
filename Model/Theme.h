
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

struct Sprite
{
	int ox0, oy0, ox1, oy1;
	int bx0, by0, bx1, by1;
};

enum EThemeElement
{
	TE_ButtonNormal,
	TE_ButtonHover,
	TE_ButtonPressed,
	TE_ButtonDisabled,
	TE_ButtonDownDisabled,

	TE_Panel,

	TE_TextboxNormal,
	TE_TextboxDisabled,

	TE_CheckBgrNormal,
	TE_CheckBgrHover,
	TE_CheckBgrPressed,
	TE_CheckBgrDisabled,

	TE_CheckMark,
	TE_CheckMarkDisabled,
	TE_CheckInd,
	TE_CheckIndDisabled,

	TE_RadioBgrNormal,
	TE_RadioBgrHover,
	TE_RadioBgrPressed,
	TE_RadioBgrDisabled,
	TE_RadioMark,
	TE_RadioMarkDisabled,

	TE_Selector32,
	TE_Selector16,
	TE_Selector12,
	TE_Outline,

	TE_TabNormal,
	TE_TabHover,
	TE_TabSelected,
	TE_TabPanel,

	TE_TreeTickClosedNormal,
	TE_TreeTickClosedHover,
	TE_TreeTickOpenNormal,
	TE_TreeTickOpenHover,

	TE__COUNT,
};

ui::AABB2f GetThemeElementBorderWidths(EThemeElement e);
void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1);

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
	StyleBlockRef object;
	StyleBlockRef text;
	StyleBlockRef property;
	StyleBlockRef propLabel;
	StyleBlockRef header;
	StyleBlockRef sliderHBase;
	StyleBlockRef sliderHTrack;
	StyleBlockRef sliderHTrackFill;
	StyleBlockRef sliderHThumb;
	StyleBlockRef tabGroup;
	StyleBlockRef tabList;
	StyleBlockRef tabButton;
	StyleBlockRef tabPanel;
	StyleBlockRef tableBase;
	StyleBlockRef tableCell;
	StyleBlockRef tableRowHeader;
	StyleBlockRef tableColHeader;
	StyleBlockRef image;
	StyleBlockRef selectorContainer;
	StyleBlockRef selector;

	virtual IPainter* GetPainter(const StaticID& id) = 0;
	virtual AABB2i GetIntRect(const StaticID& id) = 0;
	virtual StyleBlockRef GetStyle(const StaticID& id) = 0;
	virtual draw::ImageHandle GetImage(ThemeImage ti) = 0;

	static Theme* current;
};

} // ui
