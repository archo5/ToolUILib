
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


namespace ui {
namespace rhi {


static Stats g_stats;

Stats Stats::operator - (const Stats& o) const
{
	Stats r;
	r.num_SetTexture = num_SetTexture - o.num_SetTexture;
	r.num_DrawTriangles = num_DrawTriangles - o.num_DrawTriangles;
	r.num_DrawIndexedTriangles = num_DrawIndexedTriangles - o.num_DrawIndexedTriangles;
	return r;
}

Stats Stats::Get()
{
	return g_stats;
}


#define GLCHK(x) do { x; _Check(#x, __FILE__, __LINE__); } while (0)
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

struct RenderContext
{
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
	GLCHK(glDisable(GL_DEPTH_TEST));
	GLCHK(glEnable(GL_BLEND));
	GLCHK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GLCHK(glEnableClientState(GL_VERTEX_ARRAY));
	GLCHK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GLCHK(glEnableClientState(GL_COLOR_ARRAY));
	GLCHK(glEnable(GL_SCISSOR_TEST));

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

static int curRTTHeight;

void SetScissorRect(int x0, int y0, int x1, int y1)
{
	GLCHK(glScissor(x0, curRTTHeight - y1, std::max(x1 - x0, 0), std::max(y1 - y0, 0)));
}

RECT g_viewport;
void SetViewport(int x0, int y0, int x1, int y1)
{
	g_viewport = { x0, y0, x1, y1 };
	GLCHK(glViewport(x0, y0, x1 - x0, y1 - y0));
	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadIdentity());
	GLCHK(glOrtho(x0, x1, y1, y0, -1, 1));
	curRTTHeight = y1; // TODO fix
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

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height)
{
	GLuint tex;
	GLCHK(glGenTextures(1, &tex));
	GLCHK(glBindTexture(GL_TEXTURE_2D, tex));
	GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCHK(glEnable(GL_TEXTURE_2D));

	return (Texture2D*) tex;
}

Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, bool filtering)
{
	GLuint tex;
	GLCHK(glGenTextures(1, &tex));
	GLCHK(glBindTexture(GL_TEXTURE_2D, tex));
	GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering ? GL_LINEAR : GL_NEAREST));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering ? GL_LINEAR : GL_NEAREST));
	GLCHK(glEnable(GL_TEXTURE_2D));

	return (Texture2D*) tex;
}

void DestroyTexture(Texture2D* tex)
{
	GLuint texid = (GLuint)tex;
	GLCHK(glDeleteTextures(1, &texid));
}

MapData MapTexture(Texture2D* tex)
{
	// no-op
	return {};
}

void CopyToMappedTextureRect(Texture2D* tex, const MapData& md, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void* data, bool a8)
{
	GLCHK(glBindTexture(GL_TEXTURE_2D, (GLuint)tex));
	GLCHK(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, a8 ? GL_ALPHA : GL_RGBA, GL_UNSIGNED_BYTE, data));
}

void UnmapTexture(Texture2D* tex)
{
	// no-op
}

void SetTexture(Texture2D* tex)
{
	g_stats.num_SetTexture++;
	GLCHK(glBindTexture(GL_TEXTURE_2D, (GLuint)tex));
	if (tex != 0)
		GLCHK(glEnable(GL_TEXTURE_2D));
	else
		GLCHK(glDisable(GL_TEXTURE_2D));
}

void DrawTriangles(Vertex* verts, size_t num_verts)
{
	g_stats.num_DrawTriangles++;
	GLCHK(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].x));
	GLCHK(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].u));
	GLCHK(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &verts[0].col));
	GLCHK(glDrawArrays(GL_TRIANGLES, 0, num_verts));
}

void DrawIndexedTriangles(Vertex* verts, uint16_t* indices, size_t num_indices)
{
	g_stats.num_DrawIndexedTriangles++;
	GLCHK(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].x));
	GLCHK(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].u));
	GLCHK(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &verts[0].col));
	GLCHK(glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, indices));
}


void SetRenderState(unsigned drawFlags)
{
	if (drawFlags & DF_Lit)
		GLCHK(glEnable(GL_LIGHTING));
	else
		GLCHK(glDisable(GL_LIGHTING));

	if (drawFlags & DF_AlphaBlended)
		GLCHK(glEnable(GL_BLEND));
	else
		GLCHK(glDisable(GL_BLEND));

	if (drawFlags & DF_ZTestOff)
		GLCHK(glDisable(GL_DEPTH_TEST));
	else
		GLCHK(glEnable(GL_DEPTH_TEST));

	GLCHK(glDepthMask(drawFlags & DF_ZWriteOff ? GL_FALSE : GL_TRUE));

	if (drawFlags & DF_Cull)
		GLCHK(glEnable(GL_CULL_FACE));
	else
		GLCHK(glDisable(GL_CULL_FACE));
}

static Mat4f g_viewMatrix = Mat4f::Identity();
void SetViewMatrix(const Mat4f& m)
{
	g_viewMatrix = m;
}

