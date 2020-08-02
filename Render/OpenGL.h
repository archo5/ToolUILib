
#pragma once


namespace GL {

struct Col8b
{
	unsigned char r, g, b, a;
};

struct Vertex
{
	float x, y;
	float u, v;
	Col8b col;
};

struct Texture2D;
struct RenderContext;

RenderContext* CreateRenderContext(void* window);
void FreeRenderContext(RenderContext* RC);
void SetActiveContext(RenderContext* RC);

void SetViewport(int x0, int y0, int x1, int y1);
void PushScissorRect(int x0, int y0, int x1, int y1);
void PopScissorRect();

void Clear(int r, int g, int b, int a);
void Present(RenderContext* RC);

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height);
Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, bool filtering = true);
void DestroyTexture(Texture2D* tex);
void SetTexture(Texture2D* tex);
void DrawTriangles(Vertex* verts, size_t num_verts);

} // namespace GL
