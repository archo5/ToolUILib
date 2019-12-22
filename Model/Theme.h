
#pragma once
#include "Layout.h"

namespace ui {

struct Theme
{
	style::Block* object;
	style::Block* panel;
	style::Block* button;
	style::Block* checkbox;
	style::Block* radioButton;
	style::Block* collapsibleTreeNode;
	style::Block* textBoxBase;
	style::Block* tabGroup;
	style::Block* tabList;
	style::Block* tabButton;
	style::Block* tabPanel;

	static Theme* current;
};

} // ui
