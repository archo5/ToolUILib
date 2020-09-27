
#include <stdio.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "../stb_rect_pack.h"

#include "OpenGL.h"
#include "Render.h"


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
	const void* srcData = nullptr;
	TextureNode* nextInPage = nullptr;
	bool srcAlpha8 = false;
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
};

struct TextureStorage
{
	TextureNode* AllocNode(int w, int h, const void* d, bool a8, TexFlags flg)
	{
		if ((flg & TexFlags::Repeat) != TexFlags::None)
			return nullptr;
		if ((flg & TexFlags::Filter) != TexFlags::None) // TODO add 1px border?
			return nullptr;
		if (w > TEXTURE_PAGE_WIDTH / 2 || h > TEXTURE_PAGE_HEIGHT / 2)
			return nullptr;

		auto* N = new TextureNode;
		N->w = w;
		N->h = h;
		N->srcData = d;
		N->srcAlpha8 = a8;
		pendingAllocs[numPendingAllocs++] = N;
		if (numPendingAllocs == MAX_PENDING_ALLOCS)
		{
			FlushPendingAllocs();
		}
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
			R.w = pendingAllocs[i]->w;
			R.h = pendingAllocs[i]->h;
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
					N->x = R.x;
					N->y = R.y;
					N->page = page_num;
					P->pixelsAllocated += N->w * N->h;
					P->pixelsUsed += N->w * N->h;
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
						rhi::CopyToMappedTextureRect(P->rhiTex, md, R.x, R.y, R.w, R.h, pendingAllocs[R.id]->srcData, pendingAllocs[R.id]->srcAlpha8);
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

	TexturePage* pages[MAX_TEXTURE_PAGES] = {};
	int numPages = 0;
	TextureNode* pendingAllocs[MAX_PENDING_ALLOCS] = {};
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


struct Texture
{
	Texture(int w, int h, int pitch, const void* d, bool a8, TexFlags flg) :
		width(w), height(h), flags(flg)
	{
		data = new uint8_t[w * h * 4];
		if (!a8)
		{
			for (int y = 0; y < h; y++)
			{
				memcpy(&data[y * w * 4], &((const char*)d)[y * pitch], w * 4);
			}
		}
		else
		{
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					data[(x + y * w) * 4 + 0] = 255;
					data[(x + y * w) * 4 + 1] = 255;
					data[(x + y * w) * 4 + 2] = 255;
					data[(x + y * w) * 4 + 3] = ((const char*)d)[x + y * pitch];
				}
			}
		}
		if (auto* n = g_textureStorage.AllocNode(w, h, data, false, flg))
			atlasNode = n;
		else
			rhiTex = a8 ? rhi::CreateTextureA8(d, w, h, uint8_t(flg)) : rhi::CreateTextureRGBA8(d, w, h, uint8_t(flg));
	}
	~Texture()
	{
		rhi::DestroyTexture(rhiTex);
		delete[] data;
	}
	rhi::Texture2D* GetRHITex() const
	{
		return rhiTex ? rhiTex : atlasNode && atlasNode->page >= 0 ? g_textureStorage.pages[atlasNode->page]->rhiTex : nullptr;
	}

	int refcount = 1;
	uint16_t width;
	uint16_t height;
	uint8_t* data;
	TexFlags flags;
	rhi::Texture2D* rhiTex = nullptr;
	TextureNode* atlasNode = nullptr;
};


Texture* TextureCreateRGBA8(int w, int h, const void* data, TexFlags flags)
{
	return new Texture(w, h, w * 4, data, false, flags);
}

Texture* TextureCreateRGBA8(int w, int h, int pitch, const void* data, TexFlags flags)
{
	return new Texture(w, h, pitch, data, false, flags);
}

Texture* TextureCreateA8(int w, int h, const void* data, TexFlags flags)
{
	return new Texture(w, h, w, data, true, flags);
}

void TextureAddRef(Texture* tex)
{
	tex->refcount++;
}

void TextureRelease(Texture* tex)
{
	if (--tex->refcount == 0)
	{
		delete tex;
	}
}


void _TextureInitStorage()
{
}


constexpr int MAX_VERTICES = 4096;
constexpr int MAX_INDICES = 16384;
static rhi::Vertex g_bufVertices[MAX_VERTICES];
static uint16_t g_bufIndices[MAX_INDICES];
static constexpr size_t sizeOfBuffers = sizeof(g_bufVertices) + sizeof(g_bufIndices);
static int g_numVertices;
static int g_numIndices;
static Texture* g_whiteTex;
static Texture* g_curTex;
static rhi::Texture2D* g_appliedTex;

