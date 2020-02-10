
#pragma once
#include "Layout.h"

namespace ui {

struct Theme
{
	// core controls
	style::BlockRef object;
	style::BlockRef panel;
	style::BlockRef button;
	style::BlockRef checkbox;
	style::BlockRef radioButton;
	style::BlockRef collapsibleTreeNode;
	style::BlockRef textBoxBase;
	style::BlockRef listBox;
	style::BlockRef progressBarBase;
	style::BlockRef progressBarCompletion;
	style::BlockRef sliderHBase;
	style::BlockRef sliderHTrack;
	style::BlockRef sliderHTrackFill;
	style::BlockRef sliderHThumb;
	style::BlockRef scrollVTrack;
	style::BlockRef scrollVThumb;
	style::BlockRef tabGroup;
	style::BlockRef tabList;
	style::BlockRef tabButton;
	style::BlockRef tabPanel;
	style::BlockRef tableBase;
	style::BlockRef tableCell;
	style::BlockRef tableRowHeader;
	style::BlockRef tableColHeader;
	style::BlockRef colorBlock;
	style::BlockRef image;
	style::BlockRef selectorContainer;
	style::BlockRef selector;

	static Theme* current;
};

} // ui