static Mat4f g_perspAdjust = Mat4f::Translate(0, 0, -0.5f) * Mat4f::Scale(1, 1, 2);
void SetPerspectiveMatrix(const Mat4f& m)
{
	auto fm = m * g_perspAdjust;
	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadMatrixf(m.a));
	GLCHK(glMatrixMode(GL_MODELVIEW));
}

static GLint g_prevTex;
void Begin3DMode(int x0, int y0, int x1, int y1)
{
	GLCHK(glGetIntegerv(GL_TEXTURE_BINDING_2D, &g_prevTex));

	GLCHK(glViewport(x0, y0, x1 - x0, y1 - y0));

	SetRenderState(0);
	SetPerspectiveMatrix(Mat4f::Identity());
	SetAmbientLight(Color4f::White());
	for (int i = 0; i < 8; i++)
		SetLightOff(i);

	SetTexture(nullptr);
}

void End3DMode()
{
	GLCHK(glDisableClientState(GL_NORMAL_ARRAY));
	GLCHK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GLCHK(glEnableClientState(GL_COLOR_ARRAY));

	SetRenderState(DF_AlphaBlended | DF_ZTestOff | DF_ZWriteOff);

	auto r = g_viewport;
	int x0 = r.left, x1 = r.right, y0 = r.top, y1 = r.bottom;
	GLCHK(glViewport(x0, y0, x1 - x0, y1 - y0));

	GLCHK(glMatrixMode(GL_MODELVIEW));
	GLCHK(glLoadIdentity());
	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadIdentity());
	GLCHK(glOrtho(x0, x1, y1, y0, -1, 1));

	GLCHK(glBindTexture(GL_TEXTURE_2D, g_prevTex));
	if (g_prevTex != 0)
		GLCHK(glEnable(GL_TEXTURE_2D));
	else
		GLCHK(glDisable(GL_TEXTURE_2D));
}

void SetAmbientLight(const Color4f& col)
{
	GLCHK(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &col.r));
}

void SetLightOff(int n)
{
	GLCHK(glDisable(GL_LIGHT0 + n));
}

void SetDirectionalLight(int n, float x, float y, float z, const Color4f& col)
{
	GLCHK(glEnable(GL_LIGHT0 + n));
	GLCHK(glLightfv(GL_LIGHT0 + n, GL_DIFFUSE, &col.r));
	float dir[4] = { x, y, z, 0 };
	GLCHK(glLightfv(GL_LIGHT0 + n, GL_POSITION, dir));
}

static void ApplyVertexData(unsigned vertexFormat, const void* vertices)
{
	auto v = (const char*)vertices;

	GLsizei stride = sizeof(float) * 3;
	if (vertexFormat & VF_Normal)
		stride += sizeof(float) * 3;
	if (vertexFormat & VF_Texcoord)
		stride += sizeof(float) * 2;
	if (vertexFormat & VF_Color)
		stride += 4;

	GLCHK(glVertexPointer(3, GL_FLOAT, stride, v));
	v += sizeof(float) * 3;

	if (vertexFormat & VF_Normal)
	{
		GLCHK(glNormalPointer(GL_FLOAT, stride, v));
		GLCHK(glEnableClientState(GL_NORMAL_ARRAY));
		v += sizeof(float) * 3;
	}
	else
		GLCHK(glDisableClientState(GL_NORMAL_ARRAY));

	if (vertexFormat & VF_Texcoord)
	{
		GLCHK(glTexCoordPointer(2, GL_FLOAT, stride, v));
		GLCHK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		v += sizeof(float) * 2;
	}
	else
		GLCHK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

	if (vertexFormat & VF_Color)
	{
		GLCHK(glColorPointer(4, GL_UNSIGNED_BYTE, stride, v));
		GLCHK(glEnableClientState(GL_COLOR_ARRAY));
		v += 4;
	}
	else
		GLCHK(glDisableClientState(GL_COLOR_ARRAY));
}

static GLenum ConvertPrimitiveType(PrimitiveType t)
{
	switch (t)
	{
	case PT_Lines: return GL_LINES;
	case PT_Triangles: return GL_TRIANGLES;
	case PT_TriangleStrip: return GL_TRIANGLE_STRIP;
	default: return GL_TRIANGLES;
	}
}

void Draw(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices)
{
	GLCHK(glLoadMatrixf((xf * g_viewMatrix).a));
	ApplyVertexData(vertexFormat, vertices);
	GLCHK(glDrawArrays(ConvertPrimitiveType(primType), 0, numVertices));
}

void DrawIndexed(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices,
	const uint16_t* indices,
	size_t numIndices)
{
	GLCHK(glLoadMatrixf((xf * g_viewMatrix).a));
	ApplyVertexData(vertexFormat, vertices);
	GLCHK(glDrawElements(ConvertPrimitiveType(primType), numIndices, GL_UNSIGNED_SHORT, indices));
}

} // rhi
} // ui
