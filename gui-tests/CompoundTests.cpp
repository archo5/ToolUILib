
#include "pch.h"

#include "../lib-src/Model/Docking.h"


struct StateButtonsTest : ui::Buildable
{
	static std::string GetStateText(uint8_t s)
	{
		switch (s)
		{
		case 0: return "[ ]";
		case 1: return "[x]";
		default: return "[?]";
		}
	}
	static ui::Color4f GetStateColor(uint8_t s)
	{
		switch (s)
		{
		case 0: return ui::Color4f(1, 0.1f, 0);
		case 1: return ui::Color4f(0.3f, 1, 0);
		default: return ui::Color4f(0.9f, 0.8f, 0.1f);
		}
	}
	static ui::Color4f GetStateColorDark(uint8_t s)
	{
		switch (s)
		{
		case 0: return ui::Color4f(0.5f, 0.02f, 0);
		case 1: return ui::Color4f(0.1f, 0.5f, 0);
		default: return ui::Color4f(0.45f, 0.4f, 0.05f);
		}
	}

	void MakeContents(const char* text, int row)
	{
		WPush<ui::StackExpandLTRLayoutElement>();
		switch (row)
		{
		case 0:
			ui::Make<ui::CheckboxIcon>();
			ui::MakeWithText<ui::LabelFrame>(text);
			break;
		case 1:
			ui::Make<ui::RadioButtonIcon>();
			ui::MakeWithText<ui::LabelFrame>(text);
			break;
		case 2:
			ui::Make<ui::TreeExpandIcon>();
			ui::MakeWithText<ui::LabelFrame>(text);
			break;
		case 3:
			ui::MakeWithText<ui::StateButtonSkin>(text);
			break;
		case 4:
			ui::MakeWithText<ui::LabelFrame>(GetStateText(stb->GetState()) + text);
			break;
		case 5:
			ui::Make<ui::ColorBlock>().SetColor(GetStateColor(stb->GetState()));
			ui::MakeWithText<ui::LabelFrame>(text);
			break;
		case 6:
			ui::MakeWithText<ui::ColorBlock>(text)
				.SetColor(GetStateColorDark(stb->GetState()))
				.SetLayoutMode(ui::ImageLayoutMode::PreferredMin);
			break;
		case 7: {
			auto& fe = ui::MakeWithText<ui::LabelFrame>(text);
			fe.frameStyle.textColor.SetValue(GetStateColor(stb->GetState()));
			fe.frameStyle.font.weight = stb->GetState() == 1 ? ui::FontWeight::Bold : ui::FontWeight::Normal;
			fe.frameStyle.font.style = stb->GetState() > 1 ? ui::FontStyle::Italic : ui::FontStyle::Normal;
			break; }
		}
		WPop();
	}

	ui::RectAnchoredPlacement parts[6];
	void Build() override
	{
		constexpr int NUM_STYLES = 8;

		parts[0].anchor.x1 = parts[1].anchor.x0 = 0.15f;
		parts[1].anchor.x1 = parts[2].anchor.x0 = 0.3f;
		parts[2].anchor.x1 = parts[3].anchor.x0 = 0.45f;
		parts[3].anchor.x1 = parts[4].anchor.x0 = 0.6f;
		parts[4].anchor.x1 = parts[5].anchor.x0 = 0.8f;

		auto& ple = WPush<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		WMake<ui::FillerElement>();

		tmpl->placement = &parts[0];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::Text("CB activate");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				(stb = &WPush<ui::StateToggle>().InitReadOnly(cb1))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { cb1 = !cb1; Rebuild(); };
				MakeContents("one", i);
				WPop();
			}
		}
		WPop();

		tmpl->placement = &parts[1];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::Text("CB int.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				auto& cbbs = WPush<ui::StateToggle>().InitEditable(cb1, 2);
				(stb = &cbbs)->HandleEvent(ui::EventType::Change) = [this, &cbbs](ui::Event&) { cb1 = cbbs.GetState(); Rebuild(); };
				MakeContents("two", i);
				WPop();
			}
		}
		WPop();

		tmpl->placement = &parts[2];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::Text("CB ext.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				*(stb = &WPush<ui::CheckboxFlagT<bool>>().Init(cb1)) + ui::RebuildOnChange();
				MakeContents("three", i);
				WPop();
			}
		}
		WPop();

		tmpl->placement = &parts[3];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::Text("CB int.3-state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				auto& cbbs = WPush<ui::StateToggle>().InitEditable(cb3state, 3);
				(stb = &cbbs)->HandleEvent(ui::EventType::Change) = [this, &cbbs](ui::Event&) { cb3state = cbbs.GetState(); Rebuild(); };
				MakeContents("four", i);
				WPop();
			}
		}
		WPop();

		tmpl->placement = &parts[4];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::Text("RB activate");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				WPush<ui::StackExpandLTRLayoutElement>();

				(stb = &WPush<ui::StateToggle>().InitReadOnly(rb1 == 0))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { rb1 = 0; Rebuild(); };
				MakeContents("r1a", i);
				WPop();

				(stb = &WPush<ui::StateToggle>().InitReadOnly(rb1 == 1))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { rb1 = 1; Rebuild(); };
				MakeContents("r1b", i);
				WPop();

				WPop();
			}
		}
		WPop();

		tmpl->placement = &parts[5];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::Text("RB ext.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				WPush<ui::StackExpandLTRLayoutElement>();

				*(stb = &WPush<ui::RadioButtonT<int>>().Init(rb1, 0)) + ui::RebuildOnChange();
				MakeContents("r2a", i);
				WPop();

				*(stb = &WPush<ui::RadioButtonT<int>>().Init(rb1, 1)) + ui::RebuildOnChange();
				MakeContents("r2b", i);
				WPop();

				WPop();
			}
		}
		WPop();

		WPop();
	}

	ui::StateToggleBase* stb = nullptr;
	bool cb1 = false;
	int cb3state = 0;
	int rb1 = 0;
};
void Test_StateButtons()
{
	ui::Make<StateButtonsTest>();
}


