
#include "pch.h"

#include <time.h>
#include <deque>
#include "../Editors/TreeEditor.h"


struct InfoDumpContextMenuSource : ui::IListContextMenuSource
{
	void FillItemContextMenu(ui::MenuItemCollection& mic, uintptr_t item, size_t col)
	{
		char bfr[128];
		snprintf(bfr, 128, "Info [item]: item=%" PRIuPTR " col=%zu", item, col);
		mic.Add(bfr, true);
	}
	void FillListContextMenu(ui::MenuItemCollection& mic)
	{
		char bfr[128];
		snprintf(bfr, 128, "Info [list]: version=%u", unsigned(mic.GetVersion()));
		mic.Add(bfr, true);
	}
}
g_infoDumpCMS;

static void TreeFillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path)
{
	std::string tmp = "Info [item]: path=[";
	for (auto pe : path)
	{
		if (tmp.back() != '[')
			tmp += ",";
		tmp += std::to_string(pe);
	}
	tmp += "]";
	mic.Add(tmp, true);
}

static void TreeFillListContextMenu(ui::MenuItemCollection& mic)
{
	char bfr[128];
	snprintf(bfr, 128, "Info [list]: version=%u", unsigned(mic.GetVersion()));
	mic.Add(bfr, true);
}


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
			ui::imm::EditBool(ctx, setSelectionStorage, "Storage");
		}

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::vector<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(vectordata)>>(vectordata), &vectorsel);
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::list<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(listdata)>>(listdata), &listsel);
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::deque<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(dequedata)>>(dequedata), &dequesel);
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("int[5]:");
		SeqEdit(ctx, Allocate<ui::BufferSequence<int, uint8_t>>(bufdata, buflen), &bufsel);
		ctx->Pop();

		ctx->Pop();

		ctx->Make<ui::DefaultOverlayRenderer>();
	}

	void SeqEdit(UIContainer* ctx, ui::ISequence* seq, ui::ISelectionStorage* sel)
	{
		ctx->Make<ui::SequenceEditor>()
			->SetSequence(seq)
			.SetSelectionStorage(setSelectionStorage ? sel : nullptr)
			.SetSelectionMode(selectionType)
			.SetContextMenuSource(&g_infoDumpCMS)
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
	bool setSelectionStorage = true;
};
void Test_SequenceEditors(UIContainer* ctx)
{
	ctx->Make<SequenceEditorsTest>();
}


namespace cpa { // child pointer array
struct Node
{
	int num = 0;
	bool sel = false;
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
	void ClearSelection()
	{
		sel = false;
		for (Node* n : children)
			n->ClearSelection();
	}
};
struct Tree : ui::ITree
{
	std::vector<Node*> roots;

	Tree()
	{
		SetDefault();
	}
	~Tree()
	{
		Clear();
	}

