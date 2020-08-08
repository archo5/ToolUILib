
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb_truetype.h"

#include "Font.h"
#include "../Render/Render.h"

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
static std::unordered_map<FontKey, Font*, FontKey::Hasher> g_loadedFonts;

struct GlyphValue
{
	ui::draw::Texture* tex = nullptr;
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
		std::unordered_map<uint32_t, GlyphValue> glyphMap;
	};

	~Font()
	{
		g_loadedFonts.erase(key);
	}

	bool LoadFromPath(const char* path)
	{
		FILE* f = fopen(path, "rb");
		if (!f)
		{
			return false;
		}
		
		fseek(f, 0, SEEK_END);
		size_t len = ftell(f);
		data.resize(len);

		fseek(f, 0, SEEK_SET);
		if (len != fread(&data[0], 1, len, f))
		{
			return false;
		}

		return InitFromMemory();
	}

	bool InitFromMemory()
	{
		if (!stbtt_InitFont(&info, data.data(), 0))
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
		if (it != sctx.glyphMap.end() && it->second.tex)
			return it->second;

		int glyphID = stbtt_FindGlyphIndex(&info, codepoint);
		float scale = stbtt_ScaleForMappingEmToPixels(&info, sctx.size);

		GlyphValue* gv = nullptr;
		if (it == sctx.glyphMap.end())
		{
			int xadv = 0, lsb = 0, x0, y0, x1, y1;
			stbtt_GetGlyphHMetrics(&info, glyphID, &xadv, &lsb);
			stbtt_GetGlyphBitmapBox(&info, glyphID, scale, scale, &x0, &y0, &x1, &y1);

			gv = &sctx.glyphMap[codepoint];
			gv->xadv = xadv * scale;
			gv->xoff = x0;
			gv->yoff = y0;
			gv->w = x1 - x0;
			gv->h = y1 - y0;
		}
		else
			gv = &it->second;

		if (needTex && !gv->tex)
		{
			int x, y, w, h;
			auto* bitmap = stbtt_GetGlyphBitmap(&info, scale, scale, glyphID, &w, &h, &x, &y);
			gv->tex = ui::draw::TextureCreateA8(w, h, bitmap);
			stbtt_FreeBitmap(bitmap, nullptr);
		}

		return *gv;
	}

	FontKey key;
	std::vector<uint8_t> data;
	stbtt_fontinfo info;
	std::unordered_map<int, SizeContext> sizes;
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

static std::vector<uint8_t> FindFontDataByName(const char* name, int weight, bool italic)
{
	FontEnumData fed = { weight, italic };
	HDC dc = GetDC(nullptr);
	EnumFontFamiliesA(dc, name, _FontEnumCallback, (LPARAM)&fed);
	HFONT font = CreateFontIndirectA(&fed.outFont);
	SelectObject(dc, font);
	DWORD reqSize = GetFontData(dc, 0, 0, nullptr, 0);
	std::vector<uint8_t> data;
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
		return it->second;
	return nullptr;
}

Font* GetFontByPath(const char* path)
{
	FontKey key = { path, -1, false };
	if (auto* f = FindExistingFont(key))
		return f;
	auto* font = new ui::Font;
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
		ui::draw::RectColTex(gv.xoff, gv.yoff, gv.xoff + gv.w, gv.yoff + gv.h, color, gv.tex);
		x += gv.xadv;
	}
}

} // draw

// TEMPORARY to resolve C++ function finding issues
float GetTextWidth(const char* text, size_t num)
{
	return ::GetTextWidth(text, num);
}

} // ui


ui::Font* g_font;

void InitFont()
{
	g_font = ui::GetFont(ui::FONT_FAMILY_SANS_SERIF, ui::FONT_WEIGHT_NORMAL, false);
}

float GetTextWidth(const char* text, size_t num)
{
	return ui::GetTextWidth(g_font, GetFontHeight(), num == SIZE_MAX ? StringView(text) : StringView(text, num));
}

float GetFontHeight()
{
	return 12;
}

void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a)
{
	ui::draw::TextLine(g_font, GetFontHeight(), x, y, text, ui::Color4f(r, g, b, a));
}