struct PropertyListTest : ui::Buildable
{
	void Build() override
	{
		ui::Push<ui::PropertyList>();
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::Push<ui::LabeledProperty>().SetText("label for 1");
		ui::Text("test 1");
		ui::Pop();

		ui::MakeWithText<ui::Button>("interjection");

		ui::Push<ui::LabeledProperty>().SetText("and for 2").SetLabelBold();
		ui::MakeWithText<ui::Button>("test 2");
		ui::Pop();

		ui::Push<ui::PaddingElement>().SetPaddingLeft(32);
		ui::Push<ui::StackTopDownLayoutElement>();
		{
			ui::Push<ui::LabeledProperty>().SetText("also 3");
			ui::Text("test 3 elevated");
			ui::Pop();

			ui::Push<ui::LabeledProperty>().SetText("and 4 (brief sublabels)");
			ui::Push<ui::StackExpandLTRLayoutElement>();
			{
				ui::Push<ui::LabeledProperty>().SetText("X").SetBrief(true);
				ui::MakeWithText<ui::Button>("A");
				ui::Pop();

				ui::Push<ui::LabeledProperty>().SetText("Y").SetBrief(true);
				ui::MakeWithText<ui::Button>("B");
				ui::Pop();
			}
			ui::Pop();
			ui::Pop();
		}
		ui::Pop();
		ui::Pop();

		ui::Pop();
		ui::Pop();
	}
};
void Test_PropertyList()
{
	ui::Make<PropertyListTest>();
}


struct SlidersTest : ui::Buildable
{
	struct MyGradientPainter : ui::IPainter
	{
		ui::ContentPaintAdvice Paint(const ui::PaintInfo& info) override
		{
			auto r = info.rect;
			ui::draw::RectGradH(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0, 0, 0), ui::Color4f(1, 0, 0));
			return {};
		}
	};

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		static float sldval0 = 0.63f;
		ui::Make<ui::Slider>().Init(sldval0, { 0, 1 });

		ui::LabeledProperty::Begin("Slider 1: 0-2 step=0");
		static float sldval1 = 0.63f;
		ui::Make<ui::Slider>().Init(sldval1, { 0, 2 });
		ui::LabeledProperty::End();

		ui::LabeledProperty::Begin("Slider 2: 0-2 step=0.1");
		static float sldval2 = 0.63f;
		ui::Make<ui::Slider>().Init(sldval2, { 0, 2, 0.1 });
		ui::LabeledProperty::End();

		ui::LabeledProperty::Begin("Slider 3: custom track bg");
		static float sldval3 = 0.63f;
		{
			auto& s = ui::Make<ui::Slider>().Init(sldval3, { 0, 1 });
			auto lp = ui::LayerPainter::Create();
			lp->layers.Append(s.style.trackBasePainter);
			lp->layers.Append(new MyGradientPainter);
			s.style.trackBasePainter = lp;
			s.style.trackFillPainter = ui::EmptyPainter::Get();
		}
		ui::LabeledProperty::End();

		ui::LabeledProperty::Begin("Slider 4: vert stretched");
		ui::Push<ui::SizeConstraintElement>().SetHeight(40);
		ui::Make<ui::Slider>().Init(sldval2, { 0, 2, 0.1 });
		ui::Pop();
		ui::LabeledProperty::End();

		ui::LabeledProperty::Begin("Color picker parts");
		static float hue = 0.6f, sat = 0.3f, val = 0.8f;
		{
			ui::Make<ui::HueSatPicker>().SetSize(100, 100).Init(hue, sat);
		}
		{
			ui::Make<ui::ColorCompPicker2D>().SetSize(120, 100).Init(hue, sat);
		}
		ui::LabeledProperty::End();

		WPop();
	}
};
void Test_Sliders()
{
	ui::Make<SlidersTest>();
}