	void Clear()
	{
		for (Node* r : roots)
			delete r;
		roots.clear();
	}
	void SetDefault()
	{
		Clear();
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
	void SetFlat(int count)
	{
		Clear();
		for (int i = 0; i < count; i++)
			roots.push_back(new Node(i, {}));
	}
	void SetBranchy(int levels)
	{
		Clear();
		roots = { new Node(1, {}), new Node(2, {}) };
		for (int i = 0; i < levels; i++)
		{
			auto* a = new Node(i * 2 + 3, { roots[0]->Clone(), roots[1]->Clone() });
			auto* b = new Node(i * 2 + 4, { roots[0]->Clone(), roots[1]->Clone() });
			roots = { a, b };
		}
	}

	struct NodeLoc
	{
		std::vector<Node*>* arr;
		size_t idx;
	};
	NodeLoc FindNode(ui::TreePathRef path)
	{
		if (path.size() == 1)
			return { &roots, path.last() };
		else
		{
			auto parent = FindNode(path.without_last());
			return { &(*parent.arr)[parent.idx]->children, path.last() };
		}
	}

	void IterateChildren(ui::TreePathRef path, IterationFunc&& fn) override
	{
		if (path.empty())
		{
			for (auto* node : roots)
				fn(&node->num);
		}
		else
		{
			auto loc = FindNode(path);
			for (auto* node : loc.arr->at(loc.idx)->children)
				fn(&node->num);
		}
	}
	bool HasChildren(ui::TreePathRef path) override
	{
		return GetChildCount(path) != 0;
	}
	size_t GetChildCount(ui::TreePathRef path) override
	{
		if (path.empty())
			return roots.size();
		auto loc = FindNode(path);
		return loc.arr->at(loc.idx)->children.size();
	}

	void ClearSelection() override
	{
		for (auto* node : roots)
			node->ClearSelection();
	}
	bool GetSelectionState(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		return loc.arr->at(loc.idx)->sel;
	}
	void SetSelectionState(ui::TreePathRef path, bool sel) override
	{
		auto loc = FindNode(path);
		loc.arr->at(loc.idx)->sel = sel;
	}

	void FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path) override { TreeFillItemContextMenu(mic, path); }
	void FillListContextMenu(ui::MenuItemCollection& mic) override { TreeFillListContextMenu(mic); }

	void Remove(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		delete loc.arr->at(loc.idx);
		loc.arr->erase(loc.arr->begin() + loc.idx);
	}
	void Duplicate(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->push_back(loc.arr->at(loc.idx)->Clone());
	}
	void MoveTo(ui::TreePathRef node, ui::TreePathRef dest) override
	{
		auto srcLoc = FindNode(node);
		auto* srcNode = srcLoc.arr->at(srcLoc.idx);
		srcLoc.arr->erase(srcLoc.arr->begin() + srcLoc.idx);

		auto destLoc = FindNode(dest);
		destLoc.arr->insert(destLoc.arr->begin() + destLoc.idx, srcNode);
	}
};
} // cpa

namespace cva { // child value array
struct Node
{
	int num = 0;
	bool sel = false;
	std::vector<Node> children;

	Node(int n, const std::vector<Node>& ch) : num(n), children(ch) {}
	void ClearSelection()
	{
		sel = false;
		for (Node& n : children)
			n.ClearSelection();
	}
};
struct Tree : ui::ITree
{
	std::vector<Node> roots;

	Tree()
	{
		SetDefault();
	}

	void SetDefault()
	{
		roots =
		{
			Node(1,
			{
				Node(2,
				{
					Node(3,
					{
						Node(5, {}),
					}),
					Node(4, {}),
				}),
			}),
			Node(8, {}),
		};
	}
	void SetFlat(int count)
	{
		roots.clear();
		for (int i = 0; i < count; i++)
			roots.push_back(Node(i, {}));
	}
	void SetBranchy(int levels)
	{
		roots = { Node(1, {}), Node(2, {}) };
		for (int i = 0; i < levels; i++)
		{
			roots = { Node(i * 2 + 3, roots), Node(i * 2 + 4, roots) };
		}
	}

	struct NodeLoc
	{
		std::vector<Node>* arr;
		size_t idx;
	};
	NodeLoc FindNode(ui::TreePathRef path)
	{
		if (path.size() == 1)
			return { &roots, path.last() };
		else
		{
			auto parent = FindNode(path.without_last());
			return { &(*parent.arr)[parent.idx].children, path.last() };
		}
	}

	void IterateChildren(ui::TreePathRef path, IterationFunc&& fn) override
	{
		if (path.empty())
		{
			for (auto& node : roots)
				fn(&node.num);
		}
		else
		{
			auto loc = FindNode(path);
			for (auto& node : loc.arr->at(loc.idx).children)
				fn(&node.num);
		}
	}
	bool HasChildren(ui::TreePathRef path) override
	{
		return GetChildCount(path) != 0;
	}
	size_t GetChildCount(ui::TreePathRef path) override
	{
		if (path.empty())
			return roots.size();
		auto loc = FindNode(path);
		return loc.arr->at(loc.idx).children.size();
	}

