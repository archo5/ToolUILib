
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

UI_API Font* GetFontByPath(const char* path);
UI_API Font* GetFontByName(const char* name, int weight = FONT_WEIGHT_NORMAL, bool italic = false);
UI_API Font* GetFontByFamily(const char* family, int weight = FONT_WEIGHT_NORMAL, bool italic = false);
UI_API Font* GetFont(const char* nameOrFamily, int weight = FONT_WEIGHT_NORMAL, bool italic = false);

UI_API float GetTextResolutionScale();
UI_API float SetTextResolutionScale(float ntrs);
UI_API float MultiplyTextResolutionScale(float ntrs);

UI_API float GetTextWidth(Font* font, int size, StringView text);
// ongoing measurement
UI_API void TextMeasureBegin(Font* font, int size);
UI_API void TextMeasureEnd();
UI_API void TextMeasureReset();
UI_API float TextMeasureAddChar(uint32_t ch);

enum class TextHAlign
{
	Left,
	Center,
	Right,
};

enum class TextVAlign
{
	Top,
	Middle,
	Bottom,
};

enum class TextBaseline
{
	Default, // no adjustment
	Top,
	Middle,
	Bottom,
};

struct UI_API CachedFontRef
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
