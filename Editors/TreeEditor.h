
#pragma once
#include "EditCommon.h"

#include "../Model/Objects.h"
#include "../Model/Controls.h"
#include "../Model/System.h"


namespace ui {

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

	virtual void RemoveChildrenFromList(ItemLoc* nodes, size_t count);
	virtual void IndexSort(ItemLoc* nodes, size_t count);
	virtual void RemoveAll(ItemLoc* nodes, size_t count);
	virtual void DuplicateAll(ItemLoc* nodes, size_t count);

	virtual void Remove(ItemLoc node) {}
	virtual void Duplicate(ItemLoc node) {}
	virtual void MoveTo(const ItemLoc* nodes, size_t count, ItemLoc dest, bool insertAtEnd) {}
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

	void Init(TreeEditor* te, ItemLoc n);

	TreeEditor* treeEd = nullptr;
	ItemLoc node = {};
};

struct TreeEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;

	virtual void OnBuildChildList(UIContainer* ctx, ItemLoc firstNode);
	virtual void OnBuildList(UIContainer* ctx, ItemLoc firstNode);
	virtual void OnBuildItem(UIContainer* ctx, ItemLoc node);
	virtual void OnBuildDeleteButton(UIContainer* ctx, ItemLoc node);

	ITree* GetTree() const { return _tree; }
	TreeEditor& SetTree(ITree* t);
	IListContextMenuSource* GetContextMenuSource() const { return _ctxMenuSrc; }
	TreeEditor& SetContextMenuSource(IListContextMenuSource* src);

	std::function<void(UIContainer* ctx, TreeEditor* te, ItemLoc node)> itemUICallback;

	bool showDeleteButton = true;

	ITree* _tree;
	IListContextMenuSource* _ctxMenuSrc = nullptr;
};

} // ui
