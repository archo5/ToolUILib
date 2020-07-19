
#include "pch.h"

#include "../GUI.h"


struct OpenClose : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>();

		auto* cb = ctx->Make<ui::Checkbox>()->Init(open);
		cb->HandleEvent(UIEventType::Change) = [this, cb](UIEvent&) { open = !open; Rerender(); };

		ctx->MakeWithText<ui::Button>("Open")->HandleEvent(UIEventType::Activate) = [this](UIEvent&) { open = true; Rerender(); };

		ctx->MakeWithText<ui::Button>("Close")->HandleEvent(UIEventType::Activate) = [this](UIEvent&) { open = false; Rerender(); };

		if (open)
		{
			ctx->Push<ui::Panel>();
			ctx->Text("It is open!");
			ctx->Pop();
		}

		ctx->Pop();
	}
	void OnSerialize(IDataSerializer& s) override
	{
		s << open;
	}

	bool open = false;
};

struct EdgeSliceTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		auto s = ctx->Push<ui::Panel>()->GetStyle();
		s.SetLayout(style::layouts::EdgeSlice());
		s.SetBoxSizing(style::BoxSizing::BorderBox);
		s.SetMargin(0);
		s.SetHeight(style::Coord::Percent(100));

		ctx->MakeWithText<ui::Button>("Top")->GetStyle().SetEdge(style::Edge::Top);
		ctx->MakeWithText<ui::Button>("Right")->GetStyle().SetEdge(style::Edge::Right);
		ctx->MakeWithText<ui::Button>("Bottom")->GetStyle().SetEdge(style::Edge::Bottom);
		ctx->MakeWithText<ui::Button>("Left")->GetStyle().SetEdge(style::Edge::Left);

		ctx->Pop();
	}
};

struct DragDropTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Text("Transfer countables");

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < 3; i++)
		{
			auto* btn = ctx->MakeWithText<ui::Button>("Slot " + std::to_string(i + 1) + ": " + std::to_string(slots[i]));
			if (btn->flags & UIObject_DragHovered)
			{
				*btn + ui::Padding(4, 4, 10, 4);
			}
			btn->SetInputDisabled(slots[i] == 0);
			btn->HandleEvent() = [this, i, btn](UIEvent& e)
			{
				struct Data : ui::DragDropData
				{
					Data(int f) : ui::DragDropData("slot item"), from(f) {}
					int from;
				};
				if (e.context->DragCheck(e, UIMouseButton::Left))
				{
					if (slots[i] > 0)
					{
						ui::DragDrop::SetData(new Data(i));
					}
				}
				else if (e.type == UIEventType::DragDrop)
				{
					if (auto* ddd = static_cast<Data*>(ui::DragDrop::GetData("slot item")))
					{
						slots[i]++;
						slots[ddd->from]--;
						e.StopPropagation();
						Rerender();
					}
				}
				else if (e.type == UIEventType::DragEnter)
				{
					Rerender();
				}
				else if (e.type == UIEventType::DragLeave)
				{
					Rerender();
				}
			};
		}
		ctx->Pop();

		ctx->Text("Slide-reorder (instant)");

		ctx->Push<ui::ListBox>();
		for (int i = 0; i < 4; i++)
		{
			struct Data : ui::DragDropData
			{
				Data(int f) : ui::DragDropData("current location"), at(f) {}
				int at;
			};
			ctx->Push<ui::Selectable>()->HandleEvent() = [this, i](UIEvent& e)
			{
				if (e.context->DragCheck(e, UIMouseButton::Left))
				{
					ui::DragDrop::SetData(new Data(i));
					e.context->ReleaseMouse();
				}
				else if (e.type == UIEventType::DragEnter)
				{
					if (auto* ddd = static_cast<Data*>(ui::DragDrop::GetData("current location")))
					{
						// TODO transfer click state to current object
						if (ddd->at != i)
						{
							e.context->MoveClickTo(e.current);
						}
						while (ddd->at < i)
						{
							int pos = ddd->at++;
							std::swap(iids[pos], iids[pos + 1]);
						}
						while (ddd->at > i)
						{
							int pos = ddd->at--;
							std::swap(iids[pos], iids[pos - 1]);
						}
					}
					Rerender();
				}
				else if (e.type == UIEventType::DragLeave)
				{
					Rerender();
				}
			};
			char bfr[32];
			snprintf(bfr, 32, "item %d", iids[i]);
			if (auto* dd = static_cast<Data*>(ui::DragDrop::GetData("current location")))
				if (dd->at == i)
					strcat(bfr, " [dragging]");
			ctx->Text(bfr);
			ctx->Pop();
		}
		ctx->Pop();

		ctx->Text("File receiver");

		ctx->Push<ui::ListBox>()->HandleEvent(UIEventType::DragDrop) = [this](UIEvent& e)
		{
			if (auto* ddd = static_cast<ui::DragDropFiles*>(ui::DragDrop::GetData(ui::DragDropFiles::NAME)))
			{
				filePaths = ddd->paths;
				Rerender();
			}
		};
		if (filePaths.empty())
		{
			ctx->Text("Drop files here");
		}
		else
		{
			for (const auto& path : filePaths)
			{
				ctx->Text(path);
			}
		}
		ctx->Pop();

		ctx->Make<ui::DefaultOverlayRenderer>();
	}
	void OnSerialize(IDataSerializer& s) override
	{
		s << slots;
		s << iids;
	}

	int slots[3] = { 5, 2, 0 };
	int iids[4] = { 1, 2, 3, 4 };
	std::vector<std::string> filePaths;
};


static ui::DataCategoryTag DCT_Node[1];
struct NodeEditTest : ui::Node
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
NodeEditTest::TreeNode NodeEditTest::root = { "root" };


struct CompactNodeEditTest : ui::Node
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
void CompactNodeEditTest::NodeUI(UIContainer* ctx, ExprNode*& node)
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


struct SubUITest : ui::Node
{
	void OnPaint() override
	{
		style::PaintInfo info(this);
		ui::Theme::current->textBoxBase->paint_func(info);
		auto r = finalRectC;

		GL::BatchRenderer br;

		GL::SetTexture(0);
		br.Begin();
		if (subui.IsPressed(0))
			br.SetColor(1, 0, 0, 0.5f);
		else if (subui.IsHovered(0))
			br.SetColor(0, 1, 0, 0.5f);
		else
			br.SetColor(0, 0, 1, 0.5f);
		br.Quad(r.x0, r.y0, r.x0 + 50, r.y0 + 50, 0, 0, 1, 1);
		br.End();
		DrawTextLine(r.x0 + 25 - GetTextWidth("Button") / 2, r.y0 + 25 + GetFontHeight() / 2, "Button", 1, 1, 1);

		GL::SetTexture(0);
		br.Begin();
		auto ddr = UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10);
		if (subui.IsPressed(1))
			br.SetColor(1, 0, 1, 0.5f);
		else if (subui.IsHovered(1))
			br.SetColor(1, 1, 0, 0.5f);
		else
			br.SetColor(0, 1, 1, 0.5f);
		br.Quad(ddr.x0, ddr.y0, ddr.x1, ddr.y1, 0, 0, 1, 1);
		br.End();
		DrawTextLine(r.x0 + draggableX - GetTextWidth("D&D") / 2, r.y0 + draggableY + GetFontHeight() / 2, "D&D", 1, 1, 1);

