
#include "pch.h"


struct EdgeSliceTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		auto s = ctx->Push<ui::Panel>().GetStyle();
		s.SetLayout(ui::layouts::EdgeSlice());
		s.SetBoxSizing(ui::BoxSizing::BorderBox);
		s.SetMargin(0);
		s.SetHeight(ui::Coord::Percent(100));

		ctx->MakeWithText<ui::Button>("Top").GetStyle().SetEdge(ui::Edge::Top);
		ctx->MakeWithText<ui::Button>("Right").GetStyle().SetEdge(ui::Edge::Right);
		ctx->MakeWithText<ui::Button>("Bottom").GetStyle().SetEdge(ui::Edge::Bottom);
		ctx->MakeWithText<ui::Button>("Left").GetStyle().SetEdge(ui::Edge::Left);

		ctx->Pop();
	}
};
void Test_EdgeSlice(ui::UIContainer* ctx)
{
	ctx->Make<EdgeSliceTest>();
}


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

struct LayoutNestComboTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		static int parentLayout = 0;
		static int childLayout = 0;

		LayoutUI(ctx, parentLayout);
		LayoutUI(ctx, childLayout);

		SetLayout(ctx->Push<ui::Panel>().GetStyle(), parentLayout, -1);
		{
			SetLayout(ctx->Push<ui::Panel>().GetStyle(), childLayout, parentLayout);
			{
				ctx->Text("[c 1A]");
				ctx->Text("[c 1B]");
			}
			ctx->Pop();

			SetLayout(ctx->Push<ui::Panel>().GetStyle(), childLayout, parentLayout);
			{
				ctx->Text("[c 2A]");
				ctx->Text("[c 2B]");
			}
			ctx->Pop();
		}
		ctx->Pop();
	}

	void LayoutUI(ui::UIContainer* ctx, int& layout)
	{
		ctx->Push<ui::Panel>().GetStyle().SetStackingDirection(ui::StackingDirection::LeftToRight);
		for (int i = 0; i < layoutCount; i++)
		{
			BasicRadioButton2(ctx, layoutShortNames[i], layout, i) + ui::RebuildOnChange();
		}
		ctx->Text(layoutLongNames[layout]);
		ctx->Pop();
	}

	void SetLayout(ui::StyleAccessor s, int layout, int parentLayout)
	{
		switch (layout)
		{
		case 0:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::TopDown);
			break;
		case 1:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::LeftToRight);
			break;
		case 2:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::BottomUp);
			break;
		case 3:
			s.SetLayout(ui::layouts::Stack());
			s.SetStackingDirection(ui::StackingDirection::RightToLeft);
			break;
		case 4:
			s.SetLayout(ui::layouts::InlineBlock());
			break;
		case 5:
		case 6:
		case 7:
		case 8:
			s.SetLayout(ui::layouts::EdgeSlice());
			break;
		}

		switch (parentLayout)
		{
		case 5:
			s.SetEdge(ui::Edge::Top);
			break;
		case 6:
			s.SetEdge(ui::Edge::Left);
			break;
		case 7:
			s.SetEdge(ui::Edge::Bottom);
			break;
		case 8:
			s.SetEdge(ui::Edge::Right);
			break;
		}
	}
};
void Test_LayoutNestCombo(ui::UIContainer* ctx)
{
	ctx->Make<LayoutNestComboTest>();
}


