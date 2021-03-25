
#pragma once
#include <memory>
#include "Layout.h"
#include "Graphics.h"

namespace ui {

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
	StyleBlockRef panel;
	StyleBlockRef header;
	StyleBlockRef button;
	StyleBlockRef checkbox;
	StyleBlockRef radioButton;
	StyleBlockRef selectable;
	StyleBlockRef collapsibleTreeNode;
	StyleBlockRef textBoxBase;
	StyleBlockRef listBox;
	StyleBlockRef progressBarBase;
	StyleBlockRef progressBarCompletion;
	StyleBlockRef sliderHBase;
	StyleBlockRef sliderHTrack;
	StyleBlockRef sliderHTrackFill;
	StyleBlockRef sliderHThumb;
	StyleBlockRef scrollVTrack;
	StyleBlockRef scrollVThumb;
	StyleBlockRef tabGroup;
	StyleBlockRef tabList;
	StyleBlockRef tabButton;
	StyleBlockRef tabPanel;
	StyleBlockRef tableBase;
	StyleBlockRef tableCell;
	StyleBlockRef tableRowHeader;
	StyleBlockRef tableColHeader;
	StyleBlockRef colorBlock;
	StyleBlockRef colorInspectBlock;
	StyleBlockRef image;
	StyleBlockRef selectorContainer;
	StyleBlockRef selector;

	virtual std::shared_ptr<Image> GetImage(ThemeImage ti) = 0;

	static Theme* current;
};

} // ui
