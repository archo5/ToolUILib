
#pragma once
#include <vector>
#include "Objects.h"

namespace ui {

struct FrameContents;
struct Dockable;

struct DockingArea : UIElement
{
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

struct Dockable : Buildable
{
	DockingArea* _area;
	FrameContents* _contents;

	Dockable()
	{
		flags |= UIObject_NeedsTreeUpdates;
	}

	void OnEnterTree() override;
	void OnExitTree() override;
};

}