		GL::SetTexture(0);
		br.Begin();
		ddr = UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5);
		if (subui.IsPressed(2))
			br.SetColor(1, 0, 1, 0.5f);
		else if (subui.IsHovered(2))
			br.SetColor(1, 1, 0, 0.5f);
		else
			br.SetColor(0, 1, 1, 0.5f);
		br.Quad(ddr.x0, ddr.y0, ddr.x1, ddr.y1, 0, 0, 1, 1);
		br.End();
		DrawTextLine(r.x0 + draggableX - GetTextWidth("x") / 2, r.y0 + 5 + GetFontHeight() / 2, "x", 1, 1, 1);

		GL::SetTexture(0);
		br.Begin();
		ddr = UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10);
		if (subui.IsPressed(3))
			br.SetColor(1, 0, 1, 0.5f);
		else if (subui.IsHovered(3))
			br.SetColor(1, 1, 0, 0.5f);
		else
			br.SetColor(0, 1, 1, 0.5f);
		br.Quad(ddr.x0, ddr.y0, ddr.x1, ddr.y1, 0, 0, 1, 1);
		br.End();
		DrawTextLine(r.x0 + 5 - GetTextWidth("y") / 2, r.y0 + draggableY + GetFontHeight() / 2, "y", 1, 1, 1);
	}
	void OnEvent(UIEvent& e) override
	{
		auto r = finalRectC;

		subui.InitOnEvent(e);
		if (subui.ButtonOnEvent(0, UIRect{ r.x0, r.y0, r.x0 + 50, r.y0 + 50 }, e))
		{
			puts("0-50 clicked");
		}
		switch (subui.DragOnEvent(1, UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10), e))
		{
		case ui::SubUIDragState::Start:
			puts("drag start");
			dox = draggableX - e.x;
			doy = draggableY - e.y;
			break;
		case ui::SubUIDragState::Move:
			puts("drag move");
			draggableX = e.x + dox;
			draggableY = e.y + doy;
			break;
		case ui::SubUIDragState::Stop:
			puts("drag stop");
			break;
		}
		switch (subui.DragOnEvent(2, UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5), e))
		{
		case ui::SubUIDragState::Start:
			dox = draggableX - e.x;
			break;
		case ui::SubUIDragState::Move:
			draggableX = e.x + dox;
			break;
		}
		switch (subui.DragOnEvent(3, UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10), e))
		{
		case ui::SubUIDragState::Start:
			doy = draggableY - e.y;
			break;
		case ui::SubUIDragState::Move:
			draggableY = e.y + doy;
			break;
		}

		if (e.type == UIEventType::ButtonDown && !subui.IsAnyPressed())
		{
			subui.DragStart(1);
			dox = 0;
			doy = 0;
		}
	}
	void Render(UIContainer* ctx) override
	{
		GetStyle().SetPadding(3);
		GetStyle().SetWidth(100);
		GetStyle().SetHeight(100);
	}

	ui::SubUI<uint8_t> subui;
	float draggableX = 75;
	float draggableY = 75;
	float dox = 0, doy = 0;
};

struct SplitPaneTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		GetStyle().SetWidth(style::Coord::Percent(100));
		GetStyle().SetHeight(style::Coord::Percent(100));

		ctx->Push<ui::SplitPane>();

		ctx->MakeWithText<ui::Panel>("Pane A");
		ctx->MakeWithText<ui::Panel>("Pane B");

		ctx->Push<ui::SplitPane>()->SetDirection(true);

		ctx->MakeWithText<ui::Panel>("Pane C");
		ctx->MakeWithText<ui::Panel>("Pane D");
		ctx->MakeWithText<ui::Panel>("Pane E");

		ctx->Pop();

		ctx->Pop();
	}
};

struct TabTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabButtonList>();
			{
				ctx->Push<ui::TabButtonT<int>>()->Init(tab1, 0);
				ctx->Text("First tab") + ui::Padding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();

				ctx->Push<ui::TabButton>()->Init(tab1 == 1)->HandleEvent(UIEventType::Activate) = [this](UIEvent& e)
				{
					if (e.target == e.current)
					{
						tab1 = 1;
						Rerender();
					}
				};
				ctx->Text("Second tab") + ui::Padding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>()->SetVisible(tab1 == 0);
			{
				ctx->Text("Contents of the first tab (SetVisible)");
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>()->SetVisible(tab1 == 1);
			{
				ctx->Text("Contents of the second tab (SetVisible)");
			}
			ctx->Pop();
		}
		ctx->Pop();

		ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabButtonList>();
			{
				ctx->Push<ui::TabButton>()->Init(tab2 == 0)->HandleEvent(UIEventType::Activate) = [this](UIEvent& e)
				{
					if (e.target == e.current)
					{
						tab2 = 0;
						Rerender();
					}
				};
				ctx->Text("First tab") + ui::Padding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();

				ctx->Push<ui::TabButtonT<int>>()->Init(tab2, 1);
				ctx->Text("Second tab") + ui::Padding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();
			}
			ctx->Pop();

			if (tab2 == 0)
			{
				ctx->Push<ui::TabPanel>();
				{
					ctx->Text("Contents of the first tab (conditional render)");
				}
				ctx->Pop();
			}

			if (tab2 == 1)
			{
				ctx->Push<ui::TabPanel>();
				{
					ctx->Text("Contents of the second tab (conditional render)");
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
	}

	void OnSerialize(IDataSerializer& s)
	{
		s << tab1 << tab2;
	}

	int tab1 = 0, tab2 = 0;
};

static const char* layoutShortNames[] =
{
	"ST", "SL", "SB", "SR",
	"IB",
	"ET", "EL", "EB", "ER",
};
static const char* layoutLongNames[] =
{
	"Stack top-down",
	"Stack left-to-right",
	"Stack bottom-up",
	"Stack right-to-left",
	"Inline block",
	"Edge top child",
	"Edge left child",
	"Edge bottom child",
	"Edge right child",
};
constexpr int layoutCount = sizeof(layoutShortNames) / sizeof(const char*);

struct LayoutTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		static int parentLayout = 0;
		static int childLayout = 0;

		LayoutUI(ctx, parentLayout);
		LayoutUI(ctx, childLayout);

		SetLayout(ctx->Push<ui::Panel>()->GetStyle(), parentLayout, -1);
		{
			SetLayout(ctx->Push<ui::Panel>()->GetStyle(), childLayout, parentLayout);
			{
				ctx->Text("[c 1A]");
				ctx->Text("[c 1B]");
			}
			ctx->Pop();

			SetLayout(ctx->Push<ui::Panel>()->GetStyle(), childLayout, parentLayout);
			{
				ctx->Text("[c 2A]");
				ctx->Text("[c 2B]");
			}
			ctx->Pop();
		}
		ctx->Pop();
	}

	void LayoutUI(UIContainer* ctx, int& layout)
	{
		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < layoutCount; i++)
		{
			auto* rb = ctx->MakeWithText<ui::RadioButtonT<int>>(layoutShortNames[i])->Init(layout, i);
			rb->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
			rb->SetStyle(ui::Theme::current->button);
		}
		ctx->Text(layoutLongNames[layout]);
		ctx->Pop();
	}

	void SetLayout(style::Accessor s, int layout, int parentLayout)
	{
		switch (layout)
		{
		case 0:
			s.SetLayout(style::layouts::Stack());
			s.SetStackingDirection(style::StackingDirection::TopDown);
			break;
		case 1:
			s.SetLayout(style::layouts::Stack());
			s.SetStackingDirection(style::StackingDirection::LeftToRight);
			break;
		case 2:
			s.SetLayout(style::layouts::Stack());
			s.SetStackingDirection(style::StackingDirection::BottomUp);
			break;
		case 3:
			s.SetLayout(style::layouts::Stack());
			s.SetStackingDirection(style::StackingDirection::RightToLeft);
			break;
		case 4:
			s.SetLayout(style::layouts::InlineBlock());
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			s.SetLayout(style::layouts::EdgeSlice());
			break;
		}

		switch (parentLayout)
		{
		case 5:
			s.SetEdge(style::Edge::Top);
			break;
		case 6:
			s.SetEdge(style::Edge::Left);
			break;
		case 7:
			s.SetEdge(style::Edge::Bottom);
			break;
		case 8:
			s.SetEdge(style::Edge::Right);
			break;
		}
	}
};

struct LayoutTest2 : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		static int mode = 0;

		static const char* layoutShortNames[] =
		{
			"HSL",
		};
		static const char* layoutLongNames[] =
		{
			"Horizontal stacking layout",
		};
		constexpr int layoutCount = sizeof(layoutShortNames) / sizeof(const char*);

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < layoutCount; i++)
		{
			auto* rb = ctx->MakeWithText<ui::RadioButtonT<int>>(layoutShortNames[i])->Init(mode, i);
			rb->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
			rb->SetStyle(ui::Theme::current->button);
		}
		ctx->Text(layoutLongNames[mode]);
		ctx->Pop();

		if (mode == 0)
		{
			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::StackExpand()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::StackExpand()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One")->GetStyle().SetWidth(100);
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->MakeWithText<ui::Button>("The third");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::StackExpand()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one")->GetStyle().SetWidth(100);
			ctx->MakeWithText<ui::Button>("The third");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::StackExpand()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->MakeWithText<ui::Button>("The third")->GetStyle().SetWidth(100);
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::StackExpand()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One")->GetStyle().SetMinWidth(50);
			ctx->MakeWithText<ui::Button>("Another one")->GetStyle().SetMinWidth(100);
			ctx->MakeWithText<ui::Button>("The third")->GetStyle().SetMinWidth(150);
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::StackExpand()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			{ auto s = ctx->MakeWithText<ui::Button>("One")->GetStyle(); s.SetMinWidth(100); s.SetWidth(style::Coord::Percent(30)); }
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>()->GetStyle(); s.SetLayout(style::layouts::Stack()); s.SetStackingDirection(style::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->Pop();
		}
	}
};

