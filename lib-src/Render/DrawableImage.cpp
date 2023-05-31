
#include "DrawableImage.h"

#include "RHI.h"

#include "../Core/FileSystem.h"
#include "../Core/HashMap.h"
#include "../Core/Logging.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "../../ThirdParty/stb_rect_pack.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../ThirdParty/stb_image.h"


namespace ui {
double hqtime();
namespace draw {


LogCategory LOG_DRAWABLE_IMAGE("DrawableImage", LogLevel::Info);


static constexpr unsigned TEXTURE_PAGE_WIDTH = 1024;
static constexpr unsigned TEXTURE_PAGE_HEIGHT = 1024;
static constexpr unsigned MAX_TEXTURE_PAGES = 32;
static constexpr unsigned MAX_TEXTURE_PAGE_NODES = 2048;
static constexpr unsigned MAX_PENDING_ALLOCS = 1024;

struct TextureNode
{
	int page = -1;
	uint16_t x = 0;
	uint16_t y = 0;
	uint16_t w = 0;
	uint16_t h = 0;
	uint16_t rw = 0;
	uint16_t rh = 0;
	const void* srcData = nullptr;
	TextureNode* nextInPage = nullptr;
};

struct TexturePage
{
	TexturePage()
	{
		stbrp_init_target(&rectPackContext, TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT, rectPackNodes, MAX_TEXTURE_PAGE_NODES);
		rhiTex = rhi::CreateTextureRGBA8(nullptr, TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT, 0);
	}
	~TexturePage()
	{
		rhi::DestroyTexture(rhiTex);
	}

	rhi::Texture2D* rhiTex = nullptr;
	unsigned pixelsUsed = 0;
	unsigned pixelsAllocated = 0;
	stbrp_context rectPackContext;
	stbrp_node rectPackNodes[MAX_TEXTURE_PAGE_NODES];
	Array<ImageHandle> allocatedImages;
};

static bool CanPack(TexFlags f)
{
	// needs Packed flag
	if ((f & TexFlags::Packed) == TexFlags::None)
		return false;
	// does not support NoFilter flag
	if ((f & TexFlags::NoFilter) != TexFlags::None)
		return false;
	return true;
}

struct TextureStorage
{
	TextureNode* AllocNode(int w, int h, TexFlags flg, IImage* img)
	{
		if (!CanPack(flg))
			return nullptr;
		if (w + 2 > TEXTURE_PAGE_WIDTH / 2 || h + 2 > TEXTURE_PAGE_HEIGHT / 2)
			return nullptr;

		if (numPendingAllocs == MAX_PENDING_ALLOCS)
		{
			FlushPendingAllocs();
		}

		auto* N = new TextureNode;
		N->w = w;
		N->h = h;
		N->rw = w + 2;
		N->rh = h + 2;
		int pidx = numPendingAllocs++;
		pendingAllocs[pidx] = N;
		pendingImages[pidx] = img;
		return N;
	}
	void FlushPendingAllocs()
	{
		if (!numPendingAllocs)
			return;

		stbrp_rect rectsToPack[MAX_TEXTURE_PAGE_NODES] = {};
		for (int i = 0; i < numPendingAllocs; i++)
		{
			auto& R = rectsToPack[i];
			R.id = i;
			R.w = pendingAllocs[i]->rw;
			R.h = pendingAllocs[i]->rh;
		}
		int numRemainingRects = numPendingAllocs;

		for (int page_num = 0; page_num < MAX_TEXTURE_PAGES; page_num++)
		{
			auto* P = pages[page_num];

			if (!P)
			{
				pages[page_num] = P = new TexturePage;
				numPages++;
			}

			stbrp_pack_rects(&P->rectPackContext, rectsToPack, numRemainingRects);

			// update packed nodes & add them to page
			bool anyPacked = false;
			for (int i = 0; i < numRemainingRects; i++)
			{
				const auto& R = rectsToPack[i];
				if (R.was_packed)
				{
					anyPacked = true;
					auto* N = pendingAllocs[R.id];
					N->x = R.x + 1;
					N->y = R.y + 1;
					N->page = page_num;
					P->pixelsAllocated += N->rw * N->rh;
					P->pixelsUsed += N->rw * N->rh;
					P->allocatedImages.Append(pendingImages[R.id]);
				}
			}

			if (anyPacked)
			{
				// upload packed rects
				rhi::MapData md = rhi::MapTexture(P->rhiTex);
				for (int i = 0; i < numRemainingRects; i++)
				{
					const auto& R = rectsToPack[i];
					if (R.was_packed)
					{
						rhi::CopyToMappedTextureRect(
							P->rhiTex,
							md,
							R.x,
							R.y,
							R.w,
							R.h,
							pendingAllocs[R.id]->srcData,
							false);
					}
				}
				rhi::UnmapTexture(P->rhiTex);
			}

			// removed packed rects from array
			int outIdx = 0;
			for (int i = 0; i < numRemainingRects; i++)
			{
				if (!rectsToPack[i].was_packed)
				{
					if (outIdx != i)
						rectsToPack[outIdx] = rectsToPack[i];
					outIdx++;
				}
			}
			numRemainingRects = outIdx;

			if (numRemainingRects == 0)
				break;
		}

		for (int i = 0; i < numPendingAllocs; i++)
			pendingImages[i] = nullptr;

		numPendingAllocs = 0;
	}
	static void RemapUVs(rhi::Vertex* verts, size_t num_verts, TextureNode* node)
	{
		if (!node || node->page < 0)
			return;
		float xs = float(node->w) / float(TEXTURE_PAGE_WIDTH);
		float ys = float(node->h) / float(TEXTURE_PAGE_HEIGHT);
		float xo = float(node->x) / float(TEXTURE_PAGE_WIDTH);
		float yo = float(node->y) / float(TEXTURE_PAGE_HEIGHT);
		for (size_t i = 0; i < num_verts; i++)
		{
			verts[i].u = verts[i].u * xs + xo;
			verts[i].v = verts[i].v * ys + yo;
		}
	}

