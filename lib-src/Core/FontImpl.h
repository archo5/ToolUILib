
#pragma once

#include "Font.h"

#include "FileSystem.h"
#include "HashMap.h"
#include "../Render/Render.h"

#include "../../ThirdParty/stb_truetype.h"


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


struct FontSizeContext
{
	int size = 0;
	float asc = 0, desc = 0, lgap = 0, xheight = 0, capheight = 0;
	HashMap<uint32_t, GlyphValue> glyphMap;
};

struct Font
{
	using SizeContext = FontSizeContext;

	FontKey key;
	BufferHandle data;
	stbtt_fontinfo info;
	HashMap<int, SizeContext> sizes;
	HashMap<u64, int> kerning;

	~Font();

	bool LoadFromPath(const char* path);
	bool InitFromMemory();

	SizeContext& GetSizeContext(int size);
	GlyphValue FindGlyph(SizeContext& sctx, uint32_t codepoint, bool needTex);
	float FindKerning(int size, u32 prevCP, u32 currCP);
};

} // ui