struct SizeTest : ui::Node
{
	struct Test
	{
		UIObject* obj;
		std::function<std::string()> func;
	};

	void OnPaint() override
	{
		ui::Node::OnPaint();
		for (const auto& t : tests)
		{
			auto res = t.func();
			if (res.size())
			{
				DrawTextLine(t.obj->finalRectC.x1, t.obj->finalRectC.y1, res.c_str(), 1, 0, 0);
			}
		}
	}

	void Render(UIContainer* ctx) override
	{
		tests.clear();
		ctx->Text("Any errors will be drawn next to the element in red");

		TestContentSize(ctx->Text("Testing text element size"), ceilf(GetTextWidth("Testing text element size")), GetFontHeight());

		ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
		{
			auto& txt1 = ctx->Text("Text size + padding") + ui::Padding(5);
			TestContentSize(txt1, ceilf(GetTextWidth("Text size + padding")), GetFontHeight());
			TestFullSize(txt1, ceilf(GetTextWidth("Text size + padding")) + 10, GetFontHeight() + 10);
			TestEstSize(txt1, ceilf(GetTextWidth("Text size + padding")) + 10, GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock()) + ui::Padding(5);
			ctx->Text("Testing text in box");
			ctx->Pop();
			TestContentSize(box, ceilf(GetTextWidth("Testing text in box")), GetFontHeight());
			TestFullSize(box, ceilf(GetTextWidth("Testing text in box")) + 10, GetFontHeight() + 10);
			TestFullX(box, ceilf(GetTextWidth("Text size + padding")) + 10);
		}
		ctx->Pop();

		{
			auto& box = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock()) + ui::Padding(5) + ui::BoxSizing(style::BoxSizing::BorderBox);
			ctx->Text("Testing text in box [border]");
			ctx->Pop();
			TestContentSize(box, ceilf(GetTextWidth("Testing text in box [border]")), GetFontHeight());
			TestFullSize(box, ceilf(GetTextWidth("Testing text in box [border]")) + 10, GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock()) + ui::Padding(5) + ui::BoxSizing(style::BoxSizing::ContentBox);
			ctx->Text("Testing text in box [content]");
			ctx->Pop();
			TestContentSize(box, ceilf(GetTextWidth("Testing text in box [content]")), GetFontHeight());
			TestFullSize(box, ceilf(GetTextWidth("Testing text in box [content]")) + 10, GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock()) + ui::Padding(5) + ui::Width(140);
			ctx->Text("Testing text in box +W");
			ctx->Pop();
			TestContentSize(box, 140 - 10, GetFontHeight());
			TestFullSize(box, 140, GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock()) + ui::Padding(5) + ui::Width(140) + ui::BoxSizing(style::BoxSizing::BorderBox);
			ctx->Text("Testing text in box +W [border]");
			ctx->Pop();
			TestContentSize(box, 140 - 10, GetFontHeight());
			TestFullSize(box, 140, GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::Layout(style::layouts::InlineBlock()) + ui::Padding(5) + ui::Width(140) + ui::BoxSizing(style::BoxSizing::ContentBox);
			ctx->Text("Testing text in box +W [content]");
			ctx->Pop();
			TestContentSize(box, 140, GetFontHeight());
			TestFullSize(box, 140 + 10, GetFontHeight() + 10);
		}
	}

	static std::string TestSize(UIRect& r, float w, float h)
	{
		if (fabsf(r.GetWidth() - w) > 0.0001f || fabsf(r.GetHeight() - h) > 0.0001f)
		{
			char bfr[1024];
			snprintf(bfr, 1024, "expected %g;%g - got %g;%g", w, h, r.GetWidth(), r.GetHeight());
			return bfr;
		}
		return {};
	}

	void TestContentSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			return TestSize(obj->finalRectC, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	void TestFullSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			return TestSize(obj->finalRectCPB, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	void TestFullX(UIObject& obj, float x)
	{
		auto fn = [obj{ &obj }, x]()->std::string
		{
			auto r = obj->finalRectCPB;
			if (fabsf(r.x0 - x) > 0.0001f)
			{
				char bfr[1024];
				snprintf(bfr, 1024, "expected x %g - got %g", x, r.x0);
				return bfr;
			}
			return {};
		};
		tests.push_back({ &obj, fn });
	}

	void TestEstSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			auto ew = obj->GetFullEstimatedWidth({ 500, 500 }, style::EstSizeType::Exact);
			auto eh = obj->GetFullEstimatedHeight({ 500, 500 }, style::EstSizeType::Exact);
			if (ew.max < ew.min)
				return "est.width: max < min";
			if (eh.max < eh.min)
				return "est.height: max < min";
			UIRect r{ 0, 0, ew.min, eh.min };
			return TestSize(r, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	std::vector<Test> tests;
};

struct ScrollbarTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		*ctx->Push<ui::ScrollArea>() + ui::Width(300) + ui::Height(200);
		for (int i = 0; i < 20; i++)
			ctx->Text("Inside scroll area");
		ctx->Pop();
	}
};

struct ImageTest : ui::Node
{
	ImageTest()
	{
		ui::Canvas c(32, 32);
		auto p = c.GetPixels();
		for (uint32_t i = 0; i < c.GetWidth() * c.GetHeight(); i++)
		{
			p[i] = (i / 4 + i / 4 / c.GetHeight()) % 2 ? 0xffffffff : 0;
		}
		img = new ui::Image(c);
	}
	~ImageTest()
	{
		delete img;
	}
	void Render(UIContainer* ctx) override
	{
		style::BlockRef pbr = ui::Theme::current->panel;
		style::Accessor pa(pbr);
		pa.SetLayout(style::layouts::InlineBlock());
		pa.SetPadding(4);
		pa.SetMargin(0);

		style::BlockRef ibr = ui::Theme::current->image;
		style::Accessor ia(ibr);
		ia.SetHeight(25);

		style::BlockRef ibr2 = ui::Theme::current->image;
		style::Accessor ia2(ibr2);
		ia2.SetWidth(25);

		ui::ScaleMode scaleModes[3] = { ui::ScaleMode::Stretch, ui::ScaleMode::Fit, ui::ScaleMode::Fill };
		const char* scaleModeNames[3] = { "Stretch", "Fit", "Fill" };

		for (int mode = 0; mode < 6; mode++)
		{
			ctx->Push<ui::Panel>()->SetStyle(pbr);

			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					ctx->Push<ui::Panel>()->SetStyle(pbr);
					ctx->Make<ui::ImageElement>()
						->SetImage(img)
						->SetScaleMode(scaleModes[mode % 3], x, y)
						->SetStyle(mode / 3 ? ibr2 : ibr);
					ctx->Pop();
				}
			}

			ctx->Text(scaleModeNames[mode % 3]);

			ctx->Pop();
		}
	}

	ui::Image* img;
};

struct ThreadWorkerTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->MakeWithText<ui::Button>("Do it")->HandleEvent(UIEventType::Activate) = [this](UIEvent&)
		{
			wq.Push([this]()
			{
				for (int i = 0; i <= 100; i++)
				{
					if (wq.HasItems() || wq.IsQuitting())
						return;
					ui::Application::PushEvent(this, [this, i]()
					{
						progress = i / 100.0f;
						Rerender();
					});
#pragma warning (disable:4996)
					_sleep(20);
				}
			}, true);
		};
		auto* pb = ctx->MakeWithText<ui::ProgressBar>(progress < 1 ? "Processing..." : "Done");
		pb->progress = progress;
	}

	float progress = 0;

	WorkerQueue wq;
};

