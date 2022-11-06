
#include "Render.h"

#include "RHI.h"

#include "../Core/Array.h"
#include "../Core/FileSystem.h"
#include "../Core/HashMap.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "../../ThirdParty/stb_rect_pack.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../ThirdParty/stb_image.h"

#include <stdio.h>


namespace ui {
namespace draw {


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


static HashMap<StringView, IImage*> g_imageTextures;

struct ImageImpl : IImage
{
	int refcount = 0;
	uint16_t width = 0;
	uint16_t height = 0;
	uint8_t* data = nullptr;
	TexFlags flags = TexFlags::None;
	rhi::Texture2D* rhiTex = nullptr;
	TextureNode* atlasNode = nullptr;

	std::string path;

	ImageImpl(int w, int h, int pitch, const void* d, bool a8, TexFlags flg) :
		width(w), height(h), flags(flg)
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

		if (!path.empty())
		{
			g_imageTextures.Remove(path);
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
	uint16_t GetWidth() const override { return width; }
	uint16_t GetHeight() const override { return height; }
	StringView GetPath() const override { return path; }
	rhi::Texture2D* GetInternalExclusive() const override
	{
		return rhiTex;
	}
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


ImageHandle ImageLoadFromFile(StringView path, TexFlags flags)
{
	IImage* iimg = g_imageTextures.GetValueOrDefault(path);
	if (iimg && static_cast<ImageImpl*>(iimg)->flags == flags && (flags & TexFlags::NoCache) == TexFlags::None)
		return iimg;

	if (flags != TexFlags::Packed)
		flags = flags & ~TexFlags::Packed;

	auto frr = FSReadBinaryFile(path);
	if (!frr.data)
		return nullptr; // TODO return default?

	int w = 0, h = 0, n = 0;
	auto* imgData = stbi_load_from_memory((const stbi_uc*)frr.data->GetData(), frr.data->GetSize(), &w, &h, &n, 4);
	if (!imgData)
		return nullptr;
	UI_DEFER(stbi_image_free(imgData));

	auto img = ImageCreateRGBA8(w, h, imgData, flags);
	if (!img)
		return nullptr;

	auto* impl = static_cast<ImageImpl*>(img.get_ptr());
	impl->path = to_string(path);
	if ((flags & TexFlags::NoCache) == TexFlags::None)
		g_imageTextures[impl->path] = impl;
	return impl;
}


ImageSetSizeMode g_imageSetSizeModes[3] =
{
	ImageSetSizeMode::NearestScaleDown,
	ImageSetSizeMode::NearestScaleDown,
	ImageSetSizeMode::NearestScaleDown,
};

static inline ImageSetSizeMode GetFinalSizeMode(ImageSetType type, ImageSetSizeMode mode)
{
	if (mode == ImageSetSizeMode::Default)
		mode = g_imageSetSizeModes[unsigned(type)];
	return mode;
}

ImageSet::Entry* ImageSet::FindEntryForSize(Size2f size)
{
	if (entries.IsEmpty())
		return nullptr;

	auto mode = GetFinalSizeMode(type, sizeMode);

	Entry* last = entries.GetDataPtr();
	if (mode == ImageSetSizeMode::NearestScaleUp || mode == ImageSetSizeMode::NearestNoScale)
	{
		for (auto& e : entries)
		{
			if (e.image->GetWidth() > size.x || e.image->GetHeight() > size.y)
				break;
			last = &e;
		}
	}
	else
	{
		for (auto& e : entries)
		{
			last = &e;
			if (e.image->GetWidth() >= size.x && e.image->GetHeight() >= size.y)
				break;
		}
	}
	return last;
}

ImageSet::Entry* ImageSet::FindEntryForEdgeWidth(AABB2f edgeWidth)
{
	if (entries.IsEmpty())
		return nullptr;

	Entry* last = entries.GetDataPtr();
	// TODO
	return last;
}

void ImageSet::_DrawAsIcon(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForSize(rect.GetSize()))
	{
		if (sizeMode == ImageSetSizeMode::NearestNoScale)
			rect = RectGenCentered(rect, e->image->GetSizeF());
		else
			rect = RectGenFit(rect, baseSize);
		draw::RectColTex(rect.x0, rect.y0, rect.x1, rect.y1, color, e->image);
	}
}

void ImageSet::_DrawAsSliced(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForEdgeWidth(baseEdgeWidth))
	{
		AABB2f inner = rect.ShrinkBy(baseEdgeWidth);

		draw::RectColTex9Slice(rect, inner, color, e->image, { 0, 0, 1, 1 }, e->innerUV);
	}
}

void ImageSet::_DrawAsPattern(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForSize(baseSize))
	{
		draw::RectColTex(
			rect.x0, rect.y0, rect.x1, rect.y1,
			color,
			e->image,
			0, 0, rect.GetWidth() / baseSize.x, rect.GetHeight() / baseSize.y);
	}
}

void ImageSet::_DrawAsRawImage(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForSize(rect.GetSize()))
	{
		draw::RectColTex(rect.x0, rect.y0, rect.x1, rect.y1, color, e->image);
	}
}