struct SplitPaneTest : ui::Buildable
{
	void Build() override
	{
		ui::Push<ui::SplitPane>();

		ui::MakeWithText<ui::FrameElement>("Pane A").SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::MakeWithText<ui::FrameElement>("Pane B").SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);

		ui::Push<ui::SplitPane>().SetDirection(ui::Direction::Vertical);

		ui::MakeWithText<ui::FrameElement>("Pane C").SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::MakeWithText<ui::FrameElement>("Pane D").SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::MakeWithText<ui::FrameElement>("Pane E").SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);

		ui::Pop();

		ui::Pop();
	}
};
void Test_SplitPane()
{
	ui::Make<SplitPaneTest>();
}


struct TabsTest : ui::Buildable
{
	int tp1idx[2] = { 0, 1 };
	const char* tabnames[2] = { "First tab", "Second tab" };
	int tp2idx[2] = { 0, 1 };

	void Build() override
	{
		ui::Push<ui::StackTopDownLayoutElement>();

		auto& tp1 = ui::Push<ui::TabbedPanel>();
		tp1.allowDragReorder = true;
		tp1.allowDragRemove = true;

		for (int i = 0; i < 2; i++)
		{
			switch (tp1idx[i])
			{
			case 0:
				tp1.AddTextTab("First tab", 0);
				break;
			case 1: {
				auto& sltr = ui::PushNoAppend<ui::StackLTRLayoutElement>();
				sltr.paddingBetweenElements = 4;
				ui::MakeWithText<ui::LabelFrame>("Second tab");
				ui::MakeWithText<ui::Button>("button");
				ui::Pop();
				tp1.AddUITab(&sltr, 1);
				break; }
			}
		}

		tp1.SetActiveTabByUID(tab1);
		tp1.HandleEvent(&tp1, ui::EventType::SelectionChange) = [this, &tp1](ui::Event&)
		{
			tab1 = int(tp1.GetCurrentTabUID(0));
		};
		tp1.HandleEvent(&tp1, ui::EventType::Change) = [this, &tp1](ui::Event& e)
		{
			if (e.arg1 == UINTPTR_MAX)
			{
				printf("Removed tab %d\n", int(e.arg0));
				return;
			}
			// only 2 tabs
			std::swap(tp1idx[0], tp1idx[1]);
		};
		{
			ui::Push<ui::StackTopDownLayoutElement>();
			ui::Push<ui::StackTopDownLayoutElement>().SetVisible(tp1.GetCurrentTabUID() == 0);
			{
				ui::Text("Contents of the first tab (SetVisible)");
			}
			ui::Pop();

			ui::Push<ui::StackTopDownLayoutElement>().SetVisible(tp1.GetCurrentTabUID() == 1);
			{
				ui::Text("Contents of the second tab (SetVisible)");
			}
			ui::Pop();
			ui::Pop();
		}
		ui::Pop();

		auto& tp2 = ui::Push<ui::TabbedPanel>();
		tp2.allowDragReorder = true;
		tp2.allowDragRemove = true;
		tp2.showCloseButton = true;
		for (int i = 0; i < 2; i++)
			tp2.AddTextTab(tabnames[tp2idx[i]], tp2idx[i]);
		tp2.SetActiveTabByUID(tab2);
		tp2.HandleEvent(&tp2, ui::EventType::SelectionChange) = [this, &tp2](ui::Event&)
		{
			tab2 = int(tp2.GetCurrentTabUID(0));
		};
		tp2.HandleEvent(&tp2, ui::EventType::Change) = [this, &tp2](ui::Event& e)
		{
			if (e.arg1 == UINTPTR_MAX)
			{
				printf("Removed tab %d\n", int(e.arg0));
				return;
			}
			// only 2 tabs
			std::swap(tp2idx[0], tp2idx[1]);
		};
		{
			if (tab2 == 0)
			{
				ui::Text("Contents of the first tab (conditional build)");
			}

			if (tab2 == 1)
			{
				ui::Text("Contents of the second tab (conditional build)");
			}
		}
		ui::Pop(); // TabbedPanel

		//ui::Make<ui::DefaultOverlayBuilder>();

		ui::Pop(); // StackTopDownLayoutElement
	}

