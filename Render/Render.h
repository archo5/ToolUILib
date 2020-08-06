
#pragma once

#include <stdint.h>
#include "../Core/Math.h"
#include "../Core/Image.h"


struct Sprite
{
	int ox0, oy0, ox1, oy1;
	int bx0, by0, bx1, by1;
};

enum EThemeElement
{
	TE_ButtonNormal,
	TE_ButtonHover,
	TE_ButtonPressed,
	TE_ButtonDisabled,
	TE_ButtonDownDisabled,

	TE_Panel,

	TE_TextboxNormal,
	TE_TextboxDisabled,

	TE_CheckBgrNormal,
	TE_CheckBgrHover,
	TE_CheckBgrPressed,
	TE_CheckBgrDisabled,

	TE_CheckMark,
	TE_CheckMarkDisabled,
	TE_CheckInd,
	TE_CheckIndDisabled,

	TE_RadioBgrNormal,
	TE_RadioBgrHover,
	TE_RadioBgrPressed,
	TE_RadioBgrDisabled,
	TE_RadioMark,
	TE_RadioMarkDisabled,

	TE_Selector32,
	TE_Selector16,
	TE_Selector12,
	TE_Outline,

	TE_TabNormal,
	TE_TabHover,
	TE_TabSelected,
	TE_TabPanel,

	TE_TreeTickClosedNormal,
	TE_TreeTickClosedHover,
	TE_TreeTickOpenNormal,
	TE_TreeTickOpenHover,

	TE__COUNT,
};


namespace ui {
namespace rhi {

struct Texture2D;

} // rhi
namespace draw {

constexpr bool DEFAULT_FILTERING = true;

namespace debug {

int GetAtlasTextureCount();
rhi::Texture2D* GetAtlasTexture(int n, int size[2]);

} // debug

struct Texture;
Texture* TextureCreateRGBA8(int w, int h, const void* data, bool filtering = DEFAULT_FILTERING);
Texture* TextureCreateRGBA8(int w, int h, int pitch, const void* data, bool filtering = DEFAULT_FILTERING);
Texture* TextureCreateA8(int w, int h, const void* data, bool filtering = DEFAULT_FILTERING);
void TextureAddRef(Texture* tex);
void TextureRelease(Texture* tex);

namespace internals {

void OnBeginDrawFrame();
void OnEndDrawFrame();

} // internals

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
void AALineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
void RectCol(float x0, float y0, float x1, float y1, Color4b col);
void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b);
void RectTex(float x0, float y0, float x1, float y1, Texture* tex);
void RectTex(float x0, float y0, float x1, float y1, Texture* tex, float u0, float v0, float u1, float v1);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, Texture* tex);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, Texture* tex, float u0, float v0, float u1, float v1);
void RectColTex9Slice(const AABB<float>& outer, const AABB<float>& inner, Color4b col, Texture* tex, const AABB<float>& texouter, const AABB<float>& texinner);
void RectCutoutCol(const AABB<float>& rect, const AABB<float>& cutout, Color4b col);

void PushScissorRect(int x0, int y0, int x1, int y1);
void PopScissorRect();
void _ResetScissorRectStack(int x0, int y0, int x1, int y1);

} // draw
} // ui


void InitFont();
float GetTextWidth(const char* text, size_t num = SIZE_MAX);
float GetFontHeight();
void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a = 1);

extern ui::draw::Texture* g_themeTexture;
void InitTheme();
AABB<float> GetThemeElementBorderWidths(EThemeElement e);
void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1);
