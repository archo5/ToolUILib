
#include "VectorImage.h"

#include "FileSystem.h"
#include "HashMap.h"

#define NANOSVG_IMPLEMENTATION
#include "../../ThirdParty/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../../ThirdParty/nanosvgrast.h"


namespace ui {

HashMap<StringView, IVectorImage*> g_loadedVectorImages;

struct VectorImageImpl : IVectorImage
{
	int refcount = 0;
	Size2f size = {};
	NSVGrasterizer* rasterizer = nullptr;
	NSVGimage* image = nullptr;

	std::string cacheKey;

	// IRefCounted
	void AddRef() override
	{
		refcount++;
	}
	void Release() override
	{
		if (--refcount == 0)
		{
			delete this;
		}
	}

	VectorImageImpl()
	{
		rasterizer = nsvgCreateRasterizer();
	}
	~VectorImageImpl()
	{
		if (!cacheKey.empty())
			g_loadedVectorImages.Remove(cacheKey);

		nsvgDeleteRasterizer(rasterizer);
		nsvgDelete(image);
	}

	// IVectorImage
	Size2f GetSize() const override { return size; }
	StringView GetCacheKey() const override { return cacheKey; }
	Canvas GetImageWithHeight(uint16_t h) const override
	{
		float w = h * size.GetAspectRatio();
		unsigned width = unsigned(ceilf(w));
		unsigned height = h;
		float scale = h / image->height;

		Canvas C;
		C.SetSize(width, height);

		nsvgRasterize(
			rasterizer,
			image,
			(width - w) / 2,
			0,
			scale,
			C.GetBytes(),
			width,
			height,
			width * 4);

		return C;
	}
};

IVectorImage* VectorImageCacheRead(StringView key)
{
	return g_loadedVectorImages.GetValueOrDefault(key);
}

void VectorImageCacheWrite(IVectorImage* image, StringView key)
{
	if (auto* prev = VectorImageCacheRead(key))
		static_cast<VectorImageImpl*>(prev)->cacheKey.clear();

	auto* impl = static_cast<VectorImageImpl*>(image);
	impl->cacheKey <<= key;
	g_loadedVectorImages[impl->cacheKey] = image;
}

VectorImageHandle VectorImageLoadFromFile(StringView path)
{
	std::string cacheKey = to_string("file:", path);

	IVectorImage* iimg = VectorImageCacheRead(cacheKey);
	if (iimg)
		return iimg;

	auto frr = FSReadTextFile(path);
	if (!frr.data || frr.data->Size() == 0)
		return nullptr; // TODO return default?

	std::string srcChangeable = to_string(frr.data->GetStringView());
	auto* svg = nsvgParse(&srcChangeable[0], "px", 1.0f);
	if (!svg)
		return nullptr;

	auto* impl = new VectorImageImpl;
	impl->size = { svg->width, svg->height };
	impl->image = svg;
	VectorImageCacheWrite(impl, cacheKey);

	return impl;
}

} // ui
