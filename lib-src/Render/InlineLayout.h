
#pragma once

#include "../Core/Math.h"
#include "../Core/Image.h"
#include "../Core/Font.h"
#include "RenderText.h"


namespace ui {

struct FontSizeContext;

enum class IL_VAlign : u8
{
	Baseline,
	TextTop,
	TextBottom,
	Top,
	Middle,
	Bottom,
};

struct InlineLayout
{
	// moved together as much as possible (but split if doesn't fit in a line)
	struct Piece
	{
		// input
		Color4b color;
		IL_VAlign valign = IL_VAlign::Baseline;

		// quad ref
		u32 quadOff = 0;
		u32 quadCount = 0;

		// cached metrics
		float width = 0;
		// baseline is always at 0
		float textTop = 0;
		float textBtm = 0;
		float top = 0;
		float btm = 0;
		// middle is (top + btm) / 2

		// alignment offset
		float yOff = 0;

		bool IsSpace() const { return quadCount == 0; }
	};
	struct Line
	{
		u32 pieceOff;
		u32 pieceCount;

		float width;
		float lineTop;
		float lineBtm;
	};

	// accumulated data
	Array<draw::ImageQuad> quads;
	Array<Piece> pieces;
	Array<Line> lines;
	// state machine
	Font* curFont = nullptr;
	int curFontSize = 0;
	FontSizeContext* curFontSizeCtx = nullptr;
	float curLineHeight = 0;
	float curTextResScale = 1;
	u32 lastChar = 0;

	void Begin();
	void SetFont(Font* font, int size, float lineHeight);
	void AddText(StringView text, Color4b col, IL_VAlign valign = IL_VAlign::Baseline);
	void AddImage(draw::IImage* image, Vec2f size, IL_VAlign valign = IL_VAlign::Baseline);
	// returns the total height of the line boxes
	float FinishLayout(float maxWidth);
	void Render(Vec2f offset, float opacity);
};

} // ui
