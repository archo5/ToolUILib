
#pragma once

#pragma warning(disable:4996)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max


namespace GL {

using TexID = unsigned int;

class RenderContext;
RenderContext* CreateRenderContext(HWND window);
void FreeRenderContext(RenderContext* RC);
void SetActiveContext(RenderContext* RC);

void SetViewport(int x0, int y0, int x1, int y1);

void Clear(int r, int g, int b, int a);
void Present(RenderContext* RC);

TexID CreateTextureA8(unsigned char* data, unsigned width, unsigned height);
TexID CreateTextureRGBA8(unsigned char* data, unsigned width, unsigned height);
void SetTexture(TexID tex);

struct BatchRenderer
{
	void Begin();
	void End();

	void SetColor(float r, float g, float b, float a = 1);
	void Pos(float x, float y);
	void Quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);
};

} // namespace GL
