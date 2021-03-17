
#include "RHI.h"

#define WIN32_LEAN_AND_MEAN
#define NONLS
#define NOMINMAX
#include <Windows.h>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")


#include "clear.vs.h"
#include "clear.ps.h"
#include "draw2d.vs.h"
#include "draw2d.ps.h"


#define SAFE_RELEASE(x) if (x) { (x)->Release(); x = nullptr; }

#define D3DCHK(x) _Check(x, #x, __FILE__, __LINE__)
static HRESULT _Check(HRESULT hr, const char* code, const char* file, int line)
{
	if (FAILED(hr))
	{
		char bfr[2048];
		sprintf(bfr, "Error: %08X\nCode: %s\nFile: %s\nLine: %d\nDo you wish to continue?", hr, code, file, line);
		if (MessageBoxA(nullptr, bfr, "Direct3D11 error", MB_ICONERROR | MB_YESNO) == IDNO)
		{
			exit(EXIT_FAILURE);
		}
	}
	return hr;
}


static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGIFactory* g_dxgiFactory = nullptr;


struct TempBuffer
{
	UINT _bindFlags;
	ID3D11Buffer* buffer = nullptr;
	size_t curSize = 0;

	TempBuffer(UINT bindFlags, size_t initial = 16384) : _bindFlags(bindFlags)
	{
		Allocate(initial);
	}
	~TempBuffer()
	{
		SAFE_RELEASE(buffer);
	}
	void Allocate(size_t size)
	{
		SAFE_RELEASE(buffer);

		D3D11_BUFFER_DESC bd = {};
		{
			bd.ByteWidth = size;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = _bindFlags;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		}
		D3DCHK(g_dev->CreateBuffer(&bd, nullptr, &buffer));

		curSize = size;
	}
	void Write(const void* data, size_t size)
	{
		if (size > curSize)
			Allocate(curSize + size);

		D3D11_MAPPED_SUBRESOURCE msr = {};
		D3DCHK(g_ctx->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
		memcpy(msr.pData, data, size);
		g_ctx->Unmap(buffer, 0);
	}
};


namespace ui {
namespace rhi {


extern Stats g_stats;

static TempBuffer* g_tmpVB = nullptr;
static TempBuffer* g_tmpIB = nullptr;
static TempBuffer* g_tmpCB = nullptr;

static ID3D11RasterizerState* g_rs2D = nullptr;
static ID3D11DepthStencilState* g_dss2D = nullptr;
static ID3D11SamplerState* g_samplers[4] = {};

static ID3D11BlendState* g_bsClear = nullptr;
static ID3D11VertexShader* g_vsClear = nullptr;
static ID3D11PixelShader* g_psClear = nullptr;

static ID3D11BlendState* g_bsDraw2D = nullptr;
static ID3D11VertexShader* g_vsDraw2D = nullptr;
static ID3D11PixelShader* g_psDraw2D = nullptr;
static ID3D11InputLayout* g_iaDraw2D = nullptr;


struct RenderContext
{
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Texture2D* backBufferTex = nullptr;
	ID3D11RenderTargetView* backBufferRTV = nullptr;
	ID3D11Texture2D* depthStencilTex = nullptr;
	ID3D11DepthStencilView* depthStencilView = nullptr;

	unsigned width = 0;
	unsigned height = 0;

	static RenderContext* first;
	static RenderContext* last;
	RenderContext* prev = nullptr;
	RenderContext* next = nullptr;

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

	void InitSwapChain(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC scd = {};
		{
			scd.BufferDesc.Width = width;
			scd.BufferDesc.Height = height;
			scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
			scd.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
			scd.SampleDesc.Count = 1;
			scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scd.BufferCount = 2;
			scd.OutputWindow = hwnd;
			scd.Windowed = TRUE;
			scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		}

		D3DCHK(g_dxgiFactory->CreateSwapChain(g_dev, &scd, &swapChain));
	}
	void InitBuffer()
	{
		D3DCHK(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBufferTex));

		D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
		{
			rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.Texture2D.MipSlice = 0;
		}
		D3DCHK(g_dev->CreateRenderTargetView(backBufferTex, &rtvd, &backBufferRTV));

