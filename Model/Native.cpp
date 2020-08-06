
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

#include <codecvt>
#undef min
#undef max

#include "Native.h"
#include "System.h"
#include "Menu.h"

#include "../Render/OpenGL.h"
#include "../Render/Render.h"


#define WINDOW_CLASS_NAME L"UIWindow"


enum MoveSizeStateType
{
	MSST_None,
	MSST_Unknown,
	MSST_Move,
	MSST_Resize,
};


static constexpr uint32_t _bbe(uint32_t x)
{
	return (x >> 24) | ((x >> 8) & 0xff00U) | ((x & 0xff00U) << 8) | (x << 24);
}
static constexpr uint32_t operator "" _bmp(const char* bits)
{
	uint32_t o = 0;
	for (int i = 0; i < 32 && bits[i]; i++)
		o |= ((bits[i] == '1' ? 1 : 0) << i);
	return _bbe(o);
}
static constexpr uint32_t operator "" _ibmp(const char* bits)
{
	uint32_t o = 0;
	for (int i = 0; i < 32 && bits[i]; i++)
		o |= ((bits[i] == '1' ? 1 : 0) << i);
	return _bbe(~o);
}
static constexpr uint32_t R = ~0U;


static const uint32_t g_noCursorAND[] = { R };
static const uint32_t g_noCursorXOR[] = { 0 };
static const uint32_t g_resizeColCursorAND[] =
{
	R, R, R, R, R, R, R, // [0-6]
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000101111110100000000000_ibmp,
	00000000001111111111110000000000_ibmp,
	00000000011111111111111000000000_ibmp,
	00000000111111111111111100000000_ibmp,
	00000000011111111111111000000000_ibmp,
	00000000001111111111110000000000_ibmp,
	00000000000101111110100000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	00000000000001111110000000000000_ibmp,
	R, R, R, R, R, R, R, R, // [24-31]
};
static const uint32_t g_resizeColCursorXOR[] =
{
	0, 0, 0, 0, 0, 0, 0, // [0-6]
	00000000000001111110000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000101011010100000000000_bmp,
	00000000001011011011010000000000_bmp,
	00000000010011011011001000000000_bmp,
	00000000100000011000000100000000_bmp,
	00000000010011011011001000000000_bmp,
	00000000001011011011010000000000_bmp,
	00000000000101011010100000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001011010000000000000_bmp,
	00000000000001111110000000000000_bmp,
	0, 0, 0, 0, 0, 0, 0, 0, // [24-31]
};
static const uint32_t g_resizeRowCursorAND[] =
{
	R, R, R, R, R, R, R, R, // [0-7]
	00000000000000010000000000000000_ibmp,
	00000000000000111000000000000000_ibmp,
	00000000000001111100000000000000_ibmp,
	00000000000011111110000000000000_ibmp,
	00000000000001111100000000000000_ibmp,
	00000001111111111111111100000000_ibmp,
	00000001111111111111111100000000_ibmp,
	00000001111111111111111100000000_ibmp,
	00000001111111111111111100000000_ibmp,
	00000001111111111111111100000000_ibmp,
	00000001111111111111111100000000_ibmp,
	00000000000001111100000000000000_ibmp,
	00000000000011111110000000000000_ibmp,
	00000000000001111100000000000000_ibmp,
	00000000000000111000000000000000_ibmp,
	00000000000000010000000000000000_ibmp,
	R, R, R, R, R, R, R, R, // [24-31]
};
static const uint32_t g_resizeRowCursorXOR[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, // [0-7]
	00000000000000010000000000000000_bmp,
	00000000000000101000000000000000_bmp,
	00000000000001000100000000000000_bmp,
	00000000000010000010000000000000_bmp,
	00000000000001101100000000000000_bmp,
	00000001111111101111111100000000_bmp,
	00000001000000000000000100000000_bmp,
	00000001111111111111111100000000_bmp,
	00000001111111111111111100000000_bmp,
	00000001000000000000000100000000_bmp,
	00000001111111101111111100000000_bmp,
	00000000000001101100000000000000_bmp,
	00000000000010000010000000000000_bmp,
	00000000000001000100000000000000_bmp,
	00000000000000101000000000000000_bmp,
	00000000000000010000000000000000_bmp,
	0, 0, 0, 0, 0, 0, 0, 0, // [24-31]
};


static bool g_appQuit = false;
static int g_appExitCode = 0;

static HCURSOR g_defaultCursors[(int)ui::DefaultCursor::_COUNT];


