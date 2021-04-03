
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

float GetTextWidth(Font* font, int size, StringView text);

namespace draw {
void TextLine(Font* font, int size, float x, float y, StringView text, Color4b color);
} // draw

void InitFont();
void FreeFont();
float GetTextWidth(const char* text, size_t num = SIZE_MAX);
float GetFontHeight();
void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a = 1);

} // ui