struct ThreadedImageRenderingTest : ui::Node
{
	~ThreadedImageRenderingTest()
	{
		delete image;
	}
	void Render(UIContainer* ctx) override
	{
		Subscribe(ui::DCT_ResizeWindow, GetNativeWindow());

		auto* img = ctx->Make<ui::ImageElement>();
		img->GetStyle().SetWidth(style::Coord::Percent(100));
		img->GetStyle().SetHeight(style::Coord::Percent(100));
		img->SetScaleMode(ui::ScaleMode::Fill);
		img->SetImage(image);

		ui::Application::PushEvent(this, [this, img]()
		{
			auto cr = img->GetContentRect();
			int tw = cr.GetWidth();
			int th = cr.GetHeight();

			if (image && image->GetWidth() == tw && image->GetHeight() == th)
				return;

			wq.Push([this, tw, th]()
			{
				ui::Canvas canvas(tw, th);

				auto* px = canvas.GetPixels();
				for (uint32_t y = 0; y < th; y++)
				{
					if (wq.HasItems() || wq.IsQuitting())
						return;

					for (uint32_t x = 0; x < tw; x++)
					{
						float res = (((x & 1) + (y & 1)) & 1) ? 1.0f : 0.1f;
						float s = sinf(x * 0.02f) * 0.2f + 0.5f;
						double q = 1 - fabsf(y / float(th) - s) / 0.1f;
						if (q < 0)
							q = 0;
						res *= 1 - q;
						res += 0.5f * q;
						uint8_t c = res * 255;
						px[x + y * tw] = 0xff000000 | (c << 16) | (c << 8) | c;
					}
				}

				ui::Application::PushEvent(this, [this, canvas{ std::move(canvas) }]()
				{
					delete image;
					image = new ui::Image(canvas);
					Rerender();
				});
			}, true);
		});
	}

	WorkerQueue wq;
	ui::Image* image = nullptr;
};

struct SlidersTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		static float sldval0 = 0.63f;
		ctx->Make<ui::Slider>()->Init(sldval0, { 0, 1 });

		ui::Property::Begin(ctx, "Slider 1: 0-2 step=0");
		static float sldval1 = 0.63f;
		ctx->Make<ui::Slider>()->Init(sldval1, { 0, 2 });
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 2: 0-2 step=0.1");
		static float sldval2 = 0.63f;
		ctx->Make<ui::Slider>()->Init(sldval2, { 0, 2, 0.1 });
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 3: custom track bg");
		static float sldval3 = 0.63f;
		{
			auto& s = ctx->Make<ui::Slider>()->Init(sldval3, { 0, 1 });
			style::Accessor a(s.trackStyle);
			a.SetPaintFunc([fn{ a.GetPaintFunc() }](const style::PaintInfo& info)
			{
				fn(info);
				auto r = info.rect;
				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(0, 0, 0);
				br.Pos(r.x0, r.y1);
				br.Pos(r.x0, r.y0);
				br.SetColor(1, 0, 0);
				br.Pos(r.x1, r.y0);
				br.Pos(r.x1, r.y0);
				br.Pos(r.x1, r.y1);
				br.SetColor(0, 0, 0);
				br.Pos(r.x0, r.y1);
				br.End();
			});
			style::Accessor(s.trackFillStyle).SetPaintFunc([](const style::PaintInfo& info) {});
		}
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 4: vert stretched");
		ctx->Make<ui::Slider>()->Init(sldval2, { 0, 2, 0.1 }) + ui::Height(40);
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Color picker parts");
		static float hue = 0.6f, sat = 0.3f, val = 0.8f;
		{
			auto s = ctx->Make<ui::HueSatPicker>()->Init(hue, sat).GetStyle();
			s.SetWidth(100);
			s.SetHeight(100);
		}
		{
			auto s = ctx->Make<ui::ColorCompPicker2D>()->Init(hue, sat).GetStyle();
			s.SetWidth(120);
			s.SetHeight(100);
		}
		ui::Property::End(ctx);
	}
};

static Color4f colorPickerTestCol;
struct ColorPickerTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		auto& cp = ctx->Make<ui::ColorPicker>()->SetColor(colorPickerTestCol);
		cp.HandleEvent(UIEventType::Change) = [&cp](UIEvent& e)
		{
			colorPickerTestCol = cp.GetColor();
		};
	}
};

struct HighElementCountTest : ui::Node
{
	struct DummyElement : UIElement
	{
		void OnPaint() override
		{
			GL::SetTexture(0);
			GL::BatchRenderer br;
			br.Begin();
			br.SetColor(fmodf(uintptr_t(this) / (8 * 256.0f), 1.0f), 0.0f, 0.0f);
			auto r = GetContentRect();
			br.Quad(r.x0, r.y0, r.x1, r.y1, 0, 0, 1, 1);
			br.End();
		}
		void GetSize(style::Coord& outWidth, style::Coord& outHeight) override
		{
			outWidth = 100;
			outHeight = 1;
		}
	};
	void Render(UIContainer* ctx) override
	{
		ctx->PushBox();// + ui::StackingDirection(style::StackingDirection::LeftToRight); TODO FIX
		ctx->MakeWithText<ui::RadioButtonT<int>>("no styles")->Init(styleMode, 0)->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
		ctx->MakeWithText<ui::RadioButtonT<int>>("same style")->Init(styleMode, 1)->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
		ctx->MakeWithText<ui::RadioButtonT<int>>("different styles")->Init(styleMode, 2)->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
		ctx->Pop();

		for (int i = 0; i < 1000; i++)
		{
			auto* el = ctx->Make<DummyElement>();
			switch (styleMode)
			{
			case 0: break;
			case 1: *el + ui::Width(200); break;
			case 2: *el + ui::Width(100 + i * 0.02f); break;
			}
		}

		printf("# blocks: %d\n", style::g_numBlocks);
	}
	int styleMode;
};

struct ZeroRerenderTest : ui::Node
{
	bool first = true;
	void Render(UIContainer* ctx) override
	{
		if (first)
			first = false;
		else
			puts("Should not happen!");

		ui::Property::Begin(ctx, "Show?");
		cbShow = ctx->Make<ui::CheckboxBoolState>()->SetChecked(show);
		ui::Property::End(ctx);

		*cbShow + ui::EventHandler(UIEventType::Change, [this](UIEvent& e) { show = cbShow->GetChecked(); OnShowChange(); });
		*ctx->MakeWithText<ui::Button>("Show")
			+ ui::EventHandler(UIEventType::Activate, [this](UIEvent& e) { show = true; OnShowChange(); });
		*ctx->MakeWithText<ui::Button>("Hide")
			+ ui::EventHandler(UIEventType::Activate, [this](UIEvent& e) { show = false; OnShowChange(); });

		showable = ctx->Push<ui::Panel>();
		contentLabel = &ctx->Text("Contents: " + text);
		tbText = &ctx->Make<ui::Textbox>()->SetText(text);
		*tbText + ui::EventHandler(UIEventType::Change, [this](UIEvent& e)
		{
			text = tbText->GetText();
			contentLabel->SetText("Contents: " + text);
		});
		ctx->Pop();

		OnShowChange();
	}

	void OnShowChange()
	{
		cbShow->SetChecked(show);
		showable->GetStyle().SetHeight(show ? style::Coord::Undefined() : style::Coord(0));
	}

	ui::CheckboxBoolState* cbShow;
	UIObject* showable;
	ui::TextElement* contentLabel;
	ui::Textbox* tbText;

	bool show = false;
	std::string text;
};

struct GlobalEventsTest : ui::Node
{
	struct EventTest : ui::Node
	{
		static constexpr bool Persistent = true;
		void Render(UIContainer* ctx) override
		{
			Subscribe(dct);
			ctx->Push<ui::Panel>();
			count++;
			char bfr[64];
			snprintf(bfr, 64, "%s: %d", name, count);
			ctx->Text(bfr);
			infofn(bfr);
			ctx->Text(bfr);
			ctx->Pop();
		}
		const char* name;
		ui::DataCategoryTag* dct;
		std::function<void(char*)> infofn;
		int count = -1;
	};

	void Render(UIContainer* ctx) override
	{
		for (auto* e = ctx->Make<EventTest>();
			e->name = "Mouse moved",
			e->dct = ui::DCT_MouseMoved,
			e->infofn = [this](char* bfr) { snprintf(bfr, 64, "mouse pos: %g; %g", system->eventSystem.prevMouseX, system->eventSystem.prevMouseY); },
			0;);
		
		for (auto* e = ctx->Make<EventTest>();
			e->name = "Resize window",
			e->dct = ui::DCT_ResizeWindow,
			e->infofn = [this](char* bfr) { auto ws = GetNativeWindow()->GetSize(); snprintf(bfr, 64, "window size: %dx%d", ws.x, ws.y); },
			0;);

		for (auto* e = ctx->Make<EventTest>();
			e->name = "Drag/drop data changed",
			e->dct = ui::DCT_DragDropDataChanged,
			e->infofn = [this](char* bfr) { auto* ddd = ui::DragDrop::GetData(); snprintf(bfr, 64, "drag data: %s", !ddd ? "<none>" : ddd->type.c_str()); },
			0;);

		for (auto* e = ctx->Make<EventTest>();
			e->name = "Tooltip changed",
			e->dct = ui::DCT_TooltipChanged,
			e->infofn = [this](char* bfr) { snprintf(bfr, 64, "tooltip: %s", ui::Tooltip::IsSet() ? "set" : "<none>"); },
			0;);

		ctx->MakeWithText<ui::Button>("Draggable")->HandleEvent() = [](UIEvent& e)
		{
			if (e.context->DragCheck(e, UIMouseButton::Left))
			{
				ui::DragDrop::SetData(new ui::DragDropText("test", "text"));
			}
		};
		*ctx->MakeWithText<ui::Button>("Tooltip") + ui::AddTooltip("Tooltip");
		ctx->Make<ui::DefaultOverlayRenderer>();
	}
};

