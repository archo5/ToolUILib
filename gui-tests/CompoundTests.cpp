
#include "pch.h"


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
			style::Accessor a = s.GetTrackStyle();
			a.SetPaintFunc([fn{ a.GetPaintFunc() }](const style::PaintInfo& info)
			{
				fn(info);
				auto r = info.rect;
				ui::draw::RectGradH(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0, 0, 0), ui::Color4f(1, 0, 0));
			});
			s.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
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
void Test_Sliders(UIContainer* ctx)
{
	ctx->Make<SlidersTest>();
}


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
void Test_SplitPane(UIContainer* ctx)
{
	ctx->Make<SplitPaneTest>();
}


struct TabsTest : ui::Node
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
void Test_Tabs(UIContainer* ctx)
{
	ctx->Make<TabsTest>();
}


struct ScrollbarsTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		*ctx->Push<ui::ScrollArea>() + ui::Width(300) + ui::Height(200);
		for (int i = 0; i < 20; i++)
			ctx->Text("Inside scroll area");
		ctx->Pop();
	}
};
void Test_Scrollbars(UIContainer* ctx)
{
	ctx->Make<ScrollbarsTest>();
}


struct ColorBlockTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Text("Default color block");
		ctx->Make<ui::ColorBlock>()->SetColor(colorA);
		ctx->Make<ui::ColorBlock>()->SetColor(colorB);

		ctx->Text("Without edge");
		ctx->Make<ui::ColorBlock>()->SetColor(colorA) + ui::Padding(0);
		ctx->Make<ui::ColorBlock>()->SetColor(colorB) + ui::Padding(0);

		ctx->Text("Custom size");
		ctx->Make<ui::ColorBlock>()->SetColor(colorA) + ui::Width(200) + ui::Height(40);
		ctx->Make<ui::ColorBlock>()->SetColor(colorB) + ui::Width(200) + ui::Height(40);

		ctx->Text("Color inspect block");
		ctx->Make<ui::ColorInspectBlock>()->SetColor(colorA);
		ctx->Make<ui::ColorInspectBlock>()->SetColor(colorB);

		ctx->Text("Assembled");
		auto C = colorB;
		*ctx->Push<ui::Panel>()
			+ ui::Padding(3);
		{
			ctx->PushBox()
				+ ui::Layout(style::layouts::StackExpand())
				+ ui::StackingDirection(style::StackingDirection::LeftToRight);
			{
				ctx->Make<ui::ColorBlock>()->SetColor(C.GetOpaque())
					+ ui::Padding(0)
					+ ui::Width(style::Coord::Percent(50));
				ctx->Make<ui::ColorBlock>()->SetColor(C)
					+ ui::Padding(0)
					+ ui::Width(style::Coord::Percent(50));
			}
			ctx->Pop();
			ctx->Push<ui::ColorBlock>()->SetColor(ui::Color4b::Black())
				+ ui::Padding(0)
				+ ui::Width(style::Coord::Percent(100))
				+ ui::Height(4);
			{
				ctx->Make<ui::ColorBlock>()->SetColor(ui::Color4b::White())
					+ ui::Padding(0)
					+ ui::Width(style::Coord::Percent(100.f * C.a / 255.f))
					+ ui::Height(4);
			}
			ctx->Pop();
		}
		ctx->Pop();
	}

	ui::Color4b colorA = { 240, 180, 120, 255 };
	ui::Color4b colorB = { 120, 180, 240, 120 };
};
void Test_ColorBlock(UIContainer* ctx)
{
	ctx->Make<ColorBlockTest>();
}


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
		style::Accessor pa(pbr, nullptr);
		pa.SetLayout(style::layouts::InlineBlock());
		pa.SetPadding(4);
		pa.SetMargin(0);

		style::BlockRef ibr = ui::Theme::current->image;
		style::Accessor ia(ibr, nullptr);
		ia.SetHeight(25);

		style::BlockRef ibr2 = ui::Theme::current->image;
		style::Accessor ia2(ibr2, nullptr);
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
void Test_Image(UIContainer* ctx)
{
	ctx->Make<ImageTest>();
}


static ui::Color4f colorPickerTestCol;
struct ColorPickerTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		auto& cp = ctx->Make<ui::ColorPicker>()->SetColor(colorPickerTestCol);
		cp.HandleEvent(UIEventType::Change) = [&cp](UIEvent& e)
		{
			colorPickerTestCol = cp.GetColor();
		};

		ctx->Make<ui::DefaultOverlayRenderer>();
	}
};
void Test_ColorPicker(UIContainer* ctx)
{
	ctx->Make<ColorPickerTest>();
}


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
void Test_IMGUI(UIContainer* ctx)
{
	ctx->Make<IMGUITest>();
}


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
void Test_Tooltip(UIContainer* ctx)
{
	ctx->Make<TooltipTest>();
}

