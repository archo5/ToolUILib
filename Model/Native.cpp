
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>

#include <codecvt>

#include "Native.h"
#include "System.h"
#include "Menu.h"

#include "../Render/OpenGL.h"
#include "../Render/Theme.h"


namespace ui {


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

struct NativeWindow_Impl
{
	void Init(NativeWindowBase* owner)
	{
		_sys.nativeWindow = owner;

		_window = CreateWindowW(L"UIWindow", L"UI", WS_OVERLAPPEDWINDOW, 200, 200, 500, 400, NULL, NULL, GetModuleHandle(nullptr), this);
		_rctx = GL::CreateRenderContext(_window);
		ShowWindow(_window, SW_SHOW /* TODO nCmdShow */);

		auto& evsys = GetEventSys();

		RECT clientRect;
		GetClientRect(_window, &clientRect);
		evsys.width = float(clientRect.right);
		evsys.height = float(clientRect.bottom);

		// TODO fix
		static bool init = false;
		if (!init)
		{
			init = true;
			InitFont();
			InitTheme();
		}
	}
	~NativeWindow_Impl()
	{
		GL::FreeRenderContext(_rctx);
		DestroyWindow(_window);
	}

	void SetRenderFunc(std::function<void(UIContainer*)> renderFunc)
	{
		auto& cont = GetContainer();
		auto& evsys = GetEventSys();

		struct Builder : UINode
		{
			std::function<void(UIContainer*)> renderFunc;
			void Render(UIContainer* ctx) override
			{
				renderFunc(ctx);
			}
		};
		auto* N = cont.AllocIfDifferent<Builder>(cont.rootNode);
		N->renderFunc = renderFunc;
		cont._BuildUsing(N);
		evsys.RecomputeLayout();
	}

	void Redraw()
	{
		double t = hqtime();
		static double prevTime = t;

		GL::SetActiveContext(_rctx);

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
		GL::SetTexture(g_themeTexture);
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
				DebugDraw(cont.rootNode);
		}

		GL::Present(_rctx);
	}

	UIEventSystem& GetEventSys() { return _sys.eventSystem; }
	UIContainer& GetContainer() { return _sys.container; }

	ui::System _sys;

	HWND _window;
	GL::RenderContext* _rctx;

	Menu* _menu;
};

NativeWindowBase::NativeWindowBase()
{
	_impl = new NativeWindow_Impl();
	_impl->Init(this);
}

NativeWindowBase::NativeWindowBase(std::function<void(UIContainer*)> renderFunc)
{
	_impl = new NativeWindow_Impl();
	_impl->Init(this);
	_impl->SetRenderFunc(renderFunc);
}

NativeWindowBase::~NativeWindowBase()
{
	delete _impl;
}

void NativeWindowBase::SetRenderFunc(std::function<void(UIContainer*)> renderFunc)
{
	_impl->SetRenderFunc(renderFunc);
}

void NativeWindowBase::SetTitle(const char* title)
{
	SetWindowTextA(_impl->_window, title);
}

Menu* NativeWindowBase::GetMenu()
{
	return _impl->_menu;
}

void NativeWindowBase::SetMenu(Menu* m)
{
	_impl->_menu = m;
	::SetMenu(_impl->_window, m ? (HMENU)m->GetNativeHandle() : nullptr);
}

void* NativeWindowBase::GetNativeHandle() const
{
	return (void*)_impl->_window;
}


void NativeWindowNode::OnLayout(const UIRect& rect)
{
	finalRectC = finalRectCP = finalRectCPB = {};
}


static NativeWindow_Impl* GetNativeWindow(HWND hWnd)
{
	return reinterpret_cast<NativeWindow_Impl*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}


Application::Application(int argc, char* argv[])
{
}

Application::~Application()
{
	//system.container.Free();
}

int Application::Run()
{
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.hwnd)
		{
			if (auto* window = GetNativeWindow(msg.hwnd))
				window->Redraw();
		}
		//UI_Redraw(&system);
	}
	return msg.wParam;
}

static UIEventSystem* GetEventSys(HWND hWnd)
{
	if (auto* w = GetNativeWindow(hWnd))
		return &w->GetEventSys();
	return nullptr;
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
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
		SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnMouseMove(UIMouseCoord(GET_X_LPARAM(lParam)), UIMouseCoord(GET_Y_LPARAM(lParam)));
		break;
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
		break;
	case WM_CHAR:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnTextInput(wParam, lParam & 0xffff);
		break;
	case WM_ERASEBKGND:
		return FALSE;
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
	wc.hbrBackground = nullptr;// GetSysColorBrush(COLOR_3DFACE);
	wc.lpszClassName = L"UIWindow";
	RegisterClassExW(&wc);
}


int uimain(int argc, char* argv[]);

int RealMain()
{
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