struct ElementResetTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		if (first)
		{
			*ctx->MakeWithText<ui::Button>("First")
				+ ui::Width(300)
				+ ui::EventHandler(UIEventType::Click, [this](UIEvent&) { first = false; Rerender(); });

			auto& tb = *ctx->Make<ui::Textbox>();
			tb + ui::EventHandler(UIEventType::Change, [this, &tb](UIEvent&) { text[0] = tb.GetText(); });
			tb.SetText(text[0]);
		}
		else
		{
			*ctx->MakeWithText<ui::Button>("Second")
				+ ui::Height(30)
				+ ui::EventHandler(UIEventType::Click, [this](UIEvent&) { first = true; Rerender(); });

			auto& tb = *ctx->Make<ui::Textbox>();
			tb + ui::EventHandler(UIEventType::Change, [this, &tb](UIEvent&) { text[1] = tb.GetText(); });
			tb.SetText(text[1]);
		}

		auto& tb = *ctx->Make<ui::Textbox>();
		tb + ui::EventHandler(UIEventType::Change, [this, &tb](UIEvent&) { text[2] = tb.GetText(); });
		tb.SetText(text[2]);
	}
	bool first = true;
	std::string text[3] = { "first", "second", "third" };
};

struct IMGUITest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ui::Property::Begin(ctx, "buttons");
		if (ui::imm::Button(ctx, "working button"))
			puts("working button");
		if (ui::imm::Button(ctx, "disabled button", { ui::Enable(false) }))
			puts("DISABLED button SHOULD NOT APPEAR");
		ui::Property::End(ctx);

		{
			ui::Property::Begin(ctx, "bool");
			auto tmp = boolVal;
			if (ui::imm::PropEditBool(ctx, "\bworking", tmp))
				boolVal = tmp;
			if (ui::imm::CheckboxRaw(ctx, tmp))
				boolVal = !tmp;
			if (ui::imm::PropEditBool(ctx, "\bdisabled", tmp, { ui::Enable(false) }))
				boolVal = tmp;
			ui::Property::End(ctx);
		}

		{
			ui::Property::Begin(ctx, "int format: %d");
			auto tmp = intFmt;
			if (ui::imm::RadioButton(ctx, tmp, 0, "\bworking"))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(ctx, tmp == 0, " "))
				intFmt = 0;
			if (ui::imm::RadioButton(ctx, tmp, 0, "\bdisabled", { ui::Enable(false) }))
				intFmt = tmp;
			ui::Property::End(ctx);
		}
		{
			ui::Property::Begin(ctx, "int format: %x");
			auto tmp = intFmt;
			if (ui::imm::RadioButton(ctx, tmp, 1, "\bworking"))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(ctx, tmp == 1, " "))
				intFmt = 1;
			if (ui::imm::RadioButton(ctx, tmp, 1, "\bdisabled", { ui::Enable(false) }))
				intFmt = tmp;
			ui::Property::End(ctx);
		}

		{
			ui::Property::Begin(ctx, "int");
			auto tmp = intVal;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, -543, 1234, intFmt ? "%x" : "%d"))
				intVal = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, -543, 1234, intFmt ? "%x" : "%d"))
				intVal = tmp;

			ctx->Text("int: " + std::to_string(intVal)) + ui::Padding(5);
			ui::Property::End(ctx);
		}
		{
			ui::Property::Begin(ctx, "uint");
			auto tmp = uintVal;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, 0, 1234, intFmt ? "%x" : "%d"))
				uintVal = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, 0, 1234, intFmt ? "%x" : "%d"))
				uintVal = tmp;

			ctx->Text("uint: " + std::to_string(uintVal)) + ui::Padding(5);
			ui::Property::End(ctx);
		}
		{
			ui::Property::Begin(ctx, "int64");
			auto tmp = int64Val;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, -543, 1234, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, -543, 1234, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;

			ctx->Text("int64: " + std::to_string(int64Val)) + ui::Padding(5);
			ui::Property::End(ctx);
		}
		{
			ui::Property::Begin(ctx, "uint64");
			auto tmp = uint64Val;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, 0, 1234, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, 0, 1234, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;

			ctx->Text("uint64: " + std::to_string(uint64Val)) + ui::Padding(5);
			ui::Property::End(ctx);
		}
		{
			ui::Property::Begin(ctx, "float");
			auto tmp = floatVal;
			if (ui::imm::PropEditFloat(ctx, "\bworking", tmp, {}, 0.1f, -37.4f, 154.1f))
				floatVal = tmp;
			if (ui::imm::PropEditFloat(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 0.1f, -37.4f, 154.1f))
				floatVal = tmp;

			ctx->Text("float: " + std::to_string(floatVal)) + ui::Padding(5);
			ui::Property::End(ctx);
		}
		{
			ui::imm::PropEditFloatVec(ctx, "float3", float4val, "XYZ");
			ui::imm::PropEditFloatVec(ctx, "float4", float4val, "RGBA");
		}
	}

	bool boolVal = true;
	int intFmt = 0;
	int intVal = 15;
	int64_t int64Val = 123;
	unsigned uintVal = 1;
	uint64_t uint64Val = 2;
	float floatVal = 3.14f;
	float float4val[4] = { 1, 2, 3, 4 };
};

struct TooltipTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		*ctx->MakeWithText<ui::Button>("Text-only tooltip") + ui::AddTooltip("Text only");
		*ctx->MakeWithText<ui::Button>("Checklist tooltip") + ui::AddTooltip([](UIContainer* ctx)
		{
			bool t = true, f = false;
			ui::imm::PropEditBool(ctx, "Done", t, { ui::Enable(false) });
			ui::imm::PropEditBool(ctx, "Not done", f, { ui::Enable(false) });
		});

		ctx->Make<ui::DefaultOverlayRenderer>();
	}
};


static const char* numberNames[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "." };
static const char* opNames[] = { "+", "-", "*", "/" };
struct Calculator : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		{
			ctx->Make<ui::Textbox>()->SetText(operation);

			if (ui::imm::Button(ctx, "<"))
			{
				if (!operation.empty())
					operation.pop_back();
			}
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < 11; i++)
		{
			if (ui::imm::Button(ctx, numberNames[i]))
			{
				AddChar(numberNames[i][0]);
			}
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		{
			for (int i = 0; i < 4; i++)
			{
				if (ui::imm::Button(ctx, opNames[i]))
				{
					AddChar(opNames[i][0]);
				}
			}

			if (ui::imm::Button(ctx, "="))
			{
				operation = ToString(Calculate());
			}
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("=" + ToString(Calculate()));
		ctx->Pop();
	}

	void AddChar(char ch)
	{
		if (ch == '.')
		{
			// must be preceded by a number without a dot
			if (operation.empty())
				return;
			for (size_t i = operation.size(); i > 0; )
			{
				i--;

				if (operation[i] == '.')
					return;
				if (!isdigit(operation[i]))
					break;
			}
		}
		else if (ch == '+' || ch == '-' || ch == '*' || ch == '/')
		{
			// must be preceded by a number or a dot
			if (operation.empty())
				return;
			if (!isdigit(operation.back()) && operation.back() != '.')
				return;
		}
		operation.push_back(ch);
		Rerender();
	}

	int precedence(char op)
	{
		if (op == '*' || op == '/')
			return 2;
		if (op == '+' || op == '-')
			return 1;
		return 0;
	}
	void Apply(std::vector<double>& numqueue, char op)
	{
		double b = numqueue.back();
		numqueue.pop_back();
		double& a = numqueue.back();
		switch (op)
		{
		case '+': a += b; break;
		case '-': a -= b; break;
		case '*': a *= b; break;
		case '/': a /= b; break;
		}
	}
	double Calculate()
	{
		std::vector<double> numqueue;
		std::vector<char> opstack;
		bool lastwasop = false;
		for (size_t i = 0; i < operation.size(); i++)
		{
			char c = operation[i];
			if (isdigit(c))
			{
				char* end = nullptr;
				numqueue.push_back(std::strtod(operation.c_str() + i, &end));
				i = end - operation.c_str() - 1;
				lastwasop = false;
			}
			else
			{
				while (!opstack.empty() && precedence(opstack.back()) > precedence(c))
				{
					Apply(numqueue, opstack.back());
					opstack.pop_back();
				}
				opstack.push_back(c);
				lastwasop = true;
			}
		}
		if (lastwasop)
			opstack.pop_back();
		while (!opstack.empty() && numqueue.size() >= 2)
		{
			Apply(numqueue, opstack.back());
			opstack.pop_back();
		}
		return !numqueue.empty() ? numqueue.back() : 0;
	}

	std::string ToString(double val)
	{
		std::string s = std::to_string(val);
		while (!s.empty() && s.back() == '0')
			s.pop_back();
		if (s.empty())
			s.push_back('0');
		else if (s.back() == '.')
			s.pop_back();
		return s;
	}

	std::string operation;
};


