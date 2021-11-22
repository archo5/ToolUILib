
#include "pch.h"


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
		switch (row)
		{
		case 0:
			ui::Make<ui::CheckboxIcon>();
			ui::Text(text) + ui::SetPadding(4); // TODO consistent padding from theme?
			break;
		case 1:
			ui::Make<ui::RadioButtonIcon>();
			ui::Text(text) + ui::SetPadding(4);
			break;
		case 2:
			ui::Make<ui::TreeExpandIcon>();
			ui::Text(text) + ui::SetPadding(4);
			break;
		case 3:
			ui::MakeWithText<ui::StateButtonSkin>(text);
			break;
		case 4:
			ui::Text(GetStateText(stb->GetState()) + text) + ui::SetPadding(5);
			break;
		case 5:
			ui::Make<ui::ColorBlock>().SetColor(GetStateColor(stb->GetState()));
			ui::Text(text) + ui::SetPadding(4);
			break;
		case 6:
			ui::MakeWithText<ui::ColorBlock>(text)
				.SetColor(GetStateColorDark(stb->GetState()))
				+ ui::SetWidth(ui::Coord::Undefined());
			break;
		case 7: {
			auto s = ui::Text(text).GetStyle();
			s.SetPadding(5);
			s.SetTextColor(GetStateColor(stb->GetState()));
			s.SetFontWeight(stb->GetState() == 1 ? ui::FontWeight::Bold : ui::FontWeight::Normal);
			s.SetFontStyle(stb->GetState() > 1 ? ui::FontStyle::Italic : ui::FontStyle::Normal);
			break; }
		}
	}

	void Build() override
	{
		constexpr int NUM_STYLES = 8;

		GetStyle().SetStackingDirection(ui::StackingDirection::LeftToRight);

		ui::PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ui::Text("CB activate");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				(stb = &ui::Push<ui::StateToggle>().InitReadOnly(cb1))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { cb1 = !cb1; Rebuild(); };
				MakeContents("one", i);
				ui::Pop();
			}
		}
		ui::Pop();

		ui::PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ui::Text("CB int.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				auto& cbbs = ui::Push<ui::StateToggle>().InitEditable(cb1, 2);
				(stb = &cbbs)->HandleEvent(ui::EventType::Change) = [this, &cbbs](ui::Event&) { cb1 = cbbs.GetState(); Rebuild(); };
				MakeContents("two", i);
				ui::Pop();
			}
		}
		ui::Pop();

		ui::PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ui::Text("CB ext.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				*(stb = &ui::Push<ui::CheckboxFlagT<bool>>().Init(cb1)) + ui::RebuildOnChange();
				MakeContents("three", i);
				ui::Pop();
			}
		}
		ui::Pop();

		ui::PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ui::Text("CB int.3-state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				auto& cbbs = ui::Push<ui::StateToggle>().InitEditable(cb3state, 3);
				(stb = &cbbs)->HandleEvent(ui::EventType::Change) = [this, &cbbs](ui::Event&) { cb3state = cbbs.GetState(); Rebuild(); };
				MakeContents("four", i);
				ui::Pop();
			}
		}
		ui::Pop();

		ui::PushBox().GetStyle().SetWidth(ui::Coord::Percent(20));
		{
			ui::Text("RB activate");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				ui::Property::Scope ps;

				(stb = &ui::Push<ui::StateToggle>().InitReadOnly(rb1 == 0))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { rb1 = 0; Rebuild(); };
				MakeContents("r1a", i);
				ui::Pop();

				(stb = &ui::Push<ui::StateToggle>().InitReadOnly(rb1 == 1))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { rb1 = 1; Rebuild(); };
				MakeContents("r1b", i);
				ui::Pop();
			}
		}
		ui::Pop();

		ui::PushBox().GetStyle().SetWidth(ui::Coord::Percent(20));
		{
			ui::Text("RB ext.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				ui::Property::Scope ps;

				*(stb = &ui::Push<ui::RadioButtonT<int>>().Init(rb1, 0)) + ui::RebuildOnChange();
				MakeContents("r2a", i);
				ui::Pop();

				*(stb = &ui::Push<ui::RadioButtonT<int>>().Init(rb1, 1)) + ui::RebuildOnChange();
				MakeContents("r2b", i);
				ui::Pop();
			}
		}
		ui::Pop();
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

		ui::Push<ui::LabeledProperty>().SetText("label for 1");
		ui::Text("test 1");
		ui::Pop();

		ui::MakeWithText<ui::Button>("interjection");

		ui::Push<ui::LabeledProperty>().SetText("and for 2").GetLabelStyle().SetFontWeight(ui::FontWeight::Bold);
		ui::MakeWithText<ui::Button>("test 2");
		ui::Pop();

		ui::PushBox().GetStyle().SetPaddingLeft(32);
		{
			ui::Push<ui::LabeledProperty>().SetText("also 3");
			ui::Text("test 3 elevated");
			ui::Pop();

			auto s = ui::Push<ui::LabeledProperty>().SetText("and 4 (brief sublabels)").GetStyle();
			s.SetLayout(ui::layouts::StackExpand());
			s.SetStackingDirection(ui::StackingDirection::LeftToRight);
			{
				ui::Push<ui::LabeledProperty>().SetText("X").SetBrief(true);
				ui::MakeWithText<ui::Button>("A");
				ui::Pop();

				ui::Push<ui::LabeledProperty>().SetText("Y").SetBrief(true);
				ui::MakeWithText<ui::Button>("B");
				ui::Pop();
			}
			ui::Pop();
		}
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
		static float sldval0 = 0.63f;
		ui::Make<ui::Slider>().Init(sldval0, { 0, 1 });

		ui::Property::Begin("Slider 1: 0-2 step=0");
		static float sldval1 = 0.63f;
		ui::Make<ui::Slider>().Init(sldval1, { 0, 2 });
		ui::Property::End();

		ui::Property::Begin("Slider 2: 0-2 step=0.1");
		static float sldval2 = 0.63f;
		ui::Make<ui::Slider>().Init(sldval2, { 0, 2, 0.1 });
		ui::Property::End();

		ui::Property::Begin("Slider 3: custom track bg");
		static float sldval3 = 0.63f;
		{
			auto& s = ui::Make<ui::Slider>().Init(sldval3, { 0, 1 });
			ui::StyleAccessor a = s.GetTrackStyle();
			auto lp = ui::LayerPainter::Create();
			lp->layers.push_back(a.GetBackgroundPainter());
			lp->layers.push_back(new MyGradientPainter);
			a.SetBackgroundPainter(lp);
			s.GetTrackFillStyle().SetBackgroundPainter(ui::EmptyPainter::Get());
		}
		ui::Property::End();

		ui::Property::Begin("Slider 4: vert stretched");
		ui::Make<ui::Slider>().Init(sldval2, { 0, 2, 0.1 }) + ui::SetHeight(40);
		ui::Property::End();

		ui::Property::Begin("Color picker parts");
		static float hue = 0.6f, sat = 0.3f, val = 0.8f;
		{
			auto s = ui::Make<ui::HueSatPicker>().Init(hue, sat).GetStyle();
			s.SetWidth(100);
			s.SetHeight(100);
		}
		{
			auto s = ui::Make<ui::ColorCompPicker2D>().Init(hue, sat).GetStyle();
			s.SetWidth(120);
			s.SetHeight(100);
		}
		ui::Property::End();
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
		GetStyle().SetWidth(ui::Coord::Percent(100));
		GetStyle().SetHeight(ui::Coord::Percent(100));

		ui::Push<ui::SplitPane>();

		ui::MakeWithText<ui::Panel>("Pane A");
		ui::MakeWithText<ui::Panel>("Pane B");

		ui::Push<ui::SplitPane>().SetDirection(true);

		ui::MakeWithText<ui::Panel>("Pane C");
		ui::MakeWithText<ui::Panel>("Pane D");
		ui::MakeWithText<ui::Panel>("Pane E");

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
	void Build() override
	{
		ui::Push<ui::TabGroup>();
		{
			ui::Push<ui::TabButtonList>();
			{
				ui::Push<ui::TabButtonT<int>>().Init(tab1, 0);
				ui::Text("First tab") + ui::SetPadding(5);
				ui::MakeWithText<ui::Button>("button");
				ui::Pop();

				ui::Push<ui::TabButton>().Init(tab1 == 1).HandleEvent(ui::EventType::Activate) = [this](ui::Event& e)
				{
					if (e.target == e.current)
					{
						tab1 = 1;
						Rebuild();
					}
				};
				ui::Text("Second tab") + ui::SetPadding(5);
				ui::MakeWithText<ui::Button>("button");
				ui::Pop();
			}
			ui::Pop();

			ui::Push<ui::TabPanel>().SetVisible(tab1 == 0);
			{
				ui::Text("Contents of the first tab (SetVisible)");
			}
			ui::Pop();

			ui::Push<ui::TabPanel>().SetVisible(tab1 == 1);
			{
				ui::Text("Contents of the second tab (SetVisible)");
			}
			ui::Pop();
		}
		ui::Pop();

		ui::Push<ui::TabGroup>();
		{
			ui::Push<ui::TabButtonList>();
			{
				ui::Push<ui::TabButton>().Init(tab2 == 0).HandleEvent(ui::EventType::Activate) = [this](ui::Event& e)
				{
					if (e.target == e.current)
					{
						tab2 = 0;
						Rebuild();
					}
				};
				ui::Text("First tab") + ui::SetPadding(5);
				ui::MakeWithText<ui::Button>("button");
				ui::Pop();

				ui::Push<ui::TabButtonT<int>>().Init(tab2, 1);
				ui::Text("Second tab") + ui::SetPadding(5);
				ui::MakeWithText<ui::Button>("button");
				ui::Pop();
			}
			ui::Pop();

			if (tab2 == 0)
			{
				ui::Push<ui::TabPanel>();
				{
					ui::Text("Contents of the first tab (conditional build)");
				}
				ui::Pop();
			}

			if (tab2 == 1)
			{
				ui::Push<ui::TabPanel>();
				{
					ui::Text("Contents of the second tab (conditional build)");
				}
				ui::Pop();
			}
		}
		ui::Pop();
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

		ui::Push<ui::Panel>();
		{
			auto& te = ui::Push<ui::ChildScaleOffsetElement>();
			te.x = x;
			te.y = y;
			te.scale = scale;
			{
				ui::MakeWithText<ui::Button>("One button");
				ui::MakeWithText<ui::Button>("The second button");
				ui::MakeWithText<ui::Button>("#3");
			}
			ui::Pop();
		}
		ui::Pop();
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
		GetStyle().SetLayout(ui::layouts::EdgeSlice());

		ui::imm::PropEditInt("\bCount", count);
		ui::imm::PropEditBool("\bExpanding", expanding);

		auto& sa = ui::Push<ui::ScrollArea>();
		if (!expanding)
			sa + ui::SetWidth(300) + ui::SetHeight(200);
		else
			sa + ui::SetHeight(ui::Coord::Percent(100));

		for (int i = 0; i < count; i++)
			ui::Textf("Inside scroll area [%d]", i);

		ui::Pop();
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
		ui::Text("Single line / selection (read only)");
		ui::Make<ui::Textbox>().SetText("test 3.14 abc").SetInputDisabled(true);

		ui::Text("Single line / editing");
		ui::Make<ui::Textbox>().SetText("test 3.14 abc");
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
		ui::Text("Default color block");
		ui::Make<ui::ColorBlock>().SetColor(colorA);
		ui::Make<ui::ColorBlock>().SetColor(colorB);

		ui::Text("Without edge");
		ui::Make<ui::ColorBlock>().SetColor(colorA) + ui::SetPadding(0);
		ui::Make<ui::ColorBlock>().SetColor(colorB) + ui::SetPadding(0);

		ui::Text("Custom size");
		ui::Make<ui::ColorBlock>().SetColor(colorA) + ui::SetWidth(200) + ui::SetHeight(40);
		ui::Make<ui::ColorBlock>().SetColor(colorB) + ui::SetWidth(200) + ui::SetHeight(40);

		ui::Text("Color inspect block");
		ui::Make<ui::ColorInspectBlock>().SetColor(colorA);
		ui::Make<ui::ColorInspectBlock>().SetColor(colorB);

		ui::Text("Assembled");
		auto C = colorB;
		ui::Push<ui::Panel>()
			+ ui::SetPadding(3);
		{
			ui::PushBox()
				+ ui::SetLayout(ui::layouts::StackExpand())
				+ ui::Set(ui::StackingDirection::LeftToRight);
			{
				ui::Make<ui::ColorBlock>().SetColor(C.GetOpaque())
					+ ui::SetPadding(0)
					+ ui::SetWidth(ui::Coord::Percent(50));
				ui::Make<ui::ColorBlock>().SetColor(C)
					+ ui::SetPadding(0)
					+ ui::SetWidth(ui::Coord::Percent(50));
			}
			ui::Pop();
			ui::Push<ui::ColorBlock>().SetColor(ui::Color4b::Black())
				+ ui::SetPadding(0)
				+ ui::SetWidth(ui::Coord::Percent(100))
				+ ui::SetHeight(4);
			{
				ui::Make<ui::ColorBlock>().SetColor(ui::Color4b::White())
					+ ui::SetPadding(0)
					+ ui::SetWidth(ui::Coord::Percent(100.f * C.a / 255.f))
					+ ui::SetHeight(4);
			}
			ui::Pop();
		}
		ui::Pop();
	}

	ui::Color4b colorA = { 240, 180, 120, 255 };
	ui::Color4b colorB = { 120, 180, 240, 120 };
};
void Test_ColorBlock()
{
	ui::Make<ColorBlockTest>();
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
		ui::StyleBlockRef pbr = ui::Theme::current->panel;
		ui::StyleAccessor pa(pbr, nullptr);
		pa.SetLayout(ui::layouts::InlineBlock());
		pa.SetPadding(4);
		pa.SetMargin(0);

		ui::StyleBlockRef ibr = ui::Theme::current->image;
		ui::StyleAccessor ia(ibr, nullptr);
		ia.SetHeight(25);

		ui::StyleBlockRef ibr2 = ui::Theme::current->image;
		ui::StyleAccessor ia2(ibr2, nullptr);
		ia2.SetWidth(25);

		ui::ScaleMode scaleModes[3] = { ui::ScaleMode::Stretch, ui::ScaleMode::Fit, ui::ScaleMode::Fill };
		const char* scaleModeNames[3] = { "Stretch", "Fit", "Fill" };

		for (int mode = 0; mode < 6; mode++)
		{
			ui::Push<ui::Panel>().SetStyle(pbr);

			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					ui::Push<ui::Panel>().SetStyle(pbr);
					ui::Make<ui::ImageElement>()
						.SetImage(img)
						.SetScaleMode(scaleModes[mode % 3], x * 0.5f + 0.5f, y * 0.5f + 0.5f)
						.SetStyle(mode / 3 ? ibr2 : ibr);
					ui::Pop();
				}
			}

			ui::Text(scaleModeNames[mode % 3]);

			ui::Pop();
		}

		ui::Make<ui::ImageElement>()
			.SetImage(img)
			+ ui::SetWidth(50)
			+ ui::SetHeight(50);
	}

	ui::draw::ImageHandle img;
};
void Test_Image()
{
	ui::Make<ImageTest>();
}


