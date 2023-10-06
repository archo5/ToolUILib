
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
		int kern = stbtt_GetCodepointKernAdvance(&info, prevCP, currCP);
		return kern * scale;
	}

	FontKey key;
	BufferHandle data;
	stbtt_fontinfo info;
	HashMap<int, SizeContext> sizes;
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

void TextLine(Font* font, int size, float x, float y, StringView text, Color4b color, TextHAlign align, TextBaseline baseline, AABB2f* clipBox)
{
	if (clipBox && !clipBox->IsValid())
		return;

	if (align != TextHAlign::Left)
		x -= GetTextWidth(font, size, text) * (float(align) * 0.5f);
	float scale = g_textResScale;
	float invScale = 1.0f / scale;
	auto& sctx = font->GetSizeContext(int(roundf(size * scale)));
	if (baseline != TextBaseline::Default)
	{
		// https://drafts.csswg.org/css-inline/#baseline-synthesis-em
		switch (baseline)
		{
		case TextBaseline::Top:
			y += sctx.asc * (sctx.size / (sctx.asc - sctx.desc)) * invScale;
			break;
		case TextBaseline::Middle:
			y += (sctx.asc + sctx.desc) * 0.5f * (sctx.size / (sctx.asc - sctx.desc)) * invScale;
			break;
		case TextBaseline::Bottom:
			y += sctx.desc * (sctx.size / (sctx.asc - sctx.desc)) * invScale;
			break;
		}
	}
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
