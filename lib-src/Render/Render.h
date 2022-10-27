
#pragma once

#include "../Core/Array.h"
#include "../Core/Image.h"
#include "../Core/Math.h"
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
	NoFilter = 1 << 0,
	Repeat = 1 << 1,
	Packed = 1 << 2,
	NoCache = 1 << 3,
};
inline TexFlags operator | (TexFlags a, TexFlags b)
{
	return TexFlags(int(a) | int(b));
}
inline TexFlags operator & (TexFlags a, TexFlags b)
{
	return TexFlags(int(a) & int(b));
}
inline TexFlags operator ~ (TexFlags f)
{
	return TexFlags(~int(f));
}

namespace debug {

int GetAtlasTextureCount();
rhi::Texture2D* GetAtlasTexture(int n, int size[2]);

} // debug

struct IImage : IRefCounted
{
	virtual uint16_t GetWidth() const = 0;
	virtual uint16_t GetHeight() const = 0;
	virtual StringView GetPath() const = 0;
	virtual rhi::Texture2D* GetInternalExclusive() const = 0;

	Size2f GetSizeF() const { return { float(GetWidth()), float(GetHeight()) }; }
};
using ImageHandle = RCHandle<IImage>;

ImageHandle ImageCreateRGBA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateFromCanvas(const Canvas& c, TexFlags flags = TexFlags::None);

ImageHandle ImageLoadFromFile(StringView path, TexFlags flags = TexFlags::Packed);

enum class ImageSetType : uint8_t
{
	Icon = 0,
	Sliced,
	Pattern,
	RawImage,
};

enum class ImageSetSizeMode : uint8_t
{
	Default = 0, // image set defaults to app-wide setting, which defaults to nearest scale down
	NearestScaleDown,
	NearestScaleUp,
	NearestNoScale,
};

struct ImageSet : RefCountedST
{
	struct Entry
	{
		draw::ImageHandle image;
		AABB2f innerUV = { 0, 0, 1, 1 };
		AABB2f edgeWidth = {};
	};

	Array<Entry> entries;
	ImageSetType type = ImageSetType::Icon;
	ImageSetSizeMode sizeMode = ImageSetSizeMode::Default;
	AABB2f baseEdgeWidth = {};
	Size2f baseSize = { 1, 1 };

	Entry* FindEntryForSize(Size2f size);
	Entry* FindEntryForEdgeWidth(AABB2f edgeWidth);

	void _DrawAsIcon(AABB2f rect, Color4b color);
	void _DrawAsSliced(AABB2f rect, Color4b color);
	void _DrawAsPattern(AABB2f rect, Color4b color);
	void _DrawAsRawImage(AABB2f rect, Color4b color);
	void Draw(AABB2f rect, Color4b color = {});
};
using ImageSetHandle = RCHandle<ImageSet>;

ImageSetSizeMode GetDefaultImageSetSizeMode(ImageSetType type);
void SetDefaultImageSetSizeMode(ImageSetType type, ImageSetSizeMode sizeMode);
void SetDefaultImageSetSizeModeAll(ImageSetSizeMode sizeMode);

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
void RectTex(const AABB2f& r, IImage* tex);
void RectTex(float x0, float y0, float x1, float y1, IImage* tex, float u0, float v0, float u1, float v1);
void RectTex(const AABB2f& r, IImage* tex, const AABB2f& uv);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex);
void RectColTex(const AABB2f& r, Color4b col, IImage* tex);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex, float u0, float v0, float u1, float v1);
void RectColTex9Slice(const AABB2f& outer, const AABB2f& inner, Color4b col, IImage* tex, const AABB2f& texouter, const AABB2f& texinner);
void RectCutoutCol(const AABB2f& rect, const AABB2f& cutout, Color4b col);

typedef void VertexTransformFunction(void* userdata, Vertex* vertices, size_t count);
struct VertexTransformCallback
{
	void* userdata = nullptr;
	VertexTransformFunction* func = nullptr;

	UI_FORCEINLINE void Call(Vertex* vertices, size_t count)
	{
		if (func)
			func(userdata, vertices, count);
	}
};
// returns the previous callback
VertexTransformCallback SetVertexTransformCallback(VertexTransformCallback cb);

void ApplyScissor();

ScaleOffset2D GetScissorRectResolutionTransform();
ScaleOffset2D SetScissorRectResolutionTransform(ScaleOffset2D nsrrt);
ScaleOffset2D AddScissorRectResolutionTransform(ScaleOffset2D nsrrt);

void PushScissorRectRaw(const AABB2i& screen, const AABB2f& virt);
bool PushScissorRect(const AABB2f& rect);
bool PushScissorRectIfNotEmpty(const AABB2f& rect);
void PopScissorRect();
void _ResetScissorRectStack(int x0, int y0, int x1, int y1);
AABB2f GetCurrentScissorRectF();

} // draw
} // ui
