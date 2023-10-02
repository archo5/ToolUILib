
#pragma once

#include <functional>

#include "../Core/Math.h"
#include "../Core/Threading.h"
#include "../Render/Render.h"
#include "Objects.h"
#include "System.h"


namespace ui {


class Menu;


namespace platform {
uint32_t GetTimeMs();
uint32_t GetDoubleClickTime();
Point2i GetCursorScreenPos();
void SetCursorScreenPos(Point2i p);
void RequestRawMouseInput();
Color4b GetColorAtScreenPos(Point2i pos);
bool IsKeyPressed(u8 physicalKey);
std::string GetPhysicalKeyName(u8 physicalKey);
void ShowErrorMessage(StringView title, StringView text);
void BrowseToFile(StringView path);

enum class FileIconType
{
	FromFile,
	GenericFile,
	GenericDir,
};
draw::ImageSetHandle LoadFileIcon(StringView path, FileIconType type = FileIconType::FromFile);
} // platform


typedef struct Monitor_* MonitorID;
struct Monitors
{
	static Array<MonitorID> All();
	static MonitorID Primary();
	static MonitorID FindFromPoint(Vec2i point, bool nearest = false);
	static MonitorID FindFromWindow(NativeWindowBase* w);

	static bool IsPrimary(MonitorID id);
	static AABB2i GetScreenArea(MonitorID id);
	static std::string GetName(MonitorID id);
};


namespace rhi {
struct GraphicsAdapters
{
	static Array<std::string> All();

	static void GetInitial(int& index, StringView& name);
	static void SetInitialByName(StringView name); // empty = default
	static void SetInitialByIndex(int index); // -1 = default
};
} // rhi


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

	Array<Filter> filters;
	std::string defaultExt;
	std::string title;
	std::string currentDir;
	Array<std::string> selectedFiles;
	size_t maxFileNameBuffer = 65536;
	unsigned flags = 0;

	bool Show(bool save);
};


extern MulticastDelegate<NativeWindowBase*> OnWindowResized;
struct WindowKeyEvent
{
	NativeWindowBase* window;
	bool down;
	u32 virtualKey;
	u8 physicalKey;
	u8 modifiers;
	bool isRepeated;
	u16 numRepeats;
};
extern MulticastDelegate<const WindowKeyEvent&> OnWindowKeyEvent;


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

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
};

struct ExclusiveFullscreenInfo
{
	Size2i size;
	MonitorID monitor = nullptr;
	int refreshRate = 0;

	bool operator == (const ExclusiveFullscreenInfo& o) const
	{
		return size == o.size && monitor == o.monitor && refreshRate == o.refreshRate;
	}
	bool operator != (const ExclusiveFullscreenInfo& o) const { return !(*this == o); }
};


struct NativeWindow_Impl;
struct NativeWindowBase
{
	NativeWindowBase();
	~NativeWindowBase();

	virtual void OnClose();

	virtual void OnFocusReceived() {}
	virtual void OnFocusLost() {}

	void SetContents(Buildable* B, bool transferOwnership);

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

	bool IsInExclusiveFullscreen() const;
	void StopExclusiveFullscreen();
	void StartExclusiveFullscreen(ExclusiveFullscreenInfo info);

	void SetVSyncInterval(unsigned interval);

	bool IsInnerUIEnabled();
	void SetInnerUIEnabled(bool enabled);

	//float GetInnerUIScale();
	//void SetInnerUIScale(float scale); // use 1 to reset

	bool IsDebugDrawEnabled();
	void SetDebugDrawEnabled(bool enabled);

	void RebuildRoot();
	void InvalidateAll();

	void SetDefaultCursor(DefaultCursor cur);
	void CaptureMouse();
	bool HasFocus();

	void ProcessEventsExclusive();

	void* GetNativeHandle() const;
	bool IsDragged() const;

	static NativeWindowBase* FindFromScreenPos(Point2i p);

	NativeWindow_Impl* _impl = nullptr;
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

class NativeWindowNode : public UIObjectNoChildren
{
public:
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }

	NativeWindowBase* GetWindow() { return &_window; }

private:
	NativeWindowBase _window;
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
		_GetEventQueue().Push(Move(f));
		_SignalEvent();
	}
	template <class F>
	static void PushEvent(UIObject* obj, F&& f)
	{
		auto lt = obj->GetLivenessToken();
		auto fw = [lt, f{ Move(f) }]()
		{
			if (lt.IsAlive())
				f();
		};
		_GetEventQueue().Push(Move(fw));
		_SignalEvent();
	}
	static EventQueue& _GetEventQueue();
	static void _SignalEvent();

	int Run();

	// pieces for constructing a realtime loop
	// expected usage code:
	// for (;;)
	// {
	//     if (auto ec = Application::GetExitCode())
	//         return ec.GetValue();
	//     Application::ProcessSystemMessagesNonBlocking();
	//     Application::ProcessMainEventQueue();
	//     Application::RedrawAllWindows();
	// }
	static Optional<int> GetExitCode();
	static void ProcessSystemMessagesNonBlocking();
	static void ProcessMainEventQueue();
	static void RedrawAllWindows();

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
