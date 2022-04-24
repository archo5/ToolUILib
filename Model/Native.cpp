
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ShlObj.h>
#include <hidusage.h>

#include <algorithm>
#undef min
#undef max

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Comdlg32.lib")

#include "Native.h"
#include "System.h"
#include "Menu.h"
#include "Theme.h"

#include "../Render/RHI.h"
#include "../Render/Render.h"
#include "../Core/WindowsUtils.h"
#include "../Core/FileSystem.h"


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


static HGLOBAL UTF8ToWCHARGlobal(ui::StringView str)
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
	return UTF8ToWCHARGlobal(ui::StringView(str, strlen(str)));
}


namespace ui {


static constexpr const unsigned g_virtualModKeys[] =
{
	VK_LCONTROL,
	VK_LSHIFT,
	VK_LMENU,
	VK_LWIN,
	VK_RCONTROL,
	VK_RSHIFT,
	VK_RMENU,
	VK_RWIN,
};

static uint8_t GetModifierKeys()
{
	uint8_t ret = 0;
	for (int i = 0; i < sizeof(g_virtualModKeys) / sizeof(g_virtualModKeys[0]); i++)
		if (GetKeyState(g_virtualModKeys[i]) & 0x8000)
			ret |= 1 << i;
	return ret;
}

namespace platform {

uint32_t GetTimeMs()
{
	//return ::GetTickCount();
	return timeGetTime();
}

uint32_t GetDoubleClickTime()
{
	return ::GetDoubleClickTime();
}

Point2i GetCursorScreenPos()
{
	POINT p;
	if (::GetCursorPos(&p))
		return { p.x, p.y };
	return { -1, -1 };
}

static bool g_requestedRawMouseInput = false;
void RequestRawMouseInput()
{
	if (g_requestedRawMouseInput)
		return;
	RAWINPUTDEVICE rid = {};
	{
		rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
		rid.usUsage = HID_USAGE_GENERIC_MOUSE;
	}
	if (::RegisterRawInputDevices(&rid, 1, sizeof(rid)))
		g_requestedRawMouseInput = true;
}

Color4b GetColorAtScreenPos(Point2i pos)
{
	Color4b col(0);
	if (auto dc = ::GetDC(nullptr))
	{
		auto cr = ::GetPixel(dc, pos.x, pos.y);
		if (cr != CLR_INVALID)
		{
			col = Color4b(GetRValue(cr), GetGValue(cr), GetBValue(cr));
		}
		::ReleaseDC(nullptr, dc);
	}
	return col;
}

void ShowErrorMessage(StringView title, StringView text)
{
	::MessageBoxW(nullptr, UTF8toWCHAR(text).c_str(), UTF8toWCHAR(title).c_str(), MB_ICONERROR);
}

void BrowseToFile(StringView path)
{
	COMInit cominit;
	if (cominit.success)
	{
		auto absPath = PathGetAbsolute(path);
		for (auto& c : absPath)
			if (c == '/')
				c = '\\';
		if (auto* idl = ::ILCreateFromPathW(UTF8toWCHAR(absPath).c_str()))
		{
			SHOpenFolderAndSelectItems(idl, 0, nullptr, 0);

			::ILFree(idl);
		}
	}
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
			ret = WCHARtoUTF8(mem);
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


bool FileSelectionWindow::Show(bool save)
{
	OPENFILENAMEW ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);

	ofn.Flags |= OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	if (!save)
		ofn.Flags |= OFN_FILEMUSTEXIST;
	if (flags & MultiSelect)
		ofn.Flags |= OFN_ALLOWMULTISELECT;
	if (flags & CreatePrompt)
		ofn.Flags |= OFN_CREATEPROMPT;

	std::wstring filtersW;
	if (!filters.empty())
	{
		std::string fltmerge;
		for (const auto& F : filters)
		{
			fltmerge += F.name;
			fltmerge.push_back(0);
			fltmerge += F.exts;
			fltmerge.push_back(0);
		}
		filtersW = UTF8toWCHAR(fltmerge);
		ofn.lpstrFilter = filtersW.c_str();
	}

	std::wstring defExtW;
	if (!defaultExt.empty())
	{
		defExtW = UTF8toWCHAR(defaultExt);
		ofn.lpstrDefExt = defExtW.c_str();
	}

	std::wstring titleW;
	if (!title.empty())
	{
		titleW = UTF8toWCHAR(title);
		ofn.lpstrTitle = titleW.c_str();
	}

	std::wstring curDirW;
	if (!currentDir.empty())
	{
		curDirW = UTF8toWCHAR(currentDir);
		ofn.lpstrInitialDir = curDirW.c_str();
	}

	std::wstring fileBufW;
	for (auto& f : selectedFiles)
	{
		if (!currentDir.empty())
		{
			fileBufW.append(UTF8toWCHAR(currentDir));
			for (auto& c : fileBufW)
				if (c == '/')
					c = '\\';
			fileBufW.push_back('\\');
		}
		fileBufW.append(UTF8toWCHAR(f));
		fileBufW.push_back(0);
		break;
	}
	fileBufW.resize(maxFileNameBuffer, 0);
	fileBufW.back() = 0;
	ofn.lpstrFile = &fileBufW[0];
	ofn.nMaxFile = maxFileNameBuffer;

	auto cwd = GetWorkingDirectory();
	
	BOOL ret = save
		? GetSaveFileNameW(&ofn)
		: GetOpenFileNameW(&ofn);

	if (ret)
	{
		currentDir = WCHARtoUTF8(fileBufW.c_str(), ofn.nFileOffset - 1);
		NormalizePath(currentDir);
		selectedFiles.clear();
		for (size_t i = ofn.nFileOffset; i < fileBufW.size() && fileBufW[i]; i += wcslen(fileBufW.c_str() + i) + 1)
		{
			selectedFiles.push_back(WCHARtoUTF8(fileBufW.c_str() + i));
			NormalizePath(selectedFiles.back());
		}
	}

	SetWorkingDirectory(cwd);

	return ret != 0;
}


DataCategoryTag DCT_ResizeWindow[1];


static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


void DebugDrawSelf(UIObject* o)
{
	float a = 0.2f;

	auto r = o->GetPaddingRect();

	// padding
	draw::RectCutoutCol(r, o->GetPaddingRect(), Color4f(0.5f, 0.6f, 0.9f, a));
	// border
#if 1
	draw::RectCutoutCol(r, r.ExtendBy(UIRect::UniformBorder(-1)), Color4f(0.5f, 0.9f, 0.6f, a));
#endif
}

void DebugDraw(UIObject* o)
{
	DebugDrawSelf(o);

	UIObjectIterator it(o);
	while (auto* ch = it.GetNext())
		DebugDraw(ch);
}

double hqtime()
{
	LARGE_INTEGER freq, cnt;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cnt);
	return double(cnt.QuadPart) / double(freq.QuadPart);
}

extern FrameContents* g_curSystem;

struct ProxyEventSystem
{
	struct Target
	{
		EventSystem* target;
		UIRect window;
	};

