
#pragma once

#include <functional>

#include "Objects.h"
#include "System.h"


namespace ui {


class Menu;


struct NativeWindow_Impl;
class NativeWindowBase
{
public:
	NativeWindowBase();
	NativeWindowBase(std::function<void(UIContainer*)> renderFunc);
	~NativeWindowBase();

	virtual void OnClose();
	void SetRenderFunc(std::function<void(UIContainer*)> renderFunc);
	std::string GetTitle();
	void SetTitle(const char* title);
	bool IsVisible();
	void SetVisible(bool v);
	Menu* GetMenu();
	void SetMenu(Menu* m);

	void ProcessEventsExclusive();

	void* GetNativeHandle() const;

private:

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
	void Show() { ProcessEventsExclusive(); }
};

class NativeWindowNode : public UINode
{
public:
	void Render(UIContainer* ctx) override {}
	void OnLayout(const UIRect& rect) override;
	float GetFullEstimatedWidth(float containerWidth, float containerHeight) override { return 0; }
	float GetFullEstimatedHeight(float containerWidth, float containerHeight) override { return 0; }

	NativeWindowBase* GetWindow() { return &_window; }

private:
	NativeWindowBase _window;
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
		struct DefaultWindowWrapper : UINode
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
