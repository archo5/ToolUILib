
#pragma once
#include "Layout.h"

namespace ui {

struct Theme
{
	style::Block* panel;
	style::Block* button;
	style::Block* checkbox;
	style::Block* radioButton;
	style::Block* collapsibleTreeNode;
	style::Block* textBoxBase;

	static Theme* current;
};

} // ui