		D3D11_TEXTURE2D_DESC t2d = {};
		{
			t2d.Width = width;
			t2d.Height = height;
			t2d.MipLevels = 1;
			t2d.ArraySize = 1;
			t2d.Format = DXGI_FORMAT_D32_FLOAT;
			t2d.SampleDesc.Count = 1;
			t2d.Usage = D3D11_USAGE_DEFAULT;
			t2d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		}
		D3DCHK(g_dev->CreateTexture2D(&t2d, nullptr, &depthStencilTex));

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
		{
			dsvd.Format = DXGI_FORMAT_D32_FLOAT;
			dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvd.Texture2D.MipSlice = 0;
		}
		D3DCHK(g_dev->CreateDepthStencilView(depthStencilTex, &dsvd, &depthStencilView));
	}
	void ReleaseBuffer()
	{
		SAFE_RELEASE(depthStencilView);
		SAFE_RELEASE(depthStencilTex);
		SAFE_RELEASE(backBufferRTV);
		SAFE_RELEASE(backBufferTex);
	}
	void Resize(unsigned w, unsigned h)
	{
		ReleaseBuffer();

		width = w;
		height = h;
		swapChain->ResizeBuffers(2, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

		InitBuffer();
	}
};
RenderContext* RenderContext::first;
RenderContext* RenderContext::last;
static RenderContext* g_RC;


static void Reset2DRender()
{
	g_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_ctx->RSSetState(g_rs2D);
	g_ctx->OMSetDepthStencilState(g_dss2D, 0);
	float factor[4] = {};
	g_ctx->OMSetBlendState(g_bsDraw2D, factor, 0xffffffff);
	float cbData[2] = { g_RC ? 2.0f / g_RC->width : 1.0f, g_RC ? -2.0f / g_RC->height : 1.0f };
	g_tmpCB->Write(cbData, sizeof(cbData));
	g_ctx->VSSetShader(g_vsDraw2D, nullptr, 0);
	g_ctx->VSSetConstantBuffers(0, 1, &g_tmpCB->buffer);
	g_ctx->PSSetShader(g_psDraw2D, nullptr, 0);
	g_ctx->IASetInputLayout(g_iaDraw2D);
}


void GlobalInit()
{
	UINT flags = 0;
#ifndef _NDEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3DCHK(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		levels,
		sizeof(levels) / sizeof(levels[0]),
		D3D11_SDK_VERSION,
		&g_dev,
		nullptr,
		&g_ctx));

	IDXGIDevice* dxgiDevice = nullptr;
	D3DCHK(g_dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	IDXGIAdapter* dxgiAdapter = nullptr;
	D3DCHK(dxgiDevice->GetAdapter(&dxgiAdapter));
	SAFE_RELEASE(dxgiDevice);

	D3DCHK(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&g_dxgiFactory));
	SAFE_RELEASE(dxgiAdapter);

	g_tmpVB = new TempBuffer(D3D11_BIND_VERTEX_BUFFER);
	g_tmpIB = new TempBuffer(D3D11_BIND_INDEX_BUFFER);
	g_tmpCB = new TempBuffer(D3D11_BIND_CONSTANT_BUFFER);

	D3D11_RASTERIZER_DESC rd = {};
	{
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.FrontCounterClockwise = FALSE;
		rd.ScissorEnable = TRUE;
		rd.MultisampleEnable = FALSE;
		rd.AntialiasedLineEnable = FALSE;
	}
	D3DCHK(g_dev->CreateRasterizerState(&rd, &g_rs2D));

	D3D11_DEPTH_STENCIL_DESC dsd = {};
	{
		dsd.DepthEnable = FALSE;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsd.StencilEnable = FALSE;
	}
	D3DCHK(g_dev->CreateDepthStencilState(&dsd, &g_dss2D));

	for (int i = 0; i < 4; i++)
	{
		D3D11_SAMPLER_DESC sd = {};
		{
			sd.Filter = i & TF_FILTER ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
			sd.AddressU = i & TF_REPEAT ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressV = i & TF_REPEAT ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressW = i & TF_REPEAT ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.MaxAnisotropy = 1;
			sd.MaxLOD = D3D11_FLOAT32_MAX;
		}
		D3DCHK(g_dev->CreateSamplerState(&sd, &g_samplers[i]));
	}

	// clear
	{
		D3D11_BLEND_DESC bsd = {};
		{
			bsd.RenderTarget[0].BlendEnable = TRUE;
			bsd.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
			bsd.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			bsd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bsd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_BLEND_FACTOR;
			bsd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			bsd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bsd.RenderTarget[0].RenderTargetWriteMask = 0xf;
		}
		D3DCHK(g_dev->CreateBlendState(&bsd, &g_bsClear));
	}

	D3DCHK(g_dev->CreateVertexShader(g_shobj_vs_clear, sizeof(g_shobj_vs_clear), nullptr, &g_vsClear));
	D3DCHK(g_dev->CreatePixelShader(g_shobj_ps_clear, sizeof(g_shobj_ps_clear), nullptr, &g_psClear));

	// draw2d
	{
		D3D11_BLEND_DESC bsd = {};
		{
			bsd.RenderTarget[0].BlendEnable = TRUE;
			bsd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bsd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bsd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bsd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
			bsd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			bsd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bsd.RenderTarget[0].RenderTargetWriteMask = 0xf;
		}
		D3DCHK(g_dev->CreateBlendState(&bsd, &g_bsDraw2D));
	}

