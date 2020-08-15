
#include "pch.h"


static ui::DataCategoryTag DCT_Node[1];
struct BasicTreeNodeEditDemo : ui::Node
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

	struct TreeNodeUI : ui::Node
	{
		void Render(UIContainer* ctx) override
		{
			Subscribe(DCT_Node, tgt);
			ctx->Push<ui::Panel>();

			if (parent)
			{
				ctx->MakeWithText<ui::Button>("Delete")->HandleEvent(UIEventType::Activate) = [this](UIEvent&)
				{
					delete tgt;
					parent->children.erase(std::find(parent->children.begin(), parent->children.end(), tgt));
					ui::Notify(DCT_Node, parent);
				};
			}

			ui::imm::PropEditString(ctx, "Name", tgt->name.c_str(), [this](const char* v) { tgt->name = v; });

			ctx->Push<ui::Panel>();
			ctx->Text("Children");

			for (size_t i = 0; i < tgt->children.size(); i++)
			{
				AddButton(ctx, i);
				auto* tnui = ctx->Make<TreeNodeUI>();
				tnui->tgt = tgt->children[i];
				tnui->parent = tgt;
			}
			AddButton(ctx, tgt->children.size());

			ctx->Pop();

			ctx->Pop();
		}

		void AddButton(UIContainer* ctx, size_t inspos)
		{
			ctx->MakeWithText<ui::Button>("Add")->HandleEvent(UIEventType::Activate) = [this, inspos](UIEvent&)
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

	void Render(UIContainer* ctx) override
	{
		ctx->Make<TreeNodeUI>()->tgt = &root;
	}

	static TreeNode root;
};
BasicTreeNodeEditDemo::TreeNode BasicTreeNodeEditDemo::root = { "root" };
void Demo_BasicTreeNodeEdit(UIContainer* ctx)
{
	ctx->Make<BasicTreeNodeEditDemo>();
}


struct CompactTreeNodeEditDemo : ui::Node
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
		virtual void UI(UIContainer* ctx) = 0;
		virtual int Compute(ComputeInfo& cinfo) = 0;
	};
	static void NodeUI(UIContainer* ctx, ExprNode*& node);
	struct NumberNode : ExprNode
	{
		int number = 0;

		void UI(UIContainer* ctx) override
		{
			auto& lbl = ui::Property::Label(ctx, "\b#");
			ui::imm::EditInt(ctx, &lbl, number, { ui::Width(50) });
		}
		int Compute(ComputeInfo&) override { return number; }
	};
	struct VarNode : ExprNode
	{
		std::string name;

		void UI(UIContainer* ctx) override
		{
			auto& lbl = ui::Property::Label(ctx, "\bName:");
			ui::imm::EditString(ctx, name.c_str(), [this](const char* s) { name = s; }, { ui::Width(50) });
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

		void UI(UIContainer* ctx) override
		{
			*ctx->Push<ui::Panel>() + ui::Padding(2) + ui::Layout(style::layouts::InlineBlock());
			NodeUI(ctx, a);
			ctx->Text(Name()) + ui::Padding(5);
			NodeUI(ctx, b);
			ctx->Pop();
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

	void Render(UIContainer* ctx) override
	{
		ui::Property::Begin(ctx, "Expression:");

		ctx->PushBox() + ui::Layout(style::layouts::Stack()) + ui::Width(style::Coord::Percent(100));
		NodeUI(ctx, root);
		ctx->Pop();

		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Variables:");
		ctx->PushBox();

		Variable* del = nullptr;
		for (auto& v : variables)
		{
			ui::Property::Begin(ctx);
			if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
			{
				del = &v;
			}
			ui::imm::PropEditString(ctx, "\bName", v.name.c_str(), [&v](const char* s) { v.name = s; });
			ui::imm::PropEditInt(ctx, "\bValue", v.value);
			ui::Property::End(ctx);
		}

		if (del)
		{
			variables.erase(variables.begin() + (del - variables.data()));
			Rerender();
		}

		if (ui::imm::Button(ctx, "Add"))
		{
			variables.push_back({ "unnamed", 0 });
		}

		ctx->Pop();
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Computed value:");

		if (root)
		{
			std::string errors;
			ComputeInfo cinfo = { variables, errors };
			int val = root->Compute(cinfo);
			if (cinfo.errors.size())
				ctx->Text("Errors during calculation:\n" + cinfo.errors);
			else
				ctx->Text(std::to_string(val));
		}
		else
			ctx->Text("- no expression -");

		ui::Property::End(ctx);
	}

	std::vector<Variable> variables{ { "test", 5 } };
	ExprNode* root = nullptr;
};
void CompactTreeNodeEditDemo::NodeUI(UIContainer* ctx, ExprNode*& node)
{
	auto& b = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock());
	if (node)
	{
		b.HandleEvent(UIEventType::ButtonUp) = [&node](UIEvent& e)
		{
			if (e.GetButton() == UIMouseButton::Right)
			{
				ui::MenuItem items[] =
				{
					ui::MenuItem("Delete").Func([&node, &e]() { delete node; node = nullptr; e.target->RerenderNode(); }),
				};
				ui::Menu menu(items);
				menu.Show(e.target);
				e.StopPropagation();
			}
		};
		node->UI(ctx);
	}
	else
	{
		if (ui::imm::Button(ctx, "+", { ui::Layout(style::layouts::InlineBlock()) }))
		{
			ui::MenuItem items[] =
			{
				ui::MenuItem("Number").Func([&node, &b]() { node = new NumberNode; b.RerenderNode(); }),
				ui::MenuItem("Variable").Func([&node, &b]() { node = new VarNode; b.RerenderNode(); }),
				ui::MenuItem("Add").Func([&node, &b]() { node = new AddNode; b.RerenderNode(); }),
				ui::MenuItem("Subtract").Func([&node, &b]() { node = new SubNode; b.RerenderNode(); }),
				ui::MenuItem("Multiply").Func([&node, &b]() { node = new MulNode; b.RerenderNode(); }),
				ui::MenuItem("Divide").Func([&node, &b]() { node = new DivNode; b.RerenderNode(); }),
			};
			ui::Menu menu(items);
			menu.Show(&b);
		}
	}
	ctx->Pop();
}
void Demo_CompactTreeNodeEdit(UIContainer* ctx)
{
	ctx->Make<CompactTreeNodeEditDemo>();
}

