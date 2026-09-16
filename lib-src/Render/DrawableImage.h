
#pragma once

#include "../Core/Image.h"
#include "../Core/Math.h"
#include "../Core/Platform.h"
#include "../Core/RefCounted.h"


namespace ui {
namespace gfx {

struct Texture2D;

} // gfx
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

UI_API int GetAtlasTextureCount();
UI_API gfx::Texture2D* GetAtlasTexture(int n, int size[2]);

} // debug

struct IImage : IRefCounted
{
	virtual Size2i GetSize() const = 0;
	virtual StringView GetCacheKey() const = 0;
	virtual TexFlags GetFlags() const = 0;
	virtual gfx::Texture2D* GetInternal() const = 0;
	virtual gfx::Texture2D* GetInternalExclusive() const = 0;
	virtual void SetExclDebugName(StringView debugName) = 0;

	Size2f GetSizeF() const { return GetSize().Cast<float>(); }
	int GetWidth() const { return GetSize().x; }
	int GetHeight() const { return GetSize().y; }
};
using ImageHandle = RCHandle<IImage>;

UI_API ImageHandle ImageCreateRGBA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
UI_API ImageHandle ImageCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags = TexFlags::None);
UI_API ImageHandle ImageCreateA8(int w, int h, const void* data, TexFlags flags = TexFlags::None);
UI_API ImageHandle ImageCreateFromCanvas(const Canvas& c, TexFlags flags = TexFlags::None);
// for info on handle - see rhi::CreateTextureFromAPIHandle
UI_API ImageHandle ImageCreateFromAPIHandle(int w, int h, uintptr_t handle);

UI_API IImage* ImageCacheRead(StringView key);
UI_API void ImageCacheWrite(IImage* image, StringView key);
UI_API bool ImageCacheRemove(StringView key);

UI_API bool ImageIsLoadedFromFile(IImage* image, StringView path);
UI_API bool ImageIsLoadedFromFileWithFlags(IImage* image, StringView path, TexFlags flags = TexFlags::Packed);
UI_API ImageHandle ImageLoadFromFile(StringView path, TexFlags flags = TexFlags::Packed);
UI_API bool ImageCacheRemoveLoadedFromFile(StringView path);

namespace _ {

UI_API void TextureStorage_Free();
UI_API void TextureStorage_FlushPendingAllocs();
UI_API void TextureStorage_RemapUVs(Vertex* verts, size_t num_verts, IImage* image);

} // _

} // draw
} // ui
