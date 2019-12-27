
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


static bool g_appQuit = false;
static int g_appExitCode = 0;


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


namespace ui {


static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void DebugDrawSelf(UIObject* o)
{
	GL::SetTexture(0);

	float a = 0.2f;

	auto s = o->GetStyle();
	float w = o->finalRectC.x1 - o->finalRectC.x0;
	auto ml = o->ResolveUnits(s.GetMarginLeft(), w), mr = o->ResolveUnits(s.GetMarginRight(), w),
		mt = o->ResolveUnits(s.GetMarginTop(), w), mb = o->ResolveUnits(s.GetMarginBottom(), w);
	auto pl = o->ResolveUnits(s.GetPaddingLeft(), w), pr = o->ResolveUnits(s.GetPaddingRight(), w),
		pt = o->ResolveUnits(s.GetPaddingTop(), w), pb = o->ResolveUnits(s.GetPaddingBottom(), w);
	float bl = 1, br = 1, bt = 1, bb = 1;
	auto r = o->GetBorderRect();

	GL::BatchRenderer B;
	B.Begin();
	// padding
	B.SetColor(0.5f, 0.6f, 0.9f, a);
	B.Pos(r.x0, r.y0); B.Pos(r.x1, r.y0); B.Pos(r.x1 - pr, r.y0 + pt);
	B.Pos(r.x1 - pr, r.y0 + pt); B.Pos(r.x0 + pl, r.y0 + pt); B.Pos(r.x0, r.y0);
	B.Pos(r.x1, r.y0); B.Pos(r.x1, r.y1); B.Pos(r.x1 - pr, r.y1 - pb);
	B.Pos(r.x1 - pr, r.y1 - pb); B.Pos(r.x1 - pr, r.y0 + pt); B.Pos(r.x1, r.y0);
	B.Pos(r.x1, r.y1); B.Pos(r.x0, r.y1); B.Pos(r.x0 + pl, r.y1 - pb);
	B.Pos(r.x0 + pl, r.y1 - pb); B.Pos(r.x1 - pr, r.y1 - pb); B.Pos(r.x1, r.y1);
	B.Pos(r.x0, r.y1); B.Pos(r.x0, r.y0); B.Pos(r.x0 + pl, r.y0 + pt);
	B.Pos(r.x0 + pl, r.y0 + pt); B.Pos(r.x0 + pl, r.y1 - pb); B.Pos(r.x0, r.y1);
#if 0
	// border
	B.SetColor(0.5f, 0.9f, 0.6f, a);
	B.Pos(r.x0, r.y0); B.Pos(r.x1, r.y0); B.Pos(r.x1 + br, r.y0 - bt);
	B.Pos(r.x1 + br, r.y0 - bt); B.Pos(r.x0 - bl, r.y0 - bt); B.Pos(r.x0, r.y0);
	B.Pos(r.x1, r.y0); B.Pos(r.x1, r.y1); B.Pos(r.x1 + br, r.y1 + bb);
	B.Pos(r.x1 + br, r.y1 + bb); B.Pos(r.x1 + br, r.y0 - bt); B.Pos(r.x1, r.y0);
	B.Pos(r.x1, r.y1); B.Pos(r.x0, r.y1); B.Pos(r.x0 - bl, r.y1 + bb);
	B.Pos(r.x0 - bl, r.y1 + bb); B.Pos(r.x1 + br, r.y1 + bb); B.Pos(r.x1, r.y1);
	B.Pos(r.x0, r.y1); B.Pos(r.x0, r.y0); B.Pos(r.x0 - bl, r.y0 - bt);
	B.Pos(r.x0 - bl, r.y0 - bt); B.Pos(r.x0 - bl, r.y1 + bb); B.Pos(r.x0, r.y1);
#endif
	// margin
	B.SetColor(0.9f, 0.6f, 0.5f, a);
	B.Pos(r.x0, r.y0); B.Pos(r.x1, r.y0); B.Pos(r.x1 + mr, r.y0 - mt);
	B.Pos(r.x1 + mr, r.y0 - mt); B.Pos(r.x0 - ml, r.y0 - mt); B.Pos(r.x0, r.y0);
	B.Pos(r.x1, r.y0); B.Pos(r.x1, r.y1); B.Pos(r.x1 + mr, r.y1 + mb);
	B.Pos(r.x1 + mr, r.y1 + mb); B.Pos(r.x1 + mr, r.y0 - mt); B.Pos(r.x1, r.y0);
	B.Pos(r.x1, r.y1); B.Pos(r.x0, r.y1); B.Pos(r.x0 - ml, r.y1 + mb);
	B.Pos(r.x0 - ml, r.y1 + mb); B.Pos(r.x1 + mr, r.y1 + mb); B.Pos(r.x1, r.y1);
	B.Pos(r.x0, r.y1); B.Pos(r.x0, r.y0); B.Pos(r.x0 - ml, r.y0 - mt);
	B.Pos(r.x0 - ml, r.y0 - mt); B.Pos(r.x0 - ml, r.y1 + mb); B.Pos(r.x0, r.y1);
	B.End();
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
	void OnKeyAction(UIKeyAction act, uint16_t numRepeats)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnKeyAction(act, numRepeats);
	}
	void OnTextInput(uint32_t ch, uint16_t numRepeats)
	{
		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnTextInput(ch, numRepeats);
	}

	Target mainTarget;
};