	int tab1 = 0, tab2 = 0;
};
void Test_Tabs()
{
	ui::Make<TabsTest>();
}


struct TransformContainerTest : ui::Buildable
{
	float x = 0;
	float y = 0;
	float scale = 1;

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		{
			ui::LabeledProperty::Scope lp("X");
			ui::MakeWithText<ui::Button>("-") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { x -= 10; Rebuild(); });
			ui::MakeWithTextf<ui::Header>("%g", x);
			ui::MakeWithText<ui::Button>("+") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { x += 10; Rebuild(); });
		}
		{
			ui::LabeledProperty::Scope lp("Y");
			ui::MakeWithText<ui::Button>("-") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { y -= 10; Rebuild(); });
			ui::MakeWithTextf<ui::Header>("%g", y);
			ui::MakeWithText<ui::Button>("+") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { y += 10; Rebuild(); });
		}
		{
			ui::LabeledProperty::Scope lp("Scale");
			ui::MakeWithText<ui::Button>("-") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { scale -= 0.1f; Rebuild(); });
			ui::MakeWithTextf<ui::Header>("%g", scale);
			ui::MakeWithText<ui::Button>("+") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { scale += 0.1f; Rebuild(); });
		}

		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		{
			auto& te = ui::Push<ui::ChildScaleOffsetElement>();
			te.transform = ui::ScaleOffset2D::OffsetThenScale(x, y, scale);
			ui::Push<ui::StackTopDownLayoutElement>();
			{
				ui::MakeWithText<ui::Button>("One button");
				ui::MakeWithText<ui::Button>("The second button");
				ui::MakeWithText<ui::Button>("#3");
			}
			ui::Pop();
			ui::Pop();
		}
		ui::Pop();

		WPop();
	}
};
void Test_TransformContainer()
{
	ui::Make<TransformContainerTest>();
}


struct ScrollbarsTest : ui::Buildable
{
	int count = 20;
	bool expanding = true;

	void Build() override
	{
		WPush<ui::EdgeSliceLayoutElement>();

		ui::imm::PropEditInt("\bCount", count);
		ui::imm::PropEditBool("\bExpanding", expanding);

		auto& sc = WPush<ui::SizeConstraintElement>();
		if (!expanding)
			sc.SetSize(300, 200);
		WPush<ui::ScrollArea>();
		WPush<ui::StackTopDownLayoutElement>();

		for (int i = 0; i < count; i++)
			ui::Textf("Inside scroll area [%d]", i);

		WPop();
		WPop();
		WPop();

		WPop();
	}
};
void Test_Scrollbars()
{
	ui::Make<ScrollbarsTest>();
}


struct TextboxTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Single line / selection (read only)");
		ui::Make<ui::Textbox>().SetText("test 3.14 abc").SetInputDisabled(true);

		ui::Text("Single line / editing");
		ui::Make<ui::Textbox>().SetText("test 3.14 abc");

		ui::Text("Multiline / editing");
		ui::Make<ui::Textbox>().SetMultiline(true).SetText("test\n3.14\nabc");

		WPop();
	}
};
void Test_Textbox()
{
	ui::Make<TextboxTest>();
}


