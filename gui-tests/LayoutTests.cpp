
#include "pch.h"


struct EdgeSliceTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);

		WPush<ui::EdgeSliceLayoutElement>();

		auto tmpl = ui::EdgeSliceLayoutElement::GetSlotTemplate();
		tmpl->edge = ui::Edge::Top;
		WMakeWithText<ui::Button>("Top");
		tmpl->edge = ui::Edge::Right;
		WMakeWithText<ui::Button>("Right");
		tmpl->edge = ui::Edge::Bottom;
		WMakeWithText<ui::Button>("Bottom");
		tmpl->edge = ui::Edge::Left;
		WMakeWithText<ui::Button>("Left");

		WPop();

		WPop();
	}
};
void Test_EdgeSlice()
{
	WMake<EdgeSliceTest>();
}


struct StackingLayoutVariationsTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

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

		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		WPush<ui::StackLTRLayoutElement>();
		for (int i = 0; i < layoutCount; i++)
		{
			BasicRadioButton2(layoutShortNames[i], mode, i) + ui::RebuildOnChange();
		}
		WText(layoutLongNames[mode]);
		WPop();
		WPop();

		if (mode == 0)
		{
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackExpandLTRLayoutElement>();
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one");
			ui::Pop();
			ui::Pop();

			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackExpandLTRLayoutElement>();
			{
				ui::Push<ui::SizeConstraintElement>().SetWidth(100);
				ui::MakeWithText<ui::Button>("One");
				ui::Pop();
			}
			ui::MakeWithText<ui::Button>("Another one");
			ui::MakeWithText<ui::Button>("The third");
			ui::Pop();
			ui::Pop();

			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackExpandLTRLayoutElement>();
			ui::MakeWithText<ui::Button>("One");
			{
				ui::Push<ui::SizeConstraintElement>().SetWidth(100);
				ui::MakeWithText<ui::Button>("Another one");
				ui::Pop();
			}
			ui::MakeWithText<ui::Button>("The third");
			ui::Pop();
			ui::Pop();

			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackExpandLTRLayoutElement>();
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one");
			{
				ui::Push<ui::SizeConstraintElement>().SetWidth(100);
				ui::MakeWithText<ui::Button>("The third");
				ui::Pop();
			}
			ui::Pop();
			ui::Pop();

			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackExpandLTRLayoutElement>();
			{
				ui::Push<ui::SizeConstraintElement>().SetMinWidth(50);
				ui::MakeWithText<ui::Button>("One");
				ui::Pop();
			}
			{
				ui::Push<ui::SizeConstraintElement>().SetMinWidth(100);
				ui::MakeWithText<ui::Button>("Another one");
				ui::Pop();
			}
			{
				ui::Push<ui::SizeConstraintElement>().SetMinWidth(150);
				ui::MakeWithText<ui::Button>("The third");
				ui::Pop();
			}
			ui::Pop();
			ui::Pop();

#if 0 // TODO restore test when the advanced size constraint element will be implemented
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackExpandLTRLayoutElement>();
			{ auto s = ui::MakeWithText<ui::Button>("One").GetStyle(); s.SetMinWidth(100); s.SetWidth(ui::Coord::Percent(30)); }
			ui::MakeWithText<ui::Button>("Another one");
			ui::Pop();
			ui::Pop();
#endif

			WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			WPush<ui::StackLTRLayoutElement>();
			ui::MakeWithText<ui::Button>("One");
			ui::MakeWithText<ui::Button>("Another one");
			WPop();
			WPop();
		}

		WPop();
	}
};
void Test_StackingLayoutVariations()
{
	ui::Make<StackingLayoutVariationsTest>();
}


struct SizeTest : ui::Buildable
{
	struct Test
	{
		UIObject* obj;
		std::function<std::string()> func;
	};

