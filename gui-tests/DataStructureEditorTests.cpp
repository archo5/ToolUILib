
#include "pch.h"

#include <time.h>
#include <vector>
#include <list>
#include <deque>
#include "../lib-src/Editors/TreeEditor.h"
#include "../lib-src/Editors/CurveEditor.h"


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


struct SequenceEditorsTest : ui::Buildable
{
	ui::RectAnchoredPlacement parts[5];

	void Build() override
	{
		WPush<ui::LayerLayoutElement>();

		WPush<ui::EdgeSliceLayoutElement>();
		WPush<ui::StackLTRLayoutElement>();

		if (ui::imm::Button("Reset"))
		{
			arraydata = { 1, 2, 3, 4 };
			vectordata = { 1, 2, 3, 4 };
			listdata = { 1, 2, 3, 4 };
			dequedata = { 1, 2, 3, 4 };
		}
		if (ui::imm::Button("100 values"))
			AdjustSizeAll(100);
		if (ui::imm::Button("1000 values"))
			AdjustSizeAll(1000);
		if (ui::imm::Button("10000 values"))
			AdjustSizeAll(10000);

		WPop();

		{
			ui::LabeledProperty::Scope ps("\bSelection type:");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::None, "None");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::Single, "Single");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::MultipleToggle, "Multiple (toggle)");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::Multiple, "Multiple");
			ui::imm::EditBool(setSelectionStorage, "Storage");
		}

		for (int i = 0; i < 5; i++)
		{
			parts[i].anchor.x0 = i / 5.0f;
			parts[i].anchor.x1 = (i + 1) / 5.0f;
		}

		auto& ple = WPush<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		WMake<ui::FillerElement>();

		tmpl->placement = &parts[0];
		tmpl->measure = false;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::Text("Array<int>:");
		SeqEdit(UI_BUILD_ALLOC(ui::ArraySequence<decltype(arraydata)>)(arraydata), &arraysel);
		ui::Pop();

		tmpl->placement = &parts[1];
		tmpl->measure = false;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::Text("std::vector<int>:");
		SeqEdit(UI_BUILD_ALLOC(ui::StdSequence<decltype(vectordata)>)(vectordata), &vectorsel);
		ui::Pop();

		tmpl->placement = &parts[2];
		tmpl->measure = false;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::Text("std::list<int>:");
		SeqEdit(UI_BUILD_ALLOC(ui::StdSequence<decltype(listdata)>)(listdata), &listsel);
		ui::Pop();

		tmpl->placement = &parts[3];
		tmpl->measure = false;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::Text("std::deque<int>:");
		SeqEdit(UI_BUILD_ALLOC(ui::StdSequence<decltype(dequedata)>)(dequedata), &dequesel);
		ui::Pop();

		tmpl->placement = &parts[4];
		tmpl->measure = false;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::Text("int[5]:");
		SeqEdit(UI_BUILD_ALLOC(ui::BufferSequence<int, uint8_t>)(bufdata, buflen), &bufsel);
		ui::Pop();

		WPop(); // PlacementLayoutElement
		WPop(); // EdgeSliceLayoutElement

		WMake<ui::DefaultOverlayBuilder>();
		WPop(); // TODO: hack (?) for overlay builder above
	}

	void SeqEdit(ui::ISequence* seq, ui::ISelectionStorage* sel)
	{
		ui::Make<ui::SequenceEditor>()
			.SetSequence(seq)
			.SetSelectionStorage(setSelectionStorage ? sel : nullptr)
			.SetSelectionMode(selectionType)
			.SetContextMenuSource(&g_infoDumpCMS)
			.itemUICallback = [](ui::SequenceEditor* se, size_t idx, void* ptr)
		{
			ui::imm::PropEditInt("\bvalue", *static_cast<int*>(ptr));
		};
	}

	void AdjustSizeAll(size_t size)
	{
		while (arraydata.Size() > size)
			arraydata.RemoveLast();
		while (arraydata.Size() < size)
			arraydata.Append(arraydata.size() + 1);

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

	ui::Array<int> arraydata{ 1, 2, 3, 4 };
	std::vector<int> vectordata{ 1, 2, 3, 4 };
	std::list<int> listdata{ 1, 2, 3, 4 };
	std::deque<int> dequedata{ 1, 2, 3, 4 };
	int bufdata[5] = { 1, 2, 3, 4, 9999 };
	uint8_t buflen = 4;

	ui::BasicSelection arraysel;
	ui::BasicSelection vectorsel;
	ui::BasicSelection listsel;
	ui::BasicSelection dequesel;
	ui::BasicSelection bufsel;

	ui::SelectionMode selectionType = ui::SelectionMode::Single;
	bool setSelectionStorage = true;
};
void Test_SequenceEditors()
{
	ui::Make<SequenceEditorsTest>();
}


