
#include "pch.h"

#include "../Editors/TreeEditor.h"

#include "../Model/WIP.h"


static ui::DataCategoryTag DCT_Node[1];
struct BasicTreeNodeEditDemo : ui::Buildable
{
	struct TreeNode
	{
		~TreeNode()
		{
			for (auto* ch : children)
				delete ch;
		}

		std::string name;
		std::vector<TreeNode*> children;
	};

	struct TreeNodeUI : ui::Buildable
	{
		void Build() override
		{
			Subscribe(DCT_Node, tgt);
			ui::Push<ui::Panel>();

			if (parent)
			{
				ui::MakeWithText<ui::Button>("Delete").HandleEvent(ui::EventType::Activate) = [this](ui::Event&)
				{
					delete tgt;
					parent->children.erase(std::find(parent->children.begin(), parent->children.end(), tgt));
					ui::Notify(DCT_Node, parent);
				};
			}

			ui::imm::PropEditString("Name", tgt->name.c_str(), [this](const char* v) { tgt->name = v; });

			ui::Push<ui::Panel>();
			ui::Text("Children");

			for (size_t i = 0; i < tgt->children.size(); i++)
			{
				AddButton(i);
				auto& tnui = ui::Make<TreeNodeUI>();
				tnui.tgt = tgt->children[i];
				tnui.parent = tgt;
			}
			AddButton(tgt->children.size());

			ui::Pop();

			ui::Pop();
		}

		void AddButton(size_t inspos)
		{
			ui::MakeWithText<ui::Button>("Add").HandleEvent(ui::EventType::Activate) = [this, inspos](ui::Event&)
			{
				static int ctr = 0;
				auto* t = new TreeNode;
				t->name = "new child " + std::to_string(ctr++);
				tgt->children.insert(tgt->children.begin() + inspos, t);
				ui::Notify(DCT_Node, tgt);
			};
		}

		TreeNode* tgt = nullptr;
		TreeNode* parent = nullptr;
	};

	void Build() override
	{
		ui::Make<TreeNodeUI>().tgt = &root;
	}

	static TreeNode root;
};
BasicTreeNodeEditDemo::TreeNode BasicTreeNodeEditDemo::root = { "root" };
void Demo_BasicTreeNodeEdit()
{
	ui::Make<BasicTreeNodeEditDemo>();
}


struct CompactTreeNodeEditDemo : ui::Buildable
{
	struct Variable
	{
		std::string name;
		int value = 0;
	};
	struct ComputeInfo
	{
		std::vector<Variable>& variables;
		std::string& errors;
	};

	struct ExprNode
	{
		virtual void UI() = 0;
		virtual int Compute(ComputeInfo& cinfo) = 0;
	};
	static void NodeUI(ExprNode*& node);
	struct NumberNode : ExprNode
	{
		int number = 0;

		void UI() override
		{
			ui::LabeledProperty::Scope s("\b#");
			ui::imm::EditInt(s.label, number, { ui::SetWidth(50) });
		}
		int Compute(ComputeInfo&) override { return number; }
	};
	struct VarNode : ExprNode
	{
		std::string name;

		void UI() override
		{
			WPush<ui::StackLTRLayoutElement>();
			WText("Name:") + ui::SetPadding(5);
			ui::imm::EditString(name.c_str(), [this](const char* s) { name = s; }, { ui::SetWidth(50) });
			WPop();
		}
		int Compute(ComputeInfo& cinfo) override
		{
			for (auto& var : cinfo.variables)
				if (var.name == name)
					return var.value;
			cinfo.errors += "variable '" + name + "' not found\n";
			return 0;
		}
	};
	struct BinOpNode : ExprNode
	{
		ExprNode* a = nullptr;
		ExprNode* b = nullptr;

		void UI() override
		{
			WPush<ui::Panel>() + ui::SetPadding(2);
			WPush<ui::StackLTRLayoutElement>();
			NodeUI(a);
			WText(Name()) + ui::SetPadding(5);
			NodeUI(b);
			WPop();
			WPop();
		}
		int Compute(ComputeInfo& cinfo) override
		{
			int va = 0, vb = 0;
			if (a)
				va = a->Compute(cinfo);
			else
				cinfo.errors += "'" + std::string(Name()) + "': missing left operand";
			if (b)
				vb = b->Compute(cinfo);
			else
				cinfo.errors += "'" + std::string(Name()) + "': missing right operand";
			return Calculate(va, vb, cinfo);
		}
		virtual const char* Name() = 0;
		virtual int Calculate(int a, int b, ComputeInfo& cinfo) = 0;
	};
	struct AddNode : BinOpNode
	{
		const char* Name() override { return "+"; }
		int Calculate(int a, int b, ComputeInfo&) override { return a + b; }
	};
	struct SubNode : BinOpNode
	{
		const char* Name() override { return "-"; }
		int Calculate(int a, int b, ComputeInfo&) override { return a - b; }
	};
	struct MulNode : BinOpNode
	{
		const char* Name() override { return "*"; }
		int Calculate(int a, int b, ComputeInfo&) override { return a * b; }
	};
	struct DivNode : BinOpNode
	{
		const char* Name() override { return "/"; }
		int Calculate(int a, int b, ComputeInfo& cinfo) override
		{
			if (b == 0)
			{
				cinfo.errors += "division by zero\n";
				return 0;
			}
			return a / b;
		}
	};

