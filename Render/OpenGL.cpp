
#include <stdio.h>
#include <stdlib.h>

#include "OpenGL.h"
#include "../Core/Math.h"

#pragma warning(disable:4996)

#define WIN32_LEAN_AND_MEAN
#define NONLS
#define NOMINMAX
#include <Windows.h>
#include <gl/GL.h>


namespace GL {


#define GLCHK(x) do { x; ::GL::_Check(#x, __FILE__, __LINE__); } while (0)
void _Check(const char* code, const char* file, int line)
{
	auto err = glGetError();
	if (err != GL_NO_ERROR)
	{
		char bfr[2048];
		sprintf(bfr, "Error: %d\nCode: %s\nFile: %s\nLine: %d\nDo you wish to continue?", (int)err, code, file, line);
		if (MessageBoxA(nullptr, bfr, "OpenGL error", MB_ICONERROR | MB_YESNO) == IDNO)
		{
			exit(EXIT_FAILURE);
		}
	}
}

class RenderContext
{
public:
	void AddToList()
	{
		if (!first)
			first = this;
		prev = last;
		next = nullptr;
		last = this;
	}
	~RenderContext()
	{
		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;
		if (first == this)
			first = first->next;
		if (last == this)
			last = last->prev;
	}

	HDC dc;
	HGLRC rc;

	static RenderContext* first;
	static RenderContext* last;
	RenderContext* prev = nullptr;
	RenderContext* next = nullptr;
};
RenderContext* RenderContext::first;
RenderContext* RenderContext::last;

RenderContext* CreateRenderContext(void* window)
{
	RenderContext* RC = new RenderContext();
	RC->dc = GetDC((HWND)window);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	int format = ChoosePixelFormat(RC->dc, &pfd);
	SetPixelFormat(RC->dc, format, &pfd);

	RC->rc = wglCreateContext(RC->dc);
	wglMakeCurrent(RC->dc, RC->rc);
	if (RenderContext::first)
		wglShareLists(RenderContext::first->rc, RC->rc);

	((BOOL(__stdcall *)(int))wglGetProcAddress("wglSwapIntervalEXT"))(0);

	GLCHK(glDisable(GL_CULL_FACE));
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	RC->AddToList();
	return RC;
}

void FreeRenderContext(RenderContext* RC)
{
	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(RC->rc);
	// ReleaseDC(RC->dc); - TODO?
	delete RC;
}

void SetActiveContext(RenderContext* RC)
{
	wglMakeCurrent(RC->dc, RC->rc);
}

RECT scissorStack[100];
int scissorCount = 1;

void SetViewport(int x0, int y0, int x1, int y1)
{
	glViewport(x0, y0, x1 - x0, y1 - y0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLCHK(glOrtho(x0, x1, y1, y0, -1, 1));
	scissorStack[0] = { x0, y0, x1, y1 };
	scissorCount = 1;
}

void ApplyScissor()
{
	RECT r = scissorStack[scissorCount - 1];
	GLCHK(glScissor(r.left, scissorStack[0].bottom - r.bottom, std::max(r.right - r.left, 0L), std::max(r.bottom - r.top, 0L)));
}

void PushScissorRect(int x0, int y0, int x1, int y1)
{
	int i = scissorCount++;
	RECT r = scissorStack[i - 1];
	if (r.left < x0) r.left = x0;
	if (r.right > x1) r.right = x1;
	if (r.top < y0) r.top = y0;
	if (r.bottom > y1) r.bottom = y1;
	scissorStack[i] = r;
	ApplyScissor();
	GLCHK(glEnable(GL_SCISSOR_TEST));
}

void PopScissorRect()
{
	scissorCount--;
	ApplyScissor();
	if (scissorCount <= 1)
	{
		GLCHK(glDisable(GL_SCISSOR_TEST));
	}
}

void Clear(int r, int g, int b, int a)
{
	glClearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	glClearDepth(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Present(RenderContext* RC)
{
#if 0
	GLCHK(glDisable(GL_TEXTURE_2D));
	glBegin(GL_TRIANGLES);
	glVertex2f(-10, -10);
	glVertex2f(10, -10);
	glVertex2f(0, 10);
	glVertex2f(0, 10);
	glVertex2f(10, -10);
	glVertex2f(-10, -10);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#endif
#if 0
	GLCHK(glDisable(GL_TEXTURE_2D));
	glBegin(GL_QUADS);
	glVertex2f(10, 10);
	glVertex2f(20, 10);
	glVertex2f(20, 20);
	glVertex2f(10, 20);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#endif

	SwapBuffers(RC->dc);
}

TexID CreateTextureA8(const void* data, unsigned width, unsigned height)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	return tex;
}

TexID CreateTextureRGBA8(const void* data, unsigned width, unsigned height, bool filtering)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering ? GL_LINEAR : GL_NEAREST);
	glEnable(GL_TEXTURE_2D);

	return tex;
}

void DestroyTexture(TexID tex)
{
	glDeleteTextures(1, &tex);
}

void SetTexture(TexID tex)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	if (tex != 0)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
}


void BatchRenderer::Begin()
{
	glBegin(GL_TRIANGLES);
}

void BatchRenderer::End()
{
	glEnd();
}

void BatchRenderer::SetColor(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}

void BatchRenderer::Pos(float x, float y)
{
	glVertex2f(x, y);
}

void BatchRenderer::Quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1)
{
	glTexCoord2f(u0, v0); glVertex2f(x0, y0);
	glTexCoord2f(u1, v0); glVertex2f(x1, y0);
	glTexCoord2f(u1, v1); glVertex2f(x1, y1);

	glTexCoord2f(u1, v1); glVertex2f(x1, y1);
	glTexCoord2f(u0, v1); glVertex2f(x0, y1);
	glTexCoord2f(u0, v0); glVertex2f(x0, y0);
}

void DrawLine(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
	SetTexture(0);
	glBegin(GL_LINES);
	glColor4f(r, g, b, a);
	glVertex2f(x0, y0);
	glVertex2f(x1, y1);
	glEnd();
}

} // namespace GL
