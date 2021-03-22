
#pragma once

#include <functional>

#include "../Core/Math.h"
#include "../Core/Threading.h"
#include "Objects.h"
#include "System.h"


namespace ui {


class Menu;


namespace platform {
uint32_t GetTimeMs();
uint32_t GetDoubleClickTime();
Point<int> GetCursorScreenPos();
Color4b GetColorAtScreenPos(Point<int> pos);
void ShowErrorMessage(StringView title, StringView text);
} // platform


struct Clipboard
{
	static bool HasText();
	static std::string GetText();
	static void SetText(const char* text);
	static void SetText(StringView text);
};


struct FileSelectionWindow
{
	struct Filter
	{
		std::string name;
		std::string exts;
	};

	enum Flags
	{
		MultiSelect = 1 << 0,
		CreatePrompt = 1 << 1,
	};

	bool Show(bool save);

	std::vector<Filter> filters;
	std::string defaultExt;
	std::string title;
	std::string currentDir;
	std::vector<std::string> selectedFiles;
	size_t maxFileNameBuffer = 65536;
	unsigned flags = 0;
};


extern DataCategoryTag DCT_ResizeWindow[1];


enum class WindowState : uint8_t
{
	Normal,
	Minimized,
	Maximized,
};

enum WindowStyle
{
	WS_None = 0,
	WS_TitleBar = 1 << 0,
	WS_Resizable = 1 << 1,
	WS_Default = WS_Resizable | WS_TitleBar,
};
inline constexpr WindowStyle operator | (WindowStyle a, WindowStyle b)
{
	return WindowStyle(int(a) | int(b));
}

struct NativeWindowGeometry
{
	Point<int> position;
	Point<int> size;
	uint32_t state;
};


struct NativeWindow_Impl;
struct NativeWindowBase
{
	NativeWindowBase();
	//NativeWindowBase(std::function<void(UIContainer*)> renderFunc);
	~NativeWindowBase();

	virtual void OnRender(UIContainer* ctx) = 0;
	virtual void OnClose();
	//void SetRenderFunc(std::function<void(UIContainer*)> renderFunc);

	std::string GetTitle();
	void SetTitle(const char* title);

	WindowStyle GetStyle();
	void SetStyle(WindowStyle ws);

	bool IsVisible();
	void SetVisible(bool v);

	Menu* GetMenu();
	void SetMenu(Menu* m);

	Point<int> GetPosition();
	void SetPosition(int x, int y);
	void SetPosition(Point<int> p) { SetPosition(p.x, p.y); }

	Size<int> GetSize();
	void SetSize(int x, int y, bool inner = true);
	void SetSize(Size<int> p, bool inner = true) { SetSize(p.x, p.y, inner); }

	WindowState GetState();
	void SetState(WindowState ws);

	NativeWindowGeometry GetGeometry();
	void SetGeometry(const NativeWindowGeometry& geom);

	NativeWindowBase* GetParent() const;
	void SetParent(NativeWindowBase* parent);

	bool IsInnerUIEnabled();
	void SetInnerUIEnabled(bool enabled);

	bool IsDebugDrawEnabled();
	void SetDebugDrawEnabled(bool enabled);

	void InvalidateAll();

	void SetDefaultCursor(DefaultCursor cur);

	void ProcessEventsExclusive();

	void* GetNativeHandle() const;
	bool IsDragged() const;

	NativeWindow_Impl* _impl = nullptr;
};

class NativeWindowRenderFunc : public NativeWindowBase
{
public:
	void OnRender(UIContainer* ctx) override;
	void SetRenderFunc(std::function<void(UIContainer*)> renderFunc);

private:
	std::function<void(UIContainer*)> _renderFunc;
};

class NativeMainWindow : public NativeWindowBase
{
public:
	void OnClose() override;
};

class NativeDialogWindow : public NativeWindowBase
{
public:
	void Show()
	{
		SetVisible(true);
		ProcessEventsExclusive();
	}
};

class NativeWindowNode : public Node
{
public:
	void Render(UIContainer* ctx) override {}
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override;
	Range<float> GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type, bool forParentLayout) override { return {}; }
	Range<float> GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type, bool forParentLayout) override { return {}; }

	NativeWindowRenderFunc* GetWindow() { return &_window; }

private:
	NativeWindowRenderFunc _window;
};

struct AnimationRequester
{
	AnimationRequester(bool initial = false) { if (initial) BeginAnimation(); }
	~AnimationRequester() { EndAnimation(); }
	void BeginAnimation();
	void EndAnimation();
	void SetAnimating(bool v) { if (v) BeginAnimation(); else EndAnimation(); }
	bool IsAnimating() const { return _animating; }

	virtual void OnAnimationFrame() = 0;

	bool _animating = false;
};

struct AnimationCallbackRequester : AnimationRequester
{
	void OnAnimationFrame() override
	{
		if (callback)
			callback();
	}

	std::function<void()> callback;
};

struct Inspector;

class Application
{
public:
	Application(int argc, char* argv[]);
	~Application();

	static Application* GetInstance() { return _instance; }
	static void Quit(int code = 0);
	static void OpenInspector(NativeWindowBase* window = nullptr, UIObject* obj = nullptr);

	template <class F>
	static void PushEvent(UIObject* obj, F&& f)
	{
		auto lt = obj->GetLivenessToken();
		auto fw = [lt, f{ std::move(f) }]()
		{
			if (lt.IsAlive())
				f();
		};
		_GetEventQueue().Push(std::move(fw));
		_SignalEvent();
	}
	static EventQueue& _GetEventQueue();
	static void _SignalEvent();

	int Run();

#if 0
	template <class T> NativeWindow* BuildWithWindow()
	{
		struct DefaultWindowWrapper : Node
		{
			NativeWindow* w = nullptr;
			void Render(UIContainer* ctx) override
			{
				w = ctx->Push<NativeWindow>();
				ctx->Make<T>();
				ctx->Pop();
			}
		};
		return system.Build<DefaultWindowWrapper>()->w;
	}

	UISystem system;
#endif

	Inspector* _inspector = nullptr;

private:
	static Application* _instance;
};


} // ui
