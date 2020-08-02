
#pragma once


namespace GL {

struct Col32f
{
	float r, g, b, a;
};

struct Vertex
{
	float x, y;
	float u, v;
	Col32f col;
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

struct BatchRenderer
{
	void Begin();
	void End();

	void SetColor(float r, float g, float b, float a = 1);
	void Pos(float x, float y);
	void Quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);
	void Line(float x0, float y0, float x1, float y1, float w = 1);

	Col32f col;
};

} // namespace GL