struct SlidingHighlightAnim : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ui::imm::RadioButton(ctx, layout, 0, "No button", { ui::Style(ui::Theme::current->button) });
		ui::imm::RadioButton(ctx, layout, 1, "Left", { ui::Style(ui::Theme::current->button) });
		ui::imm::RadioButton(ctx, layout, 2, "Right", { ui::Style(ui::Theme::current->button) });

		tgt = nullptr;
		if (layout != 0)
		{
			ctx->PushBox() + ui::StackingDirection(layout == 1 ? style::StackingDirection::LeftToRight : style::StackingDirection::RightToLeft);
			tgt = ctx->MakeWithText<ui::Button>(layout == 1 ? "Left" : "Right button");
			ctx->Pop();
		}
	}
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override
	{
		ui::Node::OnLayout(rect, containerSize);

		UIRect tr = GetTargetRect();
		if (memcmp(&targetLayout, &tr, sizeof(tr)) != 0)
		{
			startLayout = GetCurrentRect();
			targetLayout = tr;
			startTime = ui::platform::GetTimeMs();
			endTime = startTime + 1000;
			//puts("START ANIM");
			this->system->eventSystem.SetTimer(this, 1.0f / 60.0f);
		}
	}
	void OnPaint() override
	{
		ui::Node::OnPaint();

		auto r = GetCurrentRect();
		r = r.ShrinkBy(UIRect::UniformBorder(1));
		GL::DrawLine(r.x0, r.y0, r.x1, r.y0, 1, 0, 0);
		GL::DrawLine(r.x0, r.y1, r.x1, r.y1, 1, 0, 0);
		GL::DrawLine(r.x0, r.y0, r.x0, r.y1, 1, 0, 0);
		GL::DrawLine(r.x1, r.y0, r.x1, r.y1, 1, 0, 0);
	}
	void OnEvent(UIEvent& e) override
	{
		ui::Node::OnEvent(e);

		if (e.type == UIEventType::Timer)
		{
			//puts("TIMER");
			if (ui::platform::GetTimeMs() - startTime < endTime - startTime)
			{
				//puts("INVRESET");
				GetNativeWindow()->InvalidateAll();
				this->system->eventSystem.SetTimer(this, 1.0f / 60.0f);
			}
		}
	}

	UIRect GetCurrentRect()
	{
		uint32_t t = ui::platform::GetTimeMs();
		if (startTime == endTime)
			return targetLayout;
		float q = (t - startTime) / float(endTime - startTime);
		if (q < 0)
			q = 0;
		else if (q > 1)
			q = 1;
		return
		{
			lerp(startLayout.x0, targetLayout.x0, q),
			lerp(startLayout.y0, targetLayout.y0, q),
			lerp(startLayout.x1, targetLayout.x1, q),
			lerp(startLayout.y1, targetLayout.y1, q),
		};
	}
	UIRect GetTargetRect()
	{
		if (tgt)
			return tgt->GetBorderRect();
		auto r = GetBorderRect();
		r.y1 = r.y0;
		return r;
	}

	UIObject* tgt = nullptr;

	int layout = 0;
	UIRect startLayout = {};
	UIRect targetLayout = {};
	uint32_t startTime = 0;
	uint32_t endTime = 0;
};


struct SubUIBenchmark : ui::Node
{
	SubUIBenchmark()
	{
		for (int y = 0; y < 60; y++)
		{
			for (int x = 0; x < 60; x++)
			{
				points.push_back({ x * 12.f + 10.f, y * 12.f + 10.f });
			}
		}
	}
	void OnPaint() override
	{
		style::PaintInfo info(this);
		ui::Theme::current->textBoxBase->paint_func(info);
		auto r = finalRectC;

		GL::BatchRenderer br;

		GL::SetTexture(0);
		br.Begin();

		for (uint16_t pid = 0; pid < points.size(); pid++)
		{
			auto ddr = UIRect::FromCenterExtents(r.x0 + points[pid].x, r.y0 + points[pid].y, 4);
			if (subui.IsPressed(pid))
				br.SetColor(1, 0, 1, 0.5f);
			else if (subui.IsHovered(pid))
				br.SetColor(1, 1, 0, 0.5f);
			else
				br.SetColor(0, 1, 1, 0.5f);
			br.Quad(ddr.x0, ddr.y0, ddr.x1, ddr.y1, 0, 0, 1, 1);
		}

		br.End();
	}
	void OnEvent(UIEvent& e) override
	{
		auto r = finalRectC;

		subui.InitOnEvent(e);
		for (uint16_t pid = 0; pid < points.size(); pid++)
		{
			switch (subui.DragOnEvent(pid, UIRect::FromCenterExtents(r.x0 + points[pid].x, r.y0 + points[pid].y, 4), e))
			{
			case ui::SubUIDragState::Start:
				dox = points[pid].x - e.x;
				doy = points[pid].y - e.y;
				break;
			case ui::SubUIDragState::Move:
				points[pid].x = e.x + dox;
				points[pid].y = e.y + doy;
				break;
			}
		}
	}
	void Render(UIContainer* ctx) override
	{
		GetStyle().SetPadding(3);
		GetStyle().SetWidth(820);
		GetStyle().SetHeight(820);
	}

	ui::SubUI<uint16_t> subui;
	std::vector<Point<float>> points;
	float dox = 0, doy = 0;
};


ui::DataCategoryTag DCT_ItemSelection[1];
struct DataEditor : ui::Node
{
	struct Item
	{
		std::string name;
		int value;
		bool enable;
		float value2;
	};

	struct ItemButton : ui::Node
	{
		ItemButton()
		{
			GetStyle().SetLayout(style::layouts::Stack());
			//GetStyle().SetMargin(32);
		}
		void Render(UIContainer* ctx) override
		{
			static int wat = 0;
			auto* b = ctx->Push<ui::Button>();
			b->GetStyle().SetWidth(style::Coord::Percent(100));
			//b->SetInputDisabled(wat++ % 2);
			ctx->Text(name);
			ctx->Pop();
		}
		void OnEvent(UIEvent& e) override
		{
			if (e.type == UIEventType::Activate)
			{
				callback();
			}
			if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Right)
			{
				ui::MenuItem sm[] =
				{
					ui::MenuItem("Something", "Yeah", true),
					ui::MenuItem("Another thing", "Yup").Func([]() { puts("- yeah!"); }),
					ui::MenuItem("Detailed thing", "Ooh").Func([](ui::Menu* menu, int id)
					{
						while (id >= 0)
						{
							printf("- with %d!\n", id);
							id = menu->GetItems()[id].parent;
						}
					}),
				};

				ui::MenuItem m[] =
				{
					ui::MenuItem("Item 1", "Ctrl+Enter"),
					ui::MenuItem::Separator(),
					ui::MenuItem::Submenu("Submenu", sm),
					ui::MenuItem::Submenu("Submenu 2", sm),
					ui::MenuItem("Item 2", {}, false, true),
				};

				ui::Menu menu(m);
				int ret = menu.Show(this);
				printf("%d\n", ret);
				e.StopPropagation();
			}
		}
		void Init(const char* txt, const std::function<void()> cb)
		{
			name = txt;
			callback = cb;
		}
		std::function<void()> callback;
		const char* name;
	};

