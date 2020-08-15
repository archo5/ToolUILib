
#include "pch.h"


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
void Test_EdgeSlice(UIContainer* ctx)
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

struct LayoutNestComboTest : ui::Node
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
void Test_LayoutNestCombo(UIContainer* ctx)
{
	ctx->Make<LayoutNestComboTest>();
}


struct StackingLayoutVariationsTest : ui::Node
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
void Test_StackingLayoutVariations(UIContainer* ctx)
{
	ctx->Make<StackingLayoutVariationsTest>();
}


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
void Test_Size(UIContainer* ctx)
{
	ctx->Make<SizeTest>();
}


struct PlacementTest : ui::Node
{
	PlacementTest()
	{
		buttonPlacement.bias = { 3, -2, -2, -2 };
		buttonPlacement.applyOnLayout = true;
	}
	void Render(UIContainer* ctx) override
	{
		*this + ui::Padding(20);

		ctx->Text("Expandable menu example:");
		ctx->PushBox();

		ui::imm::EditBool(ctx, open, { ui::MakeOverlay(open, 1.0f) });

		if (open)
		{
			auto* lb = ctx->Push<ui::ListBox>();
			lb->RegisterAsOverlay();
			auto* pap = Allocate<style::PointAnchoredPlacement>();
			pap->SetAnchorAndPivot({ 0, 0 });
			pap->bias = { -5, -5 };
			lb->GetStyle().SetPlacement(pap);

			// room for checkbox
			*ctx->Make<ui::BoxElement>() + ui::Width(25) + ui::Height(25);

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
			ui::EventHandler(UIEventType::GotFocus, [this](UIEvent&) { showDropdown = true; curSelection = 0; Rerender(); }),
			ui::EventHandler(UIEventType::LostFocus, [this](UIEvent&) { showDropdown = false; Rerender(); }),
			ui::EventHandler(UIEventType::KeyAction, [this](UIEvent& e)
			{
				switch (e.GetKeyAction())
				{
				case UIKeyAction::Down:
					curSelection++;
					if (curSelection >= curOptionCount)
						curSelection = 0;
					Rerender();
					break;
				case UIKeyAction::Up:
					curSelection--;
					if (curSelection < 0)
						curSelection = std::max(curOptionCount - 1, 0);
					Rerender();
					break;
				case UIKeyAction::Enter:
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
						Rerender();
					}
					break;
				}
			}),
			});
		if (showDropdown)
		{
			auto* lb = ctx->Push<ui::ListBox>();
			lb->RegisterAsOverlay();
			auto* pap = Allocate<style::PointAnchoredPlacement>();
			pap->anchor = { 0, 1 };
			lb->GetStyle().SetPlacement(pap);

			int num = 0;
			for (int i = 0; i < 7; i++)
			{
				if (strstr(suggestions[i], text.c_str()))
				{
					*ctx->MakeWithText<ui::Selectable>(suggestions[i])->Init(curSelection == num)
						+ ui::EventHandler(UIEventType::Activate, [this, i](UIEvent& e) { text = suggestions[i]; e.context->SetKeyboardFocus(nullptr); });
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
		ctx->MakeWithText<ui::Button>("This should not cover the entire parent")->GetStyle().SetPlacement(&buttonPlacement);
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
	style::RectAnchoredPlacement buttonPlacement;
};
void Test_Placement(UIContainer* ctx)
{
	ctx->Make<PlacementTest>();
}

