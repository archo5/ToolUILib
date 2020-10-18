
#pragma once
#include "../Core/Image.h"
#include "../Core/3DMath.h"


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
void ClearDepthOnly();
void Present(RenderContext* RC);

constexpr uint8_t TF_FILTER = 1 << 0;
constexpr uint8_t TF_REPEAT = 1 << 1;
Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height, uint8_t flags);
Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, uint8_t flags);
void DestroyTexture(Texture2D* tex);

struct MapData
{
	void* data;
	uint32_t pitch;
};
MapData MapTexture(Texture2D* tex);
void CopyToMappedTextureRect(Texture2D* tex, const MapData& md, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void* data, bool a8);
void UnmapTexture(Texture2D* tex);

void SetTexture(Texture2D* tex);
void DrawTriangles(Vertex* verts, size_t num_verts);
void DrawIndexedTriangles(Vertex* verts, uint16_t* indices, size_t num_indices);


void Begin3DMode(int x0, int y0, int x1, int y1);
void End3DMode();
void SetViewMatrix(const Mat4f& m);
void SetPerspectiveMatrix(const Mat4f& m);
void SetForcedColor(const Color4b& col);
void SetAmbientLight(const Color4f& col);
void SetLightOff(int n);
void SetDirectionalLight(int n, float x, float y, float z, const Color4f& col);
enum DrawFlags
{
	DF_Lit          = 1 << 2,
	DF_AlphaBlended = 1 << 3,
	DF_ZTestOff     = 1 << 4,
	DF_ZWriteOff    = 1 << 5,
	DF_Cull         = 1 << 6,
	DF_Wireframe    = 1 << 7,
	DF_ForceColor   = 1 << 8,
};
void SetRenderState(unsigned drawFlags);
enum PrimitiveType
{
	PT_Points,
	PT_Lines,
	PT_LineStrip,
	PT_Triangles,
	PT_TriangleStrip,
};
enum VertexFormat         // position:   float3
{
	VF_Normal   = 1 << 0, // + normal:   float3
	VF_Texcoord = 1 << 1, // + texcoord: float2
	VF_Color    = 1 << 2, // + color:    ubyte4
};
void Draw(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices);
void DrawIndexed(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices,
	const void* indices,
	size_t numIndices,
	bool i32 = false);


} // rhi
} // ui
