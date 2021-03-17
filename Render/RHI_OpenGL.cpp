
#include <stdio.h>
#include <stdlib.h>

#include "RHI.h"
#include "Render.h"
#include "../Core/Math.h"

#define WIN32_LEAN_AND_MEAN
#define NONLS
#define NOMINMAX
#include <Windows.h>
#include <gl/GL.h>


namespace ui {
namespace rhi {


extern Stats g_stats;


#define GLCHK(x) do { x; _Check(#x, __FILE__, __LINE__); } while (0)
static void _Check(const char* code, const char* file, int line)
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

void GlobalInit()
{
}

void GlobalFree()
{
}

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
	GLCHK(glDepthFunc(GL_LEQUAL));
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

void OnResizeWindow(RenderContext* RC, unsigned w, unsigned h)
{
}

static int curRTTHeight;

void SetScissorRect(int x0, int y0, int x1, int y1)
{
	GLCHK(glScissor(x0, curRTTHeight - y1, max(x1 - x0, 0), max(y1 - y0, 0)));
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

void ClearDepthOnly()
{
	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);
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

static void ApplyFlags(uint8_t flags)
{
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, flags & TF_FILTER ? GL_LINEAR : GL_NEAREST));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, flags & TF_FILTER ? GL_LINEAR : GL_NEAREST));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, flags & TF_REPEAT ? GL_REPEAT : GL_CLAMP));
	GLCHK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, flags & TF_REPEAT ? GL_REPEAT : GL_CLAMP));
}

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height, uint8_t flags)
{
	GLuint tex;
	GLCHK(glGenTextures(1, &tex));
	GLCHK(glBindTexture(GL_TEXTURE_2D, tex));
	GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data));
	ApplyFlags(flags);
	GLCHK(glEnable(GL_TEXTURE_2D));

	return (Texture2D*) tex;
}

Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, uint8_t flags)
{
	GLuint tex;
	GLCHK(glGenTextures(1, &tex));
	GLCHK(glBindTexture(GL_TEXTURE_2D, tex));
	GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data));
	ApplyFlags(flags);
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

void DrawIndexedTriangles(Vertex* verts, size_t /*num_verts*/, uint16_t* indices, size_t num_indices)
{
	g_stats.num_DrawIndexedTriangles++;
	GLCHK(glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].x));
	GLCHK(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &verts[0].u));
	GLCHK(glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), &verts[0].col));
	GLCHK(glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, indices));
}


static unsigned g_drawFlags;
void SetRenderState(unsigned drawFlags)
{
	if (drawFlags & DF_AlphaTest)
	{
		GLCHK(glEnable(GL_ALPHA_TEST));
		GLCHK(glAlphaFunc(GL_GREATER, 0.5f));
	}
	else
		GLCHK(glDisable(GL_ALPHA_TEST));

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

	GLCHK(glPolygonMode(GL_FRONT_AND_BACK, drawFlags & DF_Wireframe ? GL_LINE : GL_FILL));
	if (drawFlags & DF_Wireframe)
	{
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-2, -2);
	}
	else
	{
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(0, 0);
	}

	g_drawFlags = drawFlags;
}

static Mat4f g_viewMatrix = Mat4f::Identity();
void SetViewMatrix(const Mat4f& m)
{
	g_viewMatrix = m;
}

static Mat4f g_perspAdjust = Mat4f::Translate(0, 0, -0.5f) * Mat4f::Scale(1, 1, 2);
void SetProjectionMatrix(const Mat4f& m)
{
	auto fm = m * g_perspAdjust;
	GLCHK(glMatrixMode(GL_PROJECTION));
	GLCHK(glLoadMatrixf(m.a));
	GLCHK(glMatrixMode(GL_MODELVIEW));
}

static Color4b g_forcedColor;
void SetForcedColor(const Color4b& col)
{
	g_forcedColor = col;
}

static GLint g_prevTex;
static AABB<int> g_3DRect;
void Begin3DMode(const AABB<int>& rect)
{
	draw::internals::Flush();

	GLCHK(glGetIntegerv(GL_TEXTURE_BINDING_2D, &g_prevTex));

	GLCHK(glViewport(rect.x0, curRTTHeight - rect.y1, max(rect.x1 - rect.x0, 0), max(rect.y1 - rect.y0, 0)));

	SetRenderState(0);
	SetProjectionMatrix(Mat4f::Identity());
	SetAmbientLight(Color4f::White());
	for (int i = 0; i < 8; i++)
		SetLightOff(i);

	g_forcedColor = { 255, 0, 255 };

	SetTexture(nullptr);

	g_3DRect = rect;
}

AABB<int> End3DMode()
{
	auto curRect = g_3DRect;
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

	return curRect;
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

	if ((vertexFormat & VF_Color) && !(g_drawFlags & DF_ForceColor))
	{
		GLCHK(glColorPointer(4, GL_UNSIGNED_BYTE, stride, v));
		GLCHK(glEnableClientState(GL_COLOR_ARRAY));
		v += 4;
	}
	else
		GLCHK(glDisableClientState(GL_COLOR_ARRAY));
	if (g_drawFlags & DF_ForceColor)
	{
		Color4f fc = g_forcedColor;
		glColor4f(fc.r, fc.g, fc.b, fc.a);
	}
}

static GLenum ConvertPrimitiveType(PrimitiveType t)
{
	switch (t)
	{
	case PT_Points: return GL_POINTS;
	case PT_Lines: return GL_LINES;
	case PT_LineStrip: return GL_LINE_STRIP;
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
	const void* indices,
	size_t numIndices,
	bool i32)
{
	GLCHK(glLoadMatrixf((xf * g_viewMatrix).a));
	ApplyVertexData(vertexFormat, vertices);
	GLCHK(glDrawElements(ConvertPrimitiveType(primType), numIndices, i32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, indices));
}

} // rhi
} // ui
