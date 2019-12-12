
#pragma once

#include "Objects.h"
#include "System.h"


namespace ui {


class Menu;


struct NativeWindow_Impl;
class NativeWindow : public UIElement
{
public:
	NativeWindow();
	~NativeWindow();
	void OnInit() override;
	void OnPaint() override;
	void OnLayout(const UIRect& rect) override;
	float GetFullEstimatedWidth(float containerWidth, float containerHeight) override { return 0; }
	float GetFullEstimatedHeight(float containerWidth, float containerHeight) override { return 0; }

	void SetTitle(const char* title);
	Menu* GetMenu();
	void SetMenu(Menu* m);

	void* GetNativeHandle() const;

	void Redraw();

private:
	NativeWindow_Impl* _impl = nullptr;
};

class Application
{
public:
	Application(int argc, char* argv[]);
	~Application();
	int Run();

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
};


} // ui
