
#include "Font.h"
#include "FileSystem.h"
#include "HashMap.h"
#include "../Render/Render.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../../ThirdParty/stb_truetype.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>


namespace ui {

struct FontKey
{
	std::string name;
	int weight;
	bool italic;

	bool operator == (const FontKey& o) const
	{
		return name == o.name && weight == o.weight && italic == o.italic;
	}
};
inline size_t HashValue(const FontKey& k)
{
	size_t h = HashValue(k.name);
	h *= 131;
	h ^= HashValue(k.weight);
	h *= 131;
	h ^= HashValue(k.italic);
	return h;
}
static HashMap<FontKey, Font*> g_loadedFonts;

struct GlyphValue
{
	draw::ImageHandle img;
	uint16_t w;
	uint16_t h;
	int16_t xoff;
	int16_t yoff;
	int16_t xadv;
};

struct Font
{
	struct SizeContext
	{
		int size = 0;
		float asc = 0, desc = 0, lgap = 0;
		HashMap<uint32_t, GlyphValue> glyphMap;
	};

	~Font()
	{
		g_loadedFonts.Remove(key);
	}

	bool LoadFromPath(const char* path)
	{
		data = FSReadBinaryFile(path).data;
		if (!data)
			return false;

		return InitFromMemory();
	}

	bool InitFromMemory()
	{
		if (!data)
			return false;
		if (!stbtt_InitFont(&info, (const unsigned char*)data->Data(), 0))
			return false;
		return true;
	}

	SizeContext& GetSizeContext(int size)
	{
		auto& sctx = sizes[size];
		sctx.size = size;

		float scale = stbtt_ScaleForMappingEmToPixels(&info, float(size));
		int asc, desc, lgap;
		stbtt_GetFontVMetrics(&info, &asc, &desc, &lgap);
		sctx.asc = asc * scale;
		sctx.desc = desc * scale;
		sctx.lgap = lgap * scale;
		//printf("INFO size=%d em=%g asc=%g desc=%g lgap=%g\n", sctx.size, sctx.asc, sctx.desc, sctx.lgap);

		return sctx;
	}

	GlyphValue FindGlyph(SizeContext& sctx, uint32_t codepoint, bool needTex)
	{
		GlyphValue* gv = sctx.glyphMap.GetValuePtr(codepoint);
		if (gv && (!needTex || gv->img))
			return *gv;

		int glyphID = stbtt_FindGlyphIndex(&info, codepoint);
		float scale = stbtt_ScaleForMappingEmToPixels(&info, float(sctx.size));

		if (!gv)
		{
			int xadv = 0, lsb = 0, x0, y0, x1, y1;
			stbtt_GetGlyphHMetrics(&info, glyphID, &xadv, &lsb);
			stbtt_GetGlyphBitmapBox(&info, glyphID, scale, scale, &x0, &y0, &x1, &y1);
			//y0 += sctx.size;

			gv = &sctx.glyphMap[codepoint];
			gv->xadv = int16_t(roundf(xadv * scale));
			gv->xoff = int16_t(x0);
			gv->yoff = int16_t(y0);
			gv->w = x1 - x0;
			gv->h = y1 - y0;
		}

		if (needTex && !gv->img)
		{
			int x, y, w, h;
			auto* bitmap = stbtt_GetGlyphBitmap(&info, scale, scale, glyphID, &w, &h, &x, &y);
			gv->img = draw::ImageCreateA8(w, h, bitmap, draw::TexFlags::Packed);
			stbtt_FreeBitmap(bitmap, nullptr);
		}

		return *gv;
	}

	float FindKerning(int size, u32 prevCP, u32 currCP)
	{
		float scale = stbtt_ScaleForMappingEmToPixels(&info, float(size));

		u64 key = (u64(prevCP) << 32) | currCP;
		if (auto* p = kerning.GetValuePtr(key))
			return *p * scale;
		int kern = stbtt_GetCodepointKernAdvance(&info, prevCP, currCP);
		kerning.Insert(key, kern);
		return kern * scale;
	}