#if 0
	void OnInit() override
	{
		static ui::MenuItem smfile[] =
		{
			ui::MenuItem("New"),
			ui::MenuItem("Open", "Ctrl+O"),
			ui::MenuItem("Save", "Ctrl+S"),
			ui::MenuItem("Save As..."),
		};
		static ui::MenuItem smhelp[] =
		{
			ui::MenuItem("About"),
		};
		static ui::MenuItem main[] =
		{
			ui::MenuItem::Submenu("File", smfile),
			ui::MenuItem::Submenu("Help", smhelp),
		};
		topMenu = new ui::Menu(main, true);
		FindParentOfType<ui::NativeWindow>()->SetMenu(topMenu);
	}
	void OnDestroy() override
	{
		FindParentOfType<ui::NativeWindow>()->SetMenu(nullptr);
		delete topMenu;
	}
#endif

	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::MenuBarElement>();
		{
			ctx->Push<ui::MenuItemElement>()->SetText("File");
			{
				ctx->Make<ui::MenuItemElement>()->SetText("New", "Ctrl+N");
				ctx->Make<ui::MenuItemElement>()->SetText("Open", "Ctrl+O");
				ctx->Make<ui::MenuItemElement>()->SetText("Save", "Ctrl+S");
				ctx->Make<ui::MenuSeparatorElement>();
				ctx->Make<ui::MenuItemElement>()->SetText("Quit", "Ctrl+Q");
			}
			ctx->Pop();
			ctx->Push<ui::MenuItemElement>()->SetText("Help");
			{
				ctx->Make<ui::MenuItemElement>()->SetText("Documentation", "F1");
				ctx->Make<ui::MenuItemElement>()->SetText("About");
			}
			ctx->Pop();
		}
		ctx->Pop();

		ctx->MakeWithText<ui::ProgressBar>("Processing...")->progress = 0.37f;
		static float sldval = 0.63f;
		ctx->Make<ui::Slider>()->Init(sldval, { 0, 2, 0.1 });

#if 0
		struct TableDS : ui::TableDataSource
		{
			size_t GetNumRows() { return 3; }
			size_t GetNumCols() { return 4; }
			std::string GetRowName(size_t row) { return "Row" + std::to_string(row); }
			std::string GetColName(size_t col) { return "Col" + std::to_string(col); }
			std::string GetText(size_t row, size_t col) { return "R" + std::to_string(row) + "C" + std::to_string(col); }
		};
		static TableDS tblds;
		auto* tbl = ctx->Make<ui::TableView>();
		tbl->SetDataSource(&tblds);
		tbl->GetStyle().SetHeight(90);
#endif

#if 1
		struct TreeDS : ui::TreeDataSource
		{
			size_t GetNumCols() { return 4; }
			std::string GetColName(size_t col) { return "Col" + std::to_string(col); }
			size_t GetChildCount(uintptr_t id)
			{
				if (id == ROOT) return 2;
				if (id == 0) return 1;
				return 0;
			}
			uintptr_t GetChild(uintptr_t id, size_t which)
			{
				if (id == ROOT) return which;
				if (id == 0) return 2;
				return 0;
			}
			std::string GetText(uintptr_t id, size_t col) { return "ID" + std::to_string(id) + "C" + std::to_string(col); }
		};
		static TreeDS treeds;
		auto* trv = ctx->Make<ui::TreeView>();
		trv->SetDataSource(&treeds);
		trv->GetStyle().SetHeight(90);
#endif

#if 1
		auto* nw = ctx->Make<ui::NativeWindowNode>();
		nw->GetWindow()->SetTitle("Subwindow A");
		auto renderFunc = [](UIContainer* ctx)
		{
			ctx->Push<ui::Panel>();
			auto onClick = []()
			{
				struct DialogTest : ui::NativeDialogWindow
				{
					DialogTest()
					{
						SetStyle(ui::WS_Resizable);
					}
					void OnRender(UIContainer* ctx) override
					{
						auto s = ctx->Push<ui::Panel>()->GetStyle();
						s.SetLayout(style::layouts::Stack());
						s.SetStackingDirection(style::StackingDirection::RightToLeft);
						if (ui::imm::Button(ctx, "X"))
							OnClose();
						if (ui::imm::Button(ctx, "[]"))
							SetState(GetState() == ui::WindowState::Maximized ? ui::WindowState::Normal : ui::WindowState::Maximized);
						if (ui::imm::Button(ctx, "_"))
							SetState(ui::WindowState::Minimized);
						ctx->Pop();

						ctx->Push<ui::Panel>();
						ctx->Make<ItemButton>()->Init("Test", [this]() { OnClose(); });
						ctx->Pop();
					}
				} dlg;
				dlg.SetTitle("Dialog!");
				dlg.Show();
			};
			ctx->Make<ItemButton>()->Init("Only a button", []() {});
			ctx->Make<ItemButton>()->Init("Only another button (dialog)", onClick);
			ctx->Pop();
		};
		nw->GetWindow()->SetRenderFunc(renderFunc);
		nw->GetWindow()->SetVisible(true);
#endif

#if 1
		auto* frm = ctx->Make<ui::InlineFrameNode>();
		auto frf = [](UIContainer* ctx)
		{
			ctx->Push<ui::Panel>();
			ctx->MakeWithText<ui::Button>("In-frame button");
			static int cur = 1;
			ctx->Push<ui::RadioButtonT<int>>()->Init(cur, 0);
			ctx->Text("Zero");
			ctx->Pop();
			ctx->Push<ui::RadioButtonT<int>>()->Init(cur, 1)->SetStyle(ui::Theme::current->button);
			ctx->Text("One");
			ctx->Pop();
			ctx->Push<ui::RadioButtonT<int>>()->Init(cur, 2)->SetStyle(ui::Theme::current->button);
			ctx->Text("Two");
			ctx->Pop();
			ctx->Pop();
		};
		frm->CreateFrameContents(frf);
		auto frs = frm->GetStyle();
		frs.SetPadding(4);
		frs.SetMinWidth(100);
		frs.SetMinHeight(100);
#endif

		static bool lbsel0 = false;
		static bool lbsel1 = true;
		ctx->Push<ui::ListBox>();
		ctx->Push<ui::ScrollArea>();
		ctx->MakeWithText<ui::Selectable>("Item 1")->Init(lbsel0);
		ctx->MakeWithText<ui::Selectable>("Item two")->Init(lbsel1);
		ctx->Pop();
		ctx->Pop();

		Subscribe(DCT_ItemSelection);
		if (editing == SIZE_MAX)
		{
			ctx->Text("List");
			ctx->Push<ui::Panel>();
			for (size_t i = 0; i < items.size(); i++)
			{
				ctx->Make<ItemButton>()->Init(items[i].name.c_str(), [this, i]() { editing = i; ui::Notify(DCT_ItemSelection); });
			}
			ctx->Pop();

			auto* r1 = ctx->Push<ui::CollapsibleTreeNode>();
			ctx->Text("root item 1");
			if (r1->open)
			{
				ctx->Text("- data under root item 1");
				auto* r1c1 = ctx->Push<ui::CollapsibleTreeNode>();
				ctx->Text("child 1");
				if (r1c1->open)
				{
					auto* r1c1c1 = ctx->Push<ui::CollapsibleTreeNode>();
					ctx->Text("subchild 1");
					ctx->Pop();
				}
				ctx->Pop();

				auto* r1c2 = ctx->Push<ui::CollapsibleTreeNode>();
				ctx->Text("child 2");
				if (r1c2->open)
				{
					auto* r1c2c1 = ctx->Push<ui::CollapsibleTreeNode>();
					ctx->Text("subchild 1");
					ctx->Pop();
				}
				ctx->Pop();
			}
			ctx->Pop();

			auto* r2 = ctx->Push<ui::CollapsibleTreeNode>();
			ctx->Text("root item 2");
			if (r2->open)
			{
				ctx->Text("- data under root item 2");
			}
			ctx->Pop();
		}
		else
		{
			ctx->PushBox()
				+ ui::Layout(style::layouts::StackExpand())
				+ ui::StackingDirection(style::StackingDirection::LeftToRight);
			ctx->Text("Item:") + ui::Padding(5) + ui::Width(style::Coord::Fraction(0));
			ctx->Text(items[editing].name.c_str());
			if (ui::imm::Button(ctx, "Go back", { ui::Width(style::Coord::Fraction(0)) }))
			{
				editing = SIZE_MAX;
				ui::Notify(DCT_ItemSelection);
			}
			ctx->Pop();

			ui::imm::PropEditString(ctx, "Name", items[editing].name.c_str(), [&](const char* v) { items[editing].name = v; });
			ui::imm::PropEditBool(ctx, "Enable", items[editing].enable);
		}
	}

	size_t editing = SIZE_MAX;
	std::vector<Item> items =
	{
		{ "test item 1", 5, true, 36.6f },
		{ "another test item", 123, false, 12.3f },
		{ "third item", 333, true, 3.0f },
	};
	ui::Menu* topMenu;
};


