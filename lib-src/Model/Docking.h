
#pragma once
#include "../Core/Array.h"
#include "Objects.h"
#include "Native.h"

namespace ui {

struct FrameContents;
struct DockingNode;
struct DockingMainArea;
struct DockingSubwindow;

struct DockableContents : Buildable
{
	virtual StringView GetID() const = 0;
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

enum DockingInsertionSide
{
	DockingInsertionSide_Here  = -1,
	DockingInsertionSide_Above = -2,
	DockingInsertionSide_Below = -3,
	DockingInsertionSide_Left  = -4,
	DockingInsertionSide_Right = -5,
};

struct DockingInsertionTarget
{
	WeakPtr<DockingNode> node;
	// >= 0  -->  tab
	//  < 0  -->  side (DockingInsertionSide)
	int tabOrSide;
};

struct DockingNode : RefCountedST
{
	UI_DECLARE_WEAK_PTR_COMPATIBLE;

	DockingMainArea* main = nullptr;
	DockingSubwindow* subwindow = nullptr;
	WeakPtr<DockingNode> parentNode = nullptr;

	bool isLeaf = true;
	// if not leaf (contains child ndoes):
	Direction splitDir = Direction::Horizontal;
	Array<float> splits;
	Array<RCHandle<DockingNode>> childNodes;
	// if leaf (contains docked contents):
	Array<DockableContentsContainerHandle> tabs;
	DockableContentsContainerHandle curActiveTab;

	WeakPtr<UIObject> rectSource;

	void SetSubwindow(DockingSubwindow* dsw);
	DockingInsertionTarget FindInsertionTarget(Vec2f pos);
	bool HasDockable(StringView id) const;

	void Build();
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
};
using DockingNodeHandle = RCHandle<DockingNode>;

struct DockingSubwindow : NativeWindowBase, Buildable, RefCountedST
{
	DockingNodeHandle rootNode;

	DockingMainArea* main;
	bool _dragging = false;
	Point2i _dragCWPDelta = {};

	DockingSubwindow();
	~DockingSubwindow();
	void Build() override;
	void OnEvent(Event& e) override;
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);

	void StartDrag();
};
using DockingSubwindowHandle = RCHandle<DockingSubwindow>;

struct DockingWindowContentBuilder : Buildable
{
	DockingNodeHandle _root;

	void Build() override;
	void OnPaint(const UIPaintContext&) override;
};

struct DockDefNode
{
	virtual DockingNodeHandle Construct(DockingMainArea*) const = 0;
};

struct DockingMainArea : Buildable
{
	Array<DockingSubwindowHandle> _subwindows;
	DockingNodeHandle _mainAreaRootNode;
	DockingInsertionTarget _insTarget = {};

	DockableContentsSource* source = nullptr;

	void Build() override;

	void _PullOutTab(DockingNode* node, size_t tabID);
	void _CloseTab(DockingNode* node, size_t tabID);
	void _DeleteNode(DockingNode* node);
	void _UpdateInsertionTarget(Point2i screenCursorPos);
	void _FinishInsertion(DockingSubwindow* dsw);
	void _ClearInsertionTarget() { _insTarget = {}; }

	DockingMainArea& SetSource(DockableContentsSource* dcs);
	void Clear();
	void ClearMainArea();
	void RemoveSubwindows();
	void SetMainAreaContents(const DockDefNode& node);
	void AddSubwindow(const DockDefNode& node);
	bool HasDockable(StringView id) const;
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
};

namespace dockdef {

struct Split : DockDefNode
{
	struct Entry
	{
		float split;
		const DockDefNode* node;
	};

	Array<Entry> children;

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
	Array<const char*> tabContentIDs;

	Tabs(std::initializer_list<const char*> ids) : tabContentIDs(ids) {}
	DockingNodeHandle Construct(DockingMainArea*) const override;
};

} // dockdef

}
