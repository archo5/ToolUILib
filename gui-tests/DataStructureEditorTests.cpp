
#include "pch.h"

#include <time.h>
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

		{
			ui::Property::Scope ps(ctx, "\bSelection type:");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::None, "None");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::Single, "Single");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::MultipleToggle, "Multiple (toggle)");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::Multiple, "Multiple");
		}

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::vector<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(vectordata)>>(vectordata, &vectorsel));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::list<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(listdata)>>(listdata, &listsel));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::deque<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(dequedata)>>(dequedata, &dequesel));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("int[5]:");
		SeqEdit(ctx, Allocate<ui::BufferSequence<int, uint8_t>>(bufdata, buflen, &bufsel));
		ctx->Pop();

		ctx->Pop();

		ctx->Make<ui::DefaultOverlayRenderer>();
	}

	void SeqEdit(UIContainer* ctx, ui::ISequence* seq)
	{
		ctx->Make<ui::SequenceEditor>()
			->SetSequence(seq)
			.SetSelectionMode(selectionType)
			.itemUICallback = [](UIContainer* ctx, ui::SequenceEditor* se, size_t idx, void* ptr)
		{
			ui::imm::PropEditInt(ctx, "\bvalue", *static_cast<int*>(ptr));
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

	ui::BasicSelection vectorsel;
	ui::BasicSelection listsel;
	ui::BasicSelection dequesel;
	ui::BasicSelection bufsel;

	ui::SelectionMode selectionType = ui::SelectionMode::Single;
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


struct RandomNumberDataSource : ui::TableDataSource
{
	size_t GetNumRows() override { return 1000000; }
	size_t GetNumCols() override { return 5; }
	std::string GetRowName(size_t row) override { return std::to_string(row + 1); }
	std::string GetColName(size_t col) override { return std::to_string(col + 1); }
	std::string GetText(size_t row, size_t col) override
	{
		return std::to_string(((unsigned(row) * 5 + unsigned(col)) + 1013904223U) * 1664525U);
	}

	virtual void ClearSelection() { selRows.clear(); }
	virtual bool GetSelectionState(size_t row) { return selRows.count(row); }
	virtual void SetSelectionState(size_t row, bool sel)
	{
		if (sel)
			selRows.insert(row);
		else
			selRows.erase(row);
	}

	std::unordered_set<size_t> selRows;
}
g_randomNumbers;

struct TableViewTest : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		GetStyle().SetLayout(style::layouts::EdgeSlice());

		{
			ui::Property::Scope ps(ctx, "\bSelection type:");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::None, "None");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::Single, "Single");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::MultipleToggle, "Multiple (toggle)");
			ui::imm::RadioButton(ctx, selectionType, ui::SelectionMode::Multiple, "Multiple");
		}

		auto* tv = ctx->Make<ui::TableView>();
		*tv + ui::Height(style::Coord::Percent(100));
		tv->SetSelectionMode(selectionType);
		tv->SetDataSource(&g_randomNumbers);
	}

	ui::SelectionMode selectionType = ui::SelectionMode::Single;
};
void Test_TableView(UIContainer* ctx)
{
	ctx->Make<TableViewTest>();
}


struct MessageLogViewTest : ui::Node
{
	struct Message
	{
		time_t time;
		std::string text;
		std::string location;
	};

	struct MLV_Common : ui::MessageLogDataSource
	{
		size_t GetNumMessages() override { return msgs->size(); }
		std::string GetMessage(size_t msg, size_t line)
		{
			auto& M = (*msgs)[msg];
			if (line == 0)
			{
				auto* t = localtime(&M.time);
				char msg[1024];
				snprintf(msg, sizeof(msg), "[%02d:%02d:%02d] %s", t->tm_hour, t->tm_min, t->tm_sec, M.text.c_str());
				return msg;
			}
			else return M.location;
		}

		std::vector<Message>* msgs;
	};
	struct MLV_R : MLV_Common {};
	struct MLV_I : MLV_Common
	{
		size_t GetNumLines() override { return 2; }
		float GetMessageWidth(UIObject* context, size_t msg) override
		{
			return MLV_Common::GetMessageWidth(context, msg) + GetMessageHeight(context);
		}
		void OnDrawMessage(UIObject* context, size_t msg, UIRect area) override
		{
			float h = area.GetHeight();
			ui::draw::RectCol(area.x0 + 4, area.y0 + 4, area.x0 + h - 4, area.y1 - 4, ui::Color4b(200, 200, 200));
			area.x0 += h;
			MLV_Common::OnDrawMessage(context, msg, area);
		}
	};