	void ClearSelection() override
	{
		for (auto& node : roots)
			node.ClearSelection();
	}
	bool GetSelectionState(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		return loc.arr->at(loc.idx).sel;
	}
	void SetSelectionState(ui::TreePathRef path, bool sel) override
	{
		auto loc = FindNode(path);
		loc.arr->at(loc.idx).sel = sel;
	}

	void FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path) override { TreeFillItemContextMenu(mic, path); }
	void FillListContextMenu(ui::MenuItemCollection& mic) override { TreeFillListContextMenu(mic); }

	void Remove(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->erase(loc.arr->begin() + loc.idx);
	}
	void Duplicate(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->push_back(loc.arr->at(loc.idx));
	}
	void MoveTo(ui::TreePathRef node, ui::TreePathRef dest) override
	{
		auto srcLoc = FindNode(node);
		auto srcNode = std::move(srcLoc.arr->at(srcLoc.idx));
		srcLoc.arr->erase(srcLoc.arr->begin() + srcLoc.idx);

		auto destLoc = FindNode(dest);
		destLoc.arr->insert(destLoc.arr->begin() + destLoc.idx, std::move(srcNode));
	}
};
} // cva

struct TreeEditorsTest : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		{
			ui::Property::Scope ps(ctx);
			if (ui::imm::Button(ctx, "Default"))
			{
				cpaTree.SetDefault();
				cvaTree.SetDefault();
			}
			if (ui::imm::Button(ctx, "Flat 10"))
			{
				cpaTree.SetFlat(10);
				cvaTree.SetFlat(10);
			}
			if (ui::imm::Button(ctx, "Flat 100"))
			{
				cpaTree.SetFlat(100);
				cvaTree.SetFlat(100);
			}
			if (ui::imm::Button(ctx, "Flat 1K"))
			{
				cpaTree.SetFlat(1000);
				cvaTree.SetFlat(1000);
			}
			if (ui::imm::Button(ctx, "Branchy 2"))
			{
				cpaTree.SetBranchy(2);
				cvaTree.SetBranchy(2);
			}
			if (ui::imm::Button(ctx, "Branchy 4"))
			{
				cpaTree.SetBranchy(4);
				cvaTree.SetBranchy(4);
			}
			if (ui::imm::Button(ctx, "Branchy 8"))
			{
				cpaTree.SetBranchy(8);
				cvaTree.SetBranchy(8);
			}
		};

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		ctx->PushBox() + ui::Width(style::Coord::Percent(33));
		ctx->Text("child pointer array:");
		TreeEdit(ctx, &cpaTree);
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(33));
		ctx->Text("child value array:");
		TreeEdit(ctx, &cvaTree);
		ctx->Pop();

		ctx->Pop();

		ctx->Make<ui::DefaultOverlayRenderer>();
	}

	void TreeEdit(UIContainer* ctx, ui::ITree* itree)
	{
		ctx->Make<ui::TreeEditor>()
			->SetTree(itree)
			.itemUICallback = [](UIContainer* ctx, ui::TreeEditor* te, ui::TreePathRef path, void* data)
		{
			ui::imm::PropEditInt(ctx, "\bvalue", *static_cast<int*>(data));
		};
	}

	cpa::Tree cpaTree;
	cva::Tree cvaTree;
};
void Test_TreeEditors(UIContainer* ctx)
{
	ctx->Make<TreeEditorsTest>();
}


struct RandomNumberDataSource : ui::TableDataSource, ui::ISelectionStorage
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
	virtual bool GetSelectionState(uintptr_t item) { return selRows.count(item); }
	virtual void SetSelectionState(uintptr_t item, bool sel)
	{
		if (sel)
			selRows.insert(item);
		else
			selRows.erase(item);
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
		tv->SetSelectionStorage(&g_randomNumbers);
		tv->SetDataSource(&g_randomNumbers);
		tv->SetContextMenuSource(&g_infoDumpCMS);
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
