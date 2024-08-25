
#include "pch.h"

#include "../lib-src/Render/InlineLayout.h"


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

		tests.Clear();
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
		tests.Append({ &obj, fn });
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
		tests.Append({ &obj, fn });
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
		tests.Append({ &obj, fn });
	}

	ui::Array<Test> tests;
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
		ui::Push<ui::OverlayElement>().Init(1);
		ui::imm::EditBool(open, nullptr);
		ui::Pop();
		ui::Pop();

		if (open)
		{
			auto* pap = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
			pap->SetAnchorAndPivot({ 0, 0 });
			pap->bias = { -5, -5 };

			auto tmpl = ple->GetSlotTemplate();
			tmpl->placement = pap;
			tmpl->measure = false;

			ui::Push<ui::OverlayElement>();
			ui::Push<ui::ListBoxFrame>();
			ui::Push<ui::StackTopDownLayoutElement>();

			// room for checkbox & spread width a bit
			ui::Make<ui::SizeConstraintElement>().SetSize(100, 25);

			ui::Text("opened");
			ui::imm::PropEditBool("One", one);
			ui::imm::PropEditInt("Two", two);

			ui::Pop();
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
			auto* pap = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
			pap->anchor = { 0, 1 };

			auto tmpl = ple->GetSlotTemplate();
			tmpl->placement = pap;
			tmpl->measure = false;

			ui::Push<ui::OverlayElement>();
			ui::Push<ui::ListBoxFrame>();
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
			ui::Pop();
		}
		ui::Pop();

		ui::Text("Self-based placement example:");
		auto tmpl = ui::Push<ui::PlacementLayoutElement>().GetSlotTemplate();
		tmpl->placement = &buttonPlacement;
		ui::MakeWithText<ui::Button>("This should not cover the entire parent");
		ui::Pop();
		const char* LTRB[] = { "\bL", "\bT", "\bR", "\bB", nullptr };
		ui::imm::PropEditFloatVec("Anchor", &buttonPlacement.anchor.x0, LTRB, {}, 0.01f);
		ui::imm::PropEditFloatVec("Bias", &buttonPlacement.bias.x0, LTRB);

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


