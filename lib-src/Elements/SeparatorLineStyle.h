
#pragma once

#include "../Model/Painting.h"


namespace ui {

struct SeparatorLineStyle
{
	static constexpr const char* NAME = "SeparatorLineStyle";

	float size = 8;
	PainterHandle backgroundPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

} // ui
