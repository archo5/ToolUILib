
#include "RHI.h"
#include "../Core/Memory.h"

#include "../Model/Native.h" // TODO?

#include "../Core/WindowsUtils.h"

#define WIN32_LEAN_AND_MEAN
#define NONLS
#define NOMINMAX
#include <Windows.h>

#include <d3d11.h>
#include <dxgi1_4.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#define D3D_DUMP_LIVE_OBJECTS

// not needed for nvidia but without this at least one intel driver will crash
#define D3D_USE_STAGING_TEXTURE 1


#include "clear.vs.h"
#include "clear.ps.h"
#include "draw2d.vs.h"
#include "draw2d.ps.h"
#include "draw3dunlit.vs.h"
#include "draw3dunlit.ps.h"


namespace ui {
LogCategory LOG_RHI_D3D11("RHI-D3D11", LogLevel::Info);
} // ui


namespace ui {
namespace gfx {


#define SAFE_RELEASE(x) if (x) { (x)->Release(); x = nullptr; }

// https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d11-graphics-reference-returnvalues
static const char* D3D11HResultToString(HRESULT hr)
{
	switch (hr)
	{
	case E_FAIL: return "E_FAIL";
	case E_INVALIDARG: return "E_INVALIDARG";
	case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
	case E_NOTIMPL: return "E_NOTIMPL";

	case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
	case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING";

	case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
	case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND";
	case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
	case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";

	default: return "?";
	}
}

#define D3DCHK(x) _Check(x, #x, __FILE__, __LINE__)
static HRESULT _Check(HRESULT hr, const char* code, const char* file, int line)
{
	if (FAILED(hr))
	{
		const char* err = D3D11HResultToString(hr);
		LogError(LOG_RHI_D3D11, "Error %s (%08X) returned by code \"%s\" at %s:%d", err, hr, code, file, line);
		if (hr == E_OUTOFMEMORY)
		{
			const wchar_t* err = L"Ran out of video memory available to the application!\nPlease try to reduce the graphics settings.";
			MessageBoxW(nullptr, err, L"Fatal error: Direct3D 11 renderer ran out of available video memory!", MB_ICONERROR);
		}
		else
		{
			char bfr[2048];
			size_t len = snprintf(bfr, 2048, "Error: %08X (%s)\nCode: %s\nFile: %s\nLine: %d", hr, err, code, file, line);
			MessageBoxW(nullptr, UTF8toWCHAR({ bfr, len }).c_str(), L"Fatal Direct3D 11 error", MB_ICONERROR);
		}
		exit(EXIT_FAILURE);
	}
	return hr;
}

static void SetName(ID3D11DeviceChild* dch, ui::StringView name)
{
	if (!dch)
		return;
	dch->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr); // in case there's any prior data
	dch->SetPrivateData(WKPDID_D3DDebugObjectName, name.Size(), name.Data());
}


static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGIAdapter* g_dxgiAdapter = nullptr;
static IDXGIFactory* g_dxgiFactory = nullptr;
static IDXGIAdapter3* g_dxgiAdapter3 = nullptr;


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


struct Texture2D;
static Texture2D* g_prevTex;
static Texture2D* g_curTex;
struct Texture2D
{
#if D3D_USE_STAGING_TEXTURE
	ID3D11Texture2D* staging = nullptr;
	AABB2i aabb;
#endif
	ID3D11Texture2D* tex = nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	uint8_t _flags = 0;

