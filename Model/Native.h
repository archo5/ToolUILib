
#pragma once

#include <functional>

#include "../Core/Math.h"
#include "Objects.h"
#include "System.h"


namespace ui {


class Menu;


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
class NativeWindowBase
{
public:
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

	Point<int> GetSize();
	void SetSize(int x, int y);
	void SetSize(Point<int> p) { SetSize(p.x, p.y); }

	WindowState GetState();
	void SetState(WindowState ws);

	NativeWindowGeometry GetGeometry();
	void SetGeometry(const NativeWindowGeometry& geom);

	NativeWindowBase* GetParent() const;
	void SetParent(NativeWindowBase* parent);

	bool IsInnerUIEnabled();
	void SetInnerUIEnabled(bool enabled);

	void ProcessEventsExclusive();

	void* GetNativeHandle() const;
	bool IsDragged() const;

private:

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
	void OnLayout(const UIRect& rect) override;
	float GetFullEstimatedWidth(float containerWidth, float containerHeight) override { return 0; }
	float GetFullEstimatedHeight(float containerWidth, float containerHeight) override { return 0; }

	NativeWindowRenderFunc* GetWindow() { return &_window; }

private:
	NativeWindowRenderFunc _window;
};

class Application
{
public:
	Application(int argc, char* argv[]);
	~Application();

	static Application* GetInstance() { return _instance; }
	static void Quit(int code = 0);

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

private:
	static Application* _instance;
};


} // ui