	static constexpr bool Persistent = true;

	MessageLogViewTest()
	{
		AddMessages(2345);
	}

	void AddMessages(int n)
	{
		bool isAtEndR = mlvRtoken.IsAlive() && mlvR->IsAtEnd();
		bool isAtEndI = mlvItoken.IsAlive() && mlvI->IsAtEnd();

		static const char* words[10] =
		{
			"user", "interface", "library", "for", "editors",
			"designed", "with", "extensibility", "in", "mind",
		};
		auto T = time(nullptr);
		for (int i = 0; i < n; i++)
		{
			Message msg;
			msg.time = T + i;

			int nwords = rand() % 10 + 5;
			for (int j = 0; j < nwords; j++)
			{
				if (j)
					msg.text += " ";
				msg.text += words[((i + j) * nwords) % 10];
			}

			char bfr[128];
			snprintf(bfr, sizeof(bfr), "/random/generated/word/seq/%d/num/%d", i, nwords);
			msg.location = bfr;

			messages.push_back(msg);
		}

		if (isAtEndR)
			mlvR->ScrollToEnd();
		if (isAtEndI)
			mlvI->ScrollToEnd();
	}

	void Render(UIContainer* ctx) override
	{
		{
			ui::Property::Scope ps(ctx);
			if (ui::imm::Button(ctx, "Clear"))
				messages.clear();
			if (ui::imm::Button(ctx, "Add 1 line"))
				AddMessages(1);
			if (ui::imm::Button(ctx, "Add 10"))
				AddMessages(10);
			if (ui::imm::Button(ctx, "Add 100"))
				AddMessages(100);
			if (ui::imm::Button(ctx, "Add 1K"))
				AddMessages(1000);
			if (ui::imm::Button(ctx, "Add 10K"))
				AddMessages(10000);
		};
		ctx->PushBox()
			+ ui::StackingDirection(style::StackingDirection::LeftToRight)
			//+ ui::Height(style::Coord::Percent(50));
			+ ui::Height(200);
		{
			ctx->PushBox()
				+ ui::Layout(style::layouts::EdgeSlice())
				+ ui::Width(style::Coord::Percent(50))
				+ ui::Height(style::Coord::Percent(100));
			{
				ctx->Text("single line");
				*ctx->Push<ui::ListBox>()
					;// +ui::Height(style::Coord::Percent(100));
				{
					auto* rds = Allocate<MLV_R>();
					rds->msgs = &messages;
					auto* mlv = ctx->Make<ui::MessageLogView>();
					*mlv + ui::Height(style::Coord::Percent(100));
					mlv->GetLivenessToken();
					mlv->SetDataSource(rds);

					mlvR = mlv;
					mlvRtoken = mlv->GetLivenessToken();
				}
				ctx->Pop();
			}
			ctx->Pop();

			ctx->PushBox()
				+ ui::Layout(style::layouts::EdgeSlice())
				+ ui::Width(style::Coord::Percent(50))
				+ ui::Height(style::Coord::Percent(100));
			{
				ctx->Text("two lines, custom drawing");
				*ctx->Push<ui::ListBox>()
					;// +ui::Height(style::Coord::Percent(100));
				{
					auto* rds = Allocate<MLV_I>();
					rds->msgs = &messages;
					auto* mlv = ctx->Make<ui::MessageLogView>();
					*mlv + ui::Height(style::Coord::Percent(100));
					mlv->SetDataSource(rds);

					mlvI = mlv;
					mlvItoken = mlv->GetLivenessToken();
				}
				ctx->Pop();
			}
			ctx->Pop();
		}
		ctx->Pop();
	}

	std::vector<Message> messages;

	LivenessToken mlvRtoken;
	ui::MessageLogView* mlvR;
	LivenessToken mlvItoken;
	ui::MessageLogView* mlvI;
};
void Test_MessageLogView(UIContainer* ctx)
{
	ctx->Make<MessageLogViewTest>();
}
