
#pragma once
#include "EditCommon.h"

#include "../Core/Memory.h"

#include "../Model/Objects.h"
#include "../Model/Controls.h"
#include "../Model/System.h"


namespace ui {

using TreePath = std::vector<uintptr_t>;
using TreePathRef = ArrayView<uintptr_t>;

struct ITree
{
	using IterationFunc = std::function<void(void* ptr)>;
	virtual void IterateChildren(TreePathRef path, IterationFunc&& fn) = 0;
	virtual bool HasChildren(TreePathRef path) { return GetChildCount(path) != 0; }
	virtual size_t GetChildCount(TreePathRef path) = 0;

	virtual void ClearSelection() {}
	virtual bool GetSelectionState(TreePathRef path) { return false; }
	virtual void SetSelectionState(TreePathRef path, bool sel) {}

	virtual void FillItemContextMenu(MenuItemCollection& mic, TreePathRef path) {}
	virtual void FillListContextMenu(MenuItemCollection& mic) {}

	virtual void Remove(TreePathRef path) {}
	virtual void Duplicate(TreePathRef path) {}
	// remove node from the old location, then add to the new one
	// dest is pre-adjusted to assume that the node has already been removed
	virtual void MoveTo(TreePathRef node, TreePathRef dest) {}
};


struct TreeEditor;
struct TreeDragData : DragDropData
{
	static constexpr const char* NAME = "TreeDragData";

	TreeDragData(TreeEditor* s, const std::vector<TreePath>& pv) : DragDropData(NAME), scope(s), paths(pv) {}

	TreeEditor* scope;
	std::vector<TreePath> paths;
};

struct TreeItemElement : Selectable
{
	void OnInit() override;
	void OnEvent(Event& e) override;
	virtual void ContextMenu();

	void Init(TreeEditor* te, const TreePath& path);

	TreeEditor* treeEd = nullptr;
	TreePath path;
};

struct TreeEditor : Buildable
{
	void Build() override;
	void OnEvent(Event& e) override;
	void OnPaint() override;
	void OnSerialize(IDataSerializer& s) override;

	virtual void OnBuildChildList(TreePath& path);
	virtual void OnBuildList(TreePath& path);
	virtual void OnBuildItem(TreePathRef path, void* data);
	virtual void OnBuildDeleteButton();

	ITree* GetTree() const { return _tree; }
	TreeEditor& SetTree(ITree* t);
	TreeEditor& SetSelectionMode(SelectionMode mode);

	void _OnEdit(UIObject* who);
	void _OnDragMove(TreeDragData* tdd, TreePathRef hoverPath, const UIRect& rect, Event& e);
	void _OnDragDrop(TreeDragData* tdd);

	std::function<void(TreeEditor* te, TreePathRef path, void* data)> itemUICallback;

	bool showDeleteButton = true;

	ITree* _tree;

	TreePath _dragTargetLoc;
	UIRect _dragTargetLine = {};
	SelectionImplementation _selImpl;
};

} // ui