namespace rgt {
int mode = 3;
bool roundPos = true;
float basicMarginFrc = 0.3f;
float fillMarginFrc = 0.4f;
ui::Size2f size = { 100, 100 };
float aspect = 1;
ui::Rangef aspectRange = { 5.f / 4.f, 16.f / 9.f };
ui::Vec2f placement = { 0.5f, 0.5f };
struct RectGenTest : ui::Buildable
{
	struct VisualizeRectGen : ui::FillerElement
	{
		float marginFrc;
		void OnPaint(const ui::UIPaintContext& ctx) override
		{
			ui::AABB2f rect = GetFinalRect();
			float margin = roundf(marginFrc * rect.GetSize().Min());
			ui::AABB2f space = rect.ShrinkBy(margin);
			ui::AABB2f gen = space;
			if (mode == 0) // centered
			{
				gen = ui::RectGenCentered(space, size, placement, roundPos);
			}
			else if (mode == 1) // fill (size)
			{
				gen = ui::RectGenFillSize(space, size, placement, roundPos);
			}
			else if (mode == 2) // fill (aspect)
			{
				gen = ui::RectGenFillAspect(space, aspect, placement, roundPos);
			}
			else if (mode == 3) // fit (size)
			{
				gen = ui::RectGenFitSize(space, size, placement, roundPos);
			}
			else if (mode == 4) // fit (aspect)
			{
				gen = ui::RectGenFitAspect(space, aspect, placement, roundPos);
			}
			else if (mode == 5) // fit (aspect range)
			{
				gen = ui::RectGenFitAspectRange(space, aspectRange, placement, roundPos);
			}

			ui::draw::AALineCol({ space.GetP00(), space.GetP10(), space.GetP11(), space.GetP01() }, 2, { 0, 0, 255 }, true);
			ui::draw::AALineCol({ gen.GetP00(), gen.GetP10(), gen.GetP11(), gen.GetP01() }, 2, { 0, 255, 0 }, true);
		}
	};
	void Build() override
	{
		ui::Push<ui::EdgeSliceLayoutElement>();
		{
			ui::Push<ui::StackLTRLayoutElement>();
			ui::imm::RadioButton(mode, 0, "Centered", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(mode, 1, "Fill (size)", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(mode, 2, "Fill (aspect)", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(mode, 3, "Fit (size)", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(mode, 4, "Fit (aspect)", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(mode, 5, "Fit (aspect range)", {}, ui::imm::ButtonStateToggleSkin());
			ui::Pop();
			ui::imm::PropEditBool("Round pos", roundPos);
			if (mode == 1 || mode == 2)
				ui::imm::PropEditFloat("Margin (fill)", fillMarginFrc, {}, { 0.01f }, { 0, 0.5f });
			else
				ui::imm::PropEditFloat("Margin", basicMarginFrc, {}, { 0.01f }, { 0, 0.5f });
			if (mode == 1 || mode == 3)
				ui::imm::PropEditFloatVec("Size", &size.x, ui::imm::WidthHeight, {}, {}, ui::Rangef::AtLeast(0));
			if (mode == 2 || mode == 4)
				ui::imm::PropEditFloat("Aspect", aspect, {}, { 0.1f, true }, { 0, 100 });
			if (mode == 5)
				ui::imm::PropEditFloatVec("Aspect range", &aspectRange.min, ui::imm::MinMax, {}, { 0.1f, true }, { 0, 100 });
			ui::imm::PropEditFloatVec("Placement", &placement.x, ui::imm::XY, {}, { 0.01f }, { 0, 1 });
		}
		auto& v = ui::Make<VisualizeRectGen>();
		v.marginFrc = mode == 1 ? fillMarginFrc : basicMarginFrc;
		ui::Pop();
	}
};
} // rgt
void Test_RectGen()
{
	ui::Make<rgt::RectGenTest>();
}


struct InlineLayoutTest : ui::Buildable
{
	ui::draw::ImageSetHandle checkerboard = ui::GetCurrentTheme()->FindImageSetByName("bgr-checkerboard");
	ui::draw::ImageSetHandle add = ui::GetCurrentTheme()->FindImageSetByName("add");
	ui::Font* font1 = ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF);
	ui::Font* font1b = ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF, ui::FONT_WEIGHT_BOLD);
	ui::Font* font1i = ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF, ui::FONT_WEIGHT_NORMAL, true);

	void Build() override
	{
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::InlineLayout il;
		il.Begin();
		il.SetFont(font1, 12, 16);
		il.AddText("test ", { 255, 127, 127 });
		il.SetFont(font1b, 12, 16);
		il.AddText("bold", { 255, 255, 127 });
		il.SetFont(font1, 12, 16);
		il.AddText(" and ", { 127, 255, 127 });
		il.SetFont(font1i, 12, 16);
		il.AddText("italic", { 127, 255, 255 });
		il.SetFont(font1, 12, 16);
		il.AddText(" text", { 127, 127, 255 });
		if (auto* img = checkerboard->FindEntryForSize({ 32, 32 }))
		{
			il.AddImage(img->image, { 32, 32 }, ui::IL_VAlign::Baseline);
			il.AddImage(img->image, { 32, 32 }, ui::IL_VAlign::Top);
			il.AddImage(img->image, { 32, 32 }, ui::IL_VAlign::Middle);
			il.AddImage(img->image, { 32, 32 }, ui::IL_VAlign::Bottom);
		}
		if (auto* img = add->FindEntryForSize({ 10, 10 }))
		{
			il.AddImage(img->image, { 10, 10 }, ui::IL_VAlign::Baseline);
		}
		il.AddText(" longer textual fragments", {});
		il.FinishLayout(GetFinalRect().GetWidth());
		il.Render(GetFinalRect().GetMin());
		for (auto& L : il.lines)
		{
			ui::draw::LineCol(0, L.lineTop, 9999, L.lineTop, 1, { 255, 0, 0, 127 });
			if (&L == &il.lines.Last())
				ui::draw::LineCol(0, L.lineBtm, 9999, L.lineBtm, 1, { 255, 0, 0, 127 });
		}
	}
};
void Test_InlineLayout()
{
	ui::Make<InlineLayoutTest>();
}