static ui::Color4f colorPickerTestCol;
struct ColorPickerTest : ui::Buildable
{
	void Build() override
	{
		auto& cp = ui::Make<ui::ColorPicker>().SetColor(colorPickerTestCol);
		cp.HandleEvent(ui::EventType::Change) = [&cp](ui::Event& e)
		{
			colorPickerTestCol = cp.GetColor().GetRGBA();
		};

		ui::Make<ui::DefaultOverlayBuilder>();
	}
};
void Test_ColorPicker()
{
	ui::Make<ColorPickerTest>();
}


struct IMGUITest : ui::Buildable
{
	void Build() override
	{
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
			ui::LabeledProperty::Begin("int");
			auto tmp = intVal;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { -543, 1234 }, intFmt ? "%x" : "%d"))
				intVal = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { -543, 1234 }, intFmt ? "%x" : "%d"))
				intVal = tmp;

			ui::Text("int: " + std::to_string(intVal)) + ui::SetPadding(5);
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("uint");
			auto tmp = uintVal;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { 0, 1234 }, intFmt ? "%x" : "%d"))
				uintVal = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { 0, 1234 }, intFmt ? "%x" : "%d"))
				uintVal = tmp;

			ui::Text("uint: " + std::to_string(uintVal)) + ui::SetPadding(5);
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("int64");
			auto tmp = int64Val;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { -543, 1234 }, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { -543, 1234 }, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;

			ui::Text("int64: " + std::to_string(int64Val)) + ui::SetPadding(5);
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("uint64");
			auto tmp = uint64Val;
			if (ui::imm::PropEditInt("\bworking", tmp, {}, {}, { 0, 1234 }, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;
			if (ui::imm::PropEditInt("\bdisabled", tmp, { ui::Enable(false) }, {}, { 0, 1234 }, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;

			ui::Text("uint64: " + std::to_string(uint64Val)) + ui::SetPadding(5);
			ui::LabeledProperty::End();
		}
		{
			ui::LabeledProperty::Begin("float");
			auto tmp = floatVal;
			if (ui::imm::PropEditFloat("\bworking", tmp, {}, { 0.1f }, { -37.4f, 154.1f }))
				floatVal = tmp;
			if (ui::imm::PropEditFloat("\bdisabled", tmp, { ui::Enable(false) }, { 0.1f }, { -37.4f, 154.1f }))
				floatVal = tmp;

			ui::Text("float: " + std::to_string(floatVal)) + ui::SetPadding(5);
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
		ui::MakeWithText<ui::Button>("Text-only tooltip") + ui::AddTooltip("Text only");
		ui::MakeWithText<ui::Button>("Checklist tooltip") + ui::AddTooltip([]()
		{
			bool t = true, f = false;
			ui::imm::PropEditBool("Done", t, { ui::Enable(false) });
			ui::imm::PropEditBool("Not done", f, { ui::Enable(false) });
		});

		ui::Make<ui::DefaultOverlayBuilder>();
	}
};
void Test_Tooltip()
{
	ui::Make<TooltipTest>();
}


struct DropdownTest : ui::Buildable
{
	uintptr_t sel3opts = 1;
	uintptr_t selPtr = uintptr_t(&typeid(ui::Buildable));
	const type_info* selPtrReal = &typeid(ui::Buildable);

	void Build() override
	{
		*this + ui::Set(ui::StackingDirection::LeftToRight);
		ui::PushBox() + ui::SetWidth(ui::Coord::Percent(33));
		{
			ui::Make<SpecificDropdownMenu>();

			ui::Text("[zssl] unlimited options");
			MenuList(sel3opts, Allocate<ui::ZeroSepCStrOptionList>("First\0Second\0Third\0"));
			ui::Text("[zssl] limited options");
			MenuList(sel3opts, Allocate<ui::ZeroSepCStrOptionList>(2, "First\0Second"));

			static const char* options[] = { "First", "Second", "Third", nullptr };

			ui::Text("[sa] unlimited options");
			MenuList(sel3opts, Allocate<ui::CStrArrayOptionList>(options));
			ui::Text("[sa] limited options");
			MenuList(sel3opts, Allocate<ui::CStrArrayOptionList>(2, options));

			ui::Text("custom pointer options");
			MenuList(selPtr, Allocate<TypeInfoOptions>());
		}
		ui::Pop();

		ui::PushBox() + ui::SetWidth(ui::Coord::Percent(33));
		{
			ui::Text("immediate mode");
			ui::imm::DropdownMenuList(sel3opts, Allocate<ui::ZeroSepCStrOptionList>("First\0Second\0Third\0"));
			ui::imm::DropdownMenuList(selPtrReal, Allocate<TypeInfoOptions>());
		}
		ui::Pop();
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
			ui::Text("Menu");
		}
		void OnBuildMenuContents() override
		{
			static bool flag1, flag2;

			ui::Push<ui::CheckboxFlagT<bool>>().Init(flag1);
			ui::Make<ui::CheckboxIcon>();
			ui::Text("Option 1") + ui::SetPadding(5);
			ui::Pop();

			ui::Push<ui::CheckboxFlagT<bool>>().Init(flag2);
			ui::Make<ui::CheckboxIcon>();
			ui::Text("Option 2") + ui::SetPadding(5);
			ui::Pop();
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
				&typeid(ui::UIElement),
				&typeid(ui::Buildable),
				&typeid(ui::BoxElement),
				&typeid(ui::TextElement),
			};
			for (size_t i = 0; i < count && i + from < sizeof(types) / sizeof(types[0]); i++)
			{
				fn(types[i], uintptr_t(types[i]));
			}
		}
		void BuildElement(const void* ptr, uintptr_t id, bool list)
		{
			ui::Text(ptr ? static_cast<const type_info*>(ptr)->name() : "<none>");
		}
	};
};
void Test_Dropdown()
{
	ui::Make<DropdownTest>();
}