	void OnMouseMove(Point2f cursorPos, uint8_t mod)
	{
		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, mainTarget.target->container->owner);

		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnMouseMove(cursorPos - mainTarget.window.GetMin(), mod);
	}
	void OnMouseButton(bool down, MouseButton which, Point2f cursorPos, uint8_t mod)
	{
		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, mainTarget.target->container->owner);

		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnMouseButton(down, which, cursorPos - mainTarget.window.GetMin(), mod);
	}
	void OnMouseScroll(Vec2f delta)
	{
		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, mainTarget.target->container->owner);

		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnMouseScroll(delta);
	}
	void OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint8_t mod, bool isRepeated, uint16_t numRepeats)
	{
		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, mainTarget.target->container->owner);

		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnKeyInput(down, vk, pk, mod, isRepeated, numRepeats);
	}
	void OnKeyAction(KeyAction act, uint8_t mod, uint16_t numRepeats, bool modifier = false)
	{
		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, mainTarget.target->container->owner);

		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnKeyAction(act, mod, numRepeats, modifier);
	}
	void OnTextInput(uint32_t ch, uint8_t mod, uint16_t numRepeats)
	{
		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, mainTarget.target->container->owner);

		if (!mainTarget.target->GetNativeWindow()->IsInnerUIEnabled())
			return;
		mainTarget.target->OnTextInput(ch, mod, numRepeats);
	}

	Target mainTarget;
};

struct NativeWindow_Impl;
static std::vector<NativeWindow_Impl*>* g_windowRepaintList = nullptr;
static std::vector<NativeWindow_Impl*>* g_curWindowRepaintList = nullptr;
static int g_rsrcUsers = 0;

