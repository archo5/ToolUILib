
#pragma once

#include "../Core/Math.h"
#include "../Core/Image.h"
#include "../Core/Memory.h"
#include "../Core/RefCounted.h"


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

struct IImage : IRefCounted
{
	virtual uint16_t GetWidth() const = 0;
	virtual uint16_t GetHeight() const = 0;
	virtual rhi::Texture2D* GetInternalExclusive() const = 0;
};
using ImageHandle = RCHandle<IImage>;

ImageHandle ImageLoadFromFile(StringView path);

ImageHandle ImageCreateRGBA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateFromCanvas(const Canvas& c, TexFlags flags = TexFlags::None);

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
void RectTex(float x0, float y0, float x1, float y1, IImage* tex);
void RectTex(float x0, float y0, float x1, float y1, IImage* tex, float u0, float v0, float u1, float v1);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex, float u0, float v0, float u1, float v1);
void RectColTex9Slice(const AABB2f& outer, const AABB2f& inner, Color4b col, IImage* tex, const AABB2f& texouter, const AABB2f& texinner);
void RectCutoutCol(const AABB2f& rect, const AABB2f& cutout, Color4b col);

bool PushScissorRect(int x0, int y0, int x1, int y1);
void PopScissorRect();
void _ResetScissorRectStack(int x0, int y0, int x1, int y1);
AABB2f GetCurrentScissorRectF();

} // draw
} // ui