static void LoadDefaultCursors()
{
	auto hinst = (HINSTANCE)GetModuleHandleW(nullptr);
	g_defaultCursors[(int)ui::DefaultCursor::None] = ::CreateCursor(hinst, 0, 0, 1, 1, g_noCursorAND, g_noCursorXOR);
	g_defaultCursors[(int)ui::DefaultCursor::Default] = ::LoadCursor(nullptr, IDC_ARROW);
	g_defaultCursors[(int)ui::DefaultCursor::Pointer] = ::LoadCursor(nullptr, IDC_HAND);
	g_defaultCursors[(int)ui::DefaultCursor::Crosshair] = ::LoadCursor(nullptr, IDC_CROSS);
	g_defaultCursors[(int)ui::DefaultCursor::Help] = ::LoadCursor(nullptr, IDC_HELP);
	g_defaultCursors[(int)ui::DefaultCursor::Text] = ::LoadCursor(nullptr, IDC_IBEAM);
	g_defaultCursors[(int)ui::DefaultCursor::NotAllowed] = ::LoadCursor(nullptr, IDC_NO);
	g_defaultCursors[(int)ui::DefaultCursor::Progress] = ::LoadCursor(nullptr, IDC_APPSTARTING);
	g_defaultCursors[(int)ui::DefaultCursor::Wait] = ::LoadCursor(nullptr, IDC_WAIT);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeAll] = ::LoadCursor(nullptr, IDC_SIZEALL);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeHorizontal] = ::LoadCursor(nullptr, IDC_SIZEWE);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeVertical] = ::LoadCursor(nullptr, IDC_SIZENS);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeNESW] = ::LoadCursor(nullptr, IDC_SIZENESW);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeNWSE] = ::LoadCursor(nullptr, IDC_SIZENWSE);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeCol] = ::CreateCursor(hinst, 15, 15, 32, 32, g_resizeColCursorAND, g_resizeColCursorXOR);
	g_defaultCursors[(int)ui::DefaultCursor::ResizeRow] = ::CreateCursor(hinst, 15, 15, 32, 32, g_resizeRowCursorAND, g_resizeRowCursorXOR);
}

static void UnloadDefaultCursors()
{
	::DestroyCursor(g_defaultCursors[(int)ui::DefaultCursor::None]);
	::DestroyCursor(g_defaultCursors[(int)ui::DefaultCursor::ResizeCol]);
	::DestroyCursor(g_defaultCursors[(int)ui::DefaultCursor::ResizeRow]);
}

bool IsOwnWindow(HWND win)
{
	WCHAR classname[sizeof(WINDOW_CLASS_NAME)];
	UINT len = RealGetWindowClassW(win, classname, sizeof(WINDOW_CLASS_NAME));
	return 0 == wcsncmp(classname, WINDOW_CLASS_NAME, sizeof(WINDOW_CLASS_NAME));
}

BOOL CALLBACK DisableAllExcept(HWND win, LPARAM except)
{
	// may contain IME windows so filter by class
	if (win != (HWND)except && IsOwnWindow(win))
		EnableWindow(win, FALSE);
	return TRUE;
}

BOOL CALLBACK EnableAllExcept(HWND win, LPARAM except)
{
	// may contain IME windows so filter by class
	if (win != (HWND)except && IsOwnWindow(win))
		EnableWindow(win, TRUE);
	return TRUE;
}


static std::string WCHARToUTF8(const WCHAR* str)
{
	int size = ::WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
	std::string ret;
	if (size <= 0)
		return ret;
	ret.resize(size);
	size = ::WideCharToMultiByte(CP_UTF8, 0, str, -1, &ret[0], size, nullptr, nullptr);
	if (size <= 0)
		return {};
	ret.resize(size);
	return ret;
}

static HGLOBAL UTF8ToWCHARGlobal(StringView str)
{
	int size = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0);
	if (size < 0)
		return INVALID_HANDLE_VALUE;
	HGLOBAL hglb = ::GlobalAlloc(GMEM_MOVEABLE, (size + 1) * sizeof(WCHAR));
	WCHAR* dest = (WCHAR*)::GlobalLock(hglb);
	size = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), dest, size);
	if (size < 0)
	{
		::GlobalUnlock(hglb);
		::GlobalFree(hglb);
		return INVALID_HANDLE_VALUE;
	}
	dest[size + 1] = 0;
	::GlobalUnlock(hglb);
	return hglb;
}

static HGLOBAL UTF8ToWCHARGlobal(const char* str)
{
	return UTF8ToWCHARGlobal(StringView(str, SIZE_MAX));
}


namespace ui {


namespace platform {

uint32_t GetTimeMs()
{
	return ::GetTickCount();
}

uint32_t GetDoubleClickTime()
{
	return ::GetDoubleClickTime();
}

} // platform


bool Clipboard::HasText()
{
	return ::IsClipboardFormatAvailable(CF_UNICODETEXT);
}

std::string Clipboard::GetText()
{
	std::string ret;
	if (HasText())
	{
		if (!::OpenClipboard(nullptr))
			return ret;
		HANDLE hcbd = ::GetClipboardData(CF_UNICODETEXT);
		WCHAR* mem = (WCHAR*)::GlobalLock(hcbd);
		if (mem)
		{
			ret = WCHARToUTF8(mem);
			::GlobalUnlock(hcbd);
		}
		::CloseClipboard();
		return ret;
	}
	return ret;
}

void Clipboard::SetText(const char* text)
{
	if (!::OpenClipboard(nullptr))
		return;
	::SetClipboardData(CF_UNICODETEXT, UTF8ToWCHARGlobal(text));
	::CloseClipboard();
}

void Clipboard::SetText(StringView text)
{
	if (!::OpenClipboard(nullptr))
		return;
	::SetClipboardData(CF_UNICODETEXT, UTF8ToWCHARGlobal(text));
	::CloseClipboard();
}


DataCategoryTag DCT_ResizeWindow[1];


static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void DebugDrawSelf(UIObject* o)
{
	rhi::SetTexture(0);

	float a = 0.2f;

	auto s = o->GetStyle();
	float w = o->finalRectC.x1 - o->finalRectC.x0;
	auto ml = o->ResolveUnits(s.GetMarginLeft(), w), mr = o->ResolveUnits(s.GetMarginRight(), w),
		mt = o->ResolveUnits(s.GetMarginTop(), w), mb = o->ResolveUnits(s.GetMarginBottom(), w);
	auto pl = o->ResolveUnits(s.GetPaddingLeft(), w), pr = o->ResolveUnits(s.GetPaddingRight(), w),
		pt = o->ResolveUnits(s.GetPaddingTop(), w), pb = o->ResolveUnits(s.GetPaddingBottom(), w);
	float bl = 1, br = 1, bt = 1, bb = 1;
	auto r = o->GetBorderRect();

	// padding
	draw::RectCutoutCol(r, o->GetPaddingRect(), Color4f(0.5f, 0.6f, 0.9f, a));
	// border
#if 1
	draw::RectCutoutCol(r, r.ExtendBy(UIRect::UniformBorder(-1)), Color4f(0.5f, 0.9f, 0.6f, a));
#endif
	// margin
	draw::RectCutoutCol(r, r.ExtendBy({ ml, mt, mr, mb }), Color4f(0.9f, 0.6f, 0.5f, a));
}

void DebugDraw(UIObject* o)
{
	DebugDrawSelf(o);
	for (auto* ch = o->firstChild; ch; ch = ch->next)
		DebugDraw(ch);
}

double hqtime()
{
	LARGE_INTEGER freq, cnt;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cnt);
	return double(cnt.QuadPart) / double(freq.QuadPart);
}