	static Texture2D* NewFromAPIHandle(unsigned width, unsigned height, uintptr_t handle)
	{
		auto* T = new Texture2D;
		T->srv = (ID3D11ShaderResourceView*)handle;
		T->srv->AddRef();
		return T;
	}
	Texture2D() {}
	Texture2D(const void* data, unsigned width, unsigned height, uint8_t flags, bool a8) : _flags(flags & 3)
	{
		LogInfo(LOG_RHI_D3D11, "Creating a 2D %ux%u texture (fmt=%s filter=%s addr=%s)%s",
			width,
			height,
			a8 ? "A8" : "RGBA8",
			flags & TF_NOFILTER ? "nearest" : "linear",
			flags & TF_REPEAT ? "wrap" : "clamp",
			data ? " from data" : "");
#if 0
		u64 vmu = GetVideoMemoryUsage();
		double MB = 1024 * 1024;
		LogInfo(LOG_RHI_D3D11, "- video memory usage before creating: %" PRIu64 " (%.3f MB)", vmu, vmu / MB);
#endif
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
#if D3D_USE_STAGING_TEXTURE
		t2d.CPUAccessFlags = 0;
#endif

		D3D11_SUBRESOURCE_DATA srd;
		{
			srd.pSysMem = data;
			srd.SysMemPitch = width * (a8 ? 1 : 4);
		}

		D3DCHK(g_dev->CreateTexture2D(&t2d, data ? &srd : nullptr, &tex));
#if D3D_USE_STAGING_TEXTURE
		if (!data)
		{
			t2d.Usage = D3D11_USAGE_STAGING;
			t2d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			t2d.BindFlags = 0;
			D3DCHK(g_dev->CreateTexture2D(&t2d, data ? &srd : nullptr, &staging));
		}
#endif

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
		SAFE_RELEASE(staging);
		if (g_prevTex == this)
			g_prevTex = nullptr;
		if (g_curTex == this)
			g_curTex = nullptr;
	}
};


extern Stats g_stats;

static TempBuffer* g_tmpVB = nullptr;
static TempBuffer* g_tmpIB = nullptr;
static TempBuffer* g_tmpCB = nullptr;
static TempBuffer* g_defVB = nullptr;
static TempBuffer* g_defVBCC = nullptr;
static Texture2D* g_defTex = nullptr;

static ID3D11RasterizerState* g_renderState2D = nullptr;
static ID3D11RasterizerState* g_renderStates3D[4] = {};

static ID3D11DepthStencilState* g_depthStencilStates[8] = {};

static ID3D11SamplerState* g_samplers[4] = {};

static ID3D11BlendState* g_bsClear = nullptr;
static ID3D11BlendState* g_bsNoBlend = nullptr;
static ID3D11BlendState* g_bsAlphaBlend = nullptr;

static ID3D11InputLayout* g_inputLayout2D = nullptr;
static ID3D11InputLayout* g_inputLayouts3D[8] = {};

static ID3D11VertexShader* g_vsClear = nullptr;
static ID3D11PixelShader* g_psClear = nullptr;

static ID3D11VertexShader* g_vsDraw2D = nullptr;
static ID3D11PixelShader* g_psDraw2D = nullptr;

static ID3D11VertexShader* g_vsDraw3DUnlit = nullptr;
static ID3D11PixelShader* g_psDraw3DUnlit = nullptr;


ArrayView<IRHIListener*> GetListeners();


Array<GraphicsAdapters::Info> GraphicsAdapters::All(u32 flags)
{
	auto* factory = g_dxgiFactory;
	if (factory)
		factory->AddRef();
	else
	{
		D3DCHK(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));
	}

	Array<GraphicsAdapters::Info> ret;
	for (UINT i = 0;; i++)
	{
		IDXGIAdapter* adapter = nullptr;
		HRESULT hr = factory->EnumAdapters(i, &adapter);
		if (hr == DXGI_ERROR_NOT_FOUND)
			break;
		D3DCHK(hr);

		DXGI_ADAPTER_DESC desc;
		D3DCHK(adapter->GetDesc(&desc));
		ret.Append({ WCHARtoUTF8(desc.Description) });

		if (flags & Info_Monitors)
		{
			for (UINT j = 0;; j++)
			{
				IDXGIOutput* output = nullptr;
				hr = adapter->EnumOutputs(j, &output);
				if (hr == DXGI_ERROR_NOT_FOUND)
					break;
				D3DCHK(hr);

				DXGI_OUTPUT_DESC odesc;
				D3DCHK(output->GetDesc(&odesc));
				ret.Last().monitors.Append({ MonitorID(odesc.Monitor) });
				auto& M = ret.Last().monitors.Last();

				if (flags & Info_MonitorNames)
					M.name = Monitors::GetName(MonitorID(odesc.Monitor));

				if (flags & Info_DisplayModes)
				{
					auto fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
					auto flags = DXGI_ENUM_MODES_SCALING;
					Array<DXGI_MODE_DESC> modes;
					UINT modeCount = 0;
					D3DCHK(output->GetDisplayModeList(fmt, flags, &modeCount, nullptr));
					modes.Resize(modeCount);
					D3DCHK(output->GetDisplayModeList(fmt, flags, &modeCount, modes.Data()));
					modes.Resize(modeCount);

					M.displayModes.Reserve(modeCount / 2); // they seem to sometimes double/triple with scaling (unspecified/centered/stretched)
					for (auto& mode : modes)
					{
						if (mode.Scaling != DXGI_MODE_SCALING_UNSPECIFIED)
							continue;
						M.displayModes.Append({ mode.Width, mode.Height, mode.RefreshRate.Numerator, mode.RefreshRate.Denominator });
					}
				}

				SAFE_RELEASE(output);
			}
		}

		SAFE_RELEASE(adapter);
	}

	SAFE_RELEASE(factory);

	return ret;
}


struct RenderContext
{
	HWND window = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Texture2D* backBufferTex = nullptr;
	ID3D11RenderTargetView* backBufferRTV = nullptr;
	ID3D11Texture2D* depthStencilTex = nullptr;
	ID3D11DepthStencilView* depthStencilView = nullptr;

	unsigned width = 0;
	unsigned height = 0;
	unsigned vsyncInterval = 0;

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