namespace cpa { // child pointer array
struct Node
{
	int num = 0;
	bool sel = false;
	ui::Array<Node*> children;

	Node(int n, const ui::Array<Node*>& ch) : num(n), children(ch) {}
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
	ui::Array<Node*> roots;

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
		roots.Clear();
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
			roots.Append(new Node(i, {}));
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
		ui::Array<Node*>* arr;
		size_t idx;
	};
	NodeLoc FindNode(ui::TreePathRef path)
	{
		if (path.size() == 1)
			return { &roots, path.Last() };
		else
		{
			auto parent = FindNode(path.WithoutLast());
			return { &(*parent.arr)[parent.idx]->children, path.Last() };
		}
	}

	void IterateChildren(ui::TreePathRef path, IterationFunc&& fn) override
	{
		if (path.IsEmpty())
		{
			for (auto* node : roots)
				fn(&node->num);
		}
		else
		{
			auto loc = FindNode(path);
			for (auto* node : loc.arr->At(loc.idx)->children)
				fn(&node->num);
		}
	}
	size_t GetChildCount(ui::TreePathRef path) override
	{
		if (path.IsEmpty())
			return roots.size();
		auto loc = FindNode(path);
		return loc.arr->At(loc.idx)->children.size();
	}

	void ClearSelection() override
	{
		for (auto* node : roots)
			node->ClearSelection();
	}
	bool GetSelectionState(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		return loc.arr->At(loc.idx)->sel;
	}
	void SetSelectionState(ui::TreePathRef path, bool sel) override
	{
		auto loc = FindNode(path);
		loc.arr->At(loc.idx)->sel = sel;
	}

	void FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path) override { TreeFillItemContextMenu(mic, path); }
	void FillListContextMenu(ui::MenuItemCollection& mic) override { TreeFillListContextMenu(mic); }

	void Remove(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		delete loc.arr->At(loc.idx);
		loc.arr->RemoveAt(loc.idx);
	}
	void Duplicate(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->Append(loc.arr->At(loc.idx)->Clone());
	}
	void MoveTo(ui::TreePathRef node, ui::TreePathRef dest) override
	{
		auto srcLoc = FindNode(node);
		auto* srcNode = srcLoc.arr->At(srcLoc.idx);
		srcLoc.arr->RemoveAt(srcLoc.idx);

		auto destLoc = FindNode(dest);
		destLoc.arr->InsertAt(destLoc.idx, srcNode);
	}
};
} // cpa

namespace cva { // child value array
struct Node
{
	int num = 0;
	bool sel = false;
	ui::Array<Node> children;

	Node(int n, const ui::Array<Node>& ch) : num(n), children(ch) {}
	void ClearSelection()
	{
		sel = false;
		for (Node& n : children)
			n.ClearSelection();
	}
};
struct Tree : ui::ITree
{
	ui::Array<Node> roots;

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
		roots.Clear();
		for (int i = 0; i < count; i++)
			roots.Append(Node(i, {}));
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
		ui::Array<Node>* arr;
		size_t idx;
	};
	NodeLoc FindNode(ui::TreePathRef path)
	{
		if (path.size() == 1)
			return { &roots, path.Last() };
		else
		{
			auto parent = FindNode(path.WithoutLast());
			return { &(*parent.arr)[parent.idx].children, path.Last() };
		}
	}

	void IterateChildren(ui::TreePathRef path, IterationFunc&& fn) override
	{
		if (path.IsEmpty())
		{
			for (auto& node : roots)
				fn(&node.num);
		}
		else
		{
			auto loc = FindNode(path);
			for (auto& node : loc.arr->At(loc.idx).children)
				fn(&node.num);
		}
	}
	size_t GetChildCount(ui::TreePathRef path) override
	{
		if (path.IsEmpty())
			return roots.size();
		auto loc = FindNode(path);
		return loc.arr->At(loc.idx).children.size();
	}

	void ClearSelection() override
	{
		for (auto& node : roots)
			node.ClearSelection();
	}
	bool GetSelectionState(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		return loc.arr->At(loc.idx).sel;
	}
	void SetSelectionState(ui::TreePathRef path, bool sel) override
	{
		auto loc = FindNode(path);
		loc.arr->At(loc.idx).sel = sel;
	}

