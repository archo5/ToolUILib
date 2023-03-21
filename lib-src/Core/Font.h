
#pragma once

#include "String.h"
#include "Image.h"

#include <string>


namespace ui {

struct Font;

constexpr int FONT_WEIGHT_NORMAL = 400;
constexpr int FONT_WEIGHT_BOLD = 700;

static constexpr const char* FONT_FAMILY_SANS_SERIF = "sans-serif";
static constexpr const char* FONT_FAMILY_SERIF = "serif";
static constexpr const char* FONT_FAMILY_MONOSPACE = "monospace";

Font* GetFontByPath(const char* path);
Font* GetFontByName(const char* name, int weight = FONT_WEIGHT_NORMAL, bool italic = false);
Font* GetFontByFamily(const char* family, int weight = FONT_WEIGHT_NORMAL, bool italic = false);
Font* GetFont(const char* nameOrFamily, int weight = FONT_WEIGHT_NORMAL, bool italic = false);

float GetTextResolutionScale();
float SetTextResolutionScale(float ntrs);
float MultiplyTextResolutionScale(float ntrs);

float GetTextWidth(Font* font, int size, StringView text);
// ongoing measurement
void TextMeasureBegin(Font* font, int size);
void TextMeasureEnd();
void TextMeasureReset();
float TextMeasureAddChar(uint32_t ch);

enum class TextBaseline
{
	Default, // no adjustment
	Top,
	Middle,
	Bottom,
};

namespace draw {
void TextLine(Font* font, int size, float x, float y, StringView text, Color4b color, TextBaseline baseline = TextBaseline::Default, AABB2f* clipBox = nullptr);
} // draw

struct CachedFontRef
{
	mutable Font* _cachedFont = nullptr;
	mutable std::string _cacheKeyNameOrFamily;
	mutable int _cacheKeyWeight = 0;
	mutable bool _cacheKeyItalic = false;

	CachedFontRef() {}
	CachedFontRef(CachedFontRef&&) {}
	CachedFontRef(const CachedFontRef&) {}
	CachedFontRef& operator = (CachedFontRef&&) { return *this; }
	CachedFontRef& operator = (const CachedFontRef&) { return *this; }

	Font* GetCachedFont(const char* nameOrFamily, int weight = FONT_WEIGHT_NORMAL, bool italic = false) const;
};

} // ui
