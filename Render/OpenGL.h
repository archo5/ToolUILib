
#pragma once
#include "../Core/Image.h"


namespace ui {
namespace rhi {

struct Stats
{
	uint64_t num_SetTexture;
	uint64_t num_DrawTriangles;
	uint64_t num_DrawIndexedTriangles;

	Stats operator - (const Stats& o) const;

	static Stats Get();
};

struct Vertex
{
	float x, y;
	float u, v;
	Color4b col;
};

struct Texture2D;
struct RenderContext;

RenderContext* CreateRenderContext(void* window);
void FreeRenderContext(RenderContext* RC);
void SetActiveContext(RenderContext* RC);

void SetViewport(int x0, int y0, int x1, int y1);
void SetScissorRect(int x0, int y0, int x1, int y1);

void Clear(int r, int g, int b, int a);
void Present(RenderContext* RC);

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height);
Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, bool filtering = true);
void DestroyTexture(Texture2D* tex);
void SetTexture(Texture2D* tex);
void DrawTriangles(Vertex* verts, size_t num_verts);
void DrawIndexedTriangles(Vertex* verts, uint16_t* indices, size_t num_indices);

} // rhi
} // ui
