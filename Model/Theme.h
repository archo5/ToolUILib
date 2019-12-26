
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
	style::BlockRef tabGroup;
	style::BlockRef tabList;
	style::BlockRef tabButton;
	style::BlockRef tabPanel;
	style::BlockRef tableBase;
	style::BlockRef tableCell;
	style::BlockRef tableRowHeader;
	style::BlockRef tableColHeader;

	static Theme* current;
};

} // ui
