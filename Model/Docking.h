
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

struct DockableContentsSource
{
	virtual DockableContents* GetDockableContentsByID(StringView id) = 0;
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

	void SetSubwindow(DockingSubwindow* dsw);

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

struct DockDefNode
{
	virtual DockingNodeHandle Construct(DockingMainArea*) const = 0;
};

struct DockingMainArea : Buildable
{
	std::vector<DockingSubwindowHandle> _subwindows;
	DockingNodeHandle _mainAreaRootNode;

	DockableContentsSource* source = nullptr;

	void Build() override;

	void _PullOutTab(DockingNode* node, size_t tabID);
	void _CloseTab(DockingNode* node, size_t tabID);
	void _DeleteNode(DockingNode* node);

	DockingMainArea& SetSource(DockableContentsSource* dcs);
	void Clear();
	void ClearMainArea();
	void RemoveSubwindows();
	void SetMainAreaContents(const DockDefNode& node);
	void AddSubwindow(const DockDefNode& node);
};

namespace dockdef {

struct Split : DockDefNode
{
	struct Entry
	{
		float split;
		const DockDefNode* node;
	};

	std::vector<Entry> children;

	Split(const DockDefNode& a, float s1, const DockDefNode& b)
	{
		children = { { 0, &a }, { s1, &b } };
	}
	DockingNodeHandle Construct(DockingMainArea*) const override;
	virtual Direction GetSplitDirection() const = 0;
};

struct HSplit : Split
{
	using Split::Split;
	Direction GetSplitDirection() const override { return Direction::Horizontal; }
};

struct VSplit : Split
{
	using Split::Split;
	Direction GetSplitDirection() const override { return Direction::Vertical; }
};

struct Tabs : DockDefNode
{
	std::vector<const char*> tabContentIDs;

	Tabs(std::initializer_list<const char*> ids) : tabContentIDs(ids) {}
	DockingNodeHandle Construct(DockingMainArea*) const override;
};

} // dockdef

}
