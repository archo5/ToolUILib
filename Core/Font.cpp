
#include "Font.h"
#include "FileSystem.h"
#include "HashTable.h"
#include "../Render/Render.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "../ThirdParty/stb_truetype.h"

#include <vector>

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
	struct Hasher
	{
		size_t operator () (const FontKey& k) const
		{
			size_t h = std::hash<std::string>()(k.name);
			h *= 131;
			h ^= std::hash<int>()(k.weight);
			h *= 131;
			h ^= std::hash<bool>()(k.italic);
			return h;
		}
	};
};
static HashMap<FontKey, Font*, FontKey::Hasher> g_loadedFonts;

struct GlyphValue
{
	draw::ImageHandle img;
	uint16_t w;
	uint16_t h;
	float xoff;
	float yoff;
	float xadv;
};

struct Font
{
	struct SizeContext
	{
		int size = 0;
		HashMap<uint32_t, GlyphValue> glyphMap;
	};

	~Font()
	{
		g_loadedFonts.erase(key);
	}

	bool LoadFromPath(const char* path)
	{
		data = ReadBinaryFile(path);
		if (data.empty())
			return false;

		return InitFromMemory();
	}

	bool InitFromMemory()
	{
		if (!stbtt_InitFont(&info, (const unsigned char*)data.data(), 0))
			return false;
		return true;
	}

	SizeContext& GetSizeContext(int size)
	{
		auto& sctx = sizes[size];
		sctx.size = size;
		return sctx;
	}

	GlyphValue FindGlyph(SizeContext& sctx, uint32_t codepoint, bool needTex)
	{
		auto it = sctx.glyphMap.find(codepoint);
		if (it != sctx.glyphMap.end() && (!needTex || it->value.img))
			return it->value;

		int glyphID = stbtt_FindGlyphIndex(&info, codepoint);
		float scale = stbtt_ScaleForMappingEmToPixels(&info, float(sctx.size));

		GlyphValue* gv = nullptr;
		if (it == sctx.glyphMap.end())
		{
			int xadv = 0, lsb = 0, x0, y0, x1, y1;
			stbtt_GetGlyphHMetrics(&info, glyphID, &xadv, &lsb);
			stbtt_GetGlyphBitmapBox(&info, glyphID, scale, scale, &x0, &y0, &x1, &y1);

			gv = &sctx.glyphMap[codepoint];
			gv->xadv = xadv * scale;
			gv->xoff = float(x0);
			gv->yoff = float(y0);
			gv->w = x1 - x0;
			gv->h = y1 - y0;
		}
		else
			gv = &it->value;

		if (needTex && !gv->img)
		{
			int x, y, w, h;
			auto* bitmap = stbtt_GetGlyphBitmap(&info, scale, scale, glyphID, &w, &h, &x, &y);
			gv->img = draw::ImageCreateA8(w, h, bitmap, draw::TexFlags::Packed);
			stbtt_FreeBitmap(bitmap, nullptr);
		}

		return *gv;
	}

	FontKey key;
	std::string data;
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

static std::string FindFontDataByName(const char* name, int weight, bool italic)
{
	FontEnumData fed = { weight, italic };
	HDC dc = GetDC(nullptr);
	EnumFontFamiliesA(dc, name, _FontEnumCallback, (LPARAM)&fed);
	HFONT font = CreateFontIndirectA(&fed.outFont);
	SelectObject(dc, font);
	DWORD reqSize = GetFontData(dc, 0, 0, nullptr, 0);
	std::string data;
	data.resize(reqSize);
	DWORD readSize = GetFontData(dc, 0, 0, &data[0], reqSize);
	data.resize(readSize);
	ReleaseDC(nullptr, dc);
	return data;
}

static Font* FindExistingFont(const FontKey& key)
{
	auto it = g_loadedFonts.find(key);
	if (it != g_loadedFonts.end())
		return it->value;
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

float GetTextWidth(Font* font, int size, StringView text)
{
	auto& sctx = font->GetSizeContext(size);
	float out = 0;
	for (char c : text)
	{
		out += font->FindGlyph(sctx, (uint8_t)c, false).xadv;
	}
	return out;
}

namespace draw {

void TextLine(Font* font, int size, float x, float y, StringView text, Color4b color)
{
	auto& sctx = font->GetSizeContext(size);
	for (char ch : text)
	{
		auto gv = font->FindGlyph(sctx, (uint8_t)ch, true);
		gv.xoff = roundf(gv.xoff + x);
		gv.yoff = roundf(gv.yoff + y);
		draw::RectColTex(gv.xoff, gv.yoff, gv.xoff + gv.w, gv.yoff + gv.h, color, gv.img);
		x += gv.xadv;
	}
}

} // draw


Font* CachedFontRef::GetCachedFont(const char* nameOrFamily, int weight, bool italic)
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


// TODO
Font* g_font;

void InitFont()
{
	g_font = GetFont(FONT_FAMILY_SANS_SERIF, FONT_WEIGHT_NORMAL, false);
}

void FreeFont()
{
	delete g_font;
	g_font = nullptr;
	g_loadedFonts.dealloc();
}

float GetTextWidth(const char* text, size_t num)
{
	return GetTextWidth(g_font, GetFontHeight(), num == SIZE_MAX ? StringView(text) : StringView(text, num));
}

float GetFontHeight()
{
	return 12;
}

void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a)
{
	draw::TextLine(g_font, GetFontHeight(), x, y, text, Color4f(r, g, b, a));
}

} // ui