	ui::FontSettings msgFont;

	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);
		auto* font = msgFont.GetFont();
		int fsize = msgFont.size;
		float ypos = fsize;
		for (const auto& t : tests)
		{
			auto res = t.func();
			if (res.size())
			{
				float textw = ui::GetTextWidth(font, fsize, res);
				ui::draw::LineCol(t.obj->GetFinalRect().x1, t.obj->GetFinalRect().y0, GetFinalRect().x1 - textw, ypos - fsize / 2, 1, ui::Color4b(255, 0, 0));
				ui::draw::TextLine(font, fsize, GetFinalRect().x1 - textw, ypos, res, ui::Color4b(255, 0, 0));
				ypos += fsize;
			}
		}
	}

	static ui::Font* GetTestFont()
	{
		return ui::GetFont(ui::FONT_FAMILY_SANS_SERIF);
	}
	static int GetTestFontHeight()
	{
		return 12;
	}
	static int CalcTestTextWidth(const char* text)
	{
		return ceilf(ui::GetTextWidth(GetTestFont(), GetTestFontHeight(), text));
	}

	void Build() override
	{
		ui::Push<ui::StackTopDownLayoutElement>();

		tests.clear();
		ui::Text("Any errors will be drawn next to the element in red");

		TestSize(ui::Text("Testing text element size"), CalcTestTextWidth("Testing text element size"), GetTestFontHeight());

		ui::Push<ui::StackLTRLayoutElement>();
		{
			auto& txt1 = ui::MakeWithText<ui::PaddingElement>("Text size + padding");
			txt1.SetPadding(5);
			TestSize(txt1, CalcTestTextWidth("Text size + padding") + 10, GetTestFontHeight() + 10);
			TestEstSize(txt1, CalcTestTextWidth("Text size + padding") + 10, GetTestFontHeight() + 10);
		}

		{
			auto& box = ui::Push<ui::WrapperElement>();
			ui::Text("Testing text in wrapper");
			ui::Pop();
			TestSize(box, CalcTestTextWidth("Testing text in wrapper"), GetTestFontHeight());
			TestX(box, CalcTestTextWidth("Text size + padding") + 10);
		}
		ui::Pop();

		ui::Pop();
	}

	static std::string TestSize(const ui::UIRect& r, float w, float h)
	{
		if (fabsf(r.GetWidth() - w) > 0.0001f || fabsf(r.GetHeight() - h) > 0.0001f)
		{
			return ui::Format("expected %g;%g - got %g;%g", w, h, r.GetWidth(), r.GetHeight());
		}
		return {};
	}

	void TestSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			return TestSize(obj->GetFinalRect(), w, h);
		};
		tests.push_back({ &obj, fn });
	}

	void TestX(UIObject& obj, float x)
	{
		auto fn = [obj{ &obj }, x]()->std::string
		{
			auto r = obj->GetFinalRect();
			if (fabsf(r.x0 - x) > 0.0001f)
			{
				return ui::Format("expected x %g - got %g", x, r.x0);
			}
			return {};
		};
		tests.push_back({ &obj, fn });
	}

	void TestEstSize(UIObject& obj, float w, float h)
	{
		auto fn = [obj{ &obj }, w, h]()->std::string
		{
			auto ew = obj->CalcEstimatedWidth({ 500, 500 }, ui::EstSizeType::Exact);
			auto eh = obj->CalcEstimatedHeight({ 500, 500 }, ui::EstSizeType::Exact);
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
void Test_Size()
{
	ui::Make<SizeTest>();
}


struct PlacementTest : ui::Buildable
{
	PlacementTest()
	{
		buttonPlacement.bias = { 3, -2, -2, -2 };
	}
	void Build() override
	{
		ui::Push<ui::PaddingElement>().SetPadding(20);
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::Text("Expandable menu example:");

		auto* ple = &ui::Push<ui::PlacementLayoutElement>();

		ui::Push<ui::StackLTRLayoutElement>();
		ui::imm::EditBool(open, nullptr, { ui::MakeOverlay(open, 1.0f) });
		ui::Pop();

		if (open)
		{
			auto* pap = ui::BuildAlloc<ui::PointAnchoredPlacement>();
			pap->SetAnchorAndPivot({ 0, 0 });
			pap->bias = { -5, -5 };

			auto tmpl = ple->GetSlotTemplate();
			tmpl->placement = pap;
			tmpl->measure = false;

			ui::Push<ui::ListBoxFrame>() + ui::MakeOverlay();
			ui::Push<ui::StackTopDownLayoutElement>();

			// room for checkbox & spread width a bit
			ui::Make<ui::SizeConstraintElement>().SetSize(100, 25);

			ui::Text("opened");
			ui::imm::PropEditBool("One", one);
			ui::imm::PropEditInt("Two", two);

			ui::Pop();
			ui::Pop();
		}

		ui::Pop();

		ui::Text("Autocomplete example:");
		static const char* suggestions[] = { "apple", "banana", "car", "duck", "elephant", "file", "grid" };
		ple = &ui::Push<ui::PlacementLayoutElement>();
		ui::imm::EditString(text.c_str(),
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
			auto* pap = Allocate<ui::PointAnchoredPlacement>();
			pap->anchor = { 0, 1 };

			auto tmpl = ple->GetSlotTemplate();
			tmpl->placement = pap;
			tmpl->measure = false;

			ui::Push<ui::ListBoxFrame>() + ui::MakeOverlay();
			ui::Push<ui::StackTopDownLayoutElement>();

			int num = 0;
			for (int i = 0; i < 7; i++)
			{
				if (strstr(suggestions[i], text.c_str()))
				{
					ui::MakeWithText<ui::Selectable>(suggestions[i]).Init(curSelection == num)
						+ ui::AddEventHandler(ui::EventType::Activate, [this, i](ui::Event& e) { text = suggestions[i]; e.context->SetKeyboardFocus(nullptr); });
					num++;
				}
			}
			curOptionCount = num;
			if (curSelection >= curOptionCount)
				curSelection = 0;

			ui::Pop();
			ui::Pop();
		}
		ui::Pop();

		ui::Text("Self-based placement example:");
		auto tmpl = ui::Push<ui::PlacementLayoutElement>().GetSlotTemplate();
		tmpl->placement = &buttonPlacement;
		ui::MakeWithText<ui::Button>("This should not cover the entire parent");
		ui::Pop();
		ui::imm::PropEditFloatVec("Anchor", &buttonPlacement.anchor.x0, "LTRB", {}, 0.01f);
		ui::imm::PropEditFloatVec("Bias", &buttonPlacement.bias.x0, "LTRB");

		ui::Pop();
		ui::Pop();
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
void Test_Placement()
{
	ui::Make<PlacementTest>();
}

