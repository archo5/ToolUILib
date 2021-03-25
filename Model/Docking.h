
#pragma once
#include <vector>
#include "Objects.h"

namespace ui {

struct FrameContents;
class Dockable;

class DockingArea : public UIElement
{
public:
	void SetDefaultLayout();

private:
	struct Node
	{
		float splitPos = 0;

		bool childOrderVertical = false;
		std::vector<Node*> childNodes;
		std::vector<Dockable*> tabs;
	};
	struct Window
	{
		NativeWindowBase* window;
		std::vector<Dockable*> tabs;
	};

	friend Dockable;

	void RegisterDockable(Dockable* d);
	void UnregisterDockable(Dockable* d);

	Node _root;
	std::vector<Window> _windows;
};

class Dockable : public ui::Buildable
{
public:
	void OnInit() override;
	void OnDestroy() override;

private:
	DockingArea* _area;
	FrameContents* _contents;
};

}
