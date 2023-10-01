
#pragma once

#include "../Core/Image.h"
#include "../Core/Math.h"
#include "../Core/Platform.h"
#include "../Core/RefCounted.h"


namespace ui {
namespace rhi {

struct Texture2D;

} // rhi
namespace draw {

// texture flags
enum class TexFlags : u8
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
	virtual Size2i GetSize() const = 0;
	virtual StringView GetCacheKey() const = 0;
	virtual TexFlags GetFlags() const = 0;
	virtual rhi::Texture2D* GetInternal() const = 0;
	virtual rhi::Texture2D* GetInternalExclusive() const = 0;
	virtual void SetExclDebugName(StringView debugName) = 0;

	Size2f GetSizeF() const { return GetSize().Cast<float>(); }
	int GetWidth() const { return GetSize().x; }
	int GetHeight() const { return GetSize().y; }
};
using ImageHandle = RCHandle<IImage>;

ImageHandle ImageCreateRGBA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
ImageHandle ImageCreateFromCanvas(const Canvas& c, TexFlags flags = TexFlags::None);

IImage* ImageCacheRead(StringView key);
void ImageCacheWrite(IImage* image, StringView key);
bool ImageCacheRemove(StringView key);

bool ImageIsLoadedFromFile(IImage* image, StringView path);
bool ImageIsLoadedFromFileWithFlags(IImage* image, StringView path, TexFlags flags = TexFlags::Packed);
ImageHandle ImageLoadFromFile(StringView path, TexFlags flags = TexFlags::Packed);
bool ImageCacheRemoveLoadedFromFile(StringView path);

namespace _ {

void TextureStorage_Free();
void TextureStorage_FlushPendingAllocs();
void TextureStorage_RemapUVs(Vertex* verts, size_t num_verts, IImage* image);

} // _

} // draw
} // ui