	void Build() override
	{
		ui::LabeledProperty::Begin("Expression:");

		ui::Push<ui::StackTopDownLayoutElement>();
		NodeUI(root);
		ui::Pop();

		ui::LabeledProperty::End();

		ui::LabeledProperty::Begin("Variables:");
		ui::PushBox();

		Variable* del = nullptr;
		for (auto& v : variables)
		{
			ui::LabeledProperty::Begin();
			if (ui::imm::Button("X", { ui::SetWidth(20) }))
			{
				del = &v;
			}
			ui::imm::PropEditString("\bName", v.name.c_str(), [&v](const char* s) { v.name = s; });
			ui::imm::PropEditInt("\bValue", v.value);
			ui::LabeledProperty::End();
		}

		if (del)
		{
			variables.erase(variables.begin() + (del - variables.data()));
			Rebuild();
		}

		if (ui::imm::Button("Add"))
		{
			variables.push_back({ "unnamed", 0 });
		}

		ui::Pop();
		ui::LabeledProperty::End();

		ui::LabeledProperty::Begin("Computed value:");

		if (root)
		{
			std::string errors;
			ComputeInfo cinfo = { variables, errors };
			int val = root->Compute(cinfo);
			if (cinfo.errors.size())
				ui::Text("Errors during calculation:\n" + cinfo.errors);
			else
				ui::Text(std::to_string(val));
		}
		else
			ui::Text("- no expression -");

		ui::LabeledProperty::End();
	}

	std::vector<Variable> variables{ { "test", 5 } };
	ExprNode* root = nullptr;
};
void CompactTreeNodeEditDemo::NodeUI(ExprNode*& node)
{
	auto& b = WPush<ui::WrapperLTRLayoutElement>();
	if (node)
	{
		b.HandleEvent(ui::EventType::ContextMenu) = [&node](ui::Event& e)
		{
			ui::ContextMenu::Get().Add("Delete") = [&node, &e]() { delete node; node = nullptr; e.target->Rebuild(); };
			e.StopPropagation();
		};
		node->UI();
	}
	else
	{
		if (ui::imm::Button("+"))
		{
			ui::MenuItem items[] =
			{
				ui::MenuItem("Number").Func([&node, &b]() { node = new NumberNode; b.Rebuild(); }),
				ui::MenuItem("Variable").Func([&node, &b]() { node = new VarNode; b.Rebuild(); }),
				ui::MenuItem("Add").Func([&node, &b]() { node = new AddNode; b.Rebuild(); }),
				ui::MenuItem("Subtract").Func([&node, &b]() { node = new SubNode; b.Rebuild(); }),
				ui::MenuItem("Multiply").Func([&node, &b]() { node = new MulNode; b.Rebuild(); }),
				ui::MenuItem("Divide").Func([&node, &b]() { node = new DivNode; b.Rebuild(); }),
			};
			ui::Menu menu(items);
			menu.Show(&b);
		}
	}
	WPop();
}
void Demo_CompactTreeNodeEdit()
{
	ui::Make<CompactTreeNodeEditDemo>();
}


namespace script_tree {
static ui::DataCategoryTag DCT_TreeChanged[1];
struct Node
{
	std::vector<Node*> children;

	~Node()
	{
		for (Node* n : children)
			delete n;
	}
	virtual Node* CloneBase() = 0;
	Node* Clone()
	{
		Node* tmp = CloneBase();
		for (Node*& n : tmp->children)
			n = n->Clone();
		return tmp;
	}