	RHIInternalPointers GetPtrs() const { return { g_dev, g_ctx, window, swapChain, backBufferRTV, depthStencilView }; }

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
		D3DCHK(g_dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));
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

		for (auto* L : GetListeners())
			L->OnAfterInitSwapChain(GetPtrs());
	}
	void ReleaseBuffer()
	{
		for (auto* L : GetListeners())
			L->OnBeforeFreeSwapChain(GetPtrs());

		SAFE_RELEASE(depthStencilView);
		SAFE_RELEASE(depthStencilTex);
		SAFE_RELEASE(backBufferRTV);
		SAFE_RELEASE(backBufferTex);
	}
	void Resize(unsigned w, unsigned h)
	{
		if (width == w && height == h)
			return;

		LogInfo(LOG_RHI_D3D11, "Resize requested: %u x %u (from %u x %u)", w, h, width, height);

		ReleaseBuffer();

		width = w;
		height = h;
		D3DCHK(swapChain->ResizeBuffers(2, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		InitBuffer();
	}
	void SetFullScreen(const Optional<ExclusiveFullscreenInfo>& info)
	{
		if (info.HasValue())
		{
			auto efi = info.GetValue();

			IDXGIOutput* dxgiOutput = nullptr;
			if (efi.monitor)
			{
				for (UINT i = 0; ; i++)
				{
					HRESULT hr = g_dxgiAdapter->EnumOutputs(i, &dxgiOutput);
					if (hr == DXGI_ERROR_NOT_FOUND)
						break;

					DXGI_OUTPUT_DESC desc = {};
					D3DCHK(dxgiOutput->GetDesc(&desc));
					if (desc.Monitor == HMONITOR(efi.monitor))
						break; // found it

					SAFE_RELEASE(dxgiOutput);
				}
			}
			if (!dxgiOutput)
				D3DCHK(swapChain->GetContainingOutput(&dxgiOutput));

			DXGI_MODE_DESC mode = {};
			mode.Format = DXGI_FORMAT_UNKNOWN;
			if (efi.size.x && efi.size.y)
			{
				mode.Width = efi.size.x;
				mode.Height = efi.size.y;
			}
			else
			{
				DXGI_OUTPUT_DESC desc = {};
				D3DCHK(dxgiOutput->GetDesc(&desc));
				mode.Width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
				mode.Height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
			}
			if (efi.refreshRate.num)
			{
				mode.RefreshRate.Numerator = efi.refreshRate.num;
				mode.RefreshRate.Denominator = efi.refreshRate.denom;
			}
			D3DCHK(swapChain->ResizeTarget(&mode));

			D3DCHK(swapChain->SetFullscreenState(TRUE, dxgiOutput));

			// needed to allow 1080p, otherwise SetFullscreenState can set 1680x1050 instead (WTF MS?)
			mode.RefreshRate = {};
			D3DCHK(swapChain->ResizeTarget(&mode));

			SAFE_RELEASE(dxgiOutput);
		}
		else
		{
			D3DCHK(swapChain->SetFullscreenState(FALSE, nullptr));
		}
	}
};
RenderContext* RenderContext::first;
RenderContext* RenderContext::last;
static RenderContext* g_RC;


static void Reset2DRender(Optional<Vec2i> size = {})
{
	Vec2i sz = size.GetValueOrDefault(g_RC ? Vec2i(g_RC->width, g_RC->height) : Vec2i(2, 2));
	g_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_ctx->RSSetState(g_renderState2D);
	g_ctx->OMSetDepthStencilState(g_depthStencilStates[3], 0);
	g_ctx->OMSetBlendState(g_bsAlphaBlend, nullptr, 0xffffffff);
	float cbData[2] = { 2.f / sz.x, -2.f / sz.y };
	g_tmpCB->Write(cbData, sizeof(cbData));
	g_ctx->VSSetShader(g_vsDraw2D, nullptr, 0);
	g_ctx->VSSetConstantBuffers(0, 1, &g_tmpCB->buffer);
	g_ctx->PSSetShader(g_psDraw2D, nullptr, 0);
	g_ctx->IASetInputLayout(g_inputLayout2D);
}


struct DefaultVertex
{
	Vec3f pos;
	Vec3f nrm;
	Point2f tex;
	Color4b col;
};

void GraphicsAdapters_Lock(int which);
void GraphicsAdapters_Unlock();

static void DumpAdapterInfo(const DXGI_ADAPTER_DESC& desc)
{
	LogInfo(LOG_RHI_D3D11, "starting with adapter \"%s\" (vendor=%04X device=%04X)",
		WCHARtoUTF8(desc.Description).c_str(),
		desc.VendorId,
		desc.DeviceId);
	double GB = 1024 * 1024 * 1024;
	LogInfo(LOG_RHI_D3D11, "- graphics adapter memory available to the application%s:", sizeof(size_t) == 4 ? " (32-bit limit)" : "");
	LogInfo(LOG_RHI_D3D11, "--- dedicated video memory: %zu bytes (%.3f GB)", desc.DedicatedVideoMemory, desc.DedicatedVideoMemory / GB);
	LogInfo(LOG_RHI_D3D11, "--- dedicated system = %zu bytes (%.3f GB)", desc.DedicatedSystemMemory, desc.DedicatedSystemMemory / GB);
	LogInfo(LOG_RHI_D3D11, "--- shared system = %zu bytes (%.3f GB)", desc.SharedSystemMemory, desc.SharedSystemMemory / GB);
}

void GlobalInit()
{
	UINT flags = 0;
#ifndef NDEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	IDXGIFactory* factory = nullptr;
	D3DCHK(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory));

	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	DXGI_ADAPTER_DESC adapterDesc = {};

	int initIndex = -1;
	StringView initName;
	GraphicsAdapters::GetInitial(initIndex, initName);
	IDXGIAdapter* initAdapter = nullptr;
	if (initIndex != -1 || initName.NotEmpty())
	{
		if (initIndex != -1)
			LogInfo(LOG_RHI_D3D11, "requested the use of graphics adapter %d", initIndex);
		else
			LogInfo(LOG_RHI_D3D11, "requested the use of graphics adapter \"%.*s\"", int(initName.Size()), initName.Data());

		for (UINT i = 0;; i++)
		{
			IDXGIAdapter* adapter = nullptr;
			HRESULT hr = factory->EnumAdapters(i, &adapter);
			if (hr == DXGI_ERROR_NOT_FOUND)
				break;

			if (initIndex == i)
			{
				initAdapter = adapter;
				break;
			}

			D3DCHK(adapter->GetDesc(&adapterDesc));
			if (initName == WCHARtoUTF8(adapterDesc.Description))
			{
				initAdapter = adapter;
				initIndex = i;
				break;
			}

			SAFE_RELEASE(adapter);
		}
	}

	if (!initAdapter)
	{
		LogInfo(LOG_RHI_D3D11, "starting with the default adapter");
		initIndex = 0;

		IDXGIAdapter* adapter = nullptr;
		HRESULT hr = factory->EnumAdapters(0, &adapter);
		UI_DEFER(SAFE_RELEASE(adapter));
		if (hr == DXGI_ERROR_NOT_FOUND)
			LogError(LOG_RHI_D3D11, "could not find adapter 0?");
		else
		{
			D3DCHK(adapter->GetDesc(&adapterDesc));
		}
	}

	DumpAdapterInfo(adapterDesc);

	SAFE_RELEASE(factory);

	GraphicsAdapters_Lock(initIndex);

	HRESULT cdhr = D3D11CreateDevice(
		initAdapter,
		initAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		flags,
		levels,
		sizeof(levels) / sizeof(levels[0]),
		D3D11_SDK_VERSION,
		&g_dev,
		nullptr,
		&g_ctx);
	auto adapterDescText = WCHARtoUTF8(adapterDesc.Description, 128);
	if (FAILED(cdhr))
	{
		char bfr[2048];
		size_t len = snprintf(bfr, sizeof(bfr), "Failed to create a Direct3D 11 device!\nAdapter: %d%s \"%s\"\nError: %s (%08X)",
			initIndex, initAdapter ? "" : " (default)", adapterDescText.c_str(), D3D11HResultToString(cdhr), cdhr);
		MessageBoxW(nullptr, UTF8toWCHAR({ bfr, len }).c_str(), L"Fatal Direct3D 11 error", MB_ICONERROR);
		exit(EXIT_FAILURE);
	}
	LogInfo(LOG_RHI_D3D11, "created a device with adapter %d%s: \"%s\"",
		initIndex, initAdapter ? " (default)" : "", adapterDescText.c_str());

	SAFE_RELEASE(initAdapter);

	// try to improve latency
	{
		IDXGIDevice1* dxgiDevice1 = nullptr;
		if (SUCCEEDED(g_dev->QueryInterface(__uuidof(IDXGIDevice1), (void**)&dxgiDevice1)) && dxgiDevice1)
		{
			dxgiDevice1->SetMaximumFrameLatency(1);
			dxgiDevice1->Release();
		}
	}

	IDXGIDevice* dxgiDevice = nullptr;
	D3DCHK(g_dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice));

	D3DCHK(dxgiDevice->GetAdapter(&g_dxgiAdapter));
	SAFE_RELEASE(dxgiDevice);

	D3DCHK(g_dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&g_dxgiFactory));
	D3DCHK(g_dxgiAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&g_dxgiAdapter3));

	g_tmpVB = new TempBuffer(D3D11_BIND_VERTEX_BUFFER);
	g_tmpIB = new TempBuffer(D3D11_BIND_INDEX_BUFFER);
	g_tmpCB = new TempBuffer(D3D11_BIND_CONSTANT_BUFFER);
	g_defVB = new TempBuffer(D3D11_BIND_VERTEX_BUFFER, sizeof(DefaultVertex));
	g_defVBCC = new TempBuffer(D3D11_BIND_VERTEX_BUFFER, sizeof(DefaultVertex));
	{
		DefaultVertex v = {};
		v.col = Color4b::White();
		g_defVB->Write(&v, sizeof(v));
		g_defVBCC->Write(&v, sizeof(v));
	}
	auto defTexCol = Color4b::White();
	g_defTex = CreateTextureRGBA8(&defTexCol, 1, 1, 0);
	SetTextureDebugName(g_defTex, "ui:tex-default");

	D3D11_RASTERIZER_DESC rd = {};
	{
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.FrontCounterClockwise = TRUE;
		rd.ScissorEnable = TRUE;
		rd.MultisampleEnable = FALSE;
		rd.AntialiasedLineEnable = FALSE;
	}
	D3DCHK(g_dev->CreateRasterizerState(&rd, &g_renderState2D));
	SetName(g_renderState2D, "ui:rs2D");

	for (int i = 0; i < 4; i++)
	{
		D3D11_RASTERIZER_DESC rd = {};
		{
			rd.FillMode = i & 2 ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
			rd.CullMode = i & 1 ? D3D11_CULL_BACK : D3D11_CULL_NONE;
			rd.FrontCounterClockwise = TRUE;
			rd.ScissorEnable = TRUE;
			rd.MultisampleEnable = FALSE;
			rd.AntialiasedLineEnable = FALSE;
		}
		D3DCHK(g_dev->CreateRasterizerState(&rd, &g_renderStates3D[i]));
		SetName(g_renderStates3D[i], "ui:rs3D");
	}

	for (int i = 0; i < 8; i++)
	{
		D3D11_DEPTH_STENCIL_DESC dsd = {};
		{
			dsd.DepthEnable = i & 1 ? FALSE : TRUE;
			dsd.DepthWriteMask = i & 2 ? D3D11_DEPTH_WRITE_MASK_ZERO : D3D11_DEPTH_WRITE_MASK_ALL;
			dsd.DepthFunc = i & 4 ? D3D11_COMPARISON_GREATER_EQUAL : D3D11_COMPARISON_LESS_EQUAL;
		}
		D3DCHK(g_dev->CreateDepthStencilState(&dsd, &g_depthStencilStates[i]));
		SetName(g_depthStencilStates[i], "ui:dss");
	}

	for (int i = 0; i < 4; i++)
	{
		D3D11_SAMPLER_DESC sd = {};
		{
			sd.Filter = !(i & TF_NOFILTER) ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
			sd.AddressU = i & TF_REPEAT ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressV = i & TF_REPEAT ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.AddressW = i & TF_REPEAT ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
			sd.MaxAnisotropy = 1;
			sd.MaxLOD = D3D11_FLOAT32_MAX;
		}
		D3DCHK(g_dev->CreateSamplerState(&sd, &g_samplers[i]));
		SetName(g_samplers[i], "ui:sampler");
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
		SetName(g_bsClear, "ui:blend-clear");
	}

	// no blend
	{
		D3D11_BLEND_DESC bsd = {};
		{
			bsd.RenderTarget[0].BlendEnable = FALSE;
			bsd.RenderTarget[0].RenderTargetWriteMask = 0xf;
		}
		D3DCHK(g_dev->CreateBlendState(&bsd, &g_bsNoBlend));
		SetName(g_bsNoBlend, "ui:blend-off");
	}
	// alpha blend
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
		D3DCHK(g_dev->CreateBlendState(&bsd, &g_bsAlphaBlend));
		SetName(g_bsAlphaBlend, "ui:blend-alpha");
	}

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
	D3DCHK(g_dev->CreateInputLayout(ied, 3, g_shobj_vs_draw2d, sizeof(g_shobj_vs_draw2d), &g_inputLayout2D));
	SetName(g_inputLayout2D, "ui:Vertex");

	for (int i = 0; i < 8; i++)
	{
		D3D11_INPUT_ELEMENT_DESC ie[4];
		{
			UINT off = 0;
			ie[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, off, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			off += 12;

			if (i & VF_Normal)
			{
				ie[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, off, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				off += 12;
			}
			else
				ie[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 0 };

			if (i & VF_Texcoord)
			{
				ie[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, off, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				off += 8;
			}
			else
				ie[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 24, D3D11_INPUT_PER_INSTANCE_DATA, 0 };

			if (i & VF_Color)
				ie[3] = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, off, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			else
				ie[3] = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 0 };
		}
		D3DCHK(g_dev->CreateInputLayout(ie, 4, g_shobj_vs_draw3dunlit, sizeof(g_shobj_vs_draw3dunlit), &g_inputLayouts3D[i]));
		SetName(g_inputLayouts3D[i], "ui:FVF3D");
	}

	D3DCHK(g_dev->CreateVertexShader(g_shobj_vs_clear, sizeof(g_shobj_vs_clear), nullptr, &g_vsClear));
	SetName(g_vsClear, "ui:Clear");
	D3DCHK(g_dev->CreatePixelShader(g_shobj_ps_clear, sizeof(g_shobj_ps_clear), nullptr, &g_psClear));
	SetName(g_psClear, "ui:Clear");

	D3DCHK(g_dev->CreateVertexShader(g_shobj_vs_draw2d, sizeof(g_shobj_vs_draw2d), nullptr, &g_vsDraw2D));
	SetName(g_vsDraw2D, "ui:Draw2D");
	D3DCHK(g_dev->CreatePixelShader(g_shobj_ps_draw2d, sizeof(g_shobj_ps_draw2d), nullptr, &g_psDraw2D));
	SetName(g_psDraw2D, "ui:Draw2D");

	D3DCHK(g_dev->CreateVertexShader(g_shobj_vs_draw3dunlit, sizeof(g_shobj_vs_draw3dunlit), nullptr, &g_vsDraw3DUnlit));
	SetName(g_vsDraw3DUnlit, "ui:Draw3DUnlit");
	D3DCHK(g_dev->CreatePixelShader(g_shobj_ps_draw3dunlit, sizeof(g_shobj_ps_draw3dunlit), nullptr, &g_psDraw3DUnlit));
	SetName(g_psDraw3DUnlit, "ui:Draw3DUnlit");

	for (auto* L : GetListeners())
		L->OnAttach({ g_dev, g_ctx });

	Reset2DRender();
}

void GlobalFree()
{
	for (auto* L : GetListeners())
		L->OnDetach({ g_dev, g_ctx });

	SAFE_RELEASE(g_psDraw3DUnlit);
	SAFE_RELEASE(g_vsDraw3DUnlit);

	SAFE_RELEASE(g_psDraw2D);
	SAFE_RELEASE(g_vsDraw2D);
	
	SAFE_RELEASE(g_psClear);
	SAFE_RELEASE(g_vsClear);

	for (int i = 0; i < 8; i++)
		SAFE_RELEASE(g_inputLayouts3D[i]);
	SAFE_RELEASE(g_inputLayout2D);

	SAFE_RELEASE(g_bsAlphaBlend);
	SAFE_RELEASE(g_bsNoBlend);
	SAFE_RELEASE(g_bsClear);

	for (int i = 0; i < 4; i++)
		SAFE_RELEASE(g_samplers[i]);

	for (int i = 0; i < 8; i++)
		SAFE_RELEASE(g_depthStencilStates[i]);

	for (int i = 0; i < 4; i++)
		SAFE_RELEASE(g_renderStates3D[i]);
	SAFE_RELEASE(g_renderState2D);

	delete g_defTex;
	delete g_defVBCC;
	delete g_defVB;
	delete g_tmpCB;
	delete g_tmpIB;
	delete g_tmpVB;

	SAFE_RELEASE(g_dxgiFactory);
	SAFE_RELEASE(g_dxgiAdapter3);
	SAFE_RELEASE(g_dxgiAdapter);

#ifdef D3D_DUMP_LIVE_OBJECTS
	g_ctx->ClearState();
	g_ctx->Flush();
	ID3D11Debug* dbg = nullptr;
	g_dev->QueryInterface(__uuidof(ID3D11Debug), (void**)&dbg);
#endif
	SAFE_RELEASE(g_ctx);
	SAFE_RELEASE(g_dev);

#ifdef D3D_DUMP_LIVE_OBJECTS
	if (dbg)
	{
		dbg->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		dbg->Release();
	}
#endif

	GraphicsAdapters_Unlock();
}

u64 GetVideoMemoryUsage()
{
	if (g_dxgiAdapter3)
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO vmi = {};
		D3DCHK(g_dxgiAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vmi));
		return vmi.CurrentUsage;
	}
	return 0;
}

