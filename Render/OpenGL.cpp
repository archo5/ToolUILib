
#include <stdio.h>
#include <stdlib.h>

#include "OpenGL.h"

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
	HDC dc;
	HGLRC rc;
};

HGLRC prevRC;

RenderContext* CreateRenderContext(HWND window)
{
	RenderContext* RC = new RenderContext();
	RC->dc = GetDC(window);

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
	if (prevRC)
		wglShareLists(prevRC, RC->rc);
	prevRC = RC->rc; // TODO fix

	GLCHK(glDisable(GL_CULL_FACE));
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return RC;
}

void FreeRenderContext(RenderContext* RC)
{
	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(RC->rc);
	// ReleaseDC(RC->dc); - TODO?
}

void SetActiveContext(RenderContext* RC)
{
	wglMakeCurrent(RC->dc, RC->rc);
}

void SetViewport(int x0, int y0, int x1, int y1)
{
	glViewport(x0, y0, x1 - x0, y1 - y0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLCHK(glOrtho(x0, x1, y1, y0, -1, 1));
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

TexID CreateTextureA8(unsigned char* data, unsigned width, unsigned height)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	return tex;
}

TexID CreateTextureRGBA8(unsigned char* data, unsigned width, unsigned height)
{
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	return tex;
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

} // namespace GL
