
#pragma once
#include <vector>
#include "Objects.h"
#include "Native.h"

namespace ui {

struct FrameContents;
struct DockingMainArea;
struct DockingSubwindow;

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
using DockableContentsContainerHandle = RCHandle<DockableContentsContainer>;

struct DockingNode : RefCountedST
{
	DockingMainArea* main = nullptr;
	DockingSubwindow* subwindow = nullptr;
	DockingNode* parentNode = nullptr;

	bool isLeaf = true;
	// if not leaf (contains child ndoes):
	Direction splitDir = Direction::Horizontal;
	std::vector<float> splits;
	std::vector<RCHandle<DockingNode>> childNodes;
	// if leaf (contains docked contents):
	std::vector<DockableContentsContainerHandle> tabs;
	RCHandle<DockableContentsContainer> curActiveTab;

	void Build();
};
using DockingNodeHandle = RCHandle<DockingNode>;

struct DockingSubwindow : NativeWindowBase, RefCountedST
{
	DockingNodeHandle rootNode;

	Buildable* _root = nullptr;
	bool _dragging = false;
	Point2i _dragCWPDelta = {};

	void OnBuild() override;

	void StartDrag();
};
using DockingSubwindowHandle = RCHandle<DockingSubwindow>;

struct DockingWindowContentBuilder : Buildable
{
	DockingNodeHandle _root;

	void Build() override;
};

struct DockingMainArea : Buildable
{
	std::vector<DockingSubwindowHandle> _subwindows;
	DockingNodeHandle _mainAreaRootNode;

	void Build() override;

	void _PullOutTab(DockingNode* node, size_t tabID);
	void _CloseTab(DockingNode* node, size_t tabID);
};

}
