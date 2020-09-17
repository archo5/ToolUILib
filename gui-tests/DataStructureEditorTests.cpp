
#include "pch.h"

#include <deque>
#include "../Editors/TreeEditor.h"


struct SequenceEditorsTest : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		if (ui::imm::Button(ctx, "Reset"))
		{
			vectordata = { 1, 2, 3, 4 };
			listdata = { 1, 2, 3, 4 };
			dequedata = { 1, 2, 3, 4 };
		}
		if (ui::imm::Button(ctx, "100 values"))
			AdjustSizeAll(100);
		if (ui::imm::Button(ctx, "1000 values"))
			AdjustSizeAll(1000);
		if (ui::imm::Button(ctx, "10000 values"))
			AdjustSizeAll(10000);

		ctx->Pop();

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::vector<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(vectordata)>>(vectordata));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::list<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(listdata)>>(listdata));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::deque<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(dequedata)>>(dequedata));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("int[5]:");
		SeqEdit(ctx, Allocate<ui::BufferSequence<int, uint8_t>>(bufdata, buflen));
		ctx->Pop();

		ctx->Pop();

		ctx->Make<ui::DefaultOverlayRenderer>();
	}

	void SeqEdit(UIContainer* ctx, ui::ISequence* seq)
	{
		ctx->Make<ui::SequenceEditor>()->SetSequence(seq).itemUICallback = [](UIContainer* ctx, ui::SequenceEditor* se, ui::ISequenceIterator* it)
		{
			ui::imm::PropEditInt(ctx, "\bvalue", se->GetSequence()->GetValue<int>(it));
		};
	}

	void AdjustSizeAll(size_t size)
	{
		AdjustSize(vectordata, size);
		AdjustSize(listdata, size);
		AdjustSize(dequedata, size);
	}
	template <class Cont> void AdjustSize(Cont& C, size_t size)
	{
		while (C.size() > size)
			C.pop_back();
		while (C.size() < size)
			C.push_back(C.size() + 1);
	}

	std::vector<int> vectordata{ 1, 2, 3, 4 };
	std::list<int> listdata{ 1, 2, 3, 4 };
	std::deque<int> dequedata{ 1, 2, 3, 4 };
	int bufdata[5] = { 1, 2, 3, 4, 9999 };
	uint8_t buflen = 4;
};
void Test_SequenceEditors(UIContainer* ctx)
{
	ctx->Make<SequenceEditorsTest>();
}


namespace npca { // no parent, child array
struct Node
{
	int num = 0;
	std::vector<Node*> children;

	Node(int n, const std::vector<Node*>& ch) : num(n), children(ch) {}
	~Node()
	{
		for (Node* n : children)
			delete n;
	}
	Node* Clone()
	{
		Node* tmp = new Node(*this);
		for (Node*& n : tmp->children)
			n = n->Clone();
		return tmp;
	}
};
struct Tree : ui::ITree
{
	std::vector<Node*> roots;

	Tree()
	{
		roots =
		{
			new Node(1,
			{
				new Node(2,
				{
					new Node(3,
					{
						new Node(5, {}),
					}),
					new Node(4, {}),
				}),
			}),
			new Node(8, {}),
		};
	}
	~Tree()
	{
		for (Node* r : roots)
			delete r;
	}

	unsigned GetFeatureFlags() override { return HasChildArray; }
	NodeLoc GetFirstRoot() override { return { 0, &roots }; }
	NodeLoc GetFirstChild(NodeLoc node) override
	{
		return { 0, &(*static_cast<std::vector<Node*>*>(node.cont))[node.node]->children };
	}
	NodeLoc GetNext(NodeLoc node) override { return { node.node + 1, node.cont }; }
	bool AtEnd(NodeLoc node) override { return node.node >= static_cast<std::vector<Node*>*>(node.cont)->size(); }
	void* GetValuePtr(NodeLoc node) override { return &(*static_cast<std::vector<Node*>*>(node.cont))[node.node]->num; }

	void Remove(NodeLoc node) override
	{
		auto& C = *static_cast<std::vector<Node*>*>(node.cont);
		delete C[node.node];
		C.erase(C.begin() + node.node);
	}
	void Duplicate(NodeLoc node) override
	{
		auto& C = *static_cast<std::vector<Node*>*>(node.cont);
		C.push_back(C[node.node]->Clone());
	}
};
} // npca

namespace wpca { // no parent, child array
struct Node
{
	int num = 0;
	std::vector<Node*> children;
	Node* parent = nullptr;

	Node(int n, const std::vector<Node*>& ch) : num(n), children(ch)
	{
		for (Node* n : children)
			n->parent = this;
	}
	~Node()
	{
		for (Node* n : children)
			delete n;
	}
	Node* Clone()
	{
		Node* tmp = new Node(*this);
		for (Node*& n : tmp->children)
		{
			n = n->Clone();
			n->parent = this;
		}
		return tmp;
	}
};
struct Tree : ui::ITree
{
	std::vector<Node*> roots;

	Tree()
	{
		roots =
		{
			new Node(1,
			{
				new Node(2,
				{
					new Node(3,
					{
						new Node(5, {}),
					}),
					new Node(4, {}),
				}),
			}),
			new Node(8, {}),
		};
	}
	~Tree()
	{
		for (Node* r : roots)
			delete r;
	}

	unsigned GetFeatureFlags() override { return HasChildArray | CanGetParent; }
	NodeLoc GetFirstRoot() override { return { 0, &roots }; }
	NodeLoc GetFirstChild(NodeLoc node) override
	{
		return { 0, &(*static_cast<std::vector<Node*>*>(node.cont))[node.node]->children };
	}
	NodeLoc GetNext(NodeLoc node) override { return { node.node + 1, node.cont }; }
	bool AtEnd(NodeLoc node) override { return node.node >= static_cast<std::vector<Node*>*>(node.cont)->size(); }
	void* GetValuePtr(NodeLoc node) override { return &(*static_cast<std::vector<Node*>*>(node.cont))[node.node]->num; }

	void Remove(NodeLoc node) override
	{
		auto& C = *static_cast<std::vector<Node*>*>(node.cont);
		delete C[node.node];
		C.erase(C.begin() + node.node);
	}
	void Duplicate(NodeLoc node) override
	{
		auto& C = *static_cast<std::vector<Node*>*>(node.cont);
		C.push_back(C[node.node]->Clone());
	}
};
} // wpca

struct TreeEditorsTest : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		ctx->PushBox() + ui::Width(style::Coord::Percent(33));
		ctx->Text("no parent, child array:");
		TreeEdit(ctx, &npcaTree);
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(33));
		ctx->Text("with parent, child array:");
		TreeEdit(ctx, &wpcaTree);
		ctx->Pop();

		ctx->Pop();

		ctx->Make<ui::DefaultOverlayRenderer>();
	}

	void TreeEdit(UIContainer* ctx, ui::ITree* itree)
	{
		ctx->Make<ui::TreeEditor>()->SetTree(itree).itemUICallback = [](UIContainer* ctx, ui::TreeEditor* te, ui::ITree::NodeLoc node)
		{
			ui::imm::PropEditInt(ctx, "\bvalue", te->GetTree()->GetValue<int>(node));
		};
	}

	npca::Tree npcaTree;
	wpca::Tree wpcaTree;
};
void Test_TreeEditors(UIContainer* ctx)
{
	ctx->Make<TreeEditorsTest>();
}