struct StackingLayoutVariationsTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
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

		ctx->Push<ui::Panel>().GetStyle().SetStackingDirection(ui::StackingDirection::LeftToRight);
		for (int i = 0; i < layoutCount; i++)
		{
			BasicRadioButton2(ctx, layoutShortNames[i], mode, i) + ui::RebuildOnChange();
		}
		ctx->Text(layoutLongNames[mode]);
		ctx->Pop();

		if (mode == 0)
		{
			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One").GetStyle().SetWidth(100);
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->MakeWithText<ui::Button>("The third");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one").GetStyle().SetWidth(100);
			ctx->MakeWithText<ui::Button>("The third");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->MakeWithText<ui::Button>("The third").GetStyle().SetWidth(100);
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One").GetStyle().SetMinWidth(50);
			ctx->MakeWithText<ui::Button>("Another one").GetStyle().SetMinWidth(100);
			ctx->MakeWithText<ui::Button>("The third").GetStyle().SetMinWidth(150);
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::StackExpand()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			{ auto s = ctx->MakeWithText<ui::Button>("One").GetStyle(); s.SetMinWidth(100); s.SetWidth(ui::Coord::Percent(30)); }
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->Pop();

			{ auto s = ctx->Push<ui::Panel>().GetStyle(); s.SetLayout(ui::layouts::Stack()); s.SetStackingDirection(ui::StackingDirection::LeftToRight); }
			ctx->MakeWithText<ui::Button>("One");
			ctx->MakeWithText<ui::Button>("Another one");
			ctx->Pop();
		}
	}
};
void Test_StackingLayoutVariations(ui::UIContainer* ctx)
{
	ctx->Make<StackingLayoutVariationsTest>();
}


struct SizeTest : ui::Buildable
{
	struct SizedBox : ui::Placeholder
	{
		void GetSize(ui::Coord& outWidth, ui::Coord& outHeight) override
		{
			outWidth = w;
			outHeight = h;
		}
		int w = 40;
		int h = 40;
	};
	struct Test
	{
		UIObject* obj;
		std::function<std::string()> func;
	};

	void OnPaint() override
	{
		ui::Buildable::OnPaint();
		for (const auto& t : tests)
		{
			auto res = t.func();
			if (res.size())
			{
				ui::DrawTextLine(t.obj->finalRectC.x1, t.obj->finalRectC.y1, res.c_str(), 1, 0, 0);
			}
		}
	}