struct ColorBlockTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		WText("Default color block");
		WMake<ui::ColorBlock>().SetColor(colorA);
		WMake<ui::ColorBlock>().SetColor(colorB);

		WText("Without edge");
		WMake<ui::ColorBlock>().SetColor(colorA).RemoveFrameStyle();
		WMake<ui::ColorBlock>().SetColor(colorB).RemoveFrameStyle();

		WText("Custom size");
		WPush<ui::SizeConstraintElement>().SetSize(200, 40);
		WMake<ui::ColorBlock>().SetColor(colorA);
		WPop();
		WPush<ui::SizeConstraintElement>().SetSize(200, 40);
		WMake<ui::ColorBlock>().SetColor(colorB);
		WPop();

		WText("Color inspect block");
		WMake<ui::ColorInspectBlock>().SetColor(colorA);
		WMake<ui::ColorInspectBlock>().SetColor(colorB);

		WText("Assembled");
		auto C = colorB;
		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox).SetPadding(3);
		{
			auto& ple = WPush<ui::PlacementLayoutElement>();
			auto tmpl = ple.GetSlotTemplate();

			WPush<ui::SizeConstraintElement>().SetHeight(20);
			WMake<ui::FillerElement>();
			WPop();

			auto* partOC = UI_BUILD_ALLOC(ui::RectAnchoredPlacement)();
			partOC->anchor.x1 = 0.5f;
			partOC->bias.y1 = -4;
			tmpl->placement = partOC;
			tmpl->measure = false;
			WMake<ui::ColorBlock>().SetColor(C.GetOpaque()).RemoveFrameStyle();

			auto* partC = UI_BUILD_ALLOC(ui::RectAnchoredPlacement)();
			partC->anchor.x0 = 0.5f;
			partC->bias.y1 = -4;
			tmpl->placement = partC;
			tmpl->measure = false;
			WMake<ui::ColorBlock>().SetColor(C).RemoveFrameStyle();

			auto* partOA = UI_BUILD_ALLOC(ui::RectAnchoredPlacement)();
			partOA->anchor.x1 = C.a / 255.f;
			partOA->anchor.y0 = 1;
			partOA->bias.y0 = -4;
			tmpl->placement = partOA;
			tmpl->measure = false;
			WMake<ui::ColorBlock>().SetColor(ui::Color4b::White()).RemoveFrameStyle();

			auto* partTA = UI_BUILD_ALLOC(ui::RectAnchoredPlacement)();
			partTA->anchor.x0 = C.a / 255.f;
			partTA->anchor.y0 = 1;
			partTA->bias.y0 = -4;
			tmpl->placement = partTA;
			tmpl->measure = false;
			WMake<ui::ColorBlock>().SetColor(ui::Color4b::Black()).RemoveFrameStyle();

			WPop();
		}
		WPop();

		WPop();
	}

	ui::Color4b colorA = { 240, 180, 120, 255 };
	ui::Color4b colorB = { 120, 180, 240, 120 };
};
void Test_ColorBlock()
{
	WMake<ColorBlockTest>();
}


struct ImageTest : ui::Buildable
{
	ImageTest()
	{
		ui::Canvas c(32, 32);
		auto p = c.GetPixels();
		for (uint32_t i = 0; i < c.GetWidth() * c.GetHeight(); i++)
		{
			p[i] = (i / 4 + i / 4 / c.GetHeight()) % 2 ? 0xffffffff : 0;
		}
		img = ui::draw::ImageCreateFromCanvas(c);
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::ScaleMode scaleModes[3] = { ui::ScaleMode::Stretch, ui::ScaleMode::Fit, ui::ScaleMode::Fill };
		const char* scaleModeNames[3] = { "Stretch", "Fit", "Fill" };

		for (int mode = 0; mode < 6; mode++)
		{
			WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox).SetPadding(4);/*+ ui::SetMargin(0);*/
			WPush<ui::WrapperLTRLayoutElement>();

			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox).SetPadding(4);/*+ ui::SetMargin(0);*/
					auto& sc = WPush<ui::SizeConstraintElement>();
					if (mode / 3)
						sc.SetWidth(25);
					else
						sc.SetHeight(25);
					WMake<ui::ImageElement>()
						.SetImage(img)
						.SetScaleMode(scaleModes[mode % 3], x * 0.5f + 0.5f, y * 0.5f + 0.5f);
					WPop();
					WPop();
				}
			}

			WText(scaleModeNames[mode % 3]);

			WPop();
			WPop();
		}

		WPush<ui::SizeConstraintElement>().SetSize(50, 50);
		WMake<ui::ImageElement>().SetImage(img);
		WPop();

		WPop();
	}

	ui::draw::ImageHandle img;
};
void Test_Image()
{
	WMake<ImageTest>();
}


static ui::Color4f colorPickerTestCol;
struct ColorPickerTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::LayerLayoutElement>();

		auto& cp = WMake<ui::ColorPicker>().SetColor(colorPickerTestCol);
		cp.HandleEvent(ui::EventType::Change) = [&cp](ui::Event& e)
		{
			colorPickerTestCol = cp.GetColor().GetRGBA();
		};

		WMake<ui::DefaultOverlayBuilder>();

		WPop(); // TODO: hack (?), needed for overlay builder above
	}
};
void Test_ColorPicker()
{
	WMake<ColorPickerTest>();
}