	virtual void ItemUI()
	{
		ui::Text("Item");
	}
	void DoSubnodes()
	{
		for (auto* ch : children)
			ch->Do();
	}
	virtual void Do() = 0;
};

struct RepeatNode : Node
{
	int times = 5;
	Node* CloneBase() override { return new RepeatNode(*this); }
	virtual void ItemUI()
	{
		ui::imm::PropEditInt("\bRepeat #", times);
	}
	void Do() override
	{
		for (int i = 0; i < times; i++)
		{
			DoSubnodes();
		}
	}
};

struct PrintNode : Node
{
	std::string text;
	Node* CloneBase() override { return new PrintNode(*this); }
	virtual void ItemUI()
	{
		ui::imm::PropEditString("\bPrint text:", text.c_str(), [this](const char* v) { text = v; });
	}
	void Do() override
	{
		printf("PRINT: %s\n", text.c_str());
	}
};

struct Tree : ui::ITree
{
	std::vector<Node*> roots;
	Node* selected = nullptr;

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
	void RunScript()
	{
		for (Node* r : roots)
			r->Do();
	}

	struct NodeLoc
	{
		std::vector<Node*>* arr;
		size_t idx;

		Node* Get() const { return arr->at(idx); }
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
				fn(node);
		}
		else
		{
			auto loc = FindNode(path);
			for (auto* node : loc.Get()->children)
				fn(node);
		}
	}
	size_t GetChildCount(ui::TreePathRef path) override
	{
		if (path.empty())
			return roots.size();
		auto loc = FindNode(path);
		return loc.Get()->children.size();
	}

	void ClearSelection() override
	{
		selected = nullptr;
	}
	bool GetSelectionState(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		return loc.Get() == selected;
	}
	void SetSelectionState(ui::TreePathRef path, bool sel) override
	{
		auto loc = FindNode(path);
		if (sel)
			selected = loc.Get();
	}

	uint32_t lastVer = ui::ContextMenu::Get().GetVersion();
	void FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path) override
	{
		lastVer = mic.GetVersion();
		GenerateInsertMenu(mic, path);
	}
	void FillListContextMenu(ui::MenuItemCollection& mic) override
	{
		if (lastVer != mic.GetVersion())
			GenerateInsertMenu(mic, {});
		mic.basePriority += ui::MenuItemCollection::BASE_ADVANCE;
		mic.Add("Run script") = [this]() { RunScript(); };
	}
	void GenerateInsertMenu(ui::MenuItemCollection& mic, ui::TreePathRef path)
	{
		mic.basePriority += ui::MenuItemCollection::BASE_ADVANCE;
		mic.Add("Repeat") = [this, path]() { Insert(path, new RepeatNode); };
		mic.Add("Print") = [this, path]() { Insert(path, new PrintNode); };
	}
	void Insert(ui::TreePathRef path, Node* node)
	{
		if (path.empty())
			roots.push_back(node);
		else
			FindNode(path).Get()->children.push_back(node);
		ui::Notify(DCT_TreeChanged);
	}

	void Remove(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		delete loc.Get();
		if (selected == loc.Get())
			selected = nullptr;
		loc.arr->erase(loc.arr->begin() + loc.idx);
	}
	void Duplicate(ui::TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->push_back(loc.Get()->Clone());
	}
	void MoveTo(ui::TreePathRef node, ui::TreePathRef dest) override
	{
		auto srcLoc = FindNode(node);
		auto* srcNode = srcLoc.Get();
		srcLoc.arr->erase(srcLoc.arr->begin() + srcLoc.idx);

		auto destLoc = FindNode(dest);
		destLoc.arr->insert(destLoc.arr->begin() + destLoc.idx, srcNode);
	}
};
} // script_tree
struct ScriptTreeDemo : ui::Buildable
{
	void Build() override
	{
		Subscribe(script_tree::DCT_TreeChanged);
		auto& sp = ui::Push<ui::SplitPane>();
		{
			auto& te = ui::Make<ui::TreeEditor>();
			te.SetTree(&tree);
			te.itemUICallback = [](ui::TreeEditor* te, ui::TreePathRef path, void* data)
			{
				static_cast<script_tree::Node*>(data)->ItemUI();
			};
			te.HandleEvent(ui::EventType::SelectionChange) = [this](ui::Event&) { Rebuild(); };

			ui::PushBox();
			{
				if (tree.selected)
				{
					ui::Text(typeid(*tree.selected).name()) + ui::SetPadding(5);
					tree.selected->ItemUI();
				}
				else
				{
					ui::Text("No node selected...") + ui::SetPadding(5);
				}
			}
			ui::Pop();
		}
		sp.SetDirection(false);
		sp.SetSplits({ 0.5f });
	}
	void OnEvent(ui::Event& e) override
	{
		Buildable::OnEvent(e);
		if (e.type == ui::EventType::IMChange)
			Rebuild();
	}

	script_tree::Tree tree;
};
void Demo_ScriptTree()
{
	ui::Make<ScriptTreeDemo>();
}