	void Build(ui::UIContainer* ctx) override
	{
		tests.clear();
		ctx->Text("Any errors will be drawn next to the element in red");

		TestContentSize(ctx->Text("Testing text element size"), ceilf(ui::GetTextWidth("Testing text element size")), ui::GetFontHeight());

		ctx->PushBox() + ui::SetLayout(ui::layouts::StackExpand()) + ui::Set(ui::StackingDirection::LeftToRight);
		{
			auto& txt1 = ctx->Text("Text size + padding") + ui::SetPadding(5);
			TestContentSize(txt1, ceilf(ui::GetTextWidth("Text size + padding")), ui::GetFontHeight());
			TestFullSize(txt1, ceilf(ui::GetTextWidth("Text size + padding")) + 10, ui::GetFontHeight() + 10);
			TestFullEstSize(txt1, ceilf(ui::GetTextWidth("Text size + padding")) + 10, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5);
			ctx->Text("Testing text in box");
			ctx->Pop();
			TestContentSize(box, ceilf(ui::GetTextWidth("Testing text in box")), ui::GetFontHeight());
			TestFullSize(box, ceilf(ui::GetTextWidth("Testing text in box")) + 10, ui::GetFontHeight() + 10);
			TestFullX(box, ceilf(ui::GetTextWidth("Text size + padding")) + 10);
		}
		ctx->Pop();

		{
			auto& box = ctx->PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetBoxSizing(ui::BoxSizing::BorderBox);
			ctx->Text("Testing text in box [border]");
			ctx->Pop();
			TestContentSize(box, ceilf(ui::GetTextWidth("Testing text in box [border]")), ui::GetFontHeight());
			TestFullSize(box, ceilf(ui::GetTextWidth("Testing text in box [border]")) + 10, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetBoxSizing(ui::BoxSizing::ContentBox);
			ctx->Text("Testing text in box [content]");
			ctx->Pop();
			TestContentSize(box, ceilf(ui::GetTextWidth("Testing text in box [content]")), ui::GetFontHeight());
			TestFullSize(box, ceilf(ui::GetTextWidth("Testing text in box [content]")) + 10, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetWidth(140);
			ctx->Text("Testing text in box +W");
			ctx->Pop();
			TestContentSize(box, 140 - 10, ui::GetFontHeight());
			TestFullSize(box, 140, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetWidth(140) + ui::SetBoxSizing(ui::BoxSizing::BorderBox);
			ctx->Text("Testing text in box +W [border]");
			ctx->Pop();
			TestContentSize(box, 140 - 10, ui::GetFontHeight());
			TestFullSize(box, 140, ui::GetFontHeight() + 10);
		}

		{
			auto& box = ctx->PushBox() + ui::SetLayout(ui::layouts::InlineBlock()) + ui::SetPadding(5) + ui::SetWidth(140) + ui::SetBoxSizing(ui::BoxSizing::ContentBox);
			ctx->Text("Testing text in box +W [content]");
			ctx->Pop();
			TestContentSize(box, 140, ui::GetFontHeight());
			TestFullSize(box, 140 + 10, ui::GetFontHeight() + 10);
		}

		ctx->PushBox() + ui::Set(ui::StackingDirection::LeftToRight);
		{
			auto& box = ctx->Make<SizedBox>();
			TestContentSize(box, 40, 40);
			TestFullSize(box, 40, 40);
			TestFullEstSize(box, 40, 40);
		}
		{
			auto& box = ctx->Make<SizedBox>() + ui::SetWidth(50);
			TestContentSize(box, 50, 40);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		{
			auto& box = ctx->Make<SizedBox>() + ui::SetWidth(50) + ui::SetPadding(2, 7, 8, 3);
			TestContentSize(box, 40, 30);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		{
			auto& box = ctx->Make<SizedBox>() + ui::SetBoxSizing(ui::BoxSizing::BorderBox) + ui::SetWidth(50) + ui::SetPadding(2, 7, 8, 3);
			TestContentSize(box, 40, 30);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		{
			auto& box = ctx->Make<SizedBox>() + ui::SetBoxSizing(ui::BoxSizing::ContentBox) + ui::SetHeight(30) + ui::SetPadding(2, 7, 8, 3);
			TestContentSize(box, 40, 30);
			TestFullSize(box, 50, 40);
			TestFullEstSize(box, 50, 40);
		}
		ctx->Pop();
	}

	static std::string TestSize(ui::UIRect& r, float w, float h)
	{
		if (fabsf(r.GetWidth() - w) > 0.0001f || fabsf(r.GetHeight() - h) > 0.0001f)
		{
			return ui::Format("expected %g;%g - got %g;%g", w, h, r.GetWidth(), r.GetHeight());
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
				return ui::Format("expected x %g - got %g", x, r.x0);
			}
			return {};
		};
		tests.push_back({ &obj, fn });
	}

	void TestFullEstSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			auto ew = obj->GetFullEstimatedWidth({ 500, 500 }, ui::EstSizeType::Exact);
			auto eh = obj->GetFullEstimatedHeight({ 500, 500 }, ui::EstSizeType::Exact);
			if (ew.max < ew.min)
				return "est.width: max < min";
			if (eh.max < eh.min)
				return "est.height: max < min";
			ui::UIRect r{ 0, 0, ew.min, eh.min };
			return TestSize(r, w, h);
		};
		tests.push_back({ &obj, fn });
	}

	std::vector<Test> tests;
};
void Test_Size(ui::UIContainer* ctx)
{
	ctx->Make<SizeTest>();
}


struct PlacementTest : ui::Buildable
{
	PlacementTest()
	{
		buttonPlacement.bias = { 3, -2, -2, -2 };
		buttonPlacement.applyOnLayout = true;
	}
	void Build(ui::UIContainer* ctx) override
	{
		*this + ui::SetPadding(20);

		ctx->Text("Expandable menu example:");
		ctx->PushBox();

		ui::imm::EditBool(ctx, open, nullptr, { ui::MakeOverlay(open, 1.0f) });

		if (open)
		{
			auto& lb = ctx->Push<ui::ListBox>();
			lb.RegisterAsOverlay();
			auto* pap = Allocate<ui::PointAnchoredPlacement>();
			pap->SetAnchorAndPivot({ 0, 0 });
			pap->bias = { -5, -5 };
			lb.GetStyle().SetPlacement(pap);

			// room for checkbox
			ctx->Make<ui::BoxElement>() + ui::SetWidth(25) + ui::SetHeight(25);

			ctx->Text("opened");
			ui::imm::PropEditBool(ctx, "One", one);
			ui::imm::PropEditInt(ctx, "Two", two);

			ctx->Pop();
		}

		ctx->Pop();

		ctx->Text("Autocomplete example:");
		static const char* suggestions[] = { "apple", "banana", "car", "duck", "elephant", "file", "grid" };
		ctx->PushBox();
		ui::imm::EditString(ctx, text.c_str(),
			[this](const char* v) { text = v; }, {
			ui::AddEventHandler(ui::EventType::GotFocus, [this](ui::Event&) { showDropdown = true; curSelection = 0; Rebuild(); }),
			ui::AddEventHandler(ui::EventType::LostFocus, [this](ui::Event&) { showDropdown = false; Rebuild(); }),
			ui::AddEventHandler(ui::EventType::KeyAction, [this](ui::Event& e)
			{
				switch (e.GetKeyAction())
				{
				case ui::KeyAction::Down:
					curSelection++;
					if (curSelection >= curOptionCount)
						curSelection = 0;
					Rebuild();
					break;
				case ui::KeyAction::Up:
					curSelection--;
					if (curSelection < 0)
						curSelection = std::max(curOptionCount - 1, 0);
					Rebuild();
					break;
				case ui::KeyAction::Enter:
					if (text.size())
					{
						for (int num = 0, i = 0; i < 7; i++)
						{
							if (strstr(suggestions[i], text.c_str()))
							{
								if (num == curSelection)
								{
									text = suggestions[i];
									break;
								}
								num++;
							}
						}
						e.context->SetKeyboardFocus(nullptr);
						e.StopPropagation();
						Rebuild();
					}
					break;
				}
			}),
			});
		if (showDropdown)
		{
			auto& lb = ctx->Push<ui::ListBox>();
			lb.RegisterAsOverlay();
			auto* pap = Allocate<ui::PointAnchoredPlacement>();
			pap->anchor = { 0, 1 };
			lb.GetStyle().SetPlacement(pap);

			int num = 0;
			for (int i = 0; i < 7; i++)
			{
				if (strstr(suggestions[i], text.c_str()))
				{
					ctx->MakeWithText<ui::Selectable>(suggestions[i]).Init(curSelection == num)
						+ ui::AddEventHandler(ui::EventType::Activate, [this, i](ui::Event& e) { text = suggestions[i]; e.context->SetKeyboardFocus(nullptr); });
					num++;
				}
			}
			curOptionCount = num;
			if (curSelection >= curOptionCount)
				curSelection = 0;

			ctx->Pop();
		}
		ctx->Pop();

		ctx->Text("Self-based placement example:");
		ctx->MakeWithText<ui::Button>("This should not cover the entire parent").GetStyle().SetPlacement(&buttonPlacement);
		ui::imm::PropEditFloatVec(ctx, "Anchor", &buttonPlacement.anchor.x0, "LTRB", {}, 0.01f);
		ui::imm::PropEditFloatVec(ctx, "Bias", &buttonPlacement.bias.x0, "LTRB");
	}

	bool open = false;
	bool one = true;
	int two = 3;

	std::string text;
	bool showDropdown = false;
	int curSelection = 0;
	int curOptionCount = 0;
	ui::RectAnchoredPlacement buttonPlacement;
};
void Test_Placement(ui::UIContainer* ctx)
{
	ctx->Make<PlacementTest>();
}

