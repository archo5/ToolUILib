
#pragma once
#include "EditCommon.h"

#include "../Core/Memory.h"

#include "../Model/Objects.h"
#include "../Model/Controls.h"
#include "../Model/System.h"


namespace ui {

using TreePath = std::vector<uintptr_t>;
using TreePathRef = ArrayView<uintptr_t>;

struct ITreeContextMenuSource
{
	virtual void FillItemContextMenu(MenuItemCollection& mic, TreePathRef path, size_t col) = 0;
	virtual void FillListContextMenu(MenuItemCollection& mic) = 0;
};

struct ITree
{
	enum FeatureFlags
	{
		HasChildArray = 1 << 1,
	};

	virtual unsigned GetFeatureFlags() = 0;
	virtual ItemLoc GetFirstRoot() = 0;
	virtual ItemLoc GetFirstChild(ItemLoc node) = 0;
	virtual ItemLoc GetNext(ItemLoc node) = 0;
	virtual bool AtEnd(ItemLoc node) = 0;
	virtual size_t GetChildCount(TreePathRef path) = 0;
	virtual void* GetValuePtr(ItemLoc node) = 0;

	template <class T> T& GetValue(ItemLoc node) { return *static_cast<T*>(GetValuePtr(node)); }

#if 0
	virtual void RemoveChildrenFromList(ItemLoc* nodes, size_t& count);
	virtual void RemoveChildrenFromListOf(ItemLoc* nodes, size_t& count, ItemLoc parent);
	virtual void IndexSort(ItemLoc* nodes, size_t count);
	virtual void RemoveAll(TreePathRef* paths, size_t count);
	virtual void DuplicateAll(TreePathRef* paths, size_t count);
#endif

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

	TreeDragData(TreeEditor* s, const std::vector<TreePath>& pv) : ui::DragDropData(NAME), scope(s), paths(pv) {}

	TreeEditor* scope;
	std::vector<TreePath> paths;
};

struct TreeItemElement : Selectable
{
	void OnInit() override;
	void OnEvent(UIEvent& e) override;
	virtual void ContextMenu();

	void Init(TreeEditor* te, const TreePath& path);

	TreeEditor* treeEd = nullptr;
	TreePath path;
};

struct TreeEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;
	void OnSerialize(IDataSerializer& s) override;

	virtual void OnBuildChildList(UIContainer* ctx, ItemLoc firstNode, TreePath& path);
	virtual void OnBuildList(UIContainer* ctx, ItemLoc firstNode, TreePath& path);
	virtual void OnBuildItem(UIContainer* ctx, ItemLoc node, TreePath& path);
	virtual void OnBuildDeleteButton(UIContainer* ctx);

	ITree* GetTree() const { return _tree; }
	TreeEditor& SetTree(ITree* t);
	ITreeContextMenuSource* GetContextMenuSource() const { return _ctxMenuSrc; }
	TreeEditor& SetContextMenuSource(ITreeContextMenuSource* src);

	void _OnEdit(UIObject* who);
	void _OnDragMove(TreeDragData* tdd, TreePathRef hoverPath, const UIRect& rect, UIEvent& e);
	void _OnDragDrop(TreeDragData* tdd);

	std::function<void(UIContainer* ctx, TreeEditor* te, ItemLoc node)> itemUICallback;

	bool showDeleteButton = true;

	ITree* _tree;
	ITreeContextMenuSource* _ctxMenuSrc = nullptr;

	TreePath _dragTargetLoc;
	UIRect _dragTargetLine = {};
};

} // ui
