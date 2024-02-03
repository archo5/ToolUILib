
#include "Render.h"

#include "RHI.h"

#include "../Core/Array.h"
#include "../Core/FileSystem.h"
#include "../Core/HashMap.h"


namespace ui {
namespace draw {

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
	return tex ? tex->GetInternal() : nullptr;
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

namespace _ {

void InitResources()
{
}

void FreeResources()
{
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

void RestoreStates()
{
	rhi::RestoreRenderStates();
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
	_::TextureStorage_FlushPendingAllocs();
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
		_::TextureStorage_RemapUVs(&g_bufVertices[g_numVertices], num_vertices, g_curTex);
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
	auto* vdest = verts.Data();
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
	uint16_t* idest = indices.Data();
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

void RectCol(const AABB2f& r, Color4b col)
{
	RectColTex(r, col, nullptr);
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

void RectGradV(float x0, float y0, float x1, float y1, Color4b a, Color4b b)
{
	rhi::Vertex verts[4] =
	{
		{ x0, y0, 0.5f, 0.5f, a },
		{ x1, y0, 0.5f, 0.5f, a },
		{ x1, y1, 0.5f, 0.5f, b },
		{ x0, y1, 0.5f, 0.5f, b },
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
