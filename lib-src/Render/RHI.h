
#pragma once
#include "../Core/Image.h"
#include "../Core/Optional.h"
#include "../Core/3DMath.h"


namespace ui {
namespace gfx {

struct ExclusiveFullscreenInfo;

using Vertex = ::ui::Vertex;

struct UI_API Stats
{
	uint64_t num_SetTexture;
	uint64_t num_DrawTriangles;
	uint64_t num_DrawIndexedTriangles;

	Stats operator - (const Stats& o) const;

	static Stats Get();
};

struct RHIInternalPointers
{
	// ID3D11Device* / HDC
	void* device;
	// ID3D11DeviceContext* / HGLRC
	void* context;
	// HWND
	void* window;
	// IDXGISwapChain* / NULL(OpenGL)
	void* swapChain;
	// ID3D11RenderTargetView*
	void* renderTargetView;
	// ID3D11DepthStencilView*
	void* depthStencilView;
};

struct IRHIListener
{
	// OpenGL: first created context
	virtual void OnAttach(const RHIInternalPointers& ip) = 0;

	// OpenGL: last destroyed context
	virtual void OnDetach(const RHIInternalPointers& ip) = 0;

	virtual void OnAfterInitSwapChain(const RHIInternalPointers& ip) = 0;
	virtual void OnBeforeFreeSwapChain(const RHIInternalPointers& ip) = 0;

	virtual void OnBeginFrame(const RHIInternalPointers& ip) = 0;
	virtual void OnEndFrame(const RHIInternalPointers& ip) = 0;
};

struct Texture2D;
struct RenderContext;

UI_API void GlobalInit();
UI_API void GlobalFree();
UI_API u64 GetVideoMemoryUsage();

UI_API RenderContext* CreateRenderContext(void* window);
UI_API void FreeRenderContext(RenderContext* RC);
UI_API void SetActiveContext(RenderContext* RC);
UI_API void OnResizeWindow(RenderContext* RC, unsigned w, unsigned h);
UI_API void OnChangeFullscreen(RenderContext* RC, const Optional<ExclusiveFullscreenInfo>& info);
UI_API void SetVSyncInterval(RenderContext* RC, unsigned interval);

UI_API void AttachListener(IRHIListener*);
UI_API void DetachListener(IRHIListener*);

UI_API void BeginFrame(RenderContext* RC);
UI_API void EndFrame(RenderContext* RC);

UI_API void ApplyViewport();
UI_API void SetViewport(int x0, int y0, int x1, int y1);
UI_API void SetScissorRect(int x0, int y0, int x1, int y1);

UI_API void Clear(int r, int g, int b, int a);
UI_API void ClearDepthOnly(float depth = 1);
UI_API void Present(RenderContext* RC);

constexpr uint8_t TF_NOFILTER = 1 << 0;
constexpr uint8_t TF_REPEAT = 1 << 1;
UI_API Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height, uint8_t flags);
UI_API Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, uint8_t flags);
// D3D11: handle = ID3D11ShaderResourceView*
UI_API Texture2D* CreateTextureFromAPIHandle(unsigned width, unsigned height, uintptr_t handle);
UI_API void SetTextureDebugName(Texture2D* tex, StringView debugName);
UI_API void DestroyTexture(Texture2D* tex);

struct MapData
{
	void* data;
	uint32_t pitch;
};
UI_API MapData MapTexture(Texture2D* tex);
UI_API void CopyToMappedTextureRect(Texture2D* tex, const MapData& md, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void* data, bool a8);
UI_API void UnmapTexture(Texture2D* tex);

UI_API void SetTexture(Texture2D* tex);
UI_API void DrawTriangles(Vertex* verts, size_t num_verts);
UI_API void DrawIndexedTriangles(Vertex* verts, size_t num_verts, uint16_t* indices, size_t num_indices);


UI_API void Begin3DMode(const AABB2i& rect);
UI_API AABB2i End3DMode();
UI_API void RestoreRenderStates();
UI_API void SetupRenderStateForOutputSize(int w, int h);
UI_API void SetViewMatrix(const Mat4f& m);
UI_API void SetProjectionMatrix(const Mat4f& m);
UI_API void SetForcedColor(const Color4b& col);
UI_API void SetAmbientLight(const Color4f& col);
UI_API void SetLightOff(int n);
UI_API void SetDirectionalLight(int n, float x, float y, float z, const Color4f& col);
enum DrawFlags
{
	DF_AlphaTest    = 1 << 1,
	DF_Lit          = 1 << 2,
	DF_AlphaBlended = 1 << 3,
	DF_ZTestOff     = 1 << 4,
	DF_ZWriteOff    = 1 << 5,
	DF_Cull         = 1 << 6,
	DF_Wireframe    = 1 << 7,
	DF_ForceColor   = 1 << 8,
	DF_ZTestReverse = 1 << 9,
};
UI_API void SetRenderState(unsigned drawFlags);
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
UI_API size_t GetVertexSize(unsigned vertexFormat);
UI_API void Draw(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices);
UI_API void DrawIndexed(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices,
	const void* indices,
	size_t numIndices,
	bool i32 = false);


} // gfx
} // ui
