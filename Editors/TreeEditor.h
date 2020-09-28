
#pragma once

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

	struct NodeLoc
	{
		uintptr_t node; // pointer to node or offset to entry in node container
		void* cont; // pointer to node container or nothing

		bool operator == (const NodeLoc& o) const { return node == o.node && cont == o.cont; }
		bool operator != (const NodeLoc& o) const { return node != o.node || cont != o.cont; }
	};

	virtual unsigned GetFeatureFlags() = 0;
	virtual NodeLoc GetFirstRoot() = 0;
	virtual NodeLoc GetFirstChild(NodeLoc node) = 0;
	virtual NodeLoc GetNext(NodeLoc node) = 0;
	virtual NodeLoc GetParent(NodeLoc node) { return node; } // implement for CanGetParent
	virtual bool AtEnd(NodeLoc node) = 0;
	virtual void* GetValuePtr(NodeLoc node) = 0;

	template <class T> T& GetValue(NodeLoc node) { return *static_cast<T*>(GetValuePtr(node)); }

	virtual void RemoveChildrenFromList(NodeLoc* nodes, size_t count);
	virtual void IndexSort(NodeLoc* nodes, size_t count);
	virtual void RemoveAll(NodeLoc* nodes, size_t count);
	virtual void DuplicateAll(NodeLoc* nodes, size_t count);

	virtual void Remove(NodeLoc node) {}
	virtual void Duplicate(NodeLoc node) {}
	virtual void MoveTo(const NodeLoc* nodes, size_t count, NodeLoc dest, bool insertAtEnd) {}
};


struct TreeEditor;
struct TreeDragData : DragDropData
{
	static constexpr const char* NAME = "TreeDragData";

	TreeDragData(TreeEditor* s, const std::vector<ITree::NodeLoc>& nv) : ui::DragDropData(NAME), scope(s), nodes(nv) {}

	TreeEditor* scope;
	std::vector<ITree::NodeLoc> nodes;
};

struct TreeItemElement : Selectable
{
	void OnInit() override;
	void OnEvent(UIEvent& e) override;
	virtual void ContextMenu();

	void Init(TreeEditor* te, ITree::NodeLoc n);

	TreeEditor* treeEd = nullptr;
	ITree::NodeLoc node = {};
};

struct TreeEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;

	virtual void OnBuildChildList(UIContainer* ctx, ITree::NodeLoc firstNode);
	virtual void OnBuildList(UIContainer* ctx, ITree::NodeLoc firstNode);
	virtual void OnBuildItem(UIContainer* ctx, ITree::NodeLoc node);
	virtual void OnBuildDeleteButton(UIContainer* ctx, ITree::NodeLoc node);

	ITree* GetTree() const { return _tree; }
	TreeEditor& SetTree(ITree* t);

	std::function<void(UIContainer* ctx, TreeEditor* te, ITree::NodeLoc node)> itemUICallback;

	bool showDeleteButton = true;

	ITree* _tree;
};

} // ui