	void FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path) override { TreeFillItemContextMenu(mic, path); }
	void FillListContextMenu(ui::MenuItemCollection& mic) override { TreeFillListContextMenu(mic); }

	void Remove(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->RemoveAt(loc.idx);
	}
	void Duplicate(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->Append(loc.arr->At(loc.idx));
	}
	void MoveTo(ui::TreePathRef node, ui::TreePathRef dest) override
	{
		auto srcLoc = FindNode(node);
		auto srcNode = std::move(srcLoc.arr->At(srcLoc.idx));
		srcLoc.arr->RemoveAt(srcLoc.idx);

		auto destLoc = FindNode(dest);
		destLoc.arr->InsertAt(destLoc.idx, std::move(srcNode));
	}
};
} // cva

struct TreeEditorsTest : ui::Buildable
{
	ui::RectAnchoredPlacement parts[2];

	void Build() override
	{
		WPush<ui::LayerLayoutElement>();

		WPush<ui::EdgeSliceLayoutElement>();
		{
			WPush<ui::StackExpandLTRLayoutElement>();
			if (ui::imm::Button("Default"))
			{
				cpaTree.SetDefault();
				cvaTree.SetDefault();
			}
			if (ui::imm::Button("Flat 10"))
			{
				cpaTree.SetFlat(10);
				cvaTree.SetFlat(10);
			}
			if (ui::imm::Button("Flat 100"))
			{
				cpaTree.SetFlat(100);
				cvaTree.SetFlat(100);
			}
			if (ui::imm::Button("Flat 1K"))
			{
				cpaTree.SetFlat(1000);
				cvaTree.SetFlat(1000);
			}
			if (ui::imm::Button("Branchy 2"))
			{
				cpaTree.SetBranchy(2);
				cvaTree.SetBranchy(2);
			}
			if (ui::imm::Button("Branchy 4"))
			{
				cpaTree.SetBranchy(4);
				cvaTree.SetBranchy(4);
			}
			if (ui::imm::Button("Branchy 8"))
			{
				cpaTree.SetBranchy(8);
				cvaTree.SetBranchy(8);
			}
			WPop();
		};

		for (int i = 0; i < 2; i++)
		{
			parts[i].anchor.x0 = i / 3.0f;
			parts[i].anchor.x1 = (i + 1) / 3.0f;
		}

		auto& ple = WPush<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		WMake<ui::FillerElement>();

		tmpl->placement = &parts[0];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		WText("child pointer array:");
		TreeEdit(&cpaTree);
		WPop();

		tmpl->placement = &parts[1];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		WText("child value array:");
		TreeEdit(&cvaTree);
		WPop();

		WPop(); // PlacementLayoutElement
		WPop(); // EdgeSliceLayoutElement

		WMake<ui::DefaultOverlayBuilder>();
		WPop(); // TODO: hack (?) for overlay builder above
	}

	void TreeEdit(ui::ITree* itree)
	{
		WMake<ui::TreeEditor>()
			.SetTree(itree)
			.itemUICallback = [](ui::TreeEditor* te, ui::TreePathRef path, void* data)
		{
			ui::imm::PropEditInt("\bvalue", *static_cast<int*>(data));
		};
	}

	cpa::Tree cpaTree;
	cva::Tree cvaTree;
};
void Test_TreeEditors()
{
	WMake<TreeEditorsTest>();
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

	virtual void ClearSelection() { selRows.Clear(); }
	virtual bool GetSelectionState(uintptr_t item) { return selRows.Contains(item); }
	virtual void SetSelectionState(uintptr_t item, bool sel)
	{
		if (sel)
			selRows.Insert(item);
		else
			selRows.Remove(item);
	}

	ui::HashSet<size_t> selRows;
}
g_randomNumbers;

struct TableViewTest : ui::Buildable
{
	void Build() override
	{
		ui::Push<ui::EdgeSliceLayoutElement>();

		{
			ui::LabeledProperty::Scope ps("\bSelection type:");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::None, "None");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::Single, "Single");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::MultipleToggle, "Multiple (toggle)");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::Multiple, "Multiple");
		}

		auto& tv = ui::Make<ui::TableView>();
		tv.SetSelectionMode(selectionType);
		tv.SetSelectionStorage(&g_randomNumbers);
		tv.SetDataSource(&g_randomNumbers);
		tv.SetContextMenuSource(&g_infoDumpCMS);

		ui::Pop();
	}

	ui::SelectionMode selectionType = ui::SelectionMode::Single;
};
void Test_TableView()
{
	ui::Make<TableViewTest>();
}