static bool imguiTestDisableAll;
struct IMGUITest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::LabeledProperty::Begin("All disabled");
		ui::imm::EditBool(imguiTestDisableAll, nullptr);
		ui::LabeledProperty::End();

		bool oldEnabled = ui::imm::SetEnabled(!imguiTestDisableAll);

		ui::LabeledProperty::Begin("buttons");
		if (ui::imm::Button("working button"))
			puts("working button");
		if (ui::imm::Button("disabled button", { ui::Enable(false) }))
			puts("DISABLED button SHOULD NOT APPEAR");
		ui::LabeledProperty::End();

		{
			ui::LabeledProperty::Begin("bool");
			auto tmp = boolVal;
			if (ui::imm::EditBool(tmp, "working"))
				boolVal = tmp;
			if (ui::imm::CheckboxRaw(tmp, "w2", {}, ui::imm::ButtonStateToggleSkin()))
				boolVal = !tmp;
			if (ui::imm::EditBool(tmp, "disabled", { ui::Enable(false) }))
				boolVal = tmp;
			if (ui::imm::CheckboxRaw(tmp, "d2", { ui::Enable(false) }, ui::imm::ButtonStateToggleSkin()))
				boolVal = !tmp;
			ui::LabeledProperty::End();
		}

		{
			ui::LabeledProperty::Begin("int format: %d");
			auto tmp = intFmt;
			if (ui::imm::RadioButton(tmp, 0, "working"))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(tmp == 0, "w2", {}, ui::imm::ButtonStateToggleSkin()))
				intFmt = 0;
			if (ui::imm::RadioButton(tmp, 0, "disabled", { ui::Enable(false) }))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(tmp == 0, "d2", { ui::Enable(false) }, ui::imm::ButtonStateToggleSkin()))
				intFmt = 0;
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("int format: %x");
			auto tmp = intFmt;
			if (ui::imm::RadioButton(tmp, 1, "working"))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(tmp == 1, "w2", {}, ui::imm::ButtonStateToggleSkin()))
				intFmt = 1;
			if (ui::imm::RadioButton(tmp, 1, "disabled", { ui::Enable(false) }))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(tmp == 1, "d2", { ui::Enable(false) }, ui::imm::ButtonStateToggleSkin()))
				intFmt = 1;
			ui::LabeledProperty::End();
		}
		{
			ui::imm::PropDropdownMenuList("dropdown", intFmt, UI_BUILD_ALLOC(ui::ZeroSepCStrOptionList)("Decimal\0Hex\0"));
		}

		{
			ui::LabeledProperty::Begin("int");
			auto tmp = intVal;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { -543, 1234 }, intFmt ? "%x" : "%d"))
				intVal = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { -543, 1234 }, intFmt ? "%x" : "%d"))
				intVal = tmp;

			ui::MakeWithText<ui::LabelFrame>("int: " + std::to_string(intVal));
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("uint");
			auto tmp = uintVal;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { 0, 1234 }, intFmt ? "%x" : "%d"))
				uintVal = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { 0, 1234 }, intFmt ? "%x" : "%d"))
				uintVal = tmp;

			ui::MakeWithText<ui::LabelFrame>("uint: " + std::to_string(uintVal));
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("int64");
			auto tmp = int64Val;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { -543, 1234 }, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { -543, 1234 }, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;

			ui::MakeWithText<ui::LabelFrame>("int64: " + std::to_string(int64Val));
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("uint64");
			auto tmp = uint64Val;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { 0, 1234 }, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { 0, 1234 }, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;

			ui::MakeWithText<ui::LabelFrame>("uint64: " + std::to_string(uint64Val));
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("float");
			auto tmp = floatVal;
			if (ui::imm::PropEditFloat("\bworking", tmp, {}, { 0.1f }, { -37.4f, 154.1f }))
				floatVal = tmp;
			if (ui::imm::PropEditFloat("\bdisabled", tmp, { ui::Enable(false) }, { 0.1f }, { -37.4f, 154.1f }))
				floatVal = tmp;

			ui::MakeWithText<ui::LabelFrame>("float: " + std::to_string(floatVal));
			ui::LabeledProperty::End();
		}
		{
			ui::imm::PropEditFloatVec("float3", float4val, "XYZ");
			ui::imm::PropEditFloatVec("float4", float4val, "RGBA");
		}
		{
			ui::imm::PropEditFloat("multiplier", multiplierVal, {}, { 1.0f, true }, { 0.001f, 1000.0f });
		}
		{
			ui::imm::PropEditColor("color B (Delayed)", colorValB, true);
			ui::imm::PropEditColor("color F (Delayed)", colorValF, true);
		}
		{
			ui::imm::PropEditColor("color B (Immediate)", colorValB, false);
			ui::imm::PropEditColor("color F (Immediate)", colorValF, false);
		}

		ui::imm::SetEnabled(oldEnabled);

		WPop();
	}

	bool boolVal = true;
	int intFmt = 0;
	int intVal = 15;
	int64_t int64Val = 123;
	unsigned uintVal = 1;
	uint64_t uint64Val = 2;
	float floatVal = 3.14f;
	float float4val[4] = { 1, 2, 3, 4 };
	float multiplierVal = 2;
	ui::Color4b colorValB = { 180, 200, 220, 255 };
	ui::Color4f colorValF = { 0.9f, 0.7f, 0.5f, 0.8f };
};
void Test_IMGUI()
{
	ui::Make<IMGUITest>();
}


