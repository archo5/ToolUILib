
#include "RenderText.h"

#include "../Core/FontImpl.h"


namespace ui {
namespace draw {

static float BaselineToYOff(Font::SizeContext& sctx, TextBaseline baseline)
{
	if (baseline != TextBaseline::Default)
	{
		// https://drafts.csswg.org/css-inline/#baseline-synthesis-em
		switch (baseline)
		{
		case TextBaseline::Top:
			if (sctx.capheight)
				return sctx.capheight + (sctx.size - sctx.capheight) * 0.5f;
			return sctx.asc * (sctx.size / (sctx.asc - sctx.desc));
		case TextBaseline::Middle:
			if (sctx.capheight)
				return sctx.capheight * 0.5f;
			return (sctx.asc + sctx.desc) * 0.5f * (sctx.size / (sctx.asc - sctx.desc));
		case TextBaseline::Bottom:
			if (sctx.capheight)
				return -(sctx.size - sctx.capheight) * 0.5f;
			return sctx.desc * (sctx.size / (sctx.asc - sctx.desc));
		}
	}
	return 0;
}

AABB2f TextLineGenerateQuadsUntransformed(
	Array<ImageQuad>& retQuads,
	Font* font,
	float size,
	StringView text)
{
	return TextLineGenerateQuads(retQuads, font, size, 0, 0, text);
}

AABB2f TextLineGenerateQuads(
	Array<ImageQuad>& retQuads,
	Font* font,
	float size,
	float x,
	float y,
	StringView text,
	TextHAlign align,
	TextBaseline baseline)
{
	if (size <= 0)
		return { x, y, x, y };

	//if (align != TextHAlign::Left)
	//	x -= GetTextWidth(font, size, text) * (float(align) * 0.5f);
	float scale = GetTextResolutionScale();
	float invScale = 1.0f / scale;
	auto& sctx = font->GetSizeContext(int(roundf(size * scale)));
	y += BaselineToYOff(sctx, baseline) * invScale;
	x = roundf(x * scale) * invScale;
	y = roundf(y * scale) * invScale;
	float x0 = x;
	float x1 = x;
	float y0 = y - BaselineToYOff(sctx, TextBaseline::Top);
	float y1 = y - BaselineToYOff(sctx, TextBaseline::Bottom);

	size_t startRetQuad = retQuads.Size();
	u32 prevChar = 0;
	UTF8Iterator it(text);
	for (;;)
	{
		uint32_t ch = it.Read();
		if (ch == UTF8Iterator::END)
			break;

		float kern = font->FindKerning(sctx.size, prevChar, ch);
		//if (::GetTickCount() % 1000 < 300) kern = 0;
		prevChar = ch;

		auto gv = font->FindGlyph(sctx, ch, true);

		float x0 = roundf(gv.xoff + kern) * invScale + x;
		float y0 = gv.yoff * invScale + y;
		float qx1 = x0 + gv.w * invScale;

		AABB2f posbox = { x0, y0, qx1, y0 + gv.h * invScale };

		retQuads.Append({ posbox, gv.img });
		//RectCol(posbox, { 255, 0, 0, 127 });

		x += roundf(gv.xadv + kern) * invScale;
		x1 = max(x1, qx1);
	}

	// move quads according to alignment
	if (align != TextHAlign::Left)
	{
		float w = x1 - x0;
		float diff = roundf(-w * (float(align) * 0.5f));
		for (auto quad = retQuads.begin() + startRetQuad, qend = retQuads.end(); quad != qend; quad++)
		{
			quad->box.x0 += diff;
			quad->box.x1 += diff;
		}
		x0 += diff;
		x1 += diff;
	}

	return { x0, y0, x1, y1 };
}

struct CharRange
{
	size_t start;
	size_t end;
};

AABB2f TextMultilineGenerateQuadsUntransformed(
	Array<ImageQuad>& retQuads,
	Font* font,
	float size,
	float maxWidth,
	float lineHeight,
	StringView text,
	TextHAlign halign)
{
	if (size <= 0)
		return {};

	float scale = GetTextResolutionScale();
	float invScale = 1.0f / scale;
	auto& sctx = font->GetSizeContext(int(roundf(size * scale)));

	bool pointAlignX = maxWidth < 0;
	if (pointAlignX)
		maxWidth = 0;

	maxWidth = roundf(maxWidth * scale) * invScale;
	lineHeight = roundf(lineHeight * scale) * invScale;

	float y = 0;
	float yo = roundf(BaselineToYOff(sctx, TextBaseline::Top) + (lineHeight - size) / 2 * scale) * invScale;
	float ya = roundf((y + yo) * scale) * invScale;

	//descLines.Clear();
	//descLines.Append({ 0, 0 });
	CharRange lastLine = { 0, 0 };
	size_t lastQuadOff = 0;

	size_t lastBreakPos = 0;
	size_t lastBreakQuadOff = 0;
	float widthToLastBreak = 0;
	float widthSoFar = 0;
	float maxMeasuredWidth = 0;
	u32 prevCh = 0;

	ui::UTF8Iterator it(text);
	auto CommitLine = [&]()
	{
		maxMeasuredWidth = max(maxMeasuredWidth, widthSoFar);
		// align the last line
		if (halign != TextHAlign::Left)
		{
			float diff = maxWidth - widthSoFar;
			float off = roundf(diff * (float(halign) * 0.5f) * scale) * invScale;
			for (size_t i = lastQuadOff; i < retQuads.Size(); i++)
			{
				retQuads[i].box.x0 += off;
				retQuads[i].box.x1 += off;
			}
		}
	};
	for (;;)
	{
		uint32_t charStart = it.pos;
		uint32_t ch = it.Read();
		if (ch == ui::UTF8Iterator::END)
			break;
		//descLines.Last().end = it.pos;
		lastLine.end = it.pos;
		if (ch == '\n')
		{
			//descLines.Append({ it.pos, it.pos });
			CommitLine();
			lastLine = { it.pos, it.pos };
			lastQuadOff = retQuads.Size();
			lastBreakQuadOff = lastQuadOff;

			widthToLastBreak = 0;
			widthSoFar = 0;

			y += lineHeight;
			ya = roundf((y + yo) * scale) * invScale;
		}
		else
		{
			// TODO full unicode?
			if (ch == ' ' || prevCh == ' ')
			{
				widthToLastBreak = widthSoFar;
				lastBreakPos = charStart;
				lastBreakQuadOff = retQuads.Size();
			}

			float kern = font->FindKerning(sctx.size, prevCh, ch);
			auto gv = font->FindGlyph(sctx, ch, true);
			float w = widthSoFar + int(roundf((gv.xadv + kern) * invScale));

			float x0 = roundf(gv.xoff + kern) * invScale + widthSoFar;
			float y0 = gv.yoff * invScale + ya;

			AABB2f posbox = { x0, y0, x0 + gv.w * invScale, y0 + gv.h * invScale };

			if (gv.w && gv.h)
				retQuads.Append({ posbox, gv.img });

			widthSoFar = w;
			if (w > maxWidth && lastQuadOff + 1 < retQuads.Size())
			{
				size_t breakQuad;
				size_t breakChar;
				// the last section overflowed
				if (lastBreakQuadOff > lastQuadOff)
				{
					breakQuad = lastBreakQuadOff;
					breakChar = lastBreakPos;
				}
				else // the last character overflowed
				{
					breakQuad = retQuads.Size() - 1;
					breakChar = charStart;
				}
				if (lastQuadOff < breakQuad)
				{
					float lineW = retQuads[breakQuad - 1].box.x1;
					maxMeasuredWidth = max(maxMeasuredWidth, lineW);
					// align the last line
					if (halign != TextHAlign::Left)
					{
						float diff = maxWidth - lineW;
						float off = roundf(diff * (float(halign) * 0.5f) * scale) * invScale;
						for (size_t i = lastQuadOff; i < breakQuad; i++)
						{
							retQuads[i].box.x0 += off;
							retQuads[i].box.x1 += off;
						}
					}
				}
				float oldX = 0;
				if (breakQuad < retQuads.Size())
				{
					oldX = retQuads[breakQuad].box.x0;
					for (size_t i = breakQuad; i < retQuads.Size(); i++)
					{
						auto& quad = retQuads[i];
						quad.box.x0 -= oldX;
						quad.box.x1 -= oldX;
						quad.box.y0 += lineHeight;
						quad.box.y1 += lineHeight;
					}
					widthSoFar -= oldX;
					widthToLastBreak = max(0.f, widthToLastBreak - oldX);
				}
				else
				{
					widthSoFar = 0;
					widthToLastBreak = 0;
				}
				y += lineHeight;
				ya = roundf((y + yo) * scale) * invScale;
				lastQuadOff = breakQuad;
				lastBreakQuadOff = breakQuad;
				lastLine.start = breakChar;
			}
		}
		prevCh = ch;
	}

	//if (descLines.Last().start == descLines.Last().end && (descLines.Size() == 1 || !descLines[descLines.Size() - 2].sv(s).ends_with("\n")))
	//	descLines.RemoveLast();
	if (lastLine.start != lastLine.end)
	{
		CommitLine();
		y += lineHeight;
	}

	AABB2f ret;
	ret.y0 = 0;
	ret.y1 = y;// + lineHeight;
	if (pointAlignX)
	{
		// TODO measure the width of all the lines
	}
	else
	{
		ret.x0 = 0;
		ret.x1 = maxWidth;
	}
	return ret;
}

void ImageQuadsCol(ArrayView<ImageQuad> quads, Color4b color, AABB2f* clipBox)
{
	ImageQuadsColOffset(quads, {}, color, clipBox);
}

void ImageQuadsColOffset(ArrayView<ImageQuad> quads, Vec2f offset, Color4b color, AABB2f* clipBox)
{
	for (auto& quad : quads)
	{
		AABB2f box = quad.box + offset;
		if (clipBox)
		{
			if (clipBox->Overlaps(box))
			{
				if (clipBox->Contains(box))
				{
					draw::RectColTex(box, color, quad.image);
				}
				else
				{
					AABB2f clipped = box.Intersect(*clipBox);
					AABB2f uvbox = { box.InverseLerp(clipped.GetMin()), box.InverseLerp(clipped.GetMax()) };
					draw::RectColTex(clipped, color, quad.image, uvbox);
				}
			}
		}
		else
		{
			draw::RectColTex(box, color, quad.image);
		}
	}
}

static Array<ImageQuad> g_tmpTextQuads;
void TextLine(Font* font, float size, float x, float y, StringView text, Color4b color, TextHAlign align, TextBaseline baseline, AABB2f* clipBox)
{
	if (clipBox && !clipBox->IsValid())
		return;
	if (size <= 0)
		return;

#if 0
	if (align != TextHAlign::Left)
		x -= GetTextWidth(font, size, text) * (float(align) * 0.5f);
	float scale = GetTextResolutionScale();
	float invScale = 1.0f / scale;
	auto& sctx = font->GetSizeContext(int(roundf(size * scale)));
	y += BaselineToYOff(sctx, baseline) * invScale;
	x = roundf(x * scale) * invScale;
	y = roundf(y * scale) * invScale;

	u32 prevChar = 0;
	UTF8Iterator it(text);
	for (;;)
	{
		uint32_t ch = it.Read();
		if (ch == UTF8Iterator::END)
			break;

		float kern = font->FindKerning(sctx.size, prevChar, ch);
		//if (::GetTickCount() % 1000 < 300) kern = 0;
		prevChar = ch;
		auto gv = font->FindGlyph(sctx, ch, true);
		float x0 = gv.xoff * invScale + x + kern;
		float y0 = gv.yoff * invScale + y;
		AABB2f posbox = { x0, y0, x0 + gv.w * invScale, y0 + gv.h * invScale };
		if (clipBox)
		{
			if (clipBox->Overlaps(posbox))
			{
				if (clipBox->Contains(posbox))
				{
					draw::RectColTex(posbox, color, gv.img);
				}
				else
				{
					AABB2f clipped = posbox.Intersect(*clipBox);
					AABB2f uvbox = { posbox.InverseLerp(clipped.GetMin()), posbox.InverseLerp(clipped.GetMax()) };
					draw::RectColTex(clipped, color, gv.img, uvbox);
				}
			}
		}
		else
		{
			draw::RectColTex(posbox, color, gv.img);
		}
		x += roundf(gv.xadv + kern) * invScale;
	}
#else
	g_tmpTextQuads.Clear();
	TextLineGenerateQuads(g_tmpTextQuads, font, size, x, y, text, align, baseline);
	ImageQuadsCol(g_tmpTextQuads, color, clipBox);
#endif
}

void TextMultiline(
	Font* font,
	float size,
	AABB2f rect,
	float lineHeight,
	StringView text,
	Color4b color,
	TextHAlign halign,
	TextVAlign valign,
	AABB2f* clipBox)
{
	g_tmpTextQuads.Clear();
	AABB2f box = TextMultilineGenerateQuadsUntransformed(g_tmpTextQuads, font, size, rect.GetWidth(), lineHeight, text, halign);
	Vec2f off = rect.GetMin();
	if (valign == TextVAlign::Bottom)
	{
		off.y += rect.y1 - box.y1;
	}
	else if (valign == TextVAlign::Middle)
	{
		off.y += rect.GetCenterY() - box.GetCenterY();
	}
	off = off.CastRounded<float>();
	ImageQuadsColOffset(g_tmpTextQuads, off, color, clipBox);
	//draw::RectCol(box + off, { 255, 0, 0, 127 });
}

} // draw
} // ui