struct NativeWindow_Impl
{
	void Init(NativeWindowBase* owner)
	{
		system.nativeWindow = owner;

		window = CreateWindowExW(0, L"UIWindow", L"UI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 400, NULL, NULL, GetModuleHandle(nullptr), this);
		renderCtx = GL::CreateRenderContext(window);

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
	}
	~NativeWindow_Impl()
	{
		GL::FreeRenderContext(renderCtx);
		DestroyWindow(window);
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

		GL::SetActiveContext(renderCtx);

		auto& cont = GetContainer();
		auto& evsys = GetEventSys();

		evsys.ProcessTimers(float(t - prevTime));
		prevTime = t;

		cont.ProcessNodeRenderStack();
		evsys.RecomputeLayout();
		evsys.OnMouseMove(evsys.prevMouseX, evsys.prevMouseY);

		//GL::Clear(20, 40, 80, 255);
		GL::Clear(0x25, 0x25, 0x25, 255);
		if (cont.rootNode)
			cont.rootNode->OnPaint();
		//GL::SetTexture(g_themeTexture);
		//GL::BatchRenderer br;
		//br.Begin();
		//br.Quad(20, 120, 256+20, 256+120, 0, 0, 1, 1);
		//br.End();
		//DrawThemeElement(TE_ButtonNormal, 300, 20, 380, 40);
		//DrawThemeElement(TE_ButtonPressed, 300, 40, 380, 60);
		//DrawThemeElement(TE_ButtonHover, 300, 60, 380, 80);
		//DrawTextLine(32, 32, "Test text", 1, 1, 1);
		if (true)
		{
			// debug draw
			if (cont.rootNode)
				;// DebugDraw(cont.rootNode);
		}

		GL::Present(renderCtx);
	}

	void UpdateVisibilityState()
	{
		if (!visible)
			ExitExclusiveMode();
		ShowWindow(window, visible ? SW_SHOW : SW_HIDE);
	}

	void UpdateStyle()
	{
		int ws = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_DLGFRAME;
		if (style & WS_Resizable)
			ws |= WS_THICKFRAME;
		if (style & WS_TitleBar)
			ws |= WS_DLGFRAME;
		ws = WS_THICKFRAME;
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
	GL::RenderContext* renderCtx;

	Menu* menu;

	double prevTime;
	bool visible = true;
	bool exclusiveMode = false;
	bool innerUIEnabled = true;
	bool firstShow = true;
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

Point<int> NativeWindowBase::GetSize()
{
	RECT r = {};
	GetWindowRect(_impl->window, &r);
	return { r.right - r.left, r.bottom - r.top };
}

void NativeWindowBase::SetSize(int x, int y)
{
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


void NativeWindowNode::OnLayout(const UIRect& rect)
{
	finalRectC = finalRectCP = finalRectCPB = {};
}


extern void SubscriptionTable_Init();
extern void SubscriptionTable_Free();

Application::Application(int argc, char* argv[])
{
	SubscriptionTable_Init();
}

Application::~Application()
{
	//system.container.Free();
	SubscriptionTable_Free();
}

void Application::Quit(int code)
{
	g_appQuit = true;
	g_appExitCode = code;
}

int Application::Run()
{
	MSG msg;
	while (!g_appQuit && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		if (msg.hwnd)
		{
			if (auto* window = GetNativeWindow(msg.hwnd))
				window->Redraw();
		}
		//UI_Redraw(&system);
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
		return TRUE;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
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
				case VK_SPACE: evsys->OnKeyAction(UIKeyAction::Activate, numRepeats); break;
				case VK_RETURN: evsys->OnKeyAction(UIKeyAction::Enter, numRepeats); break;
				case VK_BACK: evsys->OnKeyAction(UIKeyAction::Backspace, numRepeats); break;
				case VK_DELETE: evsys->OnKeyAction(UIKeyAction::Delete, numRepeats); break;

				case VK_LEFT: evsys->OnKeyAction(UIKeyAction::Left, numRepeats); break;
				case VK_RIGHT: evsys->OnKeyAction(UIKeyAction::Right, numRepeats); break;
				case VK_UP: evsys->OnKeyAction(UIKeyAction::Up, numRepeats); break;
				case VK_DOWN: evsys->OnKeyAction(UIKeyAction::Down, numRepeats); break;
				case VK_HOME: evsys->OnKeyAction(UIKeyAction::Home, numRepeats); break;
				case VK_END: evsys->OnKeyAction(UIKeyAction::End, numRepeats); break;
				case VK_PRIOR: evsys->OnKeyAction(UIKeyAction::PageUp, numRepeats); break;
				case VK_NEXT: evsys->OnKeyAction(UIKeyAction::PageDown, numRepeats); break;
				case VK_TAB: evsys->OnKeyAction(GetKeyState(VK_SHIFT) ? UIKeyAction::FocusPrev : UIKeyAction::FocusNext, numRepeats); break;

				case 'X': if (GetKeyState(VK_CONTROL)) evsys->OnKeyAction(UIKeyAction::Cut, numRepeats); break;
				case 'C': if (GetKeyState(VK_CONTROL)) evsys->OnKeyAction(UIKeyAction::Copy, numRepeats); break;
				case 'V': if (GetKeyState(VK_CONTROL)) evsys->OnKeyAction(UIKeyAction::Paste, numRepeats); break;
				case 'A': if (GetKeyState(VK_CONTROL)) evsys->OnKeyAction(UIKeyAction::SelectAll, numRepeats); break;
				}
			}
		}
		return TRUE;
	case WM_CHAR:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnTextInput(wParam, lParam & 0xffff);
		return TRUE;
	case WM_ERASEBKGND:
		break;// return FALSE;
	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			GL::SetViewport(0, 0, LOWORD(lParam), HIWORD(lParam));
			if (auto* window = GetNativeWindow(hWnd))
			{
				auto& evsys = window->GetEventSys();
				evsys.width = LOWORD(lParam);
				evsys.height = HIWORD(lParam);
				window->Redraw();
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
	numAllocs--;
	numDelete++;
	free(p);
}
void operator delete[](void* p)
{
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