void ImageSet::Draw(AABB2f rect, Color4b color)
{
	if (rect.GetWidth() <= 0 || rect.GetHeight() <= 0)
		return;
	switch (type)
	{
	case ImageSetType::Icon:
		_DrawAsIcon(rect, color);
		break;
	case ImageSetType::Sliced:
		_DrawAsSliced(rect, color);
		break;
	case ImageSetType::Pattern:
		_DrawAsPattern(rect, color);
		break;
	case ImageSetType::RawImage:
		_DrawAsRawImage(rect, color);
		break;
	}
}

ImageSetSizeMode GetDefaultImageSetSizeMode(ImageSetType type)
{
	auto i = unsigned(type);
	assert(i < 3);
	return g_imageSetSizeModes[i];
}

void SetDefaultImageSetSizeMode(ImageSetType type, ImageSetSizeMode sizeMode)
{
	auto i = unsigned(type);
	assert(i < 3);
	if (sizeMode == ImageSetSizeMode::Default)
		sizeMode = ImageSetSizeMode::NearestScaleDown;
	g_imageSetSizeModes[i] = sizeMode;
}

void SetDefaultImageSetSizeModeAll(ImageSetSizeMode sizeMode)
{
	if (sizeMode == ImageSetSizeMode::Default)
		sizeMode = ImageSetSizeMode::NearestScaleDown;
	for (auto& m : g_imageSetSizeModes)
		m = sizeMode;
}


constexpr int MAX_VERTICES = 4096;
constexpr int MAX_INDICES = 16384;
static rhi::Vertex g_bufVertices[MAX_VERTICES];
static uint16_t g_bufIndices[MAX_INDICES];
static constexpr size_t sizeOfBuffers = sizeof(g_bufVertices) + sizeof(g_bufIndices);
static int g_numVertices;
static int g_numIndices;
static ImageHandle g_whiteTex;
static ImageHandle g_curTex;
static rhi::Texture2D* g_curTexRHI;
static rhi::Texture2D* g_appliedTex;

static ImageHandle GetWhiteTex()
{
	if (!g_whiteTex)
	{
		static uint8_t white[] = { 255, 255, 255, 255 };
		g_whiteTex = ImageCreateRGBA8(1, 1, white, draw::TexFlags::Packed);
	}
	return g_whiteTex;
}

static rhi::Texture2D* GetRHITex(IImage* tex)
{
	return tex ? static_cast<ImageImpl*>(tex)->GetRHITex() : nullptr;
}

static void ApplyRHITex(rhi::Texture2D* tex)
{
	if (g_appliedTex == tex)
		return;
	rhi::SetTexture(tex);
	g_appliedTex = tex;
}

void _Flush()
{
	if (!g_numIndices)
		return;
	ApplyRHITex(GetRHITex(g_curTex));
	rhi::DrawIndexedTriangles(g_bufVertices, g_numVertices, g_bufIndices, g_numIndices);
	g_numVertices = 0;
	g_numIndices = 0;
}