	void ReleaseResources()
	{
		for (auto*& alloc : pendingAllocs)
		{
			delete alloc;
			alloc = nullptr;
		}
		for (auto& img : pendingImages)
			img = nullptr;
		numPendingAllocs = 0;

		for (auto*& page : pages)
		{
			delete page;
			page = nullptr;
		}
		numPages = 0;
	}

	TexturePage* pages[MAX_TEXTURE_PAGES] = {};
	int numPages = 0;
	TextureNode* pendingAllocs[MAX_PENDING_ALLOCS] = {};
	ImageHandle pendingImages[MAX_PENDING_ALLOCS];
	int numPendingAllocs = 0;
}
g_textureStorage;


namespace debug {

int GetAtlasTextureCount()
{
	return g_textureStorage.numPages;
}

rhi::Texture2D* GetAtlasTexture(int n, int size[2])
{
	if (size)
	{
		size[0] = TEXTURE_PAGE_WIDTH;
		size[1] = TEXTURE_PAGE_HEIGHT;
	}
	return g_textureStorage.pages[n]->rhiTex;
}

} // debug


static HashMap<StringView, IImage*> g_loadedImages;

struct ImageImpl : IImage
{
	int refcount = 0;
	Size2i size = {};
	uint8_t* data = nullptr;
	TexFlags flags = TexFlags::None;
	rhi::Texture2D* rhiTex = nullptr;
	TextureNode* atlasNode = nullptr;

	std::string cacheKey;

	ImageImpl(int w, int h, int pitch, const void* d, bool a8, TexFlags flg) :
		size(w, h), flags(flg)
	{
		if (auto* n = g_textureStorage.AllocNode(w, h, flg, this))
		{
			int dstw = w + 2;
			int dsth = h + 2;
			data = new uint8_t[dstw * dsth * 4];
			if (!a8)
			{
				for (int y = 0; y < h; y++)
				{
					memcpy(&data[((y + 1) * dstw + 1) * 4], &((const char*)d)[y * pitch], w * 4);
				}
			}
			else
			{
				for (int y = 0; y < h; y++)
				{
					int y1 = y + 1;
					for (int x = 0; x < w; x++)
					{
						int x1 = x + 1;
						int off = (x1 + y1 * dstw) * 4;
						data[off + 0] = 255;
						data[off + 1] = 255;
						data[off + 2] = 255;
						data[off + 3] = ((const char*)d)[x + y * pitch];
					}
				}
			}

			// copy top row
			memcpy(&data[4], &data[(dstw + 1) * 4], w * 4);
			// copy bottom row
			memcpy(&data[(dstw * (dsth - 1) + 1) * 4], &data[(dstw * (dsth - 2) + 1) * 4], w * 4);
			// copy left/right edges
			for (int y = 0; y < dsth; y++)
			{
				memcpy(&data[y * dstw * 4], &data[(y * dstw + 1) * 4], 4);
				memcpy(&data[(y * dstw + dstw - 1) * 4], &data[(y * dstw + dstw - 2) * 4], 4);
			}

			n->srcData = data;
			atlasNode = n;
		}
		else
		{
			rhiTex = a8
				? rhi::CreateTextureA8(d, w, h, uint8_t(flg))
				: rhi::CreateTextureRGBA8(d, w, h, uint8_t(flg));
		}
	}
	~ImageImpl()
	{
		rhi::DestroyTexture(rhiTex);
		delete[] data;

		if (!cacheKey.empty())
		{
			g_loadedImages.Remove(cacheKey);
		}
	}
	rhi::Texture2D* GetRHITex() const
	{
		return rhiTex ? rhiTex : atlasNode && atlasNode->page >= 0 ? g_textureStorage.pages[atlasNode->page]->rhiTex : nullptr;
	}

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