struct RandomNumberTreeDataSource : ui::TreeDataSource, ui::ISelectionStorage
{
	ui::HashSet<size_t> selRows;

	size_t GetNumCols() { return 5; }
	std::string GetColName(size_t col) { return std::to_string(col + 1); }
	size_t GetChildCount(uintptr_t id)
	{
		if (id == ROOT)
			return 1000;
		if (id >= 11000)
			return 0;
		return 10;
	}
	uintptr_t GetChild(uintptr_t id, size_t which)
	{
		if (id == ROOT)
			return which;
		// level 1
		if (id < 1000)
			return 1000 + id * 10 + which;
		// level 2
		id -= 1000;
		if (id < 10000)
			return 11000 + id * 10 + which;

		return 0;
	}
	std::string GetText(uintptr_t id, size_t col)
	{
		return std::to_string(((unsigned(id) * 5 + unsigned(col)) + 1013904223U) * 1664525U);
	}

	virtual void ClearSelection() { selRows.Clear(); }
	virtual bool GetSelectionState(uintptr_t item) { return selRows.Contains(item); }
	virtual void SetSelectionState(uintptr_t item, bool sel)
	{
		if (sel)
			selRows.Insert(item);
		else
			selRows.Remove(item);
	}
}
g_randomNumberTree;

struct TreeViewTest : ui::Buildable
{
	void Build() override
	{
		ui::Push<ui::EdgeSliceLayoutElement>();

		{
			ui::LabeledProperty::Scope ps("\bSelection type:");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::None, "None");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::Single, "Single");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::MultipleToggle, "Multiple (toggle)");
			ui::imm::RadioButton(selectionType, ui::SelectionMode::Multiple, "Multiple");
		}

		auto& tv = ui::Make<ui::TableView>();
		tv.SetSelectionMode(selectionType);
		tv.SetSelectionStorage(&g_randomNumberTree);
		tv.SetDataSource(&g_randomNumberTree);
		tv.SetContextMenuSource(&g_infoDumpCMS);
		tv.CalculateColumnWidths();

		ui::Pop();
	}

	ui::SelectionMode selectionType = ui::SelectionMode::Single;
};
void Test_TreeView()
{
	ui::Make<TreeViewTest>();
}


struct FileTreeViewTest : ui::Buildable
{
	void Build() override
	{
		ui::Push<ui::EdgeSliceLayoutElement>();

		auto& tv = ui::Make<ui::TableView>();
		tv.enableRowHeader = false;
		tv.SetSelectionMode(ui::SelectionMode::Single);
		tv.SetSelectionStorage(&fds);
		tv.SetDataSource(&fds);
		tv.SetContextMenuSource(&g_infoDumpCMS);
		tv.CalculateColumnWidths();
		ui::BuildMulticastDelegateAdd(fds.onChange, [&tv]() { tv.GetNativeWindow()->InvalidateAll(); });

		ui::Pop();
	}

	ui::FileTreeDataSource fds = { "build" };
};
void Test_FileTreeView()
{
	ui::Make<FileTreeViewTest>();
}


