
#pragma once
#include <vector>
#include "Objects.h"

namespace ui {

struct FrameContents;
struct Dockable;

struct DockableContents : Buildable
{
	virtual std::string GetTitle() = 0;
};

struct DockableContentsContainer : RefCountedST
{
	DockableContents* contents;
	FrameContents* frameContents;

	DockableContentsContainer(DockableContents* C);
};

struct DockingNode : RefCountedST
{
	bool isLeaf = true;
	// if not leaf (contains child ndoes):
	Direction splitDir = Direction::Horizontal;
	std::vector<float> splits;
	std::vector<RCHandle<DockingNode>> childNodes;
	// if leaf (contains docked contents):
	std::vector<RCHandle<DockableContentsContainer>> tabs;
	RCHandle<DockableContentsContainer> curActiveTab;

	void Build();
};
using DockingNodeHandle = RCHandle<DockingNode>;

struct DockingSubwindow
{
	NativeWindowBase* window;
	DockingNodeHandle rootNode;
};

struct DockingWindowContentBuilder : Buildable
{
	DockingNodeHandle _root;

	void Build() override;
};

struct DockingMainArea : Buildable
{
	std::vector<DockingSubwindow> _subwindows;
	DockingNodeHandle _mainAreaRootNode;

	void Build() override;
};

}
