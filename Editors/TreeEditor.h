
#pragma once
#include "EditCommon.h"

#include "../Core/Memory.h"

#include "../Model/Objects.h"
#include "../Model/Controls.h"
#include "../Model/System.h"


namespace ui {

using TreePathRef = ArrayView<uintptr_t>;

struct ITreeContextMenuSource
{
	virtual void FillItemContextMenu(MenuItemCollection& mic, TreePathRef path, size_t col) = 0;
	virtual void FillListContextMenu(MenuItemCollection& mic) = 0;
};

enum class TreeInsertMode : uint8_t
{
	Before,
	After,
	Inside,
};

struct ITree
{
	enum FeatureFlags
	{
		CanGetParent = 1 << 0,
		HasChildArray = 1 << 1,
	};

	virtual unsigned GetFeatureFlags() = 0;
	virtual ItemLoc GetFirstRoot() = 0;
	virtual ItemLoc GetFirstChild(ItemLoc node) = 0;
	virtual ItemLoc GetNext(ItemLoc node) = 0;
	virtual ItemLoc GetParent(ItemLoc node) { return node; } // implement for CanGetParent
	virtual bool AtEnd(ItemLoc node) = 0;
	virtual void* GetValuePtr(ItemLoc node) = 0;

	template <class T> T& GetValue(ItemLoc node) { return *static_cast<T*>(GetValuePtr(node)); }

	virtual void RemoveChildrenFromList(ItemLoc* nodes, size_t& count);
	virtual void RemoveChildrenFromListOf(ItemLoc* nodes, size_t& count, ItemLoc parent);
	virtual void IndexSort(ItemLoc* nodes, size_t count);
	virtual void RemoveAll(TreePathRef* paths, size_t count);
	virtual void DuplicateAll(TreePathRef* paths, size_t count);

	virtual void Remove(TreePathRef path) {}
	virtual void Duplicate(TreePathRef path) {}
	// remove node from the old location, then add to the new one
	// dest is pre-adjusted to assume that the node has already been removed
	virtual void MoveTo(ui::ItemLoc node, ItemLoc dest, TreeInsertMode insDir) {}
};


struct TreeEditor;
struct TreeDragData : DragDropData
{
	static constexpr const char* NAME = "TreeDragData";

	TreeDragData(TreeEditor* s, const std::vector<ItemLoc>& nv) : ui::DragDropData(NAME), scope(s), nodes(nv) {}

	TreeEditor* scope;
	std::vector<ItemLoc> nodes;
};

struct TreeItemElement : Selectable
{
	void OnInit() override;
	void OnEvent(UIEvent& e) override;
	virtual void ContextMenu();

	void Init(TreeEditor* te, ItemLoc n, const std::vector<uintptr_t>& path);

	TreeEditor* treeEd = nullptr;
	ItemLoc node = {};
	std::vector<uintptr_t> path;
};

struct TreeEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;
	void OnSerialize(IDataSerializer& s) override;

	virtual void OnBuildChildList(UIContainer* ctx, ItemLoc firstNode, std::vector<uintptr_t>& path);
	virtual void OnBuildList(UIContainer* ctx, ItemLoc firstNode, std::vector<uintptr_t>& path);
	virtual void OnBuildItem(UIContainer* ctx, ItemLoc node, std::vector<uintptr_t>& path);
	virtual void OnBuildDeleteButton(UIContainer* ctx);

	ITree* GetTree() const { return _tree; }
	TreeEditor& SetTree(ITree* t);
	ITreeContextMenuSource* GetContextMenuSource() const { return _ctxMenuSrc; }
	TreeEditor& SetContextMenuSource(ITreeContextMenuSource* src);

	void _OnEdit(UIObject* who);
	void _OnDragMove(TreeDragData* tdd, ItemLoc item, const UIRect& rect, UIEvent& e);
	void _OnDragDrop(TreeDragData* tdd);

	std::function<void(UIContainer* ctx, TreeEditor* te, ItemLoc node)> itemUICallback;

	bool showDeleteButton = true;

	ITree* _tree;
	ITreeContextMenuSource* _ctxMenuSrc = nullptr;

	ItemLoc _dragTargetLoc;
	TreeInsertMode _dragTargetInsDir = TreeInsertMode::Before;
	UIRect _dragTargetLine = {};
};

} // ui