struct TestEntry
{
	const char* name;
	void(*func)(UIContainer* ctx);
};
static TestEntry testEntries[] =
{
	{ "Off", [](UIContainer* ctx) {} },
	{},
	{ "- Basic logic -" },
	{ "Open/Close", [](UIContainer* ctx) { ctx->Make<OpenClose>(); } },
	{ "Element reset", [](UIContainer* ctx) { ctx->Make<ElementResetTest>(); } },
	{ "Drag and drop", [](UIContainer* ctx) { ctx->Make<DragDropTest>(); } },
	{ "SubUI", [](UIContainer* ctx) { ctx->Make<SubUITest>(); } },
	{ "High element count", [](UIContainer* ctx) { ctx->Make<HighElementCountTest>(); } },
	{ "Zero-rerender", [](UIContainer* ctx) { ctx->Make<ZeroRerenderTest>(); } },
	{ "Global events", [](UIContainer* ctx) { ctx->Make<GlobalEventsTest>(); } },
	{},
	{ "- Advanced/compound UI -" },
	{ "Sliders", [](UIContainer* ctx) { ctx->Make<SlidersTest>(); } },
	{ "Split pane", [](UIContainer* ctx) { ctx->Make<SplitPaneTest>(); } },
	{ "Tabs", [](UIContainer* ctx) { ctx->Make<TabTest>(); } },
	{ "Scrollbars", [](UIContainer* ctx) { ctx->Make<ScrollbarTest>(); } },
	{ "Image", [](UIContainer* ctx) { ctx->Make<ImageTest>(); } },
	{ "Color picker", [](UIContainer* ctx) { ctx->Make<ColorPickerTest>(); } },
	{ "IMGUI test", [](UIContainer* ctx) { ctx->Make<IMGUITest>(); } },
	{ "Tooltip", [](UIContainer* ctx) { ctx->Make<TooltipTest>(); } },
	{},
	{ "- Layout -" },
	{ "Edge slice", [](UIContainer* ctx) { ctx->Make<EdgeSliceTest>(); } },
	{ "Layout", [](UIContainer* ctx) { ctx->Make<LayoutTest>(); } },
	{ "Layout 2", [](UIContainer* ctx) { ctx->Make<LayoutTest2>(); } },
	{ "Sizing", [](UIContainer* ctx) { ctx->Make<SizeTest>(); } },
	{},
	{ "- Threading -" },
	{ "Thread worker", [](UIContainer* ctx) { ctx->Make<ThreadWorkerTest>(); } },
	{ "Threaded image rendering", [](UIContainer* ctx) { ctx->Make<ThreadedImageRenderingTest>(); } },
};
static TestEntry demoEntries[] =
{
	{ "Calculator", [](UIContainer* ctx) { ctx->Make<Calculator>(); } },
	{ "Node editing", [](UIContainer* ctx) { ctx->Make<NodeEditTest>(); } },
	{ "Compact node editing", [](UIContainer* ctx) { ctx->Make<CompactNodeEditTest>(); } },
	{ "Sliding highlight anim", [](UIContainer* ctx) { ctx->Make<SlidingHighlightAnim>(); } },
	{ "SubUI benchmark", [](UIContainer* ctx) { ctx->Make<SubUIBenchmark>(); } },
};
static bool rerenderAlways;
struct TEST : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::MenuBarElement>();

		ctx->Push<ui::MenuItemElement>()->SetText("Test");
		for (auto& entry : testEntries)
		{
			if (!entry.func)
			{
				if (entry.name)
					ctx->Make<ui::MenuItemElement>()->SetText(entry.name).SetDisabled(true);
				else
					ctx->Make<ui::MenuSeparatorElement>();
				continue;
			}

			auto fn = [this, &entry]()
			{
				curTest = &entry;
				GetNativeWindow()->SetTitle(entry.name);
				Rerender();
			};
			ctx->Make<ui::MenuItemElement>()->SetText(entry.name).SetChecked(curTest == &entry).onActivate = fn;
		}
		ctx->Pop();

		ctx->Push<ui::MenuItemElement>()->SetText("Demo");
		for (auto& entry : demoEntries)
		{
			auto fn = [this, &entry]()
			{
				curTest = &entry;
				GetNativeWindow()->SetTitle(entry.name);
				Rerender();
			};
			ctx->Make<ui::MenuItemElement>()->SetText(entry.name).SetChecked(curTest == &entry).onActivate = fn;
		}
		ctx->Pop();

		ctx->Push<ui::MenuItemElement>()->SetText("Debug");
		{
			ctx->Make<ui::MenuItemElement>()->SetText("Rerender always").SetChecked(rerenderAlways).onActivate = [this]() { rerenderAlways ^= true; Rerender(); };
			ctx->Make<ui::MenuItemElement>()->SetText("Dump layout").onActivate = [this]() { DumpLayout(lastChild); };
			ctx->Make<ui::MenuItemElement>()->SetText("Draw rectangles").SetChecked(GetNativeWindow()->IsDebugDrawEnabled()).onActivate = [this]() {
				auto* w = GetNativeWindow(); w->SetDebugDrawEnabled(!w->IsDebugDrawEnabled()); Rerender(); };
		}
		ctx->Pop();

		ctx->Pop();

		if (curTest)
			curTest->func(ctx);

		if (rerenderAlways)
			Rerender();
	}

	static const char* cln(const char* s)
	{
		auto* ss = strchr(s, ' ');
		if (ss)
			s = ss + 1;
		if (strncmp(s, "style::layouts::", sizeof("style::layouts::") - 1) == 0)
			s += sizeof("style::layouts::") - 1;
		return s;
	}
	static const style::Layout* glo(UIObject* o)
	{
		auto* lo = o->GetStyle().GetLayout();
		return lo ? lo : style::layouts::Stack();
	}
	static const char* dir(const style::Accessor& a)
	{
		static const char* names[] = { "-", "Inh", "T-B", "R-L", "B-T", "L-R" };
		return names[(int)a.GetStackingDirection()];
	}
	static const char* ctu(style::CoordTypeUnit u)
	{
		static const char* names[] = { "undefined", "inherit", "auto", "px", "%", "fr" };
		return names[(int)u];
	}
	static const char* costr(const style::Coord& c)
	{
		static char buf[128];
		auto* us = ctu(c.unit);
		if (c.unit == style::CoordTypeUnit::Undefined ||
			c.unit == style::CoordTypeUnit::Inherit ||
			c.unit == style::CoordTypeUnit::Auto)
			return us;
		snprintf(buf, 128, "%g%s", c.value, us);
		return buf;
	}
	static void DumpLayout(UIObject* o, int lev = 0)
	{
		for (int i = 0; i < lev; i++)
			printf("  ");
		auto* lo = glo(o);
		printf("%s  -%s", cln(typeid(*o).name()), cln(typeid(*lo).name()));
		if (lo == style::layouts::Stack() || lo == style::layouts::StackExpand())
			printf(":%s", dir(o->GetStyle()));
		printf(" w=%s", costr(o->GetStyle().GetWidth()));
		printf(" h=%s", costr(o->GetStyle().GetHeight()));
		auto cr = o->GetContentRect();
		printf(" [%g;%g - %g;%g]", cr.x0, cr.y0, cr.x1, cr.y1);
		puts("");

		for (auto* ch = o->firstChild; ch; ch = ch->next)
		{
			DumpLayout(ch, lev + 1);
		}
	}

	TestEntry* curTest = nullptr;
};



struct CSSBuildCallback : style::IErrorCallback
{
	void OnError(const char* msg, int line, int pos) override
	{
		printf("CSS BUILD ERROR [%d:%d]: %s\n", line, pos, msg);
	}
};

void EarlyTest()
{
	style::Sheet sh;
	CSSBuildCallback cb;
	sh.Compile("a#foo.bar:first-child, b:hover:active { display: inline; }", &cb);
	puts("done");
}

struct MainWindow : ui::NativeMainWindow
{
	void OnRender(UIContainer* ctx)
	{
		//ctx->Make<DataEditor>();
		ctx->Make<TEST>();
	}
};

int uimain(int argc, char* argv[])
{
	EarlyTest();

	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();

	//uisys.Build<TEST>();
	//uisys.Build<FileStructureViewer>();
}