	FontKey key;
	BufferHandle data;
	stbtt_fontinfo info;
	HashMap<int, SizeContext> sizes;
	HashMap<u64, int> kerning;
};


struct FontEnumData
{
	int weight;
	bool italic;
	int lastProximity = INT_MAX;
	LOGFONTA outFont;

	int CalcProximity(const LOGFONTA* font) const
	{
		return abs(weight - font->lfWeight) + abs(int(italic) - int(font->lfItalic != 0)) * 2000;
	}
};

int CALLBACK _FontEnumCallback(const LOGFONTA* lf, const TEXTMETRICA* ntm, DWORD fontType, LPARAM userdata)
{
	auto* fed = (FontEnumData*)userdata;
	int prox = fed->CalcProximity(lf);
	if (prox < fed->lastProximity)
	{
		fed->outFont = *lf;
		fed->lastProximity = prox;
	}
	return 1;
}

static BufferHandle FindFontDataByName(const char* name, int weight, bool italic)
{
	FontEnumData fed = { weight, italic };
	HDC dc = GetDC(nullptr);
	EnumFontFamiliesA(dc, name, _FontEnumCallback, (LPARAM)&fed);
	HFONT font = CreateFontIndirectA(&fed.outFont);
	SelectObject(dc, font);
	DWORD reqSize = GetFontData(dc, 0, 0, nullptr, 0);

	auto ret = AsRCHandle(new OwnedMemoryBuffer);
	ret->Alloc(reqSize);
	DWORD readSize = GetFontData(dc, 0, 0, ret->data, reqSize);
	ret->size = readSize;
	ReleaseDC(nullptr, dc);
	return ret;
}

static Font* FindExistingFont(const FontKey& key)
{
	if (auto* pp = g_loadedFonts.GetValuePtr(key))
		return *pp;
	return nullptr;
}

Font* GetFontByPath(const char* path)
{
	FontKey key = { path, -1, false };
	if (auto* f = FindExistingFont(key))
		return f;
	auto* font = new Font;
	font->LoadFromPath(path);
	font->key = key;
	g_loadedFonts[key] = font;
	return font;
}

Font* GetFontByName(const char* name, int weight, bool italic)
{
	FontKey key = { name, weight, italic };
	if (auto* f = FindExistingFont(key))
		return f;
	Font* font = new Font;
	font->data = FindFontDataByName(name, weight, italic);
	font->InitFromMemory();
	font->key = key;
	g_loadedFonts[key] = font;
	return font;
}

Font* GetFontByFamily(const char* family, int weight, bool italic)
{
	if (strcmp(family, FONT_FAMILY_SANS_SERIF) == 0)
		return GetFontByName("Segoe UI", weight, italic);
	if (strcmp(family, FONT_FAMILY_SERIF) == 0)
		return GetFontByName("Times New Roman", weight, italic);
	if (strcmp(family, FONT_FAMILY_MONOSPACE) == 0)
		return GetFontByName("Consolas", weight, italic);
	return nullptr;
}

Font* GetFont(const char* nameOrFamily, int weight, bool italic)
{
	if (auto* font = GetFontByFamily(nameOrFamily, weight, italic))
		return font;
	return GetFontByName(nameOrFamily, weight, italic);
}


static float g_textResScale = 1;

float GetTextResolutionScale()
{
	return g_textResScale;
}

float SetTextResolutionScale(float ntrs)
{
	float r = g_textResScale;
	g_textResScale = ntrs;
	return r;
}

float MultiplyTextResolutionScale(float ntrs)
{
	return SetTextResolutionScale(g_textResScale * ntrs);
}


float GetTextWidth(Font* font, int size, StringView text)
{
	if (size <= 0)
		return 0;

	auto& sctx = font->GetSizeContext(int(roundf(size * g_textResScale)));
	float invScale = 1.0f / g_textResScale;
	int out = 0;
	u32 prevChar = 0;

	UTF8Iterator it(text);
	for (;;)
	{
		uint32_t ch = it.Read();
		if (ch == UTF8Iterator::END)
			break;

		float kern = font->FindKerning(sctx.size, prevChar, ch);
		prevChar = ch;
		out += int(roundf((font->FindGlyph(sctx, ch, false).xadv + kern) * invScale));
	}
	return float(out);
}

static Font* g_tmFont;
static Font::SizeContext* g_tmSizeCtx;
static float g_tmInvScale;
static int g_tmWidth;
static u32 g_tmPrevChar;
void TextMeasureBegin(Font* font, int size)
{
	g_tmFont = font;
	g_tmSizeCtx = &font->GetSizeContext(int(roundf(size * g_textResScale)));
	g_tmInvScale = 1.0f / g_textResScale;
	g_tmWidth = 0;
	g_tmPrevChar = 0;
}

void TextMeasureEnd()
{
	g_tmFont = nullptr;
	g_tmSizeCtx = nullptr;
	g_tmInvScale = 0;
	g_tmPrevChar = 0;
}

void TextMeasureReset()
{
	g_tmWidth = 0;
	g_tmPrevChar = 0;
}

float TextMeasureAddChar(uint32_t ch)
{
	float kern = g_tmFont->FindKerning(g_tmSizeCtx->size, g_tmPrevChar, ch);
	g_tmPrevChar = ch;
	return float(g_tmWidth += int(roundf((g_tmFont->FindGlyph(*g_tmSizeCtx, ch, false).xadv + kern) * g_tmInvScale)));
}


namespace draw {

static float BaselineToYOff(Font::SizeContext& sctx, TextBaseline baseline)
{
	if (baseline != TextBaseline::Default)
	{
		// https://drafts.csswg.org/css-inline/#baseline-synthesis-em
		switch (baseline)
		{
		case TextBaseline::Top:
			return sctx.asc * (sctx.size / (sctx.asc - sctx.desc));
		case TextBaseline::Middle:
			return (sctx.asc + sctx.desc) * 0.5f * (sctx.size / (sctx.asc - sctx.desc));
		case TextBaseline::Bottom:
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
	float scale = g_textResScale;
	float invScale = 1.0f / scale;
	auto& sctx = font->GetSizeContext(int(roundf(size * scale)));
	y += BaselineToYOff(sctx, baseline) * invScale;
	x = roundf(x * scale) * invScale;
	y = roundf(y * scale) * invScale;
	float x0 = x;
	float y0 = y + BaselineToYOff(sctx, TextBaseline::Top);
	float y1 = y + BaselineToYOff(sctx, TextBaseline::Bottom);

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

		AABB2f posbox = { x0, y0, x0 + gv.w * invScale, y0 + gv.h * invScale };

		retQuads.Append({ posbox, gv.img });
		//RectCol(posbox, { 255, 0, 0, 127 });

		x += roundf(gv.xadv + kern) * invScale;
	}

	// move quads according to alignment
	if (align != TextHAlign::Left)
	{
		float w = x - x0;
		float diff = roundf(-w * (float(align) * 0.5f));
		for (auto quad = retQuads.begin() + startRetQuad, qend = retQuads.end(); quad != qend; quad++)
		{
			quad->box.x0 += diff;
			quad->box.x1 += diff;
		}
		x += diff;
		x0 += diff;
	}

	return { x0, y0, x, y1 };
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

	float scale = g_textResScale;
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
	float scale = g_textResScale;
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


Font* CachedFontRef::GetCachedFont(const char* nameOrFamily, int weight, bool italic) const
{
	if (_cachedFont &&
		_cacheKeyNameOrFamily == nameOrFamily &&
		_cacheKeyWeight == weight &&
		_cacheKeyItalic == italic)
		return _cachedFont;

	_cachedFont = GetFont(nameOrFamily, weight, italic);
	_cacheKeyNameOrFamily = nameOrFamily;
	_cacheKeyWeight = weight;
	_cacheKeyItalic = italic;
	return _cachedFont;
}

} // ui
