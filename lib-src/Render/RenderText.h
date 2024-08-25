
#pragma once

#include "../Core/Font.h"


namespace ui {
namespace draw {

struct IImage;

struct ImageQuad
{
	AABB2f box;
	IImage* image;
};

AABB2f TextLineGenerateQuadsUntransformed(
	Array<ImageQuad>& retQuads,
	Font* font,
	float size,
	StringView text);
AABB2f TextLineGenerateQuads(
	Array<ImageQuad>& retQuads,
	Font* font,
	float size,
	float x,
	float y,
	StringView text,
	TextHAlign align = TextHAlign::Left,
	TextBaseline baseline = TextBaseline::Default);
AABB2f TextMultilineGenerateQuadsUntransformed(
	Array<ImageQuad>& retQuads,
	Font* font,
	float size,
	float maxWidth, // <0 = unlimited, align around 0
	float lineHeight,
	StringView text,
	TextHAlign halign = TextHAlign::Left);

void ImageQuadsCol(ArrayView<ImageQuad> quads, Color4b color, AABB2f* clipBox = nullptr);
void ImageQuadsColOffset(ArrayView<ImageQuad> quads, Vec2f offset, Color4b color, AABB2f* clipBox = nullptr);

void TextLine(
	Font* font,
	float size,
	float x,
	float y,
	StringView text,
	Color4b color,
	TextHAlign align = TextHAlign::Left,
	TextBaseline baseline = TextBaseline::Default,
	AABB2f* clipBox = nullptr);
inline void TextLine(Font* font, int size, float x, float y, StringView text, Color4b color, TextBaseline baseline, AABB2f* clipBox = nullptr)
{
	TextLine(font, float(size), x, y, text, color, TextHAlign::Left, baseline, clipBox);
}
void TextMultiline(
	Font* font,
	float size,
	AABB2f rect,
	float lineHeight,
	StringView text,
	Color4b color,
	TextHAlign halign = TextHAlign::Left,
	TextVAlign valign = TextVAlign::Top,
	AABB2f* clipBox = nullptr);

} // draw
} // ui
