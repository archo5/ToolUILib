
#include "InlineLayout.h"

#include "Render.h"
#include "RenderText.h"

#include "../Core/FontImpl.h"


namespace ui {

namespace draw {
float BaselineToYOff(Font::SizeContext& sctx, TextBaseline baseline);
} // draw

void InlineLayout::Begin()
{
	quads.Clear();
	pieces.Clear();
	lines.Clear();

	curFont = nullptr;
	curFontSize = 0;
	curFontSizeCtx = nullptr;
	curLineHeight = 0;
	curTextResScale = 1;
	lastChar = 0;
}

void InlineLayout::SetFont(Font* font, int size, float lineHeight)
{
	if (font == curFont && size == curFontSize && lineHeight == curLineHeight)
		return;
	if (size <= 0)
		return;
	curFont = font;
	curFontSize = size;
	curTextResScale = GetTextResolutionScale();
	curFontSizeCtx = &font->GetSizeContext(int(roundf(size * curTextResScale)));
	curLineHeight = lineHeight;
	// reset since characters aren't guaranteed to fit
	lastChar = 0;
}

static bool IsBreakingSpace(u32 ch)
{
	return ch == ' ';
}

void InlineLayout::AddText(StringView text, Color4b col, IL_VAlign valign)
{
	Piece p;
	p.color = col;
	p.valign = valign;
	p.quadOff = quads.Size();

	float scale = curTextResScale;
	float invScale = 1.0f / scale;
	auto& sctx = *curFontSizeCtx;
	float x = 0;
	float x0 = 0;
	//float x1 = 0;
	float y0 = -draw::BaselineToYOff(sctx, TextBaseline::Top);
	float y1 = -draw::BaselineToYOff(sctx, TextBaseline::Bottom);

	p.textTop = y0;
	p.textBtm = y1;
	p.top = p.textTop - floorf((curLineHeight - curFontSize) / 2);
	p.btm = p.textBtm + ceilf((curLineHeight - curFontSize) / 2);

	size_t startRetQuad = quads.Size();
	bool prevBreaking = IsBreakingSpace(lastChar);
	bool isFirstChar = true;
	UTF8Iterator it(text);
	for (;;)
	{
		uint32_t ch = it.Read();
		if (ch == UTF8Iterator::END)
			break;

		bool curBreaking = IsBreakingSpace(ch);
		if (curBreaking != prevBreaking && !isFirstChar)
		{
			// commit the piece so far and start a new one
			p.quadCount = quads.Size() - p.quadOff;
			p.width = x - x0;
			pieces.Append(p);

			p.quadOff = quads.Size();
			x0 = x = 0;
		}
		isFirstChar = false;
		prevBreaking = curBreaking;

		float kern = curFont->FindKerning(sctx.size, lastChar, ch);
		//if (::GetTickCount() % 1000 < 300) kern = 0;
		lastChar = ch;

		auto gv = curFont->FindGlyph(sctx, ch, true);

		float x0 = roundf(gv.xoff + kern) * invScale + x;
		float y0 = gv.yoff * invScale;
		float qx1 = x0 + gv.w * invScale;

		AABB2f posbox = { x0, y0, qx1, y0 + gv.h * invScale };

		if (gv.img && gv.img->GetSize() != Size2i())
			quads.Append({ posbox, gv.img });
		//RectCol(posbox, { 255, 0, 0, 127 });

		x += roundf(gv.xadv + kern) * invScale;
		//x1 = max(x1, qx1);
	}

	p.quadCount = quads.Size() - p.quadOff;
	p.width = x - x0;
	pieces.Append(p);
}

void InlineLayout::AddImage(draw::IImage* image, Vec2f size, IL_VAlign valign)
{
	Piece p;
	{
		p.valign = valign;

		p.quadOff = quads.Size();
		p.quadCount = 1;

		p.width = size.x;
		p.top = -size.y;
		p.textTop = -size.y;
	}
	pieces.Append(p);

	draw::ImageQuad q;
	{
		q.image = image;
		q.box = { 0, -size.y, size.x, 0 };
	}
	quads.Append(q);

	lastChar = 0;
}

float InlineLayout::FinishLayout(float maxWidth)
{
	// split into lines
	lines.Clear();
	lines.Append({});
	float curLineW = 0;
	for (u32 i = 0; i < pieces.Size(); i++)
	{
		Piece& P = pieces[i];
		if (curLineW + P.width > maxWidth && !P.IsSpace())
		{
			curLineW = 0;
			lines.Append({ i, 0 });
		}
		lines.Last().pieceCount++;
		curLineW += P.width;
		if (!P.IsSpace())
			lines.Last().width = curLineW;
		// TODO split piece if too long for line
	}

	float totalHeight = 0;
	for (Line& L : lines)
	{
		// vertically align text alignments
		for (u32 i = L.pieceOff + 1, end = L.pieceOff + L.pieceCount; i < end; i++)
		{
			Piece& P = pieces[i];
			if (P.valign == IL_VAlign::Baseline || P.valign == IL_VAlign::TextTop || P.valign == IL_VAlign::TextBottom)
			{
			}
		}

		// measure line height so far for the text-aligned elements
		float lineTop = 0, lineBtm = 0;
		for (u32 i = L.pieceOff, end = L.pieceOff + L.pieceCount; i < end; i++)
		{
			Piece& P = pieces[i];
			if (P.valign == IL_VAlign::Baseline || P.valign == IL_VAlign::TextTop || P.valign == IL_VAlign::TextBottom)
			{
				lineTop = min(lineTop, P.top + P.yOff);
				lineBtm = max(lineBtm, P.btm + P.yOff);
			}
		}
		float lineMid = (lineTop + lineBtm) / 2;
		float lineHeight = lineBtm - lineTop;

		// expand line height around the middle if needed by non-text-aligned elements
		for (u32 i = L.pieceOff, end = L.pieceOff + L.pieceCount; i < end; i++)
		{
			Piece& P = pieces[i];
			if (P.valign == IL_VAlign::Top || P.valign == IL_VAlign::Bottom || P.valign == IL_VAlign::Middle)
			{
				float h = P.btm - P.top;
				if (h > lineHeight)
				{
					float hd = (h - lineHeight) / 2;
					lineTop -= ceilf(hd);
					lineBtm += floorf(hd);
					lineHeight = lineBtm - lineTop;
				}
			}
		}

		// vertically align to line box
		for (u32 i = L.pieceOff, end = L.pieceOff + L.pieceCount; i < end; i++)
		{
			Piece& P = pieces[i];
			if (P.valign == IL_VAlign::Top)
			{
				P.yOff = lineTop - P.top;
			}
			else if (P.valign == IL_VAlign::Bottom)
			{
				P.yOff = lineBtm - P.btm;
			}
			else if (P.valign == IL_VAlign::Middle)
			{
				P.yOff = lineMid - (P.top + P.btm) / 2;
			}
		}

		// generate quad offsets
		float pposX = 0;
		for (u32 i = L.pieceOff, end = L.pieceOff + L.pieceCount; i < end; i++)
		{
			Piece& P = pieces[i];
			Vec2f quadOff(pposX, roundf(P.yOff - lineTop + totalHeight));
			pposX += P.width;

			// move the quads so that layout origin is at 0,0
			for (u32 j = P.quadOff, jend = P.quadOff + P.quadCount; j < jend; j++)
			{
				quads[j].box += quadOff;
			}
		}

		L.lineTop = totalHeight;
		L.lineBtm = totalHeight + lineHeight;

		totalHeight += lineHeight;
	}

	curFont = nullptr;
	curFontSize = 0;
	curFontSizeCtx = nullptr;
	curLineHeight = 0;
	curTextResScale = 1;
	lastChar = 0;

	return totalHeight;
}

void InlineLayout::Render(Vec2f offset)
{
	for (auto& P : pieces)
	{
		for (auto& Q : quads.View().Subview(P.quadOff, P.quadCount))
		{
			ui::draw::RectColTex(Q.box + offset, P.color, Q.image);
		}
	}
}

} // ui