struct ProxyEventSystem
{
	struct Target
	{
		UIEventSystem* target;
		UIRect window;
	};

	void OnMouseMove(UIMouseCoord x, UIMouseCoord y)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnMouseMove(x - mainTarget.window.x0, y - mainTarget.window.y0);
	}
	void OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnMouseButton(down, which, x - mainTarget.window.x0, y - mainTarget.window.y0);
	}
	void OnMouseScroll(UIMouseCoord dx, UIMouseCoord dy)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnMouseScroll(dx, dy);
	}
	void OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint16_t numRepeats)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnKeyInput(down, vk, pk, numRepeats);
	}
	void OnKeyAction(UIKeyAction act, uint16_t numRepeats, bool modifier = false)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnKeyAction(act, numRepeats, modifier);
	}
	void OnTextInput(uint32_t ch, uint16_t numRepeats)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnTextInput(ch, numRepeats);
	}

	Target mainTarget;
};

struct NativeWindow_Impl;
static std::vector<NativeWindow_Impl*>* g_windowRepaintList = nullptr;
static std::vector<NativeWindow_Impl*>* g_curWindowRepaintList = nullptr;

struct NativeWindow_Impl
{
	void Init(NativeWindowBase* owner)
	{
		system.nativeWindow = owner;

		window = CreateWindowExW(0, L"UIWindow", L"UI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, NULL, NULL, GetModuleHandle(nullptr), this);
		DragAcceptFiles(window, TRUE);
		renderCtx = rhi::CreateRenderContext(window);

		//UpdateVisibilityState();

		RECT clientRect;
		GetClientRect(window, &clientRect);

		auto& evsys = GetEventSys();

		evsys.width = float(clientRect.right);
		evsys.height = float(clientRect.bottom);

		auto& pes = GetProxyEventSys();

		pes.mainTarget = { &evsys, { 0, 0, evsys.width, evsys.height } };

		// TODO fix
		static bool init = false;
		if (!init)
		{
			init = true;
			InitFont();
			InitTheme();
		}

		prevTime = hqtime();

		owner->InvalidateAll();
		cursor = g_defaultCursors[(int)DefaultCursor::Default];
	}
	~NativeWindow_Impl()
	{
		if (invalidated)
			g_windowRepaintList->erase(std::remove(g_windowRepaintList->begin(), g_windowRepaintList->end(), this), g_windowRepaintList->end());
		rhi::FreeRenderContext(renderCtx);
		DestroyWindow(window);
	}

	bool AddToInvalidationList()
	{
		if (invalidated)
			return false;
		invalidated = true;
		g_windowRepaintList->push_back(this);
		return true;
	}

	void SetRenderFunc(std::function<void(UIContainer*)> renderFunc)
	{
		auto& cont = GetContainer();
		auto& evsys = GetEventSys();

		auto* N = cont.AllocIfDifferent<RenderNode>(cont.rootNode);
		N->renderFunc = renderFunc;
		cont._BuildUsing(N);
		evsys.RecomputeLayout();
	}

	void Redraw()
	{
		if (!innerUIEnabled)
			return;

		double t = hqtime();

		rhi::SetActiveContext(renderCtx);

		auto& cont = GetContainer();
		auto& evsys = GetEventSys();

		float minTime = evsys.ProcessTimers(float(t - prevTime));
		if (minTime < FLT_MAX)
			SetTimer(window, 1, minTime * 1000, nullptr);
		else
			KillTimer(window, 1);
		prevTime = t;

		cont.ProcessNodeRenderStack();
		evsys.RecomputeLayout();
		evsys.OnMouseMove(evsys.prevMouseX, evsys.prevMouseY);
		cont.ProcessNodeRenderStack();
		evsys.RecomputeLayout();

#if DRAW_STATS
		double t0 = hqtime();
		auto stats0 = rhi::Stats::Get();
#endif

		rhi::SetActiveContext(renderCtx);
		rhi::SetViewport(0, 0, evsys.width, evsys.height);
		draw::_ResetScissorRectStack(0, 0, evsys.width, evsys.height);
		draw::internals::OnBeginDrawFrame();

		//GL::Clear(20, 40, 80, 255);
		rhi::Clear(0x25, 0x25, 0x25, 255);
		if (cont.rootNode)
			cont.rootNode->OnPaint();

		system.overlays.UpdateSorted();
		for (auto& ovr : system.overlays.sorted)
			ovr.obj->OnPaint();

#if 0
		draw::RectTex(20, 120, 256 + 20, 128 + 120, g_themeTexture);
		DrawThemeElement(TE_ButtonNormal, 300, 20, 380, 40);
		DrawThemeElement(TE_ButtonPressed, 300, 40, 380, 60);
		DrawThemeElement(TE_ButtonHover, 300, 60, 380, 80);
		DrawTextLine(32, 32, "Test text", 1, 1, 1);
#endif

		if (debugDrawEnabled)
		{
			// debug draw
			if (cont.rootNode)
				DebugDraw(cont.rootNode);
		}

		draw::internals::OnEndDrawFrame();

#if DRAW_STATS
		double t1 = hqtime();
		auto stats1 = rhi::Stats::Get();
		auto statsdiff = stats1 - stats0;
		printf("render time: %g ms\n", (t1 - t0) * 1000);
		printf("# SetTexture: %u\n", unsigned(statsdiff.num_SetTexture));
		printf("# DrawTriangles: %u\n", unsigned(statsdiff.num_DrawTriangles));
		printf("# DrawIndexedTriangles: %u\n", unsigned(statsdiff.num_DrawIndexedTriangles));
#endif

#if DEBUG_DRAW_ATLAS
		if (draw::debug::GetAtlasTextureCount())
		{
			rhi::SetTexture(draw::debug::GetAtlasTexture(0, nullptr));
			int D = 4;
			rhi::Vertex verts[4] =
			{
				{ evsys.width / D, evsys.height / D, 0, 0, Color4b::White() },
				{ evsys.width, evsys.height / D, 1, 0, Color4b::White() },
				{ evsys.width, evsys.height, 1, 1, Color4b::White() },
				{ evsys.width / D, evsys.height, 0, 1, Color4b::White() },
			};
			uint16_t indices[6] = { 0, 1, 2,  2, 3, 0 };
			rhi::DrawIndexedTriangles(verts, indices, 6);
		}
#endif

		rhi::Present(renderCtx);
	}

	void UpdateVisibilityState()
	{
		if (!visible)
			ExitExclusiveMode();
		ShowWindow(window, visible ? SW_SHOW : SW_HIDE);
	}

	void UpdateStyle()
	{
		int ws = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_DLGFRAME | WS_MAXIMIZEBOX);
		if (style & WS_Resizable)
			ws |= WS_THICKFRAME | WS_MAXIMIZEBOX;
		if (style & WS_TitleBar)
			ws |= WS_DLGFRAME;
		SetWindowLong(window, GWL_STYLE, ws);
		SetWindowPos(window, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}

	void EnterExclusiveMode()
	{
		if (exclusiveMode)
			return;
		assert(visible);
		EnumThreadWindows(GetCurrentThreadId(), DisableAllExcept, (LPARAM)window);
		exclusiveMode = true;
	}

	void ExitExclusiveMode()
	{
		if (!exclusiveMode)
			return;
		EnumThreadWindows(GetCurrentThreadId(), EnableAllExcept, (LPARAM)window);
		exclusiveMode = false;
	}

	UIEventSystem& GetEventSys() { return system.eventSystem; }
	ProxyEventSystem& GetProxyEventSys() { return proxyEventSystem; }
	UIContainer& GetContainer() { return system.container; }
	NativeWindowBase* GetOwner() { return system.nativeWindow; }

	ui::FrameContents system;
	ProxyEventSystem proxyEventSystem;

	HWND window;
	HCURSOR cursor;
	rhi::RenderContext* renderCtx;

	Menu* menu;

	double prevTime;
	bool visible = true;
	bool exclusiveMode = false;
	bool innerUIEnabled = true;
	bool debugDrawEnabled = false;
	bool firstShow = true;
	bool invalidated = false;
	uint8_t sysMoveSizeState = MSST_None;
	WindowStyle style = WS_Default;
};

static NativeWindow_Impl* GetNativeWindow(HWND hWnd)
{
	return reinterpret_cast<NativeWindow_Impl*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

static ProxyEventSystem* GetEventSys(HWND hWnd)
{
	if (auto* w = GetNativeWindow(hWnd))
		return &w->GetProxyEventSys();
	return nullptr;
}


NativeWindowBase::NativeWindowBase()
{
	_impl = new NativeWindow_Impl();
	_impl->Init(this);
}

#if 0
NativeWindowBase::NativeWindowBase(std::function<void(UIContainer*)> renderFunc)
{
	_impl = new NativeWindow_Impl();
	_impl->Init(this);
	_impl->SetRenderFunc(renderFunc);
}
#endif

NativeWindowBase::~NativeWindowBase()
{
	_impl->GetContainer().Free();
	delete _impl;
}

void NativeWindowBase::OnClose()
{
	SetVisible(false);
}

#if 0
void NativeWindowBase::SetRenderFunc(std::function<void(UIContainer*)> renderFunc)
{
	_impl->SetRenderFunc(renderFunc);
}
#endif

std::string NativeWindowBase::GetTitle()
{
	std::string title;
	// TODO unicode
	title.resize(GetWindowTextLengthA(_impl->window));
	GetWindowTextA(_impl->window, &title[0], title.size());
	return title;
}

void NativeWindowBase::SetTitle(const char* title)
{
	// TODO unicode
	SetWindowTextA(_impl->window, title);
}

WindowStyle NativeWindowBase::GetStyle()
{
	return _impl->style;
}

void NativeWindowBase::SetStyle(WindowStyle ws)
{
	_impl->style = ws;
	_impl->UpdateStyle();
}

bool NativeWindowBase::IsVisible()
{
	return _impl->visible;
}

void NativeWindowBase::SetVisible(bool v)
{
	_impl->visible = v;
	_impl->UpdateVisibilityState();

	if (v && _impl->firstShow)
	{
		_impl->firstShow = false;

		auto& cont = _impl->GetContainer();
		auto& evsys = _impl->GetEventSys();

		auto* N = cont.AllocIfDifferent<RenderNode>(cont.rootNode);
		N->renderFunc = [this](UIContainer* ctx) { OnRender(ctx); };
		cont._BuildUsing(N);
		evsys.RecomputeLayout();
	}
}

Menu* NativeWindowBase::GetMenu()
{
	return _impl->menu;
}

void NativeWindowBase::SetMenu(Menu* m)
{
	_impl->menu = m;
	::SetMenu(_impl->window, m ? (HMENU)m->GetNativeHandle() : nullptr);
	// TODO is there a way to prevent partial redrawing results from showing up?
}

Point<int> NativeWindowBase::GetPosition()
{
	RECT r = {};
	GetWindowRect(_impl->window, &r);
	return { r.left, r.top };
}

void NativeWindowBase::SetPosition(int x, int y)
{
	SetWindowPos(_impl->window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

void NativeWindowBase::ProcessEventsExclusive()
{
	_impl->EnterExclusiveMode();

	MSG msg;
	while (!g_appQuit && _impl->visible && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		if (msg.hwnd)
		{
			if (auto* window = GetNativeWindow(msg.hwnd))
				window->Redraw();
		}
	}

	_impl->ExitExclusiveMode();
}

Size<int> NativeWindowBase::GetSize()
{
	RECT r = {};
	GetWindowRect(_impl->window, &r);
	return { r.right - r.left, r.bottom - r.top };
}

void NativeWindowBase::SetSize(int x, int y, bool inner)
{
	if (inner)
	{
		RECT r = { 0, 0, x, y };
		if (AdjustWindowRect(&r, GetWindowStyle(_impl->window), !!_impl->menu))
		{
			x = r.right - r.left;
			y = r.bottom - r.top;
		}
	}
	SetWindowPos(_impl->window, nullptr, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

WindowState NativeWindowBase::GetState()
{
	if (IsIconic(_impl->window))
		return WindowState::Minimized;
	if (IsZoomed(_impl->window))
		return WindowState::Maximized;
	return WindowState::Normal;
}

static int StateToShowCmd(WindowState ws)
{
	switch (ws)
	{
	case WindowState::Minimized: return SW_MINIMIZE;
	case WindowState::Maximized: return SW_MAXIMIZE;
	default: return SW_RESTORE;
	}
}

void NativeWindowBase::SetState(WindowState ws)
{
	ShowWindow(_impl->window, StateToShowCmd(ws));
}

NativeWindowGeometry NativeWindowBase::GetGeometry()
{
	WINDOWPLACEMENT wpl = {};
	wpl.length = sizeof(wpl);
	GetWindowPlacement(_impl->window, &wpl);
	auto rnp = wpl.rcNormalPosition;
	return { { rnp.left, rnp.top }, { rnp.right - rnp.left, rnp.bottom - rnp.top }, wpl.showCmd | (wpl.flags << 4) };
}

void NativeWindowBase::SetGeometry(const NativeWindowGeometry& geom)
{
	WINDOWPLACEMENT wpl = {};
	wpl.length = sizeof(wpl);
	wpl.rcNormalPosition = { geom.position.x, geom.position.y, geom.position.x + geom.size.x, geom.position.y + geom.size.y };
	wpl.showCmd = geom.state & 0xf;
	wpl.flags = geom.state >> 4;
	SetWindowPlacement(_impl->window, &wpl);
}

NativeWindowBase* NativeWindowBase::GetParent() const
{
	auto pw = ::GetParent(_impl->window);
	if (pw)
	{
		if (auto* pwh = GetNativeWindow(pw))
			return pwh->GetOwner();
	}
	return nullptr;
}

void NativeWindowBase::SetParent(NativeWindowBase* parent)
{
	::SetParent(_impl->window, parent ? parent->_impl->window : nullptr);
}

bool NativeWindowBase::IsInnerUIEnabled()
{
	return _impl->innerUIEnabled;
}

void NativeWindowBase::SetInnerUIEnabled(bool enabled)
{
	_impl->innerUIEnabled = enabled;
}

bool NativeWindowBase::IsDebugDrawEnabled()
{
	return _impl->debugDrawEnabled;
}

void NativeWindowBase::SetDebugDrawEnabled(bool enabled)
{
	_impl->debugDrawEnabled = enabled;
}

void NativeWindowBase::InvalidateAll()
{
	if (!_impl->AddToInvalidationList())
		return;
	::SetTimer(_impl->window, 1, 0, nullptr);
	//PostMessage(_impl->window, WM_USER + 2, 0, 0);
}

void NativeWindowBase::SetDefaultCursor(DefaultCursor cur)
{
	_impl->cursor = g_defaultCursors[(int)cur];
}

void* NativeWindowBase::GetNativeHandle() const
{
	return (void*)_impl->window;
}

bool NativeWindowBase::IsDragged() const
{
	return _impl->sysMoveSizeState == MSST_Move;
}


void NativeWindowRenderFunc::OnRender(UIContainer* ctx)
{
	if (_renderFunc)
		_renderFunc(ctx);
}

void NativeWindowRenderFunc::SetRenderFunc(std::function<void(UIContainer*)> renderFunc)
{
	_renderFunc = renderFunc;
}


void NativeMainWindow::OnClose()
{
	Application::Quit();
}


void NativeWindowNode::OnLayout(const UIRect& rect, const Size<float>& containerSize)
{
	finalRectC = finalRectCP = finalRectCPB = {};
}


struct Inspector : ui::NativeDialogWindow
{
	Inspector()
	{
		SetTitle("Inspector");
		SetSize(1024, 768);
	}

	struct InspectorUI : ui::Node
	{
		static constexpr int TAB_W = 10;

		Inspector* insp;

		void Render(UIContainer* ctx) override {}

		static const char* CleanName(const char* name)
		{
			if (strncmp(name, "struct ", 7) == 0) name += 7;
			if (strncmp(name, "class ", 6) == 0) name += 6;
			if (strncmp(name, "ui::", 4) == 0) name += 4;
			if (strncmp(name, "style::layouts::", 5 + 7 + 4) == 0) name += 5 + 7 + 4;
			return name;
		}

		void PaintObject(UIObject* obj, int x, int& y)
		{
			int h = GetFontHeight();

			if (obj == insp->selObj)
			{
				draw::RectCol(0, y, 400, y + h, Color4f(0.5f, 0, 0));
			}

			y += h;
			DrawTextLine(x, y, CleanName(typeid(*obj).name()), 1, 1, 1);
			char bfr[1024];
			auto& fr = obj->finalRectC;
			snprintf(bfr, 1024, "%g;%g - %g;%g", fr.x0, fr.y0, fr.x1, fr.y1);
			DrawTextLine(300, y, bfr, 1, 1, 1);
			if (obj->GetStyle().GetLayout())
				DrawTextLine(400, y, CleanName(typeid(*obj->GetStyle().GetLayout()).name()), 1, 1, 1);

			for (auto* ch = obj->firstChild; ch; ch = ch->next)
			{
				PaintObject(ch, x + TAB_W, y);
			}
		}

		void OnPaint() override
		{
			auto& c = insp->selWindow->_impl->GetContainer();
			int y = GetFontHeight();
			DrawTextLine(0, y, "Name", 1, 1, 1, 0.6f);
			DrawTextLine(300, y, "Rect", 1, 1, 1, 0.6f);
			DrawTextLine(400, y, "Layout", 1, 1, 1, 0.6f);
			PaintObject(c.rootNode, 0, y);
			//DrawTextLine(10, 10, "inspector", 1, 1, 1);
		}

		void OnEvent(UIEvent& e) override
		{
		}
	};

	void OnRender(UIContainer* ctx) override
	{
		ui = ctx->Make<InspectorUI>();
		*ui + ui::Layout(style::layouts::EdgeSlice());
		ui->insp = this;
	}

	void Select(NativeWindowBase* window, UIObject* obj)
	{
		selWindow = window;
		selObj = obj;
		if (ui)
			ui->Rerender();
	}

	InspectorUI* ui = nullptr;

	NativeWindowBase* selWindow = nullptr;
	UIObject* selObj = nullptr;
};


extern void SubscriptionTable_Init();
extern void SubscriptionTable_Free();

static EventQueue* g_mainEventQueue;
static DWORD g_mainThreadID;

Application* Application::_instance;

Application::Application(int argc, char* argv[])
{
	assert(_instance == nullptr);
	_instance = this;

	g_mainEventQueue = new EventQueue;
	g_mainThreadID = GetCurrentThreadId();
	g_windowRepaintList = new std::vector<NativeWindow_Impl*>;
	g_curWindowRepaintList = new std::vector<NativeWindow_Impl*>;

	LoadDefaultCursors();
	SubscriptionTable_Init();
}

Application::~Application()
{
	if (_inspector)
		delete _inspector;

	//system.container.Free();
	SubscriptionTable_Free();
	UnloadDefaultCursors();

	delete g_windowRepaintList;
	g_windowRepaintList = nullptr;
	delete g_curWindowRepaintList;
	g_curWindowRepaintList = nullptr;
	delete g_mainEventQueue;
	g_mainEventQueue = nullptr;

	assert(_instance == this);
	_instance = nullptr;
}

void Application::Quit(int code)
{
	g_appQuit = true;
	g_appExitCode = code;
}

void Application::OpenInspector(NativeWindowBase* window, UIObject* obj)
{
	if (!_instance->_inspector)
		_instance->_inspector = new Inspector();
	_instance->_inspector->Select(window, obj);
	_instance->_inspector->SetVisible(true);
}

EventQueue& Application::_GetEventQueue()
{
	return *g_mainEventQueue;
}

void Application::_SignalEvent()
{
	PostThreadMessageW(g_mainThreadID, WM_USER + 1, 0, 0);
}

int Application::Run()
{
	MSG msg;
	while (!g_appQuit && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);

		g_mainEventQueue->RunAllCurrent();

		if (msg.hwnd)
		{
			if (auto* window = GetNativeWindow(msg.hwnd))
				window->GetOwner()->_impl->AddToInvalidationList();
		}

		assert(g_curWindowRepaintList->empty());

		std::swap(g_windowRepaintList, g_curWindowRepaintList);
		for (auto* win : *g_curWindowRepaintList)
			win->invalidated = false;

		for (auto* win : *g_curWindowRepaintList)
			win->Redraw();
		g_curWindowRepaintList->clear();

		assert(g_curWindowRepaintList->empty());
	}
	return g_appExitCode;
}

static void AdjustMouseCapture(HWND hWnd, WPARAM wParam)
{
	auto w = LOWORD(wParam);
	if (w & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2))
		SetCapture(hWnd);
	else if (GetCapture() == hWnd)
		ReleaseCapture();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NCCREATE:
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCTW*)lParam)->lpCreateParams);
		SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
		break;
	case WM_CLOSE:
		if (auto* win = GetNativeWindow(hWnd))
		{
			win->GetOwner()->OnClose();
			return TRUE;
		}
		break;
	case WM_MOUSEMOVE:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnMouseMove(UIMouseCoord(GET_X_LPARAM(lParam)), UIMouseCoord(GET_Y_LPARAM(lParam)));
		{
			TRACKMOUSEEVENT tme = {};
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;
			TrackMouseEvent(&tme);
		}
		return 0;
	case WM_MOUSELEAVE:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnMouseMove(-1, -1);
		return 0;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT)
		{
			if (auto* win = GetNativeWindow(hWnd))
				SetCursor(win->cursor);
			return TRUE;
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove(UIMouseCoord(GET_X_LPARAM(lParam)), UIMouseCoord(GET_Y_LPARAM(lParam)));
			evsys->OnMouseButton(
				message == WM_LBUTTONDOWN,
				UIMouseButton::Left,
				UIMouseCoord(GET_X_LPARAM(lParam)),
				UIMouseCoord(GET_Y_LPARAM(lParam)));
		}
		return TRUE;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove(UIMouseCoord(GET_X_LPARAM(lParam)), UIMouseCoord(GET_Y_LPARAM(lParam)));
			evsys->OnMouseButton(
				message == WM_RBUTTONDOWN,
				UIMouseButton::Right,
				UIMouseCoord(GET_X_LPARAM(lParam)),
				UIMouseCoord(GET_Y_LPARAM(lParam)));
		}
		return TRUE;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove(UIMouseCoord(GET_X_LPARAM(lParam)), UIMouseCoord(GET_Y_LPARAM(lParam)));
			evsys->OnMouseButton(
				message == WM_MBUTTONDOWN,
				UIMouseButton::Middle,
				UIMouseCoord(GET_X_LPARAM(lParam)),
				UIMouseCoord(GET_Y_LPARAM(lParam)));
		}
		return TRUE;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove(UIMouseCoord(GET_X_LPARAM(lParam)), UIMouseCoord(GET_Y_LPARAM(lParam)));
			evsys->OnMouseButton(
				message == WM_XBUTTONDOWN,
				HIWORD(wParam) == XBUTTON1 ? UIMouseButton::X1 : UIMouseButton::X2,
				UIMouseCoord(GET_X_LPARAM(lParam)),
				UIMouseCoord(GET_Y_LPARAM(lParam)));
		}
		return TRUE;
	case WM_MOUSEWHEEL:
		if (auto* evsys = GetEventSys(hWnd))
		{
			evsys->OnMouseScroll(0, GET_WHEEL_DELTA_WPARAM(wParam));
		}
		return TRUE;
	case WM_MOUSEHWHEEL:
		if (auto* evsys = GetEventSys(hWnd))
		{
			evsys->OnMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam), 0);
		}
		return TRUE;
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			uint16_t numRepeats = lParam & 0xffff;
			evsys->OnKeyInput(message == WM_KEYDOWN, wParam, (lParam >> 16) & 0xff, numRepeats);
			if (message == WM_KEYDOWN)
			{
				switch (wParam)
				{
				case VK_SPACE: evsys->OnKeyAction(UIKeyAction::ActivateDown, numRepeats); break;
				case VK_RETURN: evsys->OnKeyAction(UIKeyAction::Enter, numRepeats); break;

				case VK_BACK:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? UIKeyAction::DelPrevWord : UIKeyAction::DelPrevLetter,
						numRepeats);
					break;
				case VK_DELETE:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? UIKeyAction::DelNextWord : UIKeyAction::DelNextLetter,
						numRepeats);
					evsys->OnKeyAction(UIKeyAction::Delete, numRepeats);
					break;

				case VK_LEFT:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? UIKeyAction::PrevWord : UIKeyAction::PrevLetter,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_RIGHT:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? UIKeyAction::NextWord : UIKeyAction::NextLetter,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_UP:
					evsys->OnKeyAction(UIKeyAction::Up, numRepeats, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_DOWN:
					evsys->OnKeyAction(UIKeyAction::Down, numRepeats, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_HOME:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? UIKeyAction::GoToStart : UIKeyAction::GoToLineStart,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_END:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? UIKeyAction::GoToEnd : UIKeyAction::GoToLineEnd,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;

				case VK_PRIOR: evsys->OnKeyAction(UIKeyAction::PageUp, numRepeats); break;
				case VK_NEXT: evsys->OnKeyAction(UIKeyAction::PageDown, numRepeats); break;

				case VK_TAB: evsys->OnKeyAction(GetKeyState(VK_SHIFT) & 0x8000 ? UIKeyAction::FocusPrev : UIKeyAction::FocusNext, numRepeats); break;

				case 'X': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(UIKeyAction::Cut, numRepeats); break;
				case 'C': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(UIKeyAction::Copy, numRepeats); break;
				case 'V': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(UIKeyAction::Paste, numRepeats); break;
				case 'A': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(UIKeyAction::SelectAll, numRepeats); break;

				case VK_F11: evsys->OnKeyAction(UIKeyAction::Inspect, numRepeats); break;
				}
			}
			else
			{
				switch (wParam)
				{
				case VK_SPACE: evsys->OnKeyAction(UIKeyAction::ActivateUp, numRepeats); break;
				}
			}
		}
		return TRUE;
	case WM_CHAR:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnTextInput(wParam, lParam & 0xffff);
		return TRUE;
	case WM_ERASEBKGND:
		return FALSE;
	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			rhi::SetViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
			draw::_ResetScissorRectStack(0, 0, LOWORD(lParam), HIWORD(lParam));
			if (auto* window = GetNativeWindow(hWnd))
			{
				auto& evsys = window->GetEventSys();
				auto w = LOWORD(lParam);
				auto h = HIWORD(lParam);
				if (w != evsys.width || h != evsys.height)
				{
					ui::Notify(DCT_ResizeWindow, window->GetOwner());
				}
				evsys.width = w;
				evsys.height = h;
				window->GetOwner()->InvalidateAll();
			}
		}
		break;
	case WM_ENTERSIZEMOVE:
		if (auto* window = GetNativeWindow(hWnd))
			window->sysMoveSizeState = MSST_Unknown;
		break;
	case WM_EXITSIZEMOVE:
		if (auto* window = GetNativeWindow(hWnd))
			window->sysMoveSizeState = MSST_None;
		break;
	case WM_MOVING:
		if (auto* window = GetNativeWindow(hWnd))
			if (window->sysMoveSizeState == MSST_Unknown)
				window->sysMoveSizeState = MSST_Move;
		break;
	case WM_SIZING:
		if (auto* window = GetNativeWindow(hWnd))
		{
			//auto r = *(RECT*)lParam;
			RECT r = {};
			GetClientRect(hWnd, &r);
			auto w = r.right - r.left;
			auto h = r.bottom - r.top;
			rhi::SetViewport(0, 0, w, h);
			draw::_ResetScissorRectStack(0, 0, w, h);
			auto& evsys = window->GetEventSys();
			if (w != evsys.width || h != evsys.height)
			{
				ui::Notify(DCT_ResizeWindow, window->GetOwner());
			}
			evsys.width = w;
			evsys.height = h;
			window->GetOwner()->InvalidateAll();
			window->Redraw();
		}
		break;
	case WM_COMMAND:
		if (auto* window = GetNativeWindow(hWnd))
		{
			if (HIWORD(wParam) == 0 && window->menu)
			{
				window->menu->CallActivationFunction(LOWORD(wParam) - 1);
				return TRUE;
			}
		}
		break;
	case WM_DROPFILES:
		if (auto* window = GetNativeWindow(hWnd))
		{
			auto hdrop = (HDROP)wParam;
			POINT pt;
			if (!DragQueryPoint(hdrop, &pt))
				goto error;
			UINT numFiles = DragQueryFileW(hdrop, 0xffffffff, nullptr, 0);
			DragDropFiles* files = new DragDropFiles;
			files->paths.reserve(numFiles);
			for (UINT i = 0; i < numFiles; i++)
			{
				WCHAR path[MAX_PATH + 1];
				UINT len = DragQueryFileW(hdrop, i, path, MAX_PATH);
				path[len] = 0;
				char bfr[MAX_PATH * 3 + 1];
				int len2 = WideCharToMultiByte(CP_UTF8, 0, path, len, bfr, MAX_PATH * 3, nullptr, nullptr);
				bfr[len2] = 0;
				files->paths.push_back(bfr);
			}

			DragDrop::SetData(files);
			{
				auto* tgt = window->GetEventSys().FindObjectAtPosition(pt.x, pt.y);
				UIEvent ev(&window->GetEventSys(), tgt, UIEventType::DragDrop);
				window->GetEventSys().BubblingEvent(ev);
			}
			DragDrop::SetData(nullptr);

		error:
			DragFinish(hdrop);
			return 0;
		}
		break;
	}

	return DefWindowProcW(hWnd, message, wParam, lParam);
}


} // ui