static StaticID_Color sid_color_clear("clear");

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

		if (!g_rsrcUsers)
		{
			draw::internals::InitResources();
			if (!GetCurrentTheme())
				SetCurrentTheme(LoadTheme("theme_default"));
		}
		g_rsrcUsers++;

		prevTime = hqtime();

		owner->InvalidateAll();
		cursor = g_defaultCursors[(int)DefaultCursor::Default];
	}
	~NativeWindow_Impl()
	{
		if (--g_rsrcUsers == 0)
		{
			SetCurrentTheme(nullptr);
			draw::internals::FreeResources();
		}

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

	void SetBuildFunc(std::function<void()> buildFunc)
	{
		auto* cb = CreateUIObject<BuildCallback>();
		cb->TEMP_LAYOUT_MODE = FILLER;
		cb->buildFunc = buildFunc;
		system.BuildRoot(cb);
		system.eventSystem.RecomputeLayout();
		GetOwner()->InvalidateAll();
	}

	void Redraw(bool canRebuild)
	{
		if (!innerUIEnabled)
			return;

		double t = hqtime();

		rhi::BeginFrame(renderCtx);

		auto& cont = GetContainer();
		auto& evsys = GetEventSys();

		TmpEdit<decltype(g_curSystem)> tmp(g_curSystem, cont.owner);

		float minTime = evsys.ProcessTimers(float(t - prevTime));
		if (minTime < FLT_MAX)
			SetTimer(window, 1, minTime * 1000, nullptr);
		else
			KillTimer(window, 1);
		prevTime = t;

		if (canRebuild)
			cont.ProcessBuildStack();
		cont.ProcessLayoutStack();
		evsys.OnMouseMove(evsys.prevMousePos, GetModifierKeys());
		if (canRebuild)
			cont.ProcessBuildStack();
		cont.ProcessLayoutStack();

#if DRAW_STATS
		double t0 = hqtime();
		auto stats0 = rhi::Stats::Get();
#endif

		rhi::SetActiveContext(renderCtx);
		rhi::SetViewport(0, 0, evsys.width, evsys.height);
		draw::_ResetScissorRectStack(0, 0, evsys.width, evsys.height);
		draw::internals::OnBeginDrawFrame();

		auto clearColor = GetCurrentTheme()->GetBackgroundColor(sid_color_clear);
		rhi::Clear(clearColor.r, clearColor.g, clearColor.b, 255);
		if (cont.rootBuildable)
			cont.rootBuildable->RootPaint();

		system.overlays.UpdateSorted();
		for (auto& ovr : system.overlays.sorted)
			ovr.obj->RootPaint();

		if (debugDrawEnabled)
		{
			// debug draw
			if (cont.rootBuildable)
				DebugDraw(cont.rootBuildable);
		}

#if 0
		for (auto* o = evsys.hoverObj; o; o = o->parent)
		{
			auto r = o->GetContentRect();
			auto r2 = r.ExtendBy(UIRect::UniformBorder(1));
			draw::RectCutoutCol(r2, r, Color4f(0.9f, 0.1f, 0.1f, 0.5f));
		}
		for (auto* o = evsys.dragHoverObj; o; o = o->parent)
		{
			auto r = o->GetContentRect();
			auto r2 = r.ExtendBy(UIRect::UniformBorder(1));
			draw::RectCutoutCol(r2, r, Color4f(0.9f, 0.9f, 0.1f, 0.5f));
		}
#endif

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

		rhi::EndFrame(renderCtx);
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

	EventSystem& GetEventSys() { return system.eventSystem; }
	ProxyEventSystem& GetProxyEventSys() { return proxyEventSystem; }
	UIContainer& GetContainer() { return system.container; }
	NativeWindowBase* GetOwner() { return system.nativeWindow; }

	FrameContents system;
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
NativeWindowBase::NativeWindowBase(std::function<void(UIContainer*)> buildFunc)
{
	_impl = new NativeWindow_Impl();
	_impl->Init(this);
	_impl->SetBuildFunc(buildFunc);
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
void NativeWindowBase::SetBuildFunc(std::function<void(UIContainer*)> buildFunc)
{
	_impl->SetBuildFunc(buildFunc);
}
#endif

std::string NativeWindowBase::GetTitle()
{
	std::wstring titleW;
	titleW.resize(GetWindowTextLengthW(_impl->window));
	GetWindowTextW(_impl->window, &titleW[0], titleW.size());
	return WCHARtoUTF8s(titleW);
}

void NativeWindowBase::SetTitle(StringView title)
{
	SetWindowTextW(_impl->window, UTF8toWCHAR(title).c_str());
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

		_impl->SetBuildFunc([this]() { OnBuild(); });
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

Point2i NativeWindowBase::GetPosition()
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
				window->Redraw(window == _impl);
		}
	}

	_impl->ExitExclusiveMode();
}

Size2i NativeWindowBase::GetSize()
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

void NativeWindowBase::Rebuild()
{
	// don't rebuild if the first build hasn't happened yet
	// TODO rebuild always and clear the flag?
	if (!_impl->firstShow && _impl->GetContainer().rootBuildable)
		_impl->GetContainer().rootBuildable->Rebuild();
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


void NativeWindowBuildFunc::OnBuild()
{
	if (_buildFunc)
		_buildFunc();
}

void NativeWindowBuildFunc::SetBuildFunc(std::function<void()> buildFunc)
{
	_buildFunc = buildFunc;
}


void NativeMainWindow::OnClose()
{
	Application::Quit();
}


void NativeWindowNode::OnLayout(const UIRect& rect)
{
	finalRectC = finalRectCP = {};
}


static HashMap<AnimationRequester*, bool> g_animRequesters;
static UINT_PTR g_animReqTimerID;
static bool g_inAnimTimerProc;

static void CALLBACK AnimTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
	if (g_inAnimTimerProc)
		return;
	g_inAnimTimerProc = true;
	for (auto& kvp : g_animRequesters)
	{
		kvp.key->OnAnimationFrame();
	}
	g_inAnimTimerProc = false;
}

void AnimationRequester::BeginAnimation()
{
	if (_animating)
		return;
	if (g_animRequesters.size() == 0)
	{
		g_animReqTimerID = ::SetTimer(nullptr, 0, 1, AnimTimerProc);
		timeBeginPeriod(1);
	}
	g_animRequesters.insert(this, true);
	_animating = true;
}

void AnimationRequester::EndAnimation()
{
	if (!_animating)
		return;
	g_animRequesters.erase(this);
	if (g_animRequesters.size() == 0)
	{
		::KillTimer(nullptr, g_animReqTimerID);
		timeEndPeriod(1);
	}
	_animating = false;
}


static IntrusiveLinkedList<RawMouseInputRequester> g_rawMouseInputRequesters;

void RawMouseInputRequester::BeginRawMouseInput()
{
	if (!_requesting)
	{
		platform::RequestRawMouseInput();
		g_rawMouseInputRequesters.AddToEnd(this);
		_requesting = true;
	}
}

void RawMouseInputRequester::EndRawMouseInput()
{
	if (_requesting)
	{
		_requesting = false;
		g_rawMouseInputRequesters.Remove(this);
	}
}


struct Inspector : NativeDialogWindow
{
	Inspector()
	{
		SetTitle("Inspector");
		SetSize(1024, 768);
	}

	struct InspectorUI : Buildable
	{
		static constexpr int TAB_W = 10;
		float scrollPos = 0;
		float scrollMax = FLT_MAX;
		FontSettings mainFont;

		Inspector* insp;

		void Build() override {}

		static StringView CleanName(StringView name)
		{
			if (name.starts_with("struct ")) name = name.substr(7);
			if (name.starts_with("class ")) name = name.substr(6);
			if (name.starts_with("ui::")) name = name.substr(4);
			if (name.starts_with("layouts::")) name = name.substr(9);

			if (name.ends_with("Layout")) name = name.substr(0, name.size() - 6);

			return name;
		}

		void PaintObject(UIObject* obj, int x, int& y)
		{
			auto* font = mainFont.GetFont();
			int fontSize = mainFont.size;

			float ys = y - scrollPos;
			if (obj == insp->selObj)
			{
				draw::RectCol(0, ys, 9999, ys + fontSize, Color4f(0.5f, 0, 0));
			}

			y += fontSize - 1;
			ys = y - scrollPos;
			if (ys >= fontSize * 2 - 1)
			{
				draw::TextLine(font, fontSize, x, ys, Format("%p", obj), Color4b::White());
				draw::TextLine(font, fontSize, x + 60, ys, CleanName(typeid(*obj).name()), Color4b::White());
				char bfr[1024];
				{
					auto& fr = obj->finalRectC;
					snprintf(bfr, 1024, "%g;%g - %g;%g", fr.x0, fr.y0, fr.x1, fr.y1);
					draw::TextLine(font, fontSize, 400, ys, bfr, Color4b::White());
				}
				{
					auto& fr = obj->finalRectCP;
					snprintf(bfr, 1024, "%g;%g - %g;%g", fr.x0, fr.y0, fr.x1, fr.y1);
					draw::TextLine(font, fontSize, 600, ys, bfr, Color4b::White());
				}
			}

			y++;

			UIObjectIterator it(obj);
			while (UIObject* ch = it.GetNext())
			{
				PaintObject(ch, x + TAB_W, y);
			}
		}

		Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
		{
			return Rangef::Exact(containerSize.x);
		}
		Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
		{
			return Rangef::Exact(containerSize.y);
		}
		void OnLayout(const ui::UIRect& rect) override
		{
			finalRectC = finalRectCP = rect;
		}

		void OnPaint(const UIPaintContext& ctx) override
		{
			auto* font = mainFont.GetFont();

			auto& c = insp->selWindow->_impl->GetContainer();
			int y = mainFont.size;
			draw::TextLine(font, mainFont.size, 0, y, "Address / Name", Color4b(255, 153));
			draw::TextLine(font, mainFont.size, 400, y, "Content rect", Color4b(255, 153));
			draw::TextLine(font, mainFont.size, 600, y, "Padding rect", Color4b(255, 153));
			PaintObject(c.rootBuildable, 0, y);
			scrollMax = y;
			//draw::TextLine(font, mainFont.size, 10, 10, "inspector", Color4b::White());
		}

		void OnEvent(Event& e) override
		{
			if (e.type == EventType::MouseScroll)
			{
				scrollPos = clamp(scrollPos - e.delta.y, 0.0f, scrollMax);
				system->nativeWindow->InvalidateAll();
			}
		}
	};

	void OnBuild() override
	{
		ui = &Make<InspectorUI>();
		ui->insp = this;
	}

	void Select(NativeWindowBase* window, UIObject* obj)
	{
		selWindow = window;
		selObj = obj;
		if (ui)
			ui->Rebuild();
	}

	InspectorUI* ui = nullptr;

	NativeWindowBase* selWindow = nullptr;
	UIObject* selObj = nullptr;
};


BufferHandle LoadEmbeddedData()
{
	HRSRC hrsrc = FindResource(GetModuleHandle(nullptr), MAKEINTRESOURCE(50), RT_RCDATA);
	if (!hrsrc)
		return {};
	HGLOBAL hglobal = LoadResource(nullptr, hrsrc);
	if (!hglobal)
		return {};
	char* data = (char*)LockResource(hglobal);
	size_t size = SizeofResource(nullptr, hrsrc);

	auto mbh = AsRCHandle(new BasicMemoryBuffer);
	mbh->data = data;
	mbh->size = size;
	return mbh;
}


extern void SubscriptionTable_Init();
extern void SubscriptionTable_Free();

static EventQueue* g_mainEventQueue;
static DWORD g_mainThreadID;

Application* Application::_instance;

Application::Application(int argc, char* argv[])
{
	assert(_instance == nullptr);
	_instance = this;

	if (FSGetDefault()->fileSystems.empty())
	{
		FSGetDefault()->fileSystems.push_back(CreateFileSystemSource(PathJoin(PathGetParent(PathFromSystem(argv[0])), "data")));
		// TODO pipeline for avoiding this?
		FSGetDefault()->fileSystems.push_back(CreateFileSystemSource("data"));
	}

	if (auto embData = LoadEmbeddedData())
		FSGetDefault()->fileSystems.push_back(CreateZipFileMemorySource(embData));

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
			win->Redraw(true);
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
	case WM_SETFOCUS:
		if (auto* win = GetNativeWindow(hWnd))
		{
			win->GetOwner()->OnFocusReceived();
			return TRUE;
		}
		break;
	case WM_KILLFOCUS:
		if (auto* win = GetNativeWindow(hWnd))
		{
			win->GetOwner()->OnFocusLost();
			return TRUE;
		}
		break;
	case WM_INPUT:
		if (g_rawMouseInputRequesters.HasAny())
		{
			RAWINPUT ri = {};
			UINT size = sizeof(ri);
			UINT ret = ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &ri, &size, sizeof(ri.header));
			if (ret != size)
				return 0;
			if (ri.header.dwType == RIM_TYPEMOUSE)
			{
				int dx = ri.data.mouse.lLastX;
				int dy = ri.data.mouse.lLastY;

				if ((ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE)
				{
					static int prevAbsX = -1;
					static int prevAbsY = -1;

					bool isVirtualDesktop = (ri.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;

					int width = GetSystemMetrics(isVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
					int height = GetSystemMetrics(isVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

					int absoluteX = int((ri.data.mouse.lLastX / 65535.0f) * width);
					int absoluteY = int((ri.data.mouse.lLastY / 65535.0f) * height);

					if (prevAbsX >= 0 && prevAbsY >= 0)
					{
						dx = absoluteX - prevAbsX;
						dy = absoluteY - prevAbsY;
					}

					prevAbsX = absoluteX;
					prevAbsY = absoluteY;
				}
				//printf("dx=%d dy=%d flags=%04X\n", dx, dy, ri.data.mouse.usFlags);

				if (dx || dy)
				{
					for (auto* n = g_rawMouseInputRequesters._first; n; n = n->_next)
					{
						n->OnRawInputEvent(dx, dy);
					}
				}
			}
			return 0;
		}
		break;
	case WM_MOUSEMOVE:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnMouseMove({ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) }, GetModifierKeys());
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
			evsys->OnMouseMove({ -1, -1 }, GetModifierKeys());
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
			evsys->OnMouseMove({ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) }, GetModifierKeys());
			evsys->OnMouseButton(
				message == WM_LBUTTONDOWN,
				MouseButton::Left,
				{ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) },
				GetModifierKeys());
		}
		return TRUE;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove({ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) }, GetModifierKeys());
			evsys->OnMouseButton(
				message == WM_RBUTTONDOWN,
				MouseButton::Right,
				{ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) },
				GetModifierKeys());
		}
		return TRUE;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove({ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) }, GetModifierKeys());
			evsys->OnMouseButton(
				message == WM_MBUTTONDOWN,
				MouseButton::Middle,
				{ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) },
				GetModifierKeys());
		}
		return TRUE;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			AdjustMouseCapture(hWnd, wParam);
			evsys->OnMouseMove({ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) }, GetModifierKeys());
			evsys->OnMouseButton(
				message == WM_XBUTTONDOWN,
				HIWORD(wParam) == XBUTTON1 ? MouseButton::X1 : MouseButton::X2,
				{ float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)) },
				GetModifierKeys());
		}
		return TRUE;
	case WM_MOUSEWHEEL:
		if (auto* evsys = GetEventSys(hWnd))
		{
			evsys->OnMouseScroll({ 0, float(GET_WHEEL_DELTA_WPARAM(wParam)) });
		}
		return TRUE;
	case WM_MOUSEHWHEEL:
		if (auto* evsys = GetEventSys(hWnd))
		{
			evsys->OnMouseScroll({ float(GET_WHEEL_DELTA_WPARAM(wParam)), 0 });
		}
		return TRUE;
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (auto* evsys = GetEventSys(hWnd))
		{
			uint16_t numRepeats = lParam & 0xffff;
			auto M = GetModifierKeys();
			uint8_t origScanCode = (lParam >> 16) & 0xff;
			uint8_t extMask = lParam & (1 << 24) ? 0x80 : 0;
			uint8_t scanCode = origScanCode | extMask;
			evsys->OnKeyInput(message == WM_KEYDOWN, wParam, scanCode, M, (lParam & (1U << 31)) != 0U, numRepeats);
			if (message == WM_KEYDOWN)
			{
				switch (wParam)
				{
				case VK_SPACE: evsys->OnKeyAction(KeyAction::ActivateDown, M, numRepeats); break;
				case VK_RETURN: evsys->OnKeyAction(KeyAction::Enter, M, numRepeats); break;

				case VK_BACK:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? KeyAction::DelPrevWord : KeyAction::DelPrevLetter,
						M,
						numRepeats);
					break;
				case VK_DELETE:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? KeyAction::DelNextWord : KeyAction::DelNextLetter,
						M,
						numRepeats);
					evsys->OnKeyAction(KeyAction::Delete, M, numRepeats);
					break;

				case VK_LEFT:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? KeyAction::PrevWord : KeyAction::PrevLetter,
						M,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_RIGHT:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? KeyAction::NextWord : KeyAction::NextLetter,
						M,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_UP:
					evsys->OnKeyAction(KeyAction::Up, M, numRepeats, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_DOWN:
					evsys->OnKeyAction(KeyAction::Down, M, numRepeats, (GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_HOME:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? KeyAction::GoToStart : KeyAction::GoToLineStart,
						M,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;
				case VK_END:
					evsys->OnKeyAction(
						GetKeyState(VK_CONTROL) & 0x8000 ? KeyAction::GoToEnd : KeyAction::GoToLineEnd,
						M,
						numRepeats,
						(GetKeyState(VK_SHIFT) & 0x8000) != 0);
					break;

				case VK_PRIOR: evsys->OnKeyAction(KeyAction::PageUp, M, numRepeats); break;
				case VK_NEXT: evsys->OnKeyAction(KeyAction::PageDown, M, numRepeats); break;

				case VK_TAB: evsys->OnKeyAction(GetKeyState(VK_SHIFT) & 0x8000 ? KeyAction::FocusPrev : KeyAction::FocusNext, M, numRepeats); break;

				case 'X': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(KeyAction::Cut, M, numRepeats); break;
				case 'C': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(KeyAction::Copy, M, numRepeats); break;
				case 'V': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(KeyAction::Paste, M, numRepeats); break;
				case 'A': if (GetKeyState(VK_CONTROL) & 0x8000) evsys->OnKeyAction(KeyAction::SelectAll, M, numRepeats); break;

				case VK_F11: evsys->OnKeyAction(KeyAction::Inspect, M, numRepeats); break;
				}
			}
			else
			{
				switch (wParam)
				{
				case VK_SPACE: evsys->OnKeyAction(KeyAction::ActivateUp, M, numRepeats); break;
				}
			}
		}
		return TRUE;
	case WM_CHAR:
		if (auto* evsys = GetEventSys(hWnd))
			evsys->OnTextInput(wParam, GetModifierKeys(), lParam & 0xffff);
		return TRUE;
	case WM_ERASEBKGND:
		return FALSE;
	case WM_PAINT:
		if (auto* window = GetNativeWindow(hWnd))
		{
			window->Redraw(false);
		}
		break;
	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			if (auto* window = GetNativeWindow(hWnd))
			{
				auto w = LOWORD(lParam);
				auto h = HIWORD(lParam);
				rhi::OnResizeWindow(window->renderCtx, w, h);
				draw::_ResetScissorRectStack(0, 0, w, h);

				auto& evsys = window->GetEventSys();
				if (w != evsys.width || h != evsys.height)
				{
					Notify(DCT_ResizeWindow, window->GetOwner());
				}
				evsys.width = w;
				evsys.height = h;
				evsys.RecomputeLayout();
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
				auto* tgt = window->GetEventSys().FindObjectAtPosition({ float(pt.x), float(pt.y) });
				Event ev(&window->GetEventSys(), tgt, EventType::DragDrop);
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
	wc.hIcon = LoadIcon(GetModuleHandle(nullptr), "MAINICON");
	wc.hIconSm = LoadIcon(GetModuleHandle(nullptr), "MAINICON");
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


namespace ui { void RegisterPainters(); }
struct GlobalResources
{
	GlobalResources()
	{
		ui::RegisterPainters();
		InitializeWin32();
		ui::rhi::GlobalInit();
	}
	~GlobalResources()
	{
		ui::rhi::GlobalFree();
	}
};


int uimain(int argc, char* argv[]);
void IncludeContainerTests();

int RealMain()
{
	IncludeContainerTests();
	UI_DEFER(dumpallocinfo());

	int argc = 0;
	auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::string> args;
	std::vector<char*> argp;
	args.reserve(argc);
	argp.reserve(argc);
	for (int i = 0; i < argc; i++)
		args.push_back(ui::WCHARtoUTF8(argv[i]));
	for (int i = 0; i < argc; i++)
		argp.push_back(const_cast<char*>(args[i].c_str()));
	LocalFree(argv);

	GlobalResources grsrc;

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