namespace internals {

void InitResources()
{
}

void FreeResources()
{
	g_textureStorage.ReleaseResources();
	g_whiteTex = nullptr;
}

void OnBeginDrawFrame()
{
	g_appliedTex = nullptr;
}

void OnEndDrawFrame()
{
	_Flush();
	if (g_curTex)
		g_curTex = nullptr;
}

void Flush()
{
	_Flush();
}

} // internals

VertexTransformCallback g_curVertXFormCB;

static void DebugOffScale(rhi::Vertex* verts, size_t count, float x, float y, float s)
{
	for (size_t i = 0; i < count; i++)
	{
		verts[i].x = (verts[i].x - x) * s;
		verts[i].y = (verts[i].y - y) * s;
	}
}

#if DEBUG_SUBPIXEL
float XOFF = 0;
float YOFF = 0;
float SCALE = 4;
#endif

void IndexedTriangles(IImage* tex, rhi::Vertex* verts, size_t num_vertices, uint16_t* indices, size_t num_indices)
{
	g_curVertXFormCB.Call(verts, num_vertices);
#if DEBUG_SUBPIXEL
	DebugOffScale(verts, num_vertices, XOFF, YOFF, SCALE);//10, 200, 4);
#endif
	if (!tex)
		tex = GetWhiteTex();
	// TODO limit this for faster JIT glyph uploads
	g_textureStorage.FlushPendingAllocs();
#if 1
	if (GetRHITex(g_curTex) != GetRHITex(tex) || g_numVertices + num_vertices > MAX_VERTICES || g_numIndices + num_indices > MAX_INDICES)
	{
		_Flush();
	}
	if (num_vertices > MAX_VERTICES || num_indices > MAX_INDICES)
	{
		_Flush();
		g_curTex = tex;
		ApplyRHITex(GetRHITex(g_curTex));
		rhi::DrawIndexedTriangles(verts, num_vertices, indices, num_indices);
		return;
	}

	if (g_curTex != tex)
		g_curTex = tex;

	memcpy(&g_bufVertices[g_numVertices], verts, sizeof(*verts) * num_vertices);
	if (g_curTex)
		TextureStorage::RemapUVs(&g_bufVertices[g_numVertices], num_vertices, static_cast<ImageImpl*>(g_curTex.get_ptr())->atlasNode);
	uint16_t baseVertex = g_numVertices;
	for (size_t i = 0; i < num_indices; i++)
		g_bufIndices[g_numIndices + i] = indices[i] + baseVertex;
	g_numVertices += num_vertices;
	g_numIndices += num_indices;
#else // for comparing performance
	rhi::SetTexture(tex);
	rhi::DrawIndexedTriangles(verts, indices, num_indices);
#endif
}

static inline void MidpixelAdjust(Point2f& p, const Point2f& d)
{
	p.x += 0.5f;
	p.y += 0.5f;
#if 0
	if (d.x + d.y >= 0)
		p -= d;
	else
		p += d;
#endif
}

static UI_FORCEINLINE rhi::Vertex ColorVert(const Point2f& p, Color4b col)
{
	return { p.x, p.y, 0.5f, 0.5f, col };
}

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel)
{
	if (x0 == x1 && y0 == y1)
		return;

	Point2f p0 = { x0, y0 };
	Point2f p1 = { x1, y1 };
	Point2f d = (p1 - p0).Normalized() * 0.5f;
	Point2f t = d.Perp() * w;

	if (midpixel)
	{
		MidpixelAdjust(p0, d);
		MidpixelAdjust(p1, d);
	}

	rhi::Vertex verts[4] =
	{
		ColorVert(p0 + t, col),
		ColorVert(p1 + t, col),
		ColorVert(p1 - t, col),
		ColorVert(p0 - t, col),
	};
	uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	IndexedTriangles(nullptr, verts, 4, indices, 6);
}

void AALineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel)
{
	if (x0 == x1 && y0 == y1)
		return;

	Point2f p0 = { x0, y0 };
	Point2f p1 = { x1, y1 };
	Point2f d = (p1 - p0).Normalized() * 0.5f;
	Point2f t = d.Perp();

	if (midpixel)
	{
		MidpixelAdjust(p0, d);
		MidpixelAdjust(p1, d);
	}

	Color4b colA0 = col;
	colA0.a = 0;

	if (w <= 1)
	{
		t *= w * 2;

		Color4b colM = col;
		colM.a = uint8_t(colM.a * w);

		rhi::Vertex verts[6] =
		{
			// + side
			ColorVert(p0 + t - d, colA0),
			ColorVert(p1 + t + d, colA0),
			// middle
			ColorVert(p0 + d, colM),
			ColorVert(p1 - d, colM),
			// - side
			ColorVert(p0 - t - d, colA0),
			ColorVert(p1 - t + d, colA0),
		};
		uint16_t indices[6 * 3] =
		{
			0, 1, 3,  3, 2, 0,
			5, 4, 2,  2, 3, 5,
			0, 2, 4,
			5, 3, 1,
		};
		IndexedTriangles(nullptr, verts, 6, indices, 6 * 3);
	}
	else
	{
		Point2f tw = t * w;

		rhi::Vertex verts[8] =
		{
			// + side
			ColorVert(p0 + tw + t - d, colA0),
			ColorVert(p1 + tw + t + d, colA0),
			// + middle
			ColorVert(p0 + tw - t + d, col),
			ColorVert(p1 + tw - t - d, col),
			// - middle
			ColorVert(p0 - tw + t + d, col),
			ColorVert(p1 - tw + t - d, col),
			// - side
			ColorVert(p0 - tw - t - d, colA0),
			ColorVert(p1 - tw - t + d, colA0),
		};
		uint16_t indices[10 * 3] =
		{
			0, 1, 3,  3, 2, 0,
			2, 3, 5,  5, 4, 2,
			4, 5, 7,  7, 6, 4,
			0, 2, 4,  4, 6, 0,
			7, 5, 3,  3, 1, 7,
		};
		IndexedTriangles(nullptr, verts, 8, indices, 10 * 3);
	}
}

void LineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel)
{
	size_t size = points.size();
	if (size < 2)
		return;

	Point2f t_prev = {};
	if (closed)
		t_prev = (points[0] - points[size - 1]).Normalized().Perp();
	else
		t_prev = (points[1] - points[0]).Normalized().Perp();

	Array<rhi::Vertex> verts;
	verts.Reserve(size * 2);
	for (size_t i = 0; i < size; i++)
	{
		Point2f p0 = points[i];
		Point2f p1 = points[i + 1 < size ? i + 1 : closed ? 0 : size - 1];
		Point2f t_next = (p1 - p0).Normalized().Perp();
		Point2f t_avg = (t_prev + t_next).Normalized();

		if (midpixel)
		{
			MidpixelAdjust(p0, {}/*t_avg.Perp2() * 0.5f*/);
		}

		Point2f t = t_avg * (w * 0.5f / Vec2Dot(t_avg, t_prev));
		verts.Append(ColorVert(p0 + t, col));
		verts.Append(ColorVert(p0 - t, col));

		t_prev = t_next;
	}

	Array<uint16_t> indices;
	indices.Reserve((size - 1) * 6);
	for (size_t i = 0; i + (closed ? 0 : 1) < size; i++)
	{
		size_t i1 = (i + 1) % size;
		indices.Append(uint16_t(i * 2 + 0));
		indices.Append(uint16_t(i1 * 2 + 0));
		indices.Append(uint16_t(i1 * 2 + 1));

		indices.Append(uint16_t(i1 * 2 + 1));
		indices.Append(uint16_t(i * 2 + 1));
		indices.Append(uint16_t(i * 2 + 0));
	}

	IndexedTriangles(nullptr, verts.data(), verts.size(), indices.data(), indices.size());
}

void AALineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel)
{
	size_t size = points.size();
	if (size < 2)
		return;

	Color4b colA0 = col;
	colA0.a = 0;
	Color4b colM = col;
	colM.a = uint8_t(colM.a * w);

	Point2f t_prev = {};
	if (closed)
		t_prev = (points[0] - points[size - 1]).Normalized().Perp();
	else
		t_prev = (points[1] - points[0]).Normalized().Perp();

	size_t ncols = w <= 1 ? 3 : 4;

	Array<rhi::Vertex> verts;
	verts.Resize(size * ncols);
	auto* vdest = verts.GetDataPtr();
	for (size_t i = 0; i < size; i++)
	{
		Point2f p0 = points[i];
		Point2f p1 = points[i + 1 < size ? i + 1 : closed ? 0 : size - 1];
		Point2f t_next = (p1 - p0).Normalized().Perp();
		Point2f t_avg = (t_prev + t_next).Normalized();

		if (midpixel)
		{
			MidpixelAdjust(p0, {}/*t_avg.Perp2() * 0.5f*/);
		}

		float q = 1.0f / Vec2Dot(t_avg, t_prev);
		if (w <= 1)
		{
			auto t = t_avg * w * q;
			*vdest++ = ColorVert(p0 + t, colA0);
			*vdest++ = ColorVert(p0, colM);
			*vdest++ = ColorVert(p0 - t, colA0);
		}
		else
		{
			auto t0 = t_avg * (w + 1) * 0.5f * q;
			auto t1 = t_avg * (w - 1) * 0.5f * q;
			*vdest++ = ColorVert(p0 + t0, colA0);
			*vdest++ = ColorVert(p0 + t1, col);
			*vdest++ = ColorVert(p0 - t1, col);
			*vdest++ = ColorVert(p0 - t0, colA0);
		}

		t_prev = t_next;
	}

	Array<uint16_t> indices;
	indices.Resize((closed ? size : size - 1) * (ncols - 1) * 6);
	uint16_t* idest = indices.GetDataPtr();
	for (size_t i = 0; i + (closed ? 0 : 1) < size; i++)
	{
		size_t i1 = (i + 1) % size;
		for (size_t j = 0; j + 1 < ncols; j++)
		{
			*idest++ = uint16_t(i * ncols + j);
			*idest++ = uint16_t(i1 * ncols + j);
			*idest++ = uint16_t(i1 * ncols + j + 1);

			*idest++ = uint16_t(i1 * ncols + j + 1);
			*idest++ = uint16_t(i * ncols + j + 1);
			*idest++ = uint16_t(i * ncols + j);
		}
	}

	IndexedTriangles(nullptr, verts.data(), verts.size(), indices.data(), indices.size());
}

struct CircleList
{
	Array<Point2f> points;

	CircleList(Point2f center, float radius)
	{
		auto size = size_t(max(radius, 0.0f) * 3.14159f);
		if (size < 3)
			size = 3;
		if (size > 4096)
			size = 4096;

		points.Reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			float a = i * 3.14159f * 2 / size;
			points.Append({ sinf(a) * radius + center.x, cosf(a) * radius + center.y });
		}
	}
};

void CircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel)
{
	LineCol(CircleList(center, rad).points, w, col, true, midpixel);
}

void AACircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel)
{
	AALineCol(CircleList(center, rad).points, w, col, true, midpixel);
}

void RectCol(float x0, float y0, float x1, float y1, Color4b col)
{
	RectColTex(x0, y0, x1, y1, col, nullptr, 0.5f, 0.5f, 0.5f, 0.5f);
}

void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b)
{
	rhi::Vertex verts[4] =
	{
		{ x0, y0, 0.5f, 0.5f, a },
		{ x1, y0, 0.5f, 0.5f, b },
		{ x1, y1, 0.5f, 0.5f, b },
		{ x0, y1, 0.5f, 0.5f, a },
	};
	uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	IndexedTriangles(nullptr, verts, 4, indices, 6);
}