void OnListenerAdd(IRHIListener* L)
{
	L->OnAttach({ g_dev, g_ctx });

	for (auto* rc = RenderContext::first; rc; rc = rc->next)
		L->OnAfterInitSwapChain(rc->GetPtrs());
}

void OnListenerRemove(IRHIListener* L)
{
	for (auto* rc = RenderContext::first; rc; rc = rc->next)
		L->OnBeforeFreeSwapChain(rc->GetPtrs());

	L->OnDetach({ g_dev, g_ctx });
}

RenderContext* CreateRenderContext(void* window)
{
	RenderContext* RC = new RenderContext();

	auto hwnd = (HWND)window;
	RC->window = hwnd;

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

void OnChangeFullscreen(RenderContext* RC, const Optional<ExclusiveFullscreenInfo>& info)
{
	RC->SetFullScreen(info);
}

void SetVSyncInterval(RenderContext* RC, unsigned interval)
{
	RC->vsyncInterval = interval;
}

void BeginFrame(RenderContext* RC)
{
	SetActiveContext(RC);
	for (auto* L : GetListeners())
		L->OnBeginFrame(RC->GetPtrs());
}

void EndFrame(RenderContext* RC)
{
	for (auto* L : GetListeners())
		L->OnEndFrame(RC->GetPtrs());
	Present(RC);
}


void SetScissorRect(int x0, int y0, int x1, int y1)
{
	D3D11_RECT rect = { x0, y0, x1, y1 };
	g_ctx->RSSetScissorRects(1, &rect);
}

static AABB2i g_realVP;
static void _SetViewport(const AABB2i& vp)
{
	g_realVP = vp;
	D3D11_VIEWPORT viewport = { float(vp.x0), float(vp.y0), float(vp.x1 - vp.x0), float(vp.y1 - vp.y0), 0.0f, 1.0f };
	g_ctx->RSSetViewports(1, &viewport);
}

static AABB2i g_viewport;
void SetViewport(int x0, int y0, int x1, int y1)
{
	g_viewport = { x0, y0, x1, y1 };
	_SetViewport(g_viewport);
}

void ApplyViewport()
{
	_SetViewport(g_viewport);
}

void Clear(int r, int g, int b, int a)
{
	Color4f col = Color4b(r, g, b, a);
	if (g_realVP.x0 == 0 && g_realVP.y0 == 0 && g_realVP.x1 == g_RC->width && g_realVP.y1 == g_RC->height)
	{
		g_ctx->ClearRenderTargetView(g_RC->backBufferRTV, &col.r);
		g_ctx->ClearDepthStencilView(g_RC->depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	else
	{
		g_ctx->VSSetShader(g_vsClear, nullptr, 0);
		g_ctx->PSSetShader(g_psClear, nullptr, 0);

		g_ctx->OMSetBlendState(g_bsClear, &col.r, 0xffffffff);

		g_ctx->Draw(3, 0);
	}
}

void ClearDepthOnly(float depth)
{
	g_ctx->ClearDepthStencilView(g_RC->depthStencilView, D3D11_CLEAR_DEPTH, depth, 0);
}

void Present(RenderContext* RC)
{
	RC->swapChain->Present(RC->vsyncInterval, 0);
}

Texture2D* CreateTextureA8(const void* data, unsigned width, unsigned height, uint8_t flags)
{
	return new Texture2D(data, width, height, flags, true);
}

Texture2D* CreateTextureRGBA8(const void* data, unsigned width, unsigned height, uint8_t flags)
{
	return new Texture2D(data, width, height, flags, false);
}

Texture2D* CreateTextureFromAPIHandle(unsigned width, unsigned height, uintptr_t handle)
{
	return Texture2D::NewFromAPIHandle(width, height, handle);
}

void SetTextureDebugName(Texture2D* tex, StringView debugName)
{
	if (!tex)
		return;
	SetName(tex->tex, debugName);
	SetName(tex->srv, debugName);
}

void DestroyTexture(Texture2D* tex)
{
	delete tex;
}

MapData MapTexture(Texture2D* tex)
{
#if D3D_USE_STAGING_TEXTURE
	tex->aabb = AABB2i::Empty();
	D3D11_MAPPED_SUBRESOURCE msr = {};
	D3DCHK(g_ctx->Map(tex->staging, 0, D3D11_MAP_WRITE, 0, &msr));

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
#if D3D_USE_STAGING_TEXTURE
	for (uint16_t curY = 0; curY < h; curY++)
	{
		memcpy(
			static_cast<char*>(md.data) + (y + curY) * md.pitch + x * bpp,
			static_cast<const char*>(data) + curY * w * bpp,
			w * bpp);
	}
	AABB2i bbox(x, y, x + w, y + h);
	tex->aabb.Include(bbox);
#else
	if (w == 0 || h == 0)
		return;
	D3D11_BOX box = { x, y, 0U, UINT(x + w), UINT(y + h), 1U };
	g_ctx->UpdateSubresource(tex->tex, 0, &box, data, w * bpp, 0);
#endif
}

void UnmapTexture(Texture2D* tex)
{
#if D3D_USE_STAGING_TEXTURE
	g_ctx->Unmap(tex->staging, 0);
	D3D11_BOX box = {};
	box.left = tex->aabb.x0;
	box.top = tex->aabb.y0;
	box.right = tex->aabb.x1;
	box.bottom = tex->aabb.y1;
	box.back = 1;
	g_ctx->CopySubresourceRegion(
		tex->tex, 0,
		tex->aabb.x0, tex->aabb.y0, 0,
		tex->staging, 0,
		&box);
#endif
}

void SetTexture(Texture2D* tex)
{
	g_curTex = tex;
	if (!tex)
		tex = g_defTex;
	g_ctx->PSSetShaderResources(0, 1, &tex->srv);
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

static unsigned g_drawFlags;
void SetRenderState(unsigned drawFlags)
{
	g_drawFlags = drawFlags;
}

static Mat4f g_viewMatrix = Mat4f::Identity();
static Mat4f g_projMatrix = Mat4f::Identity();

void SetViewMatrix(const Mat4f& m)
{
	g_viewMatrix = m;
}

void SetProjectionMatrix(const Mat4f& m)
{
	g_projMatrix = m;
}

void SetForcedColor(const Color4b& col)
{
	DefaultVertex dv = {};
	dv.col = col;
	g_defVBCC->Write(&dv, sizeof(dv));
}

static AABB2i g_3DRect;
void Begin3DMode(const AABB2i& rect)
{
	_SetViewport(rect);

	SetRenderState(0);
	SetProjectionMatrix(Mat4f::Identity());
	SetAmbientLight(Color4f::White());
	for (int i = 0; i < 8; i++)
		SetLightOff(i);

	SetForcedColor({ 255, 0, 255 });

	g_prevTex = g_curTex;
	SetTexture(nullptr);

	g_3DRect = rect;
}

AABB2i End3DMode()
{
	auto curRect = g_3DRect;

	Reset2DRender();
	SetTexture(g_prevTex);
	_SetViewport(g_viewport);

	return curRect;
}

void RestoreRenderStates()
{
	Reset2DRender();
	SetTexture(g_prevTex);
}

void SetupRenderStateForOutputSize(int w, int h)
{
	Reset2DRender(Vec2i(w, h));
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

static D3D11_PRIMITIVE_TOPOLOGY ConvertPrimitiveType(PrimitiveType t)
{
	switch (t)
	{
	case PT_Points: return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PT_Lines: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case PT_LineStrip: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case PT_Triangles: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PT_TriangleStrip: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
}

static void ApplyVertexData(unsigned vertexFormat, const void* vertices, size_t numVertices)
{
	size_t size = GetVertexSize(vertexFormat);
	g_tmpVB->Write(vertices, size * numVertices);
	ID3D11Buffer* buffers[2] = { g_tmpVB->buffer, (g_drawFlags & DF_ForceColor ? g_defVBCC : g_defVB)->buffer };
	UINT strides[2] = { size, 0 };
	UINT offsets[2] = {};
	g_ctx->IASetVertexBuffers(0, 2, buffers, strides, offsets);
	if (g_drawFlags & DF_ForceColor)
		vertexFormat &= ~VF_Color;
	g_ctx->IASetInputLayout(g_inputLayouts3D[vertexFormat & 0x7]);
}

struct CBuf3D
{
	Mat4f worldViewProjMtx;
	float alphaTestShift;
	float padding[3];
};

static void UploadShaderData(const Mat4f& world)
{
	// DF_Lit (TODO)
	g_ctx->VSSetShader(g_vsDraw3DUnlit, nullptr, 0);

	g_ctx->PSSetShader(g_psDraw3DUnlit, nullptr, 0);

	// DF_Cull / DF_Wireframe
	unsigned rsidx = (g_drawFlags & DF_Cull ? 1 : 0) | (g_drawFlags & DF_Wireframe ? 2 : 0);
	g_ctx->RSSetState(g_renderStates3D[rsidx]);

	// DF_AlphaBlended
	g_ctx->OMSetBlendState(g_drawFlags & DF_AlphaBlended ? g_bsAlphaBlend : g_bsNoBlend, nullptr, 0xffffffff);

	// DF_ZTestOff / DF_ZWriteOff / DF_ZTestReverse
	unsigned dssidx = (g_drawFlags & DF_ZTestOff ? 1 : 0)
		| (g_drawFlags & DF_ZWriteOff ? 2 : 0)
		| (g_drawFlags & DF_ZTestReverse ? 4 : 0);
	g_ctx->OMSetDepthStencilState(g_depthStencilStates[dssidx], 0);

	CBuf3D cbuf = {};
	cbuf.worldViewProjMtx = world * g_viewMatrix * g_projMatrix;
	cbuf.alphaTestShift = g_drawFlags & DF_AlphaTest ? 0.5f : 0;
	g_tmpCB->Write(&cbuf, sizeof(cbuf));
	g_ctx->VSSetConstantBuffers(0, 1, &g_tmpCB->buffer);
	g_ctx->PSSetConstantBuffers(0, 1, &g_tmpCB->buffer);
}

void Draw(
	const Mat4f& xf,
	PrimitiveType primType,
	unsigned vertexFormat,
	const void* vertices,
	size_t numVertices)
{
	g_ctx->IASetPrimitiveTopology(ConvertPrimitiveType(primType));
	ApplyVertexData(vertexFormat, vertices, numVertices);
	UploadShaderData(xf);

	g_ctx->Draw(numVertices, 0);
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
	g_ctx->IASetPrimitiveTopology(ConvertPrimitiveType(primType));
	ApplyVertexData(vertexFormat, vertices, numVertices);
	UploadShaderData(xf);

	g_tmpIB->Write(indices, numIndices * (i32 ? 4 : 2));
	g_ctx->IASetIndexBuffer(g_tmpIB->buffer, i32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);

	g_ctx->DrawIndexed(numIndices, 0, 0);
}


} // gfx
} // ui