	// IImage
	Size2i GetSize() const override { return size; }
	StringView GetCacheKey() const override { return cacheKey; }
	TexFlags GetFlags() const override { return flags; }
	rhi::Texture2D* GetInternal() const override { return GetRHITex(); }
	rhi::Texture2D* GetInternalExclusive() const override { return rhiTex; }
};


ImageHandle ImageCreateRGBA8(int w, int h, const void* data, TexFlags flags)
{
	return new ImageImpl(w, h, w * 4, data, false, flags);
}

ImageHandle ImageCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags)
{
	return new ImageImpl(w, h, pitch, data, false, flags);
}

ImageHandle ImageCreateA8(int w, int h, const void* data, TexFlags flags)
{
	return new ImageImpl(w, h, w, data, true, flags);
}

ImageHandle ImageCreateFromCanvas(const Canvas& c, TexFlags flags)
{
	return ImageCreateRGBA8(c.GetWidth(), c.GetHeight(), c.GetPixels(), flags);
}


IImage* ImageCacheRead(StringView key)
{
	return g_loadedImages.GetValueOrDefault(key);
}

void ImageCacheWrite(IImage* image, StringView key)
{
	if (auto* prev = ImageCacheRead(key))
		static_cast<ImageImpl*>(prev)->cacheKey.clear();

	auto* impl = static_cast<ImageImpl*>(image);
	impl->cacheKey = to_string(key);
	g_loadedImages[impl->cacheKey] = image;
}


bool ImageIsLoadedFromFile(IImage* image, StringView path)
{
	if (!image)
		return false;
	auto ck = image->GetCacheKey();
	return ck.starts_with("file:") && ck.substr(5) == path;
}

bool ImageIsLoadedFromFileWithFlags(IImage* image, StringView path, TexFlags flags)
{
	if (!image)
		return false;
	auto ck = image->GetCacheKey();
	return ck.starts_with("file:") && ck.substr(5) == path && image->GetFlags() == flags;
}

ImageHandle ImageLoadFromFile(StringView path, TexFlags flags)
{
	std::string cacheKey = to_string("file:", path);

	IImage* iimg = ImageCacheRead(cacheKey);
	if (iimg && static_cast<ImageImpl*>(iimg)->flags == flags && (flags & TexFlags::NoCache) == TexFlags::None)
		return iimg;

	LogDebug(LOG_DRAWABLE_IMAGE, "ImageLoadFromFile: loading image %s (flags:%X)", cacheKey.c_str(), unsigned(flags));
	double t0 = hqtime();

	if (flags != TexFlags::Packed)
		flags = flags & ~TexFlags::Packed;

	auto frr = FSReadBinaryFile(path);
	if (!frr.data)
	{
		LogError(LOG_DRAWABLE_IMAGE, "ImageLoadFromFile: failed to load %s - could not read file", cacheKey.c_str());
		return nullptr; // TODO return default?
	}

	int w = 0, h = 0, n = 0;
	auto* imgData = stbi_load_from_memory((const stbi_uc*)frr.data->Data(), frr.data->Size(), &w, &h, &n, 4);
	if (!imgData)
	{
		LogError(LOG_DRAWABLE_IMAGE, "ImageLoadFromFile: failed to load %s - could not parse file", cacheKey.c_str());
		return nullptr;
	}
	UI_DEFER(stbi_image_free(imgData));

	auto img = ImageCreateRGBA8(w, h, imgData, flags);
	if (!img)
	{
		LogError(LOG_DRAWABLE_IMAGE, "ImageLoadFromFile: failed to load %s - could not create image", cacheKey.c_str());
		return nullptr;
	}

	auto* impl = static_cast<ImageImpl*>(img.get_ptr());
	if ((flags & TexFlags::NoCache) == TexFlags::None)
		ImageCacheWrite(impl, cacheKey);

	LogInfo(
		LOG_DRAWABLE_IMAGE,
		"ImageLoadFromFile: image %s (flags:%X) loaded in %.2f ms",
		cacheKey.c_str(),
		unsigned(flags),
		(hqtime() - t0) * 1000);

	return img;
}


namespace _ {

void TextureStorage_Free()
{
	g_textureStorage.ReleaseResources();
}

void TextureStorage_FlushPendingAllocs()
{
	g_textureStorage.FlushPendingAllocs();
}

void TextureStorage_RemapUVs(Vertex* verts, size_t num_verts, IImage* image)
{
	TextureStorage::RemapUVs(verts, num_verts, static_cast<ImageImpl*>(image)->atlasNode);
}

} // _


} // draw
} // ui