void RectTex(float x0, float y0, float x1, float y1, IImage* tex)
{
	RectColTex(x0, y0, x1, y1, Color4b::White(), tex, 0, 0, 1, 1);
}

void RectTex(const AABB2f& r, IImage* tex)
{
	RectColTex(r.x0, r.y0, r.x1, r.y1, Color4b::White(), tex, 0, 0, 1, 1);
}

void RectTex(float x0, float y0, float x1, float y1, IImage* tex, float u0, float v0, float u1, float v1)
{
	RectColTex(x0, y0, x1, y1, Color4b::White(), tex, u0, v0, u1, v1);
}

void RectTex(const AABB2f& r, IImage* tex, const AABB2f& uv)
{
	RectColTex(r.x0, r.y0, r.x1, r.y1, Color4b::White(), tex, uv.x0, uv.y0, uv.x1, uv.y1);
}

void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex)
{
	RectColTex(x0, y0, x1, y1, col, tex, 0, 0, 1, 1);
}

void RectColTex(const AABB2f& r, Color4b col, IImage* tex)
{
	RectColTex(r.x0, r.y0, r.x1, r.y1, col, tex, 0, 0, 1, 1);
}

void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex, float u0, float v0, float u1, float v1)
{
	rhi::Vertex verts[4] =
	{
		{ x0, y0, u0, v0, col },
		{ x1, y0, u1, v0, col },
		{ x1, y1, u1, v1, col },
		{ x0, y1, u0, v1, col },
	};
	uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	IndexedTriangles(tex, verts, 4, indices, 6);
}

void RectColTex(const AABB2f& r, Color4b col, IImage* tex, const AABB2f& uv)
{
	RectColTex(r.x0, r.y0, r.x1, r.y1, col, tex, uv.x0, uv.y0, uv.x1, uv.y1);
}

void RectColTex9Slice(const AABB2f& outer, const AABB2f& inner, Color4b col, IImage* tex, const AABB2f& texouter, const AABB2f& texinner)
{
	//  0  1  2  3
	//  4  5  6  7
	//  8  9 10 11
	// 12 13 14 15
	rhi::Vertex verts[16] =
	{
		// row 0
		{ outer.x0, outer.y0, texouter.x0, texouter.y0, col },
		{ inner.x0, outer.y0, texinner.x0, texouter.y0, col },
		{ inner.x1, outer.y0, texinner.x1, texouter.y0, col },
		{ outer.x1, outer.y0, texouter.x1, texouter.y0, col },
		// row 1
		{ outer.x0, inner.y0, texouter.x0, texinner.y0, col },
		{ inner.x0, inner.y0, texinner.x0, texinner.y0, col },
		{ inner.x1, inner.y0, texinner.x1, texinner.y0, col },
		{ outer.x1, inner.y0, texouter.x1, texinner.y0, col },
		// row 2
		{ outer.x0, inner.y1, texouter.x0, texinner.y1, col },
		{ inner.x0, inner.y1, texinner.x0, texinner.y1, col },
		{ inner.x1, inner.y1, texinner.x1, texinner.y1, col },
		{ outer.x1, inner.y1, texouter.x1, texinner.y1, col },
		// row 3
		{ outer.x0, outer.y1, texouter.x0, texouter.y1, col },
		{ inner.x0, outer.y1, texinner.x0, texouter.y1, col },
		{ inner.x1, outer.y1, texinner.x1, texouter.y1, col },
		{ outer.x1, outer.y1, texouter.x1, texouter.y1, col },
	};
	uint16_t indices[6 * 9] =
	{
		// top row
		0, 1, 5,  5, 4, 0,
		1, 2, 6,  6, 5, 1,
		2, 3, 7,  7, 6, 2,
		// middle row
		4, 5, 9,  9, 8, 4,
		5, 6, 10,  10, 9, 5,
		6, 7, 11,  11, 10, 6,
		// bottom row
		8, 9, 13,  13, 12, 8,
		9, 10, 14,  14, 13, 9,
		10, 11, 15,  15, 14, 10,
	};

	IndexedTriangles(tex, verts, 16, indices, 6 * 9);
}