struct MessageLogViewTest : ui::Buildable
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

		ui::Array<Message>* msgs;
	};
	struct MLV_R : MLV_Common {};
	struct MLV_I : MLV_Common
	{
		size_t GetNumLines() override { return 2; }
		float GetMessageWidth(UIObject* context, size_t msg) override
		{
			return MLV_Common::GetMessageWidth(context, msg) + GetMessageHeight(context);
		}
		void OnDrawMessage(UIObject* context, size_t msg, ui::UIRect area) override
		{
			float h = area.GetHeight();
			ui::draw::RectCol(area.x0 + 4, area.y0 + 4, area.x0 + h - 4, area.y1 - 4, ui::Color4b(200, 200, 200));
			area.x0 += h;
			MLV_Common::OnDrawMessage(context, msg, area);
		}
	};

	ui::RectAnchoredPlacement parts[2];

	MessageLogViewTest()
	{
		for (int i = 0; i < 2; i++)
		{
			parts[i].anchor.x0 = i / 2.0f;
			parts[i].anchor.x1 = (i + 1) / 2.0f;
			parts[i].anchor.y1 = 0.5f;
		}

		mlvRData.msgs = &messages;
		mlvIData.msgs = &messages;

		mlvR = ui::CreateUIObject<ui::MessageLogView>();
		mlvI = ui::CreateUIObject<ui::MessageLogView>();

		mlvR->SetDataSource(&mlvRData);
		mlvI->SetDataSource(&mlvIData);

		// TODO: scrolling to end in ctor results in scrolling a page or so too far - should probably fix that?
		AddMessages(2345);
	}

	~MessageLogViewTest()
	{
		ui::DeleteUIObject(mlvR);
		ui::DeleteUIObject(mlvI);
	}

	void AddMessages(int n)
	{
		bool isAtEndR = mlvR->IsAtEnd();
		bool isAtEndI = mlvI->IsAtEnd();

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

			messages.Append(msg);
		}

		if (isAtEndR)
			mlvR->ScrollToEnd();
		if (isAtEndI)
			mlvI->ScrollToEnd();
	}

	void OnEnable() override
	{
		Buildable::OnEnable();
	}

	void Build() override
	{
		WPush<ui::EdgeSliceLayoutElement>();

		WPush<ui::StackExpandLTRLayoutElement>();
		{
			if (ui::imm::Button("Clear"))
				messages.Clear();
			if (ui::imm::Button("Add 1 line"))
				AddMessages(1);
			if (ui::imm::Button("Add 10"))
				AddMessages(10);
			if (ui::imm::Button("Add 100"))
				AddMessages(100);
			if (ui::imm::Button("Add 1K"))
				AddMessages(1000);
			if (ui::imm::Button("Add 10K"))
				AddMessages(10000);
		};
		WPop();

		auto& ple = WPush<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		WMake<ui::FillerElement>();

		{
			tmpl->placement = &parts[0];
			tmpl->measure = false;
			ui::Push<ui::EdgeSliceLayoutElement>();
			{
				ui::Text("single line");
				ui::Push<ui::ListBoxFrame>();
				{
					ui::Add(mlvR);
				}
				ui::Pop();
			}
			ui::Pop();

			tmpl->placement = &parts[1];
			tmpl->measure = false;
			ui::Push<ui::EdgeSliceLayoutElement>();
			{
				ui::Text("two lines, custom drawing");
				ui::Push<ui::ListBoxFrame>();
				{
					ui::Add(mlvI);
				}
				ui::Pop();
			}
			ui::Pop();
		}

		WPop(); // PlacementLayoutElement
		WPop(); // EdgeSliceLayoutElement
	}

	ui::Array<Message> messages;

	MLV_R mlvRData;
	MLV_I mlvIData;
	ui::MessageLogView* mlvR;
	ui::MessageLogView* mlvI;
};
void Test_MessageLogView()
{
	ui::Make<MessageLogViewTest>();
}


struct CurveEditorTest : ui::Buildable
{
	void OnEnable() override
	{
		basicLinear01Curve.points =
		{
			{ 0.0f, 0.3f },
			{ 1.3f, 0.7f },
			{ 3.0f, 0.0f },
		};

		sequence01Curve.points =
		{
			{ 0, 0, 0, ui::Sequence01Curve::Mode::Hold, 0 },
			{ 1, 1, 1, ui::Sequence01Curve::Mode::SinglePowerCurve, 0 },
			{ 1, 2, 0, ui::Sequence01Curve::Mode::DoublePowerCurve, 0 },
			{ 1, 3, 1, ui::Sequence01Curve::Mode::SawWave, 0 },
		};
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		WPush<ui::SizeConstraintElement>().SetHeight(50);
		auto& ce = WMake<ui::CurveEditorElement>();
		WPop();
		ce.curveView = &basicLinear01Curve;
		ce.viewport = { 0, 0, 5, 1 };

		WPush<ui::SizeConstraintElement>().SetHeight(50);
		auto& ce3 = ui::Make<ui::CurveEditorElement>();
		WPop();
		auto* s01cv = UI_BUILD_ALLOC(ui::Sequence01CurveView)();
		s01cv->curve = &sequence01Curve;
		ce3.curveView = s01cv;
		ce3.viewport = { 0, 0, 5, 1 };

		WPush<ui::SizeConstraintElement>().SetHeight(200);
		auto& ce2 = ui::Make<ui::CurveEditorElement>();
		WPop();
		auto* cnrcv = UI_BUILD_ALLOC(ui::CubicNormalizedRemapCurveView)();
		cnrcv->curve = &cubicNormalizedRemapCurve;
		ce2.curveView = cnrcv;

		WPop();
	}
	ui::BasicLinear01Curve basicLinear01Curve;
	ui::Sequence01Curve sequence01Curve;
	ui::CubicNormalizedRemapCurve cubicNormalizedRemapCurve;
};
void Test_CurveEditor()
{
	ui::Make<CurveEditorTest>();
}