struct TooltipTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::MakeWithText<ui::Button>("Text-only tooltip") + ui::AddTooltip("Text only");
		ui::MakeWithText<ui::Button>("Checklist tooltip") + ui::AddTooltip([]()
		{
			ui::Push<ui::SizeConstraintElement>().SetMinWidth(100);
			ui::Push<ui::StackTopDownLayoutElement>();
			bool t = true, f = false;
			ui::imm::EditBool(t, "Done", { ui::Enable(false) });
			ui::imm::EditBool(f, "Not done", { ui::Enable(false) });
			ui::Pop();
			ui::Pop();
		});

		ui::Make<ui::DefaultOverlayBuilder>();

		WPop();
	}
};
void Test_Tooltip()
{
	ui::Make<TooltipTest>();
}


struct DropdownTest : ui::Buildable
{
	ui::RectAnchoredPlacement parts[2];
	uintptr_t sel3opts = 1;
	uintptr_t selPtr = uintptr_t(&typeid(ui::Buildable));
	uintptr_t selLON = 1;
	const type_info* selPtrReal = &typeid(ui::Buildable);

	void Build() override
	{
		for (int i = 0; i < 2; i++)
		{
			parts[i].anchor.x0 = i / 3.f;
			parts[i].anchor.x1 = (i + 1) / 3.f;
		}

		auto& ple = WPush<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		WMake<ui::FillerElement>();

		tmpl->placement = &parts[0];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			WMake<SpecificDropdownMenu>();

			WText("[zssl] unlimited options");
			MenuList(sel3opts, UI_BUILD_ALLOC(ui::ZeroSepCStrOptionList)("First\0Second\0Third\0"));
			WText("[zssl] limited options");
			MenuList(sel3opts, UI_BUILD_ALLOC(ui::ZeroSepCStrOptionList)(2, "First\0Second", "<none>"));

			static const char* options[] = { "First", "Second", "Third", nullptr };

			WText("[sa] unlimited options");
			MenuList(sel3opts, UI_BUILD_ALLOC(ui::CStrArrayOptionList)(options));
			WText("[sa] limited options");
			MenuList(sel3opts, UI_BUILD_ALLOC(ui::CStrArrayOptionList)(2, options));

			WText("custom pointer options");
			MenuList(selPtr, UI_BUILD_ALLOC(TypeInfoOptions)());

			WText("lots of options");
			MenuList(selLON, UI_BUILD_ALLOC(LotsOfOptions)());
		}
		WPop();

		tmpl->placement = &parts[1];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		{
			WText("immediate mode");
			ui::imm::DropdownMenuList(sel3opts, UI_BUILD_ALLOC(ui::ZeroSepCStrOptionList)("First\0Second\0Third\0"));
			ui::imm::DropdownMenuList(selPtrReal, UI_BUILD_ALLOC(TypeInfoOptions)());

			WPush<ui::PropertyList>();
			{
				WPush<ui::LabeledProperty>().SetText("label 1");
				WMake<SpecificDropdownMenu>();
				WPop();
			}
			WPop();

			WPush<ui::LabeledProperty>().SetText("label 2");
			WMake<SpecificDropdownMenu>();
			WPop();
		}
		WPop();