void RectCutoutCol(const AABB2f& rect, const AABB2f& cutout, Color4b col)
{
	auto& cutr = cutout;
	rhi::Vertex verts[8] =
	{
		{ rect.x0, rect.y0, 0.5f, 0.5f, col },
		{ rect.x1, rect.y0, 0.5f, 0.5f, col },
		{ rect.x1, rect.y1, 0.5f, 0.5f, col },
		{ rect.x0, rect.y1, 0.5f, 0.5f, col },

		{ cutr.x0, cutr.y0, 0.5f, 0.5f, col },
		{ cutr.x1, cutr.y0, 0.5f, 0.5f, col },
		{ cutr.x1, cutr.y1, 0.5f, 0.5f, col },
		{ cutr.x0, cutr.y1, 0.5f, 0.5f, col },
	};

	uint16_t indices[24] =
	{
		0, 1, 5,  5, 4, 0, // top
		1, 2, 6,  6, 5, 1, // right
		2, 3, 7,  7, 6, 2, // bottom
		3, 0, 4,  4, 7, 3, // left
	};

	IndexedTriangles(nullptr, verts, 8, indices, 24);
}


VertexTransformCallback SetVertexTransformCallback(VertexTransformCallback cb)
{
	auto prev = g_curVertXFormCB;
	g_curVertXFormCB = cb;
	return prev;
}


struct ScissorRectStackEntry
{
	AABB2i raw;
	AABB2f input;
};

static ScaleOffset2D g_scissorRectResTransform;
static ScissorRectStackEntry scissorStack[100];
static int scissorCount = 1;

void ApplyScissor()
{
	_Flush();
	AABB2i r = scissorStack[scissorCount - 1].raw;
	rhi::SetScissorRect(r.x0, r.y0, r.x1, r.y1);
}


ScaleOffset2D GetScissorRectResolutionTransform()
{
	return g_scissorRectResTransform;
}

ScaleOffset2D SetScissorRectResolutionTransform(ScaleOffset2D nsrrt)
{
	auto r = g_scissorRectResTransform;
	g_scissorRectResTransform = nsrrt;
	return r;
}

ScaleOffset2D AddScissorRectResolutionTransform(ScaleOffset2D nsrrt)
{
	return SetScissorRectResolutionTransform(nsrrt * g_scissorRectResTransform);
}


void PushScissorRectRaw(const AABB2i& screen, const AABB2f& virt)
{
	int i = scissorCount++;
	scissorStack[i] = { screen, virt };
	ApplyScissor();
}

bool PushScissorRect(const AABB2f& rect)
{
	auto xrect = rect * g_scissorRectResTransform;

	AABB2i r = scissorStack[scissorCount - 1].raw.Intersect(xrect.Cast<int>());

	PushScissorRectRaw(r, rect);

	return r.x0 < r.x1 && r.y0 < r.y1;
}

bool PushScissorRectIfNotEmpty(const AABB2f& rect)
{
	auto xrect = rect * g_scissorRectResTransform;

	AABB2i r = scissorStack[scissorCount - 1].raw.Intersect(xrect.Cast<int>());

	bool notEmpty = r.x0 < r.x1 && r.y0 < r.y1;
	if (notEmpty)
		PushScissorRectRaw(r, rect);
	return notEmpty;
}

void PopScissorRect()
{
	scissorCount--;
	ApplyScissor();
}

void _ResetScissorRectStack(int x0, int y0, int x1, int y1)
{
	AABB2i rect = { x0, y0, x1, y1 };
	scissorStack[0] = { rect, rect.Cast<float>() };
	scissorCount = 1;
	ApplyScissor();
}

AABB2f GetCurrentScissorRectF()
{
	return scissorStack[scissorCount - 1].input;
}

} // draw
} // ui
