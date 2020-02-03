
#include "pch.h"

#include "../GUI.h"


struct OpenClose : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>();

		ctx->Make<ui::Checkbox>()->Init(open)->onChange = [this]() { Rerender(); };

		ctx->MakeWithText<ui::Button>("Open")->onClick = [this]() { open = true; Rerender(); };

		ctx->MakeWithText<ui::Button>("Close")->onClick = [this]() { open = false; Rerender(); };

		if (open)
		{
			ctx->Push<ui::Panel>();
			ctx->Text("It is open!");
			ctx->Pop();
		}

		ctx->Pop();
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
		GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < 3; i++)
		{
			auto* btn = ctx->MakeWithText<ui::Button>("Slot " + std::to_string(i + 1) + ": " + std::to_string(slots[i]));
			btn->SetInputDisabled(slots[i] == 0);
			HandleEvent(btn) = [this, i, btn](UIEvent& e)
			{
				struct Data : ui::DragDropData
				{
					Data(int f) : ui::DragDropData("slot item"), from(f) {}
					int from;
				};
				if (e.type == UIEventType::DragStart)
				{
					if (slots[i] > 0)
						ui::DragDrop::SetData(new Data(i));
				}
				else if (e.type == UIEventType::DragDrop)
				{
					if (auto* ddd = static_cast<Data*>(ui::DragDrop::GetData("slot item")))
					{
						slots[i]++;
						slots[ddd->from]--;
						e.handled = true;
						Rerender();
					}
				}
				else if (e.type == UIEventType::DragEnter)
				{
					btn->GetStyle().SetPaddingBottom(10);
				}
				else if (e.type == UIEventType::DragLeave)
				{
					btn->GetStyle().SetPaddingBottom(4);
				}
			};
		}
	}

	int slots[3] = { 5, 2, 0 };
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
				ctx->MakeWithText<ui::Button>("Delete")->onClick = [this]()
				{
					delete tgt;
					parent->children.erase(std::find(parent->children.begin(), parent->children.end(), tgt));
					ui::Notify(DCT_Node, parent);
				};
			}

			ctx->Text("Name");
			auto* tbname = ctx->Make<ui::Textbox>();
			tbname->text = tgt->name;
			HandleEvent(tbname, UIEventType::Commit) = [this, tbname](UIEvent&)
			{
				tgt->name = tbname->text;
			};

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
			ctx->MakeWithText<ui::Button>("Add")->onClick = [this, inspos]()
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

	TreeNode root = { "root" };
};

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
			rb->onChange = [this]() { Rerender(); };
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
			rb->onChange = [this]() { Rerender(); };
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
		ctx->MakeWithText<ui::Button>("Do it")->onClick = [this]()
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
		ctx->Make<ui::Slider>()->Init(&sldval0, 0, 1);

		ui::Property::Begin(ctx, "Slider 1: 0-2 step=0");
		static float sldval1 = 0.63f;
		ctx->Make<ui::Slider>()->Init(&sldval1, 0, 2);
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 2: 0-2 step=0.1");
		static float sldval2 = 0.63f;
		ctx->Make<ui::Slider>()->Init(&sldval2, 0, 2, 0.1f);
		ui::Property::End(ctx);
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
			ctx->Make<ui::Textbox>()->text = operation;

			ctx->MakeWithText<ui::Button>("<")->onClick = [this]() { if (!operation.empty()) operation.pop_back(); Rerender(); };
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < 11; i++)
		{
			ctx->MakeWithText<ui::Button>(numberNames[i])->onClick = [this, i]() { AddChar(numberNames[i][0]); };
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		{
			for (int i = 0; i < 4; i++)
			{
				ctx->MakeWithText<ui::Button>(opNames[i])->onClick = [this, i]() { AddChar(opNames[i][0]); };
			}

			ctx->MakeWithText<ui::Button>("=")->onClick = [this]() { operation = ToString(Calculate()); Rerender(); };
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
				e.handled = true;
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
		ctx->Make<ui::Slider>()->Init(&sldval, 0, 2, 0.1f);

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
						ctx->MakeWithText<ui::Button>("X")->onClick = [this]() { OnClose(); };
						ctx->MakeWithText<ui::Button>("[]")->onClick = [this]() {
							SetState(GetState() == ui::WindowState::Maximized ? ui::WindowState::Normal : ui::WindowState::Maximized); };
						ctx->MakeWithText<ui::Button>("_")->onClick = [this]() { SetState(ui::WindowState::Minimized); };
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

#if 0
		auto* tg = ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabList>();
			{
				ctx->MakeWithText<ui::TabButton>("First tab");
				ctx->MakeWithText<ui::TabButton>("Second tab");
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>()->SetVisible(tg->active == 0);
			{
				ctx->Text("Contents of the first tab");
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>()->SetVisible(tg->active == 1);
			{
				ctx->Text("Contents of the second tab");
			}
			ctx->Pop();
		}
		ctx->Pop();

		tg = ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabList>();
			{
				ctx->MakeWithText<ui::TabButton>("First tab");
				ctx->MakeWithText<ui::TabButton>("Second tab");
			}
			ctx->Pop();

			if (tg->active == 0)
			{
				ctx->Push<ui::TabPanel>()->id = 0;
				{
					ctx->Text("Contents of the first tab");
				}
				ctx->Pop();
			}

			if (tg->active == 1)
			{
				ctx->Push<ui::TabPanel>()->id = 1;
				{
					ctx->Text("Contents of the second tab");
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
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
			auto* b = ctx->PushBox();
			b->GetStyle().SetLayout(style::layouts::StackExpand());
			b->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
			{ auto s = ctx->Text("Item:")->GetStyle(); s.SetPadding(5); s.SetWidth(style::Coord::Fraction(0)); }
			ctx->Text(items[editing].name.c_str());
			{
				auto* btn = ctx->Push<ui::Button>();
				btn->GetStyle().SetWidth(style::Coord::Fraction(0));
				btn->onClick = [this]() { editing = SIZE_MAX; ui::Notify(DCT_ItemSelection); };
			}
			ctx->Text("Go back");
			ctx->Pop();
			ctx->Pop();

			ui::Property::Make(ctx, "Name", [&]()
			{
				auto tbName = ctx->Make<ui::Textbox>();
				tbName->text = items[editing].name;
				HandleEvent(tbName, UIEventType::Commit) = [this, tbName](UIEvent&)
				{
					items[editing].name = tbName->text;
					Rerender();
				};
			});

			ui::Property::Make(ctx, "Enable", [&]()
			{
				ctx->Make<ui::Checkbox>()->Init(items[editing].enable);//->SetInputDisabled(true);
			});
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


static const char* testNames[] =
{
	"Test: Off",
	"Test: Open/Close",
	"Test: Calculator",
	"Test: Edge slice",
	"Test: Drag and drop",
	"Test: Node editing",
	"Test: SubUI",
	"Test: Split pane",
	"Test: Layout",
	"Test: Layout 2",
	"Test: Image",
	"Test: Thread worker test",
	"Test: Threaded image rendering test",
	"Test: Sliders",
};
struct TEST : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::MenuBarElement>();
		ctx->Push<ui::MenuItemElement>()->SetText("Test");
		for (size_t i = 0; i < sizeof(testNames) / sizeof(testNames[0]); i++)
		{
			auto fn = [this, i]()
			{
				curTest = i;
				GetNativeWindow()->SetTitle(testNames[i]);
				Rerender();
			};
			ctx->Make<ui::MenuItemElement>()->SetText(testNames[i]).SetChecked(curTest == i).onActivate = fn;
		}
		ctx->Pop();
		ctx->Pop();

		switch (curTest)
		{
		case 0: break;
		case 1: ctx->Make<OpenClose>(); break;
		case 2: ctx->Make<Calculator>(); break;
		case 3: ctx->Make<EdgeSliceTest>(); break;
		case 4: ctx->Make<DragDropTest>(); break;
		case 5: ctx->Make<NodeEditTest>(); break;
		case 6: ctx->Make<SubUITest>(); break;
		case 7: ctx->Make<SplitPaneTest>(); break;
		case 8: ctx->Make<LayoutTest>(); break;
		case 9: ctx->Make<LayoutTest2>(); break;
		case 10: ctx->Make<ImageTest>(); break;
		case 11: ctx->Make<ThreadWorkerTest>(); break;
		case 12: ctx->Make<ThreadedImageRenderingTest>(); break;
		case 13: ctx->Make<SlidersTest>(); break;
		}
	}

	int curTest = 0;
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
