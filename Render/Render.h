
#pragma once

#include <stdint.h>
#include "../Core/Math.h"
#include "../Core/Image.h"
#include "../Core/Memory.h"


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

// texture flags
enum class TexFlags : uint8_t
{
	None = 0,
	Filter = 1 << 0,
	Repeat = 1 << 1,
};
constexpr TexFlags TF_None = TexFlags::None;
constexpr TexFlags TF_Filter = TexFlags::Filter;
constexpr TexFlags TF_Repeat = TexFlags::Repeat;
inline TexFlags operator | (TexFlags a, TexFlags b)
{
	return TexFlags(int(a) | int(b));
}
inline TexFlags operator & (TexFlags a, TexFlags b)
{
	return TexFlags(int(a) & int(b));
}

namespace debug {

int GetAtlasTextureCount();
rhi::Texture2D* GetAtlasTexture(int n, int size[2]);

} // debug

struct Texture;
Texture* TextureCreateRGBA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
Texture* TextureCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags = TexFlags::None);
Texture* TextureCreateA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
void TextureAddRef(Texture* tex);
void TextureRelease(Texture* tex);
rhi::Texture2D* TextureGetInternal(Texture* tex);

namespace internals {

void InitResources();
void FreeResources();
void OnBeginDrawFrame();
void OnEndDrawFrame();
void Flush();

} // internals

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
void AALineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
void LineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel = true);
void AALineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel = true);
void CircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel = true);
void AACircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel = true);
void RectCol(float x0, float y0, float x1, float y1, Color4b col);
void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b);
void RectTex(float x0, float y0, float x1, float y1, Texture* tex);
void RectTex(float x0, float y0, float x1, float y1, Texture* tex, float u0, float v0, float u1, float v1);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, Texture* tex);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, Texture* tex, float u0, float v0, float u1, float v1);
void RectColTex9Slice(const AABB2f& outer, const AABB2f& inner, Color4b col, Texture* tex, const AABB2f& texouter, const AABB2f& texinner);
void RectCutoutCol(const AABB2f& rect, const AABB2f& cutout, Color4b col);

bool PushScissorRect(int x0, int y0, int x1, int y1);
void PopScissorRect();
void _ResetScissorRectStack(int x0, int y0, int x1, int y1);
AABB2f GetCurrentScissorRectF();

} // draw
} // ui


// TODO move
void InitTheme();
ui::AABB2f GetThemeElementBorderWidths(EThemeElement e);
void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1);