void InitializeWin32()
{
	WNDCLASSEXW wc;

	memset(&wc, 0, sizeof(WNDCLASSEXW));
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = ui::WindowProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockBrush(BLACK_BRUSH);
	wc.lpszClassName = WINDOW_CLASS_NAME;
	RegisterClassExW(&wc);
}




size_t numAllocs = 0, numNew = 0, numDelete = 0;
void* operator new(size_t s)
{
	numAllocs++;
	numNew++;
	return malloc(s);
}
void* operator new[](size_t s)
{
	numAllocs++;
	numNew++;
	return malloc(s);
}
void operator delete(void* p)
{
	if (!p)
		return;
	numAllocs--;
	numDelete++;
	free(p);
}
void operator delete[](void* p)
{
	if (!p)
		return;
	numAllocs--;
	numDelete++;
	free(p);
}

void dumpallocinfo()
{
	printf("- allocs:%u new:%u delete:%u\n", (unsigned)numAllocs, (unsigned)numNew, (unsigned)numDelete);
}



int uimain(int argc, char* argv[]);

int RealMain()
{
#define DEFER(f) struct _defer_##__LINE__ { ~_defer_##__LINE__() { f; } } _defer_inst_##__LINE__
	DEFER(dumpallocinfo());

	int argc = 0;
	auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::string> args;
	std::vector<char*> argp;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	for (int i = 0; i < argc; i++)
	{
		args.push_back(myconv.to_bytes(argv[i]));
		argp.push_back(const_cast<char*>(args[i].c_str()));
	}
	LocalFree(argv);

	InitializeWin32();

	return uimain(argc, argp.data());
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return RealMain();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	return RealMain();
}

int main(int argc, char* argv[])
{
	return RealMain();
}
