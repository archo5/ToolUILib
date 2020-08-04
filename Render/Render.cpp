
#include <stdio.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "../stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb_truetype.h"

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
		rhiTex = rhi::CreateTextureRGBA8(nullptr, TEXTURE_PAGE_WIDTH, TEXTURE_PAGE_HEIGHT, DEFAULT_FILTERING);
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
	TextureNode* AllocNode(int w, int h, const void* d, bool a8)
	{
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


struct Texture
{
	Texture(int w, int h, const void* d, bool a8, bool flt) :
		width(w), height(h), isFilteringEnabled(flt)
	{
		data = new uint8_t[w * h * 4];
		if (!a8)
			memcpy(data, d, w * h * 4);
		else
		{
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					data[(x + y * w) * 4 + 0] = 255;
					data[(x + y * w) * 4 + 1] = 255;
					data[(x + y * w) * 4 + 2] = 255;
					data[(x + y * w) * 4 + 3] = ((const char*)d)[x + y * w];
				}
			}
		}
		if (auto* n = g_textureStorage.AllocNode(w, h, data, false))
			atlasNode = n;
		else
			rhiTex = a8 ? rhi::CreateTextureA8(d, w, h) : rhi::CreateTextureRGBA8(d, w, h, flt);
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
	bool isFilteringEnabled;
	rhi::Texture2D* rhiTex = nullptr;
	TextureNode* atlasNode = nullptr;
};


Texture* TextureCreateRGBA8(int w, int h, const void* data, bool filtering)
{
	return new Texture(w, h, data, false, filtering);
}

Texture* TextureCreateA8(int w, int h, const void* data, bool filtering)
{
	return new Texture(w, h, data, true, filtering);
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
static Texture* g_curTex;

static rhi::Texture2D* GetRHITex(Texture* tex)
{
	return tex ? tex->GetRHITex() : nullptr;
}

void _Flush()
{
	if (!g_numIndices)
		return;
	rhi::SetTexture(GetRHITex(g_curTex));
	rhi::DrawIndexedTriangles(g_bufVertices, g_bufIndices, g_numIndices);
	g_numVertices = 0;
	g_numIndices = 0;
}

void IndexedTriangles(Texture* tex, rhi::Vertex* verts, size_t num_vertices, uint16_t* indices, size_t num_indices)
{
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
		rhi::SetTexture(GetRHITex(g_curTex));
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

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col)
{
	if (x0 == x1 && y0 == y1)
		return;

	float dx = x1 - x0;
	float dy = y1 - y0;
	float lensq = dx * dx + dy * dy;
	float invlen = 1.0f / sqrtf(lensq);
	dx *= invlen;
	dy *= invlen;
	float tx = -dy * 0.5f * w;
	float ty = dx * 0.5f * w;

	rhi::Vertex verts[4] =
	{
		{ x0 + tx, y0 + ty, 0, 0, col },
		{ x1 + tx, y1 + ty, 0, 0, col },
		{ x1 - tx, y1 - ty, 0, 0, col },
		{ x0 - tx, y0 - ty, 0, 0, col },
	};
	uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	IndexedTriangles(0, verts, 4, indices, 6);
}

void RectCol(float x0, float y0, float x1, float y1, Color4b col)
{
	RectColTex(x0, y0, x1, y1, col, nullptr, 0, 0, 1, 1);
}

void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b)
{
	rhi::Vertex verts[4] =
	{
		{ x0, y0, 0, 0, a },
		{ x1, y0, 0, 0, b },
		{ x1, y1, 0, 0, b },
		{ x0, y1, 0, 0, a },
	};
	uint16_t indices[6] = { 0, 1, 2, 2, 3, 0 };

	IndexedTriangles(0, verts, 4, indices, 6);
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
		{ rect.x0, rect.y0, 0, 0, col },
		{ rect.x1, rect.y0, 0, 0, col },
		{ rect.x1, rect.y1, 0, 0, col },
		{ rect.x0, rect.y1, 0, 0, col },

		{ cutr.x0, cutr.y0, 0, 0, col },
		{ cutr.x1, cutr.y0, 0, 0, col },
		{ cutr.x1, cutr.y1, 0, 0, col },
		{ cutr.x0, cutr.y1, 0, 0, col },
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

} // draw
} // ui


unsigned char ttf_buffer[1 << 20];
unsigned char temp_bitmap[512 * 512];

stbtt_fontinfo g_fontInfo;
stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
ui::draw::Texture* g_fontGlyphTextures[96];

void InitFont()
{
	fread(ttf_buffer, 1, 1 << 20, fopen("c:/windows/fonts/segoeui.ttf", "rb"));
	stbtt_InitFont(&g_fontInfo, ttf_buffer, 0);
	stbtt_BakeFontBitmap(ttf_buffer, 0, -12.0f, temp_bitmap, 512, 512, 32, 96, cdata); // no guarantee this fits!
}

float GetTextWidth(const char* text, size_t num)
{
	float out = 0;
	for (size_t i = 0; num != SIZE_MAX ? i < num : *text; i++)
	{
		if (*text >= 32 && *text < 128)
			out += cdata[*text - 32].xadvance;
		text++;
	}
	return out;
}

float GetFontHeight()
{
	return 12;
}

void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a)
{
	float scale = stbtt_ScaleForMappingEmToPixels(&g_fontInfo, 12.0f);
	// assume orthographic projection with units = screen pixels, origin at top left
	ui::Color4b col = ui::Color4f(r, g, b, a);
	while (*text)
	{
		if (*text >= 32 && *text < 128)
		{
			ui::draw::Texture*& GT = g_fontGlyphTextures[*text - 32];
			if (!GT)
			{
				int gid = stbtt_FindGlyphIndex(&g_fontInfo, *text);
				if (gid != 0)
				{
					int x, y, w, h;
					auto* bitmap = stbtt_GetGlyphBitmap(&g_fontInfo, scale, scale, gid, &w, &h, &x, &y);
					GT = ui::draw::TextureCreateA8(w, h, bitmap);
					stbtt_FreeBitmap(bitmap, nullptr);
				}
			}
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata, 512, 512, *text - 32, &x, &y, &q, 1);//1=opengl & d3d10+,0=d3d9
			ui::draw::RectColTex(q.x0, q.y0, q.x1, q.y1, col, GT);
		}
		++text;
	}
}


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

ui::draw::Texture* g_themeTexture;
int g_themeWidth, g_themeHeight;

void InitTheme()
{
	int size[2];
	auto* data = LoadTGA("gui-theme2.tga", size);
	g_themeTexture = ui::draw::TextureCreateRGBA8(size[0], size[1], data);
	g_themeWidth = size[0];
	g_themeHeight = size[1];
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
	float ifw = 1.0f / g_themeWidth, ifh = 1.0f / g_themeHeight;
	int s_ix0 = s.ox0 + s.bx0, s_iy0 = s.oy0 + s.by0, s_ix1 = s.ox1 - s.bx1, s_iy1 = s.oy1 - s.by1;
	AABB<float> texouter = { s.ox0 * ifw, s.oy0 * ifh, s.ox1 * ifw, s.oy1 * ifh };
	AABB<float> texinner = { s_ix0 * ifw, s_iy0 * ifh, s_ix1 * ifw, s_iy1 * ifh };
	ui::draw::RectColTex9Slice(outer, inner, ui::Color4b::White(), g_themeTexture, texouter, texinner);
}
