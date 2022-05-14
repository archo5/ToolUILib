
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
Point2i GetCursorScreenPos();
void RequestRawMouseInput();
Color4b GetColorAtScreenPos(Point2i pos);
void ShowErrorMessage(StringView title, StringView text);
void BrowseToFile(StringView path);
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
	Point2i position;
	Point2i size;
	uint32_t state;
};


struct NativeWindow_Impl;
struct NativeWindowBase
{
	NativeWindowBase();
	//NativeWindowBase(std::function<void(UIContainer*)> buildFunc);
	~NativeWindowBase();

	virtual void OnBuild() = 0;
	virtual void OnClose();
	//void SetBuildFunc(std::function<void(UIContainer*)> buildFunc);

	virtual void OnFocusReceived() {}
	virtual void OnFocusLost() {}

	std::string GetTitle();
	void SetTitle(StringView title);

	WindowStyle GetStyle();
	void SetStyle(WindowStyle ws);

	bool IsVisible();
	void SetVisible(bool v);

	// can the window be reached by using the mouse cursor
	bool IsHitTestEnabled();
	void SetHitTestEnabled(bool e);

	Menu* GetMenu();
	void SetMenu(Menu* m);

	AABB2i GetOuterRect();
	AABB2i GetInnerRect();
	void SetOuterRect(AABB2i bb);
	void SetInnerRect(AABB2i bb);

	Point2i GetOuterPosition();
	Point2i GetInnerPosition();
	void SetOuterPosition(int x, int y);
	void SetOuterPosition(Point2i p) { SetOuterPosition(p.x, p.y); }
	void SetInnerPosition(int x, int y);
	void SetInnerPosition(Point2i p) { SetInnerPosition(p.x, p.y); }

	Size2i GetOuterSize();
	Size2i GetInnerSize();
	void SetOuterSize(int x, int y);
	void SetOuterSize(Size2i p) { SetOuterSize(p.x, p.y); }
	void SetInnerSize(int x, int y);
	void SetInnerSize(Size2i p) { SetInnerSize(p.x, p.y); }

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

	void Rebuild();
	void InvalidateAll();

	void SetDefaultCursor(DefaultCursor cur);
	void CaptureMouse();

	void ProcessEventsExclusive();

	void* GetNativeHandle() const;
	bool IsDragged() const;

	static NativeWindowBase* FindFromScreenPos(Point2i p);

	NativeWindow_Impl* _impl = nullptr;
};

class NativeWindowBuildFunc : public NativeWindowBase
{
public:
	void OnBuild() override;
	void SetBuildFunc(std::function<void()> buildFunc);

private:
	std::function<void()> _buildFunc;
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

class NativeWindowNode : public Buildable
{
public:
	void Build() override {}
	void OnLayout(const UIRect& rect) override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }

	NativeWindowBuildFunc* GetWindow() { return &_window; }

private:
	NativeWindowBuildFunc _window;
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

template <class T>
struct IntrusiveLinkedList
{
	T* _first = nullptr;
	T* _last = nullptr;

	bool HasAny() const { return _first; }

	void AddToStart(T* node)
	{
		assert(node->_prev == nullptr && node->_next == nullptr);
		node->_next = _first;
		if (_first)
			_first->_prev = node;
		_first = node;
		if (!_last)
			_last = node;
	}
	void AddToEnd(T* node)
	{
		assert(node->_prev == nullptr && node->_next == nullptr);
		node->_prev = _last;
		if (_last)
			_last->_next = node;
		_last = node;
		if (!_first)
			_first = node;
	}
	void Remove(T* node)
	{
		if (node->_prev)
			node->_prev->_next = node->_next;
		else if (_first == node)
			_first = _first->_next;

		if (node->_next)
			node->_next->_prev = node->_prev;
		else if (_last == node)
			_last = _last->_prev;
	}
};

template <class T>
struct IntrusiveLinkedListNode
{
	T* _prev = nullptr;
	T* _next = nullptr;
};

struct RawMouseInputRequester : IntrusiveLinkedListNode<RawMouseInputRequester>
{
	bool _requesting = false;

	RawMouseInputRequester(bool initial = true) { if (initial) BeginRawMouseInput(); }
	~RawMouseInputRequester() { EndRawMouseInput(); }
	void BeginRawMouseInput();
	void EndRawMouseInput();

	virtual void OnRawInputEvent(float dx, float dy) = 0;
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
	static void PushEvent(F&& f)
	{
		_GetEventQueue().Push(std::move(f));
		_SignalEvent();
	}
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
			void Build() override
			{
				w = Push<NativeWindow>();
				Make<T>();
				Pop();
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