static Texture* GetWhiteTex()
{
	if (!g_whiteTex)
	{
		static uint8_t white[] = { 255, 255, 255, 255 };
		g_whiteTex = TextureCreateRGBA8(1, 1, white);
	}
	return g_whiteTex;
}

static rhi::Texture2D* GetRHITex(Texture* tex)
{
	return tex ? tex->GetRHITex() : nullptr;
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
	rhi::DrawIndexedTriangles(g_bufVertices, g_bufIndices, g_numIndices);
	g_numVertices = 0;
	g_numIndices = 0;
}

namespace internals {

void OnBeginDrawFrame()
{
	g_appliedTex = nullptr;
}

void OnEndDrawFrame()
{
	_Flush();
}

} // internals

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

void IndexedTriangles(Texture* tex, rhi::Vertex* verts, size_t num_vertices, uint16_t* indices, size_t num_indices)
{
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
		rhi::DrawIndexedTriangles(verts, indices, num_indices);
		return;
	}

	g_curTex = tex;
	memcpy(&g_bufVertices[g_numVertices], verts, sizeof(*verts) * num_vertices);
	if (g_curTex)
		TextureStorage::RemapUVs(&g_bufVertices[g_numVertices], num_vertices, g_curTex->atlasNode);
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

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel)
{
	if (x0 == x1 && y0 == y1)
		return;

	float dx = x1 - x0;
	float dy = y1 - y0;
	float lensq = dx * dx + dy * dy;
	float hinvlen = 0.5f / sqrtf(lensq);
	dx *= hinvlen;
	dy *= hinvlen;
	float tx = -dy * w;
	float ty = dx * w;

	if (midpixel)
	{
		x0 += 0.5f;
		y0 += 0.5f;
		x1 += 0.5f;
		y1 += 0.5f;
		if (dx + dy >= 0)
		{
			x0 -= dx;
			y0 -= dy;
			x1 -= dx;
			y1 -= dy;
		}
		else
		{
			x0 += dx;
			y0 += dy;
			x1 += dx;
			y1 += dy;
		}
	}

	rhi::Vertex verts[4] =
	{
		{ x0 + tx, y0 + ty, 0.5f, 0.5f, col },
		{ x1 + tx, y1 + ty, 0.5f, 0.5f, col },
		{ x1 - tx, y1 - ty, 0.5f, 0.5f, col },
		{ x0 - tx, y0 - ty, 0.5f, 0.5f, col },
	};
	uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	IndexedTriangles(nullptr, verts, 4, indices, 6);
}

void AALineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel)
{
	if (x0 == x1 && y0 == y1)
		return;

	float dx = x1 - x0;
	float dy = y1 - y0;
	float lensq = dx * dx + dy * dy;
	float hinvlen = 0.5f / sqrtf(lensq);
	dx *= hinvlen;
	dy *= hinvlen;
	float tx = dy;
	float ty = -dx;

	if (midpixel)
	{
		x0 += 0.5f;
		y0 += 0.5f;
		x1 += 0.5f;
		y1 += 0.5f;
		if (dx + dy >= 0)
		{
			x0 -= dx;
			y0 -= dy;
			x1 -= dx;
			y1 -= dy;
		}
		else
		{
			x0 += dx;
			y0 += dy;
			x1 += dx;
			y1 += dy;
		}
	}

	Color4b colA0 = col;
	colA0.a = 0;

	if (w <= 1)
	{
		tx *= w * 2;
		ty *= w * 2;

		Color4b colM = col;
		colM.a = colM.a * w;

		rhi::Vertex verts[6] =
		{
			// + side
			{ x0 + tx - dx, y0 + ty - dy, 0.5f, 0.5f, colA0 },
			{ x1 + tx + dx, y1 + ty + dy, 0.5f, 0.5f, colA0 },
			// middle
			{ x0 + dx, y0 + dy, 0.5f, 0.5f, colM },
			{ x1 - dx, y1 - dy, 0.5f, 0.5f, colM },
			// - side
			{ x0 - tx - dx, y0 - ty - dy, 0.5f, 0.5f, colA0 },
			{ x1 - tx + dx, y1 - ty + dy, 0.5f, 0.5f, colA0 },
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
		float twx = tx * w;
		float twy = ty * w;

		rhi::Vertex verts[8] =
		{
			// + side
			{ x0 + twx + tx - dx, y0 + twy + ty - dy, 0.5f, 0.5f, colA0 },
			{ x1 + twx + tx + dx, y1 + twy + ty + dy, 0.5f, 0.5f, colA0 },
			// + middle
			{ x0 + twx - tx + dx, y0 + twy - ty + dy, 0.5f, 0.5f, col },
			{ x1 + twx - tx - dx, y1 + twy - ty - dy, 0.5f, 0.5f, col },
			// - middle
			{ x0 - twx + tx + dx, y0 - twy + ty + dy, 0.5f, 0.5f, col },
			{ x1 - twx + tx - dx, y1 - twy + ty - dy, 0.5f, 0.5f, col },
			// - side
			{ x0 - twx - tx - dx, y0 - twy - ty - dy, 0.5f, 0.5f, colA0 },
			{ x1 - twx - tx + dx, y1 - twy - ty + dy, 0.5f, 0.5f, colA0 },
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

void RectTex(float x0, float y0, float x1, float y1, Texture* tex)
{
	RectColTex(x0, y0, x1, y1, Color4b::White(), tex, 0, 0, 1, 1);
}

void RectTex(float x0, float y0, float x1, float y1, Texture* tex, float u0, float v0, float u1, float v1)
{
	RectColTex(x0, y0, x1, y1, Color4b::White(), tex, u0, v0, u1, v1);
}

void RectColTex(float x0, float y0, float x1, float y1, Color4b col, Texture* tex)
{
	RectColTex(x0, y0, x1, y1, col, tex, 0, 0, 1, 1);
}

void RectColTex(float x0, float y0, float x1, float y1, Color4b col, Texture* tex, float u0, float v0, float u1, float v1)
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

void RectColTex9Slice(const AABB<float>& outer, const AABB<float>& inner, Color4b col, Texture* tex, const AABB<float>& texouter, const AABB<float>& texinner)
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

void RectCutoutCol(const AABB<float>& rect, const AABB<float>& cutout, Color4b col)
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

static AABB<int> scissorStack[100];
static int scissorCount = 1;

void ApplyScissor()
{
	_Flush();
	AABB<int> r = scissorStack[scissorCount - 1];
	rhi::SetScissorRect(r.x0, r.y0, r.x1, r.y1);
}

void PushScissorRect(int x0, int y0, int x1, int y1)
{
	int i = scissorCount++;
	AABB<int> r = scissorStack[i - 1];
	if (r.x0 < x0) r.x0 = x0;
	if (r.x1 > x1) r.x1 = x1;
	if (r.y0 < y0) r.y0 = y0;
	if (r.y1 > y1) r.y1 = y1;
	scissorStack[i] = r;
	ApplyScissor();
}

void PopScissorRect()
{
	scissorCount--;
	ApplyScissor();
}

void _ResetScissorRectStack(int x0, int y0, int x1, int y1)
{
	scissorStack[0] = { x0, y0, x1, y1 };
	scissorCount = 1;
	ApplyScissor();
}

AABB<float> GetCurrentScissorRectF()
{
	return scissorStack[scissorCount - 1].Cast<float>();
}

} // draw
} // ui


unsigned char* LoadTGA(const char* img, int size[2])
{
	FILE* f = fopen(img, "rb");
	char idlen = getc(f);
	assert(idlen == 0); // no id
	char cmtype = getc(f);
	assert(cmtype == 0); // no colormap
	char imgtype = getc(f);
	assert(imgtype == 2); // only uncompressed true color image supported
	fseek(f, 5, SEEK_CUR); // skip colormap

	fseek(f, 4, SEEK_CUR); // skip x/y origin
	short fsize[2];
	fread(&fsize, 2, 2, f); // x/y size
	char bpp = getc(f);
	char imgdesc = getc(f);

	// image id here?
	// colormap here?
	// image data
	size[0] = fsize[0];
	size[1] = fsize[1];
	unsigned char* out = new unsigned char[fsize[0] * fsize[1] * 4];
	for (int i = 0; i < fsize[0] * fsize[1]; i++)
	{
		out[i * 4 + 2] = getc(f);
		out[i * 4 + 1] = getc(f);
		out[i * 4 + 0] = getc(f);
		out[i * 4 + 3] = getc(f);
	}
	return out;
}

Sprite g_themeSprites[TE__COUNT] =
{
#if 1
	// button
	{ 0, 0, 32, 32, 5, 5, 5, 5 },
	{ 32, 0, 64, 32, 5, 5, 5, 5 },
	{ 64, 0, 96, 32, 5, 5, 5, 5 },
	{ 96, 0, 128, 32, 5, 5, 5, 5 },
	{ 128, 0, 160, 32, 5, 5, 5, 5 },

	// panel
	{ 160, 0, 192, 32, 6, 6, 6, 6 },

	// textbox
	{ 192, 0, 224, 32, 5, 5, 5, 5 },
	{ 224, 0, 256, 32, 5, 5, 5, 5 },

	// checkbgr
	{ 0, 32, 32, 64, 5, 5, 5, 5 },
	{ 32, 32, 64, 64, 5, 5, 5, 5 },
	{ 64, 32, 96, 64, 5, 5, 5, 5 },
	{ 96, 32, 128, 64, 5, 5, 5, 5 },

	// checkmarks
	{ 128, 32, 160, 64, 5, 5, 5, 5 },
	{ 160, 32, 192, 64, 5, 5, 5, 5 },
	{ 192, 32, 224, 64, 5, 5, 5, 5 },
	{ 224, 32, 256, 64, 5, 5, 5, 5 },

	// radiobgr
	{ 0, 64, 32, 96, 0, 0, 0, 0 },
	{ 32, 64, 64, 96, 0, 0, 0, 0 },
	{ 64, 64, 96, 96, 0, 0, 0, 0 },
	{ 96, 64, 128, 96, 0, 0, 0, 0 },

	// radiomark
	{ 128, 64, 160, 96, 0, 0, 0, 0 },
	{ 160, 64, 192, 96, 0, 0, 0, 0 },

	// selector
	{ 192, 64, 224, 96, 0, 0, 0, 0 },
	{ 224, 64, 240, 80, 0, 0, 0, 0 },
	{ 240, 64, 252, 76, 0, 0, 0, 0 },
	// outline
	{ 224, 80, 240, 96, 4, 4, 4, 4 },

	// tab
	{ 0, 96, 32, 128, 5, 5, 5, 5 },
	{ 32, 96, 64, 128, 5, 5, 5, 5 },
	{ 64, 96, 96, 128, 5, 5, 5, 5 },
	{ 96, 96, 128, 128, 5, 5, 5, 5 },

	// treetick
	{ 128, 96, 160, 128, 0, 0, 0, 0 },
	{ 160, 96, 192, 128, 0, 0, 0, 0 },
	{ 192, 96, 224, 128, 0, 0, 0, 0 },
	{ 224, 96, 256, 128, 0, 0, 0, 0 },

#else
	{ 0, 0, 64, 64, 5, 5, 5, 5 },
	{ 64, 0, 128, 64, 5, 5, 5, 5 },
	{ 128, 0, 192, 64, 5, 5, 5, 5 },

	{ 192, 0, 224, 32, 5, 5, 5, 5 },
	{ 224, 0, 256, 32, 5, 5, 5, 5 },
	{ 192, 32, 224, 64, 5, 5, 5, 5 },
	{ 224, 32, 256, 64, 5, 5, 5, 5 },
#endif
};

ui::draw::Texture* g_themeTextures[TE__COUNT];

void InitTheme()
{
	int size[2];
	auto* data = LoadTGA("gui-theme2.tga", size);
	for (int i = 0; i < TE__COUNT; i++)
	{
		auto& s = g_themeSprites[i];
		g_themeTextures[i] = ui::draw::TextureCreateRGBA8(s.ox1 - s.ox0, s.oy1 - s.oy0, size[0] * 4, &data[(s.ox0 + s.oy0 * size[0]) * 4]);
	}
	delete[] data;
}

AABB<float> GetThemeElementBorderWidths(EThemeElement e)
{
	auto& s = g_themeSprites[e];
	return { float(s.bx0), float(s.by0), float(s.bx1), float(s.by1) };
}

void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1)
{
	const Sprite& s = g_themeSprites[e];
	AABB<float> outer = { x0, y0, x1, y1 };
	AABB<float> inner = outer.ShrinkBy({ float(s.bx0), float(s.by0), float(s.bx1), float(s.by1) });
	float ifw = 1.0f / (s.ox1 - s.ox0), ifh = 1.0f / (s.oy1 - s.oy0);
	int s_ix0 = s.bx0, s_iy0 = s.by0, s_ix1 = (s.ox1 - s.ox0) - s.bx1, s_iy1 = (s.oy1 - s.oy0) - s.by1;
	AABB<float> texinner = { s_ix0 * ifw, s_iy0 * ifh, s_ix1 * ifw, s_iy1 * ifh };
	ui::draw::RectColTex9Slice(outer, inner, ui::Color4b::White(), g_themeTextures[e], { 0, 0, 1, 1 }, texinner);
}