	D3DCHK(g_dev->CreateVertexShader(g_shobj_vs_draw2d, sizeof(g_shobj_vs_draw2d), nullptr, &g_vsDraw2D));
	D3DCHK(g_dev->CreatePixelShader(g_shobj_ps_draw2d, sizeof(g_shobj_ps_draw2d), nullptr, &g_psDraw2D));

	D3D11_INPUT_ELEMENT_DESC ied[3] = {};
	{
		ied[0].SemanticName = "POSITION";
		ied[0].SemanticIndex = 0;
		ied[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		ied[0].InputSlot = 0;
		ied[0].AlignedByteOffset = offsetof(Vertex, x);
		ied[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		ied[0].InstanceDataStepRate = 0;

		ied[1].SemanticName = "TEXCOORD";
		ied[1].SemanticIndex = 0;
		ied[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		ied[1].InputSlot = 0;
		ied[1].AlignedByteOffset = offsetof(Vertex, u);
		ied[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		ied[1].InstanceDataStepRate = 0;

		ied[2].SemanticName = "COLOR";
		ied[2].SemanticIndex = 0;
		ied[2].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		ied[2].InputSlot = 0;
		ied[2].AlignedByteOffset = offsetof(Vertex, col);
		ied[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		ied[2].InstanceDataStepRate = 0;
	}
	D3DCHK(g_dev->CreateInputLayout(ied, 3, g_shobj_vs_draw2d, sizeof(g_shobj_vs_draw2d), &g_iaDraw2D));

	Reset2DRender();
}

void GlobalFree()
{
	SAFE_RELEASE(g_iaDraw2D);
	SAFE_RELEASE(g_psDraw2D);
	SAFE_RELEASE(g_vsDraw2D);
	SAFE_RELEASE(g_bsDraw2D);
	
	SAFE_RELEASE(g_psClear);
	SAFE_RELEASE(g_vsClear);
	SAFE_RELEASE(g_bsClear);

	for (int i = 0; i < 4; i++)
		SAFE_RELEASE(g_samplers[i]);
	SAFE_RELEASE(g_dss2D);
	SAFE_RELEASE(g_rs2D);

	delete g_tmpCB;
	delete g_tmpIB;
	delete g_tmpVB;

	SAFE_RELEASE(g_dxgiFactory);
	SAFE_RELEASE(g_ctx);
	SAFE_RELEASE(g_dev);
}

RenderContext* CreateRenderContext(void* window)
{
	RenderContext* RC = new RenderContext();

	auto hwnd = (HWND)window;
	RECT size = {};
	GetClientRect(hwnd, &size);

	RC->width = size.right;
	RC->height = size.bottom;

	RC->InitSwapChain(hwnd);
	RC->InitBuffer();

	g_RC = RC;

	return RC;
}

void FreeRenderContext(RenderContext* RC)
{
	RC->ReleaseBuffer();
	SAFE_RELEASE(RC->swapChain);
	delete RC;
}

void SetActiveContext(RenderContext* RC)
{
	g_RC = RC;
	g_ctx->OMSetRenderTargets(1, &RC->backBufferRTV, RC->depthStencilView);
	Reset2DRender();
}

void OnResizeWindow(RenderContext* RC, unsigned w, unsigned h)
{
	RC->Resize(w, h);
}


void SetScissorRect(int x0, int y0, int x1, int y1)
{
	D3D11_RECT rect = { x0, y0, x1, y1 };
	g_ctx->RSSetScissorRects(1, &rect);
}

void SetViewport(int x0, int y0, int x1, int y1)
{
	D3D11_VIEWPORT viewport = { float(x0), float(y0), float(x1 - x0), float(y1 - y0), 0.0f, 1.0f };
	g_ctx->RSSetViewports(1, &viewport);
}

void Clear(int r, int g, int b, int a)
{
	Color4f col = Color4b(r, g, b, a);
	g_ctx->ClearRenderTargetView(g_RC->backBufferRTV, &col.r);

	g_ctx->VSSetShader(g_vsClear, nullptr, 0);
	g_ctx->PSSetShader(g_psClear, nullptr, 0);

	g_ctx->OMSetBlendState(g_bsClear, &col.r, 0xffffffff);

	g_ctx->Draw(3, 0);

	Reset2DRender();
}

void ClearDepthOnly()
{
	g_ctx->ClearDepthStencilView(g_RC->depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Present(RenderContext* RC)
{
	RC->swapChain->Present(0, 0);
}

struct Texture2D
{
	ID3D11Texture2D* tex = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	uint8_t _flags;

	Texture2D(const void* data, unsigned width, unsigned height, uint8_t flags, bool a8) : _flags(flags)
	{
		D3D11_TEXTURE2D_DESC t2d = {};
		{
			t2d.Width = width;
			t2d.Height = height;
			t2d.MipLevels = 1;
			t2d.ArraySize = 1;
			t2d.Format = a8 ? DXGI_FORMAT_A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
			t2d.SampleDesc.Count = 1;
			t2d.Usage = D3D11_USAGE_DEFAULT;
			t2d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			t2d.CPUAccessFlags = data ? 0 : D3D11_CPU_ACCESS_WRITE;
		}

		D3D11_SUBRESOURCE_DATA srd;
		{
			srd.pSysMem = data;
			srd.SysMemPitch = width * (a8 ? 1 : 4);
		}

		D3DCHK(g_dev->CreateTexture2D(&t2d, data ? &srd : nullptr, &tex));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
		{
			srvd.Format = a8 ? DXGI_FORMAT_A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
			srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvd.Texture2D.MipLevels = 1;
		}

		D3DCHK(g_dev->CreateShaderResourceView(tex, &srvd, &srv));
	}
	~Texture2D()
	{
		SAFE_RELEASE(srv);
		SAFE_RELEASE(tex);
	}
};

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height, uint8_t flags)
{
	return new Texture2D(data, width, height, flags, true);
}

Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, uint8_t flags)
{
	return new Texture2D(data, width, height, flags, false);
}

void DestroyTexture(Texture2D* tex)
{
	delete tex;
}

MapData MapTexture(Texture2D* tex)
{
#if 0
	D3D11_MAPPED_SUBRESOURCE msr = {};
	D3DCHK(g_ctx->Map(tex->tex, 0, D3D11_MAP_WRITE, 0, &msr));

	MapData omd;
	{
		omd.data = msr.pData;
		omd.pitch = msr.RowPitch;
	}
	return omd;
#else
	return {};
#endif
}

void CopyToMappedTextureRect(Texture2D* tex, const MapData& md, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const void* data, bool a8)
{
	size_t bpp = a8 ? 1 : 4;
#if 0
	for (uint16_t curY = 0; curY < h; curY++)
	{
		memcpy(
			static_cast<char*>(md.data) + (y + curY) * md.pitch + x * bpp,
			static_cast<const char*>(data) + curY * w * bpp,
			w * bpp);
	}
#else
	if (w == 0 || h == 0)
		return;
	D3D11_BOX box = { x, y, 0U, UINT(x + w), UINT(y + h), 1U };
	g_ctx->UpdateSubresource(tex->tex, 0, &box, data, w * bpp, 0);
#endif
}

void UnmapTexture(Texture2D* tex)
{
#if 0
	g_ctx->Unmap(tex->tex, 0);
#endif
}

void SetTexture(Texture2D* tex)
{
	auto* srv = tex ? tex->srv : nullptr;
	g_ctx->PSSetShaderResources(0, 1, &srv);
	g_ctx->PSSetSamplers(0, 1, &g_samplers[tex->_flags]);
}

void DrawTriangles(Vertex* verts, size_t num_verts)
{
	g_stats.num_DrawTriangles++;

	g_tmpVB->Write(verts, sizeof(*verts) * num_verts);
	UINT stride = sizeof(*verts);
	UINT offset = 0;
	g_ctx->IASetVertexBuffers(0, 1, &g_tmpVB->buffer, &stride, &offset);

	g_ctx->Draw(num_verts, 0);
}

void DrawIndexedTriangles(Vertex* verts, size_t num_verts, uint16_t* indices, size_t num_indices)
{
	g_stats.num_DrawIndexedTriangles++;

	g_tmpVB->Write(verts, sizeof(*verts) * num_verts);
	UINT stride = sizeof(*verts);
	UINT offset = 0;
	g_ctx->IASetVertexBuffers(0, 1, &g_tmpVB->buffer, &stride, &offset);

	g_tmpIB->Write(indices, sizeof(*indices) * num_indices);
	g_ctx->IASetIndexBuffer(g_tmpIB->buffer, DXGI_FORMAT_R16_UINT, 0);

	g_ctx->DrawIndexed(num_indices, 0, 0);
}

void SetRenderState(unsigned drawFlags)
{
	// TODO
}

void SetViewMatrix(const Mat4f& m)
{
	// TODO
}

void SetProjectionMatrix(const Mat4f& m)
{
	// TODO
}

void SetForcedColor(const Color4b& col)
{
	// TODO
}

void Begin3DMode(const AABB<int>& rect)
{
	// TODO
}

AABB<int> End3DMode()
{
	// TODO
	Reset2DRender();
	return {};
}

void SetAmbientLight(const Color4f& col)
{
	// TODO
}

void SetLightOff(int n)
{
	// TODO
}

void SetDirectionalLight(int n, float x, float y, float z, const Color4f& col)
{
	// TODO
}

void Draw(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices)
{
	// TODO
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
	// TODO
}


} // rhi
} // ui