		WPop();
	}

	void MenuList(uintptr_t& sel, ui::OptionList* list)
	{
		auto& ddml = ui::Make<ui::DropdownMenuList>();
		ddml.SetSelected(sel);
		ddml.SetOptions(list);
		ddml.HandleEvent(ui::EventType::Commit) = [this, &ddml, &sel](ui::Event& e)
		{
			if (e.target != &ddml)
				return;
			sel = ddml.GetSelected();
			Rebuild();
		};
	}

	struct SpecificDropdownMenu : ui::DropdownMenu
	{
		void OnBuildButtonContents() override
		{
			WText("Menu");
		}
		void OnBuildMenuContents() override
		{
			static bool flag1, flag2;

			WPush<ui::CheckboxFlagT<bool>>().Init(flag1);
			WPush<ui::StackExpandLTRLayoutElement>();
			WMake<ui::CheckboxIcon>();
			WMakeWithText<ui::LabelFrame>("Option 1");
			WPop();
			WPop();

			WPush<ui::CheckboxFlagT<bool>>().Init(flag2);
			WPush<ui::StackExpandLTRLayoutElement>();
			WMake<ui::CheckboxIcon>();
			WMakeWithText<ui::LabelFrame>("Option 2");
			WPop();
			WPop();
		}
	};

	struct TypeInfoOptions : ui::OptionList
	{
		void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn)
		{
			static const type_info* types[] =
			{
				nullptr,
				&typeid(ui::UIObject),
				&typeid(ui::WrapperElement),
				&typeid(ui::FillerElement),
				&typeid(ui::Buildable),
				&typeid(ui::TextElement),
			};
			for (size_t i = 0; i < count && i + from < sizeof(types) / sizeof(types[0]); i++)
			{
				fn(types[i], uintptr_t(types[i]));
			}
		}
		void BuildElement(const void* ptr, uintptr_t id, bool list)
		{
			WText(ptr ? static_cast<const type_info*>(ptr)->name() : "<none>");
		}
	};

	struct LotsOfOptions : ui::OptionList
	{
		void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override
		{
			for (size_t i = 0; i < count && i + from < 1000; i++)
			{
				fn(nullptr, i + from);
			}
		}
		void BuildElement(const void* ptr, uintptr_t id, bool list) override
		{
			WText(ui::Format("%d", int(id)));
		}
	};
};
void Test_Dropdown()
{
	WMake<DropdownTest>();
}


struct DockingTest : ui::Buildable, ui::DockableContentsSource
{
	struct DockableTest : ui::DockableContents
	{
		ui::StringView id;
		std::string name;

		ui::StringView GetID() const override
		{
			return this->id;
		}
		std::string GetTitle() override
		{
			return name;
		}
		void Build() override
		{
			WMakeWithText<ui::Button>(name);
		}
	};

	ui::DockingMainArea* area;
	DockableTest* d1;
	DockableTest* d2;
	DockableTest* d3;
	std::string mem;

	ui::DockableContents* GetDockableContentsByID(ui::StringView id) override
	{
		if (id == "dockable1")
			return d1;
		if (id == "dockable2")
			return d2;
		if (id == "dockable3")
			return d3;
		return nullptr;
	}

	DockingTest()
	{
		area = ui::CreateUIObject<ui::DockingMainArea>();
		area->SetSource(this);
		d1 = ui::CreateUIObject<DockableTest>();
		d1->id = "dockable1";
		d1->name = "Dockable 1";
		d2 = ui::CreateUIObject<DockableTest>();
		d2->id = "dockable2";
		d2->name = "Dockable 2";
		d3 = ui::CreateUIObject<DockableTest>();
		d3->id = "dockable3";
		d3->name = "Dockable 3";

		Reset();
	}
	void Reset()
	{
		using namespace ui::dockdef;
		area->RemoveSubwindows();
		area->SetMainAreaContents
		(
			HSplit
			(
				Tabs({ "dockable1" }),
				0.5f,
				VSplit
				(
					Tabs({ "dockable2" }),
					0.5f,
					Tabs({ "dockable3" })
				)
			)
		);
	}
	~DockingTest()
	{
		ui::DeleteUIObject(d3);
		ui::DeleteUIObject(d2);
		ui::DeleteUIObject(d1);
		ui::DeleteUIObject(area);
	}
	void Build() override
	{
		using namespace ui::imm;

		WPush<ui::EdgeSliceLayoutElement>();
		{
			WPush<ui::StackLTRLayoutElement>();
			if (Button("Reset"))
				Reset();
			if (Button("Show 1"))
			{
				if (!area->HasDockable("dockable1"))
					area->AddSubwindow(ui::dockdef::Tabs({ "dockable1" }));
			}
			if (Button("Show 2"))
			{
				if (!area->HasDockable("dockable2"))
					area->AddSubwindow(ui::dockdef::Tabs({ "dockable2" }));
			}
			if (Button("Show 3"))
			{
				if (!area->HasDockable("dockable3"))
					area->AddSubwindow(ui::dockdef::Tabs({ "dockable3" }));
			}
			if (Button("->Mem"))
			{
				ui::JSONSerializerObjectIterator w;
				area->OnSerialize(w, {});
				mem = w.GetData();
				printf("[->Mem] serialized to json:\n%s\n", mem.c_str());
			}
			if (Button("Mem->"))
			{
				ui::JSONUnserializerObjectIterator u;
				if (u.Parse(mem))
					area->OnSerialize(u, {});
				else
					printf("[Mem->] failed to parse json?\n%s\n", mem.c_str());
			}
			WPop();

			ui::Add(area);
		}
		WPop();
	}
};
void Test_Docking()
{
	WMake<DockingTest>();
}

