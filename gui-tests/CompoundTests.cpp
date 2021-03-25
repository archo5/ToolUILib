
#include "pch.h"
#include "../Core/3DMath.h"
#include "../Model/Gizmo.h"
#include "../Render/RHI.h"
#include "../Render/Primitives.h"


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

	void MakeContents(ui::UIContainer* ctx, const char* text, int row)
	{
		switch (row)
		{
		case 0:
			ctx->Make<ui::CheckboxIcon>();
			ctx->Text(text) + ui::SetPadding(4); // TODO consistent padding from theme?
			break;
		case 1:
			ctx->Make<ui::RadioButtonIcon>();
			ctx->Text(text) + ui::SetPadding(4);
			break;
		case 2:
			ctx->Make<ui::TreeExpandIcon>();
			ctx->Text(text) + ui::SetPadding(4);
			break;
		case 3:
			ctx->MakeWithText<ui::StateButtonSkin>(text);
			break;
		case 4:
			ctx->Text(GetStateText(stb->GetState()) + text) + ui::SetPadding(5);
			break;
		case 5:
			ctx->Make<ui::ColorBlock>().SetColor(GetStateColor(stb->GetState()));
			ctx->Text(text) + ui::SetPadding(4);
			break;
		case 6:
			ctx->MakeWithText<ui::ColorBlock>(text)
				.SetColor(GetStateColorDark(stb->GetState()))
				+ ui::SetWidth(ui::Coord::Undefined());
			break;
		case 7: {
			auto s = ctx->Text(text).GetStyle();
			s.SetPadding(5);
			s.SetTextColor(GetStateColor(stb->GetState()));
			s.SetFontWeight(stb->GetState() == 1 ? ui::FontWeight::Bold : ui::FontWeight::Normal);
			s.SetFontStyle(stb->GetState() > 1 ? ui::FontStyle::Italic : ui::FontStyle::Normal);
			break; }
		}
	}

	void Build(ui::UIContainer* ctx) override
	{
		constexpr int NUM_STYLES = 8;

		GetStyle().SetStackingDirection(ui::StackingDirection::LeftToRight);

		ctx->PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ctx->Text("CB activate");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				(stb = &ctx->Push<ui::StateToggle>().InitReadOnly(cb1))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { cb1 = !cb1; Rebuild(); };
				MakeContents(ctx, "one", i);
				ctx->Pop();
			}
		}
		ctx->Pop();

		ctx->PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ctx->Text("CB int.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				auto& cbbs = ctx->Push<ui::StateToggle>().InitEditable(cb1, 2);
				(stb = &cbbs)->HandleEvent(ui::EventType::Change) = [this, &cbbs](ui::Event&) { cb1 = cbbs.GetState(); Rebuild(); };
				MakeContents(ctx, "two", i);
				ctx->Pop();
			}
		}
		ctx->Pop();

		ctx->PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ctx->Text("CB ext.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				*(stb = &ctx->Push<ui::CheckboxFlagT<bool>>().Init(cb1)) + ui::RebuildOnChange();
				MakeContents(ctx, "three", i);
				ctx->Pop();
			}
		}
		ctx->Pop();

		ctx->PushBox().GetStyle().SetWidth(ui::Coord::Percent(15));
		{
			ctx->Text("CB int.3-state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				auto& cbbs = ctx->Push<ui::StateToggle>().InitEditable(cb3state, 3);
				(stb = &cbbs)->HandleEvent(ui::EventType::Change) = [this, &cbbs](ui::Event&) { cb3state = cbbs.GetState(); Rebuild(); };
				MakeContents(ctx, "four", i);
				ctx->Pop();
			}
		}
		ctx->Pop();

		ctx->PushBox().GetStyle().SetWidth(ui::Coord::Percent(20));
		{
			ctx->Text("RB activate");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				ui::Property::Scope ps(ctx);

				(stb = &ctx->Push<ui::StateToggle>().InitReadOnly(rb1 == 0))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { rb1 = 0; Rebuild(); };
				MakeContents(ctx, "r1a", i);
				ctx->Pop();

				(stb = &ctx->Push<ui::StateToggle>().InitReadOnly(rb1 == 1))->HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { rb1 = 1; Rebuild(); };
				MakeContents(ctx, "r1b", i);
				ctx->Pop();
			}
		}
		ctx->Pop();

		ctx->PushBox().GetStyle().SetWidth(ui::Coord::Percent(20));
		{
			ctx->Text("RB ext.state");

			for (int i = 0; i < NUM_STYLES; i++)
			{
				ui::Property::Scope ps(ctx);

				*(stb = &ctx->Push<ui::RadioButtonT<int>>().Init(rb1, 0)) + ui::RebuildOnChange();
				MakeContents(ctx, "r2a", i);
				ctx->Pop();

				*(stb = &ctx->Push<ui::RadioButtonT<int>>().Init(rb1, 1)) + ui::RebuildOnChange();
				MakeContents(ctx, "r2b", i);
				ctx->Pop();
			}
		}
		ctx->Pop();
	}

	ui::StateToggleBase* stb = nullptr;
	bool cb1 = false;
	int cb3state = 0;
	int rb1 = 0;
};
void Test_StateButtons(ui::UIContainer* ctx)
{
	ctx->Make<StateButtonsTest>();
}


struct PropertyListTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Push<ui::PropertyList>();

		ctx->Push<ui::LabeledProperty>().SetText("label for 1");
		ctx->Text("test 1");
		ctx->Pop();

		ctx->MakeWithText<ui::Button>("interjection");

		ctx->Push<ui::LabeledProperty>().SetText("and for 2").GetLabelStyle().SetFontWeight(ui::FontWeight::Bold);
		ctx->MakeWithText<ui::Button>("test 2");
		ctx->Pop();

		ctx->PushBox().GetStyle().SetPaddingLeft(32);
		{
			ctx->Push<ui::LabeledProperty>().SetText("also 3");
			ctx->Text("test 3 elevated");
			ctx->Pop();

			auto s = ctx->Push<ui::LabeledProperty>().SetText("and 4 (brief sublabels)").GetStyle();
			s.SetLayout(ui::layouts::StackExpand());
			s.SetStackingDirection(ui::StackingDirection::LeftToRight);
			{
				ctx->Push<ui::LabeledProperty>().SetText("X").SetBrief(true);
				ctx->MakeWithText<ui::Button>("A");
				ctx->Pop();

				ctx->Push<ui::LabeledProperty>().SetText("Y").SetBrief(true);
				ctx->MakeWithText<ui::Button>("B");
				ctx->Pop();
			}
			ctx->Pop();
		}
		ctx->Pop();

		ctx->Pop();
	}
};
void Test_PropertyList(ui::UIContainer* ctx)
{
	ctx->Make<PropertyListTest>();
}


struct SlidersTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		static float sldval0 = 0.63f;
		ctx->Make<ui::Slider>().Init(sldval0, { 0, 1 });

		ui::Property::Begin(ctx, "Slider 1: 0-2 step=0");
		static float sldval1 = 0.63f;
		ctx->Make<ui::Slider>().Init(sldval1, { 0, 2 });
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 2: 0-2 step=0.1");
		static float sldval2 = 0.63f;
		ctx->Make<ui::Slider>().Init(sldval2, { 0, 2, 0.1 });
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 3: custom track bg");
		static float sldval3 = 0.63f;
		{
			auto& s = ctx->Make<ui::Slider>().Init(sldval3, { 0, 1 });
			ui::StyleAccessor a = s.GetTrackStyle();
			a.SetPaintFunc([fn{ a.GetPaintFunc() }](const ui::PaintInfo& info)
			{
				fn(info);
				auto r = info.rect;
				ui::draw::RectGradH(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0, 0, 0), ui::Color4f(1, 0, 0));
			});
			s.GetTrackFillStyle().SetPaintFunc([](const ui::PaintInfo& info) {});
		}
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Slider 4: vert stretched");
		ctx->Make<ui::Slider>().Init(sldval2, { 0, 2, 0.1 }) + ui::SetHeight(40);
		ui::Property::End(ctx);

		ui::Property::Begin(ctx, "Color picker parts");
		static float hue = 0.6f, sat = 0.3f, val = 0.8f;
		{
			auto s = ctx->Make<ui::HueSatPicker>().Init(hue, sat).GetStyle();
			s.SetWidth(100);
			s.SetHeight(100);
		}
		{
			auto s = ctx->Make<ui::ColorCompPicker2D>().Init(hue, sat).GetStyle();
			s.SetWidth(120);
			s.SetHeight(100);
		}
		ui::Property::End(ctx);
	}
};
void Test_Sliders(ui::UIContainer* ctx)
{
	ctx->Make<SlidersTest>();
}


struct SplitPaneTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		GetStyle().SetWidth(ui::Coord::Percent(100));
		GetStyle().SetHeight(ui::Coord::Percent(100));

		ctx->Push<ui::SplitPane>();

		ctx->MakeWithText<ui::Panel>("Pane A");
		ctx->MakeWithText<ui::Panel>("Pane B");

		ctx->Push<ui::SplitPane>().SetDirection(true);

		ctx->MakeWithText<ui::Panel>("Pane C");
		ctx->MakeWithText<ui::Panel>("Pane D");
		ctx->MakeWithText<ui::Panel>("Pane E");

		ctx->Pop();

		ctx->Pop();
	}
};
void Test_SplitPane(ui::UIContainer* ctx)
{
	ctx->Make<SplitPaneTest>();
}


struct TabsTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabButtonList>();
			{
				ctx->Push<ui::TabButtonT<int>>().Init(tab1, 0);
				ctx->Text("First tab") + ui::SetPadding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();

				ctx->Push<ui::TabButton>().Init(tab1 == 1).HandleEvent(ui::EventType::Activate) = [this](ui::Event& e)
				{
					if (e.target == e.current)
					{
						tab1 = 1;
						Rebuild();
					}
				};
				ctx->Text("Second tab") + ui::SetPadding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>().SetVisible(tab1 == 0);
			{
				ctx->Text("Contents of the first tab (SetVisible)");
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>().SetVisible(tab1 == 1);
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
				ctx->Push<ui::TabButton>().Init(tab2 == 0).HandleEvent(ui::EventType::Activate) = [this](ui::Event& e)
				{
					if (e.target == e.current)
					{
						tab2 = 0;
						Rebuild();
					}
				};
				ctx->Text("First tab") + ui::SetPadding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();

				ctx->Push<ui::TabButtonT<int>>().Init(tab2, 1);
				ctx->Text("Second tab") + ui::SetPadding(5);
				ctx->MakeWithText<ui::Button>("button");
				ctx->Pop();
			}
			ctx->Pop();

			if (tab2 == 0)
			{
				ctx->Push<ui::TabPanel>();
				{
					ctx->Text("Contents of the first tab (conditional build)");
				}
				ctx->Pop();
			}

			if (tab2 == 1)
			{
				ctx->Push<ui::TabPanel>();
				{
					ctx->Text("Contents of the second tab (conditional build)");
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
	}

	void OnSerialize(ui::IDataSerializer& s)
	{
		s << tab1 << tab2;
	}

	int tab1 = 0, tab2 = 0;
};
void Test_Tabs(ui::UIContainer* ctx)
{
	ctx->Make<TabsTest>();
}


struct ScrollbarsTest : ui::Buildable
{
	static constexpr bool Persistent = true;

	int count = 20;
	bool expanding = true;

	void Build(ui::UIContainer* ctx) override
	{
		GetStyle().SetLayout(ui::layouts::EdgeSlice());

		ui::imm::PropEditInt(ctx, "\bCount", count);
		ui::imm::PropEditBool(ctx, "\bExpanding", expanding);

		auto& sa = ctx->Push<ui::ScrollArea>();
		if (!expanding)
			sa + ui::SetWidth(300) + ui::SetHeight(200);
		else
			sa + ui::SetHeight(ui::Coord::Percent(100));

		for (int i = 0; i < count; i++)
			ctx->Textf("Inside scroll area [%d]", i);

		ctx->Pop();
	}
};
void Test_Scrollbars(ui::UIContainer* ctx)
{
	ctx->Make<ScrollbarsTest>();
}


struct ColorBlockTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Text("Default color block");
		ctx->Make<ui::ColorBlock>().SetColor(colorA);
		ctx->Make<ui::ColorBlock>().SetColor(colorB);

		ctx->Text("Without edge");
		ctx->Make<ui::ColorBlock>().SetColor(colorA) + ui::SetPadding(0);
		ctx->Make<ui::ColorBlock>().SetColor(colorB) + ui::SetPadding(0);

		ctx->Text("Custom size");
		ctx->Make<ui::ColorBlock>().SetColor(colorA) + ui::SetWidth(200) + ui::SetHeight(40);
		ctx->Make<ui::ColorBlock>().SetColor(colorB) + ui::SetWidth(200) + ui::SetHeight(40);

		ctx->Text("Color inspect block");
		ctx->Make<ui::ColorInspectBlock>().SetColor(colorA);
		ctx->Make<ui::ColorInspectBlock>().SetColor(colorB);

		ctx->Text("Assembled");
		auto C = colorB;
		ctx->Push<ui::Panel>()
			+ ui::SetPadding(3);
		{
			ctx->PushBox()
				+ ui::SetLayout(ui::layouts::StackExpand())
				+ ui::Set(ui::StackingDirection::LeftToRight);
			{
				ctx->Make<ui::ColorBlock>().SetColor(C.GetOpaque())
					+ ui::SetPadding(0)
					+ ui::SetWidth(ui::Coord::Percent(50));
				ctx->Make<ui::ColorBlock>().SetColor(C)
					+ ui::SetPadding(0)
					+ ui::SetWidth(ui::Coord::Percent(50));
			}
			ctx->Pop();
			ctx->Push<ui::ColorBlock>().SetColor(ui::Color4b::Black())
				+ ui::SetPadding(0)
				+ ui::SetWidth(ui::Coord::Percent(100))
				+ ui::SetHeight(4);
			{
				ctx->Make<ui::ColorBlock>().SetColor(ui::Color4b::White())
					+ ui::SetPadding(0)
					+ ui::SetWidth(ui::Coord::Percent(100.f * C.a / 255.f))
					+ ui::SetHeight(4);
			}
			ctx->Pop();
		}
		ctx->Pop();
	}

	ui::Color4b colorA = { 240, 180, 120, 255 };
	ui::Color4b colorB = { 120, 180, 240, 120 };
};
void Test_ColorBlock(ui::UIContainer* ctx)
{
	ctx->Make<ColorBlockTest>();
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
		img = new ui::Image(c);
	}
	~ImageTest()
	{
		delete img;
	}
	void Build(ui::UIContainer* ctx) override
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
			ctx->Push<ui::Panel>().SetStyle(pbr);

			for (int y = -1; y <= 1; y++)
			{
				for (int x = -1; x <= 1; x++)
				{
					ctx->Push<ui::Panel>().SetStyle(pbr);
					ctx->Make<ui::ImageElement>()
						.SetImage(img)
						.SetScaleMode(scaleModes[mode % 3], x * 0.5f + 0.5f, y * 0.5f + 0.5f)
						.SetStyle(mode / 3 ? ibr2 : ibr);
					ctx->Pop();
				}
			}

			ctx->Text(scaleModeNames[mode % 3]);

			ctx->Pop();
		}
	}

	ui::Image* img;
};
void Test_Image(ui::UIContainer* ctx)
{
	ctx->Make<ImageTest>();
}


static ui::Color4f colorPickerTestCol;
struct ColorPickerTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		auto& cp = ctx->Make<ui::ColorPicker>().SetColor(colorPickerTestCol);
		cp.HandleEvent(ui::EventType::Change) = [&cp](ui::Event& e)
		{
			colorPickerTestCol = cp.GetColor().GetRGBA();
		};

		ctx->Make<ui::DefaultOverlayBuilder>();
	}
};
void Test_ColorPicker(ui::UIContainer* ctx)
{
	ctx->Make<ColorPickerTest>();
}


struct The3DViewTest : ui::Buildable
{
	static constexpr bool Persistent = true;

	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>()
			+ ui::SetMargin(0)
			+ ui::SetHeight(ui::Coord::Percent(100));
		{
			auto& v = ctx->Push<ui::View3D>();
			v.SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			v.HandleEvent() = [this](ui::Event& e)
			{
				if (e.type == ui::EventType::MouseMove)
					mousePos = e.position;
				camera.OnEvent(e);
			};
			v.onRender = [this](ui::UIRect r) { Render3DView(r); };
			v + ui::SetHeight(ui::Coord::Percent(100));
			{
				ctx->Text("Overlay text");
				ctx->Make<ui::ColorBlock>().SetColor({ 100, 0, 200, 255 });
				ctx->MakeWithText<ui::Button>("Reset")
					+ ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { camera = {}; })
					+ ui::SetWidth(40)
					+ ui::SetLayout(ui::layouts::InlineBlock()); // TODO FIX
			}
			ctx->Pop();
		}
		ctx->Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::rhi;

		camera.SetWindowRect(rect);
		camera.SetProjectionMatrix(ui::Mat4f::PerspectiveFOVLH(90, rect.GetAspectRatio(), 0.01f, 1000));

		Clear(16, 15, 14, 255);
		SetProjectionMatrix(camera.GetProjectionMatrix());
		SetViewMatrix(camera.GetViewMatrix());
		SetForcedColor(ui::Color4f(0.5f));
		VertPC verts[] =
		{
			{ -1, -1, 0, { 100, 150, 200, 255 } },
			{ 1, -1, 0, { 100, 0, 200, 255 } },
			{ -1, 1, 0, { 200, 150, 0, 255 } },
			{ 1, 1, 0, { 150, 50, 0, 255 } },
		};
		uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

		{
			constexpr ui::prim::PlaneSettings S = { 2, 3 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GeneratePlane(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.5f, 0.2f, 0.8f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(0.4f, 0, 0));
		}

		{
			constexpr ui::prim::BoxSettings S = { 2, 3, 4 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBox(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.2f, 0.5f, 0.8f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(0.2f, 0, 0));
		}

		{
			constexpr ui::prim::ConeSettings S = { 31 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateCone(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.2f, 0.8f, 0.5f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(0.0f, 0, 0));
		}

		{
			constexpr ui::prim::UVSphereSettings S = { 31, 13 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateUVSphere(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.5f, 0.8f, 0.2f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(-0.2f, 0, 0));
		}

		{
			constexpr ui::prim::BoxSphereSettings S = { 5 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBoxSphere(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.8f, 0.5f, 0.2f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(-0.4f, 0, 0));
		}

		SetRenderState(DF_ZTestOff | DF_ZWriteOff);
		auto ray = camera.GetRayWP(mousePos);
		auto rpir = RayPlaneIntersect(ray.origin, ray.direction, { 0, 0, 1, -1 });
		ui::Vec3f isp = ray.GetPoint(rpir.dist);
		DrawIndexed(ui::Mat4f::Scale(0.1f, 0.1f, 0.1f) * ui::Mat4f::Translate(isp), PT_Triangles, VF_Color, verts, 4, indices, 6);
	}
	void DrawPrim(ui::Vertex_PF3CB4* verts, uint16_t vc, uint16_t* idcs, unsigned ic, const ui::Color4b& col, const ui::Mat4f& m)
	{
		using namespace ui::rhi;

		ui::prim::SetVertexColor(verts, vc, col);

		SetRenderState(DF_AlphaBlended | DF_Cull);
		DrawIndexed(m, PT_Triangles, VF_Color, verts, vc, idcs, ic);

		SetRenderState(DF_Wireframe | DF_ForceColor);
		DrawIndexed(m, PT_Triangles, VF_Color, verts, vc, idcs, ic);
	}

	ui::OrbitCamera camera;
	ui::Point2f mousePos = {};
};
void Test_3DView(ui::UIContainer* ctx)
{
	ctx->Make<The3DViewTest>();
}


struct GizmoTest : ui::Buildable
{
	static constexpr bool Persistent = true;

	ui::OrbitCamera camera;
	ui::Gizmo gizmo;
	float gizmoSize = 100;
	ui::GizmoSizeMode gizmoSizeMode = ui::GizmoSizeMode::ViewPixels;
	ui::Mat4f xf = ui::Mat4f::Translate(0.01f, 0.02f, 0.03f);
	float fov = 90;

	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>()
			+ ui::SetMargin(0)
			+ ui::SetHeight(ui::Coord::Percent(100));
		{
			auto& v = ctx->Push<ui::View3D>();
			v.SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			v.HandleEvent() = [this](ui::Event& e)
			{
				if (e.type == ui::EventType::ButtonDown)
					e.context->SetKeyboardFocus(e.current);
				if (gizmo.OnEvent(e, camera, ui::GizmoEditableMat4f(xf)))
					Rebuild();
				camera.OnEvent(e);
			};
			v.onRender = [this](ui::UIRect r) { Render3DView(r); };
			v + ui::SetHeight(ui::Coord::Percent(100));
			{
				auto* leftTop = Allocate<ui::PointAnchoredPlacement>();
				leftTop->SetAnchorAndPivot({ 0, 0 });
				ctx->Push<ui::Panel>() + ui::SetWidth(120) + ui::SetPlacement(leftTop);
				{
					ctx->MakeWithText<ui::Header>("Camera");
					ui::imm::PropEditFloat(ctx, "FOV", fov, {}, 1.0f, 1.0f, 179.0f);

					{
						ui::LabeledProperty::Scope ps(ctx);
						ctx->MakeWithText<ui::Header>("Object");
						if (ui::imm::Button(ctx, "Reset"))
							xf = ui::Mat4f::Translate(0.01f, 0.02f, 0.03f);
					}

					auto pos = xf.TransformPoint({ 0, 0, 0 });
					ctx->Textf("pos=%g;%g;%g", pos.x, pos.y, pos.z) + ui::SetPadding(5);
				}
				ctx->Pop();

				auto* rightTop = Allocate<ui::PointAnchoredPlacement>();
				rightTop->SetAnchorAndPivot({ 1, 0 });
				ctx->Push<ui::Panel>() + ui::SetWidth(180) + ui::SetPlacement(rightTop);
				{
					ctx->MakeWithText<ui::Header>("Gizmo");
					ui::imm::PropEditFloat(ctx, "Size", gizmoSize, {}, 1.0f, 0.001f, 200.0f);
					ui::imm::PropDropdownMenuList(ctx, "Size mode", gizmoSizeMode, Allocate<ui::ZeroSepCStrOptionList>("Scene\0View normalized (Y)\0View pixels\0"));
					{
						ui::LabeledProperty::Scope ps(ctx, "Type");
						ui::imm::RadioButton(ctx, gizmo.type, ui::GizmoType::Move, "M", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(ctx, gizmo.type, ui::GizmoType::Rotate, "R", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(ctx, gizmo.type, ui::GizmoType::Scale, "S", {}, ui::imm::ButtonStateToggleSkin());
					}
					{
						ui::LabeledProperty::Scope ps(ctx, "Space");
						ui::imm::RadioButton(ctx, gizmo.isWorldSpace, false, "Local", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(ctx, gizmo.isWorldSpace, true, "World", {}, ui::imm::ButtonStateToggleSkin());
					}
				}
				ctx->Pop();
			}
			ctx->Pop();
		}
		ctx->Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::rhi;

		camera.SetWindowRect(rect);
		camera.SetProjectionMatrix(ui::Mat4f::PerspectiveFOVLH(fov, rect.GetAspectRatio(), 0.01f, 1000));

		Clear(16, 15, 14, 255);
		SetProjectionMatrix(camera.GetProjectionMatrix());
		SetViewMatrix(camera.GetViewMatrix());
		VertPC verts[] =
		{
			{ -1, -1, 0, { 100, 150, 200, 255 } },
			{ 1, -1, 0, { 100, 0, 200, 255 } },
			{ -1, 1, 0, { 200, 150, 0, 255 } },
			{ 1, 1, 0, { 150, 50, 0, 255 } },
		};
		uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

		RenderObject(ui::Mat4f::Scale(0.1f) * xf);

		gizmo.SetTransform(xf.RemoveScale());
		gizmo.Render(camera, gizmoSize, gizmoSizeMode);
	}

	void RenderObject(const ui::Mat4f& mtx)
	{
		using namespace ui::rhi;

		SetRenderState(DF_Cull);

		{
			constexpr ui::prim::BoxSettings S = {};
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBox(S, verts, idcs);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0.1f, 1));
			DrawIndexed(mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
		}

		{
			constexpr ui::prim::ConeSettings S = { 32 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateCone(S, verts, idcs);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0.2f, 0, 0, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * ui::Mat4f::RotateY(-90) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0, 0.2f, 0, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * ui::Mat4f::RotateX(90) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0, 0, 0.2f, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
		}
	}
};
void Test_Gizmo(ui::UIContainer* ctx)
{
	ctx->Make<GizmoTest>();
}


struct IMGUITest : ui::Buildable
{
	static constexpr bool Persistent = true;

	void Build(ui::UIContainer* ctx) override
	{
		ui::LabeledProperty::Begin(ctx, "buttons");
		if (ui::imm::Button(ctx, "working button"))
			puts("working button");
		if (ui::imm::Button(ctx, "disabled button", { ui::Enable(false) }))
			puts("DISABLED button SHOULD NOT APPEAR");
		ui::LabeledProperty::End(ctx);

		{
			ui::LabeledProperty::Begin(ctx, "bool");
			auto tmp = boolVal;
			if (ui::imm::EditBool(ctx, tmp, "working"))
				boolVal = tmp;
			if (ui::imm::CheckboxRaw(ctx, tmp, "w2", {}, ui::imm::ButtonStateToggleSkin()))
				boolVal = !tmp;
			if (ui::imm::EditBool(ctx, tmp, "disabled", { ui::Enable(false) }))
				boolVal = tmp;
			if (ui::imm::CheckboxRaw(ctx, tmp, "d2", { ui::Enable(false) }, ui::imm::ButtonStateToggleSkin()))
				boolVal = !tmp;
			ui::LabeledProperty::End(ctx);
		}

		{
			ui::LabeledProperty::Begin(ctx, "int format: %d");
			auto tmp = intFmt;
			if (ui::imm::RadioButton(ctx, tmp, 0, "working"))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(ctx, tmp == 0, "w2", {}, ui::imm::ButtonStateToggleSkin()))
				intFmt = 0;
			if (ui::imm::RadioButton(ctx, tmp, 0, "disabled", { ui::Enable(false) }))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(ctx, tmp == 0, "d2", { ui::Enable(false) }, ui::imm::ButtonStateToggleSkin()))
				intFmt = 0;
			ui::LabeledProperty::End(ctx);
		}
		{
			ui::LabeledProperty::Begin(ctx, "int format: %x");
			auto tmp = intFmt;
			if (ui::imm::RadioButton(ctx, tmp, 1, "working"))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(ctx, tmp == 1, "w2", {}, ui::imm::ButtonStateToggleSkin()))
				intFmt = 1;
			if (ui::imm::RadioButton(ctx, tmp, 1, "disabled", { ui::Enable(false) }))
				intFmt = tmp;
			if (ui::imm::RadioButtonRaw(ctx, tmp == 1, "d2", { ui::Enable(false) }, ui::imm::ButtonStateToggleSkin()))
				intFmt = 1;
			ui::LabeledProperty::End(ctx);
		}

		{
			ui::LabeledProperty::Begin(ctx, "int");
			auto tmp = intVal;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, -543, 1234, intFmt ? "%x" : "%d"))
				intVal = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, -543, 1234, intFmt ? "%x" : "%d"))
				intVal = tmp;

			ctx->Text("int: " + std::to_string(intVal)) + ui::SetPadding(5);
			ui::LabeledProperty::End(ctx);
		}
		{
			ui::LabeledProperty::Begin(ctx, "uint");
			auto tmp = uintVal;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, 0, 1234, intFmt ? "%x" : "%d"))
				uintVal = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, 0, 1234, intFmt ? "%x" : "%d"))
				uintVal = tmp;

			ctx->Text("uint: " + std::to_string(uintVal)) + ui::SetPadding(5);
			ui::LabeledProperty::End(ctx);
		}
		{
			ui::LabeledProperty::Begin(ctx, "int64");
			auto tmp = int64Val;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, -543, 1234, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, -543, 1234, intFmt ? "%" PRIx64 : "%" PRId64))
				int64Val = tmp;

			ctx->Text("int64: " + std::to_string(int64Val)) + ui::SetPadding(5);
			ui::LabeledProperty::End(ctx);
		}
		{
			ui::LabeledProperty::Begin(ctx, "uint64");
			auto tmp = uint64Val;
			if (ui::imm::PropEditInt(ctx, "\bworking", tmp, {}, 1, 0, 1234, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;
			if (ui::imm::PropEditInt(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 1, 0, 1234, intFmt ? "%" PRIx64 : "%" PRIu64))
				uint64Val = tmp;

			ctx->Text("uint64: " + std::to_string(uint64Val)) + ui::SetPadding(5);
			ui::LabeledProperty::End(ctx);
		}
		{
			ui::LabeledProperty::Begin(ctx, "float");
			auto tmp = floatVal;
			if (ui::imm::PropEditFloat(ctx, "\bworking", tmp, {}, 0.1f, -37.4f, 154.1f))
				floatVal = tmp;
			if (ui::imm::PropEditFloat(ctx, "\bdisabled", tmp, { ui::Enable(false) }, 0.1f, -37.4f, 154.1f))
				floatVal = tmp;

			ctx->Text("float: " + std::to_string(floatVal)) + ui::SetPadding(5);
			ui::LabeledProperty::End(ctx);
		}
		{
			ui::imm::PropEditFloatVec(ctx, "float3", float4val, "XYZ");
			ui::imm::PropEditFloatVec(ctx, "float4", float4val, "RGBA");
		}
		{
			ui::imm::PropEditColor(ctx, "color B", colorValB);
			ui::imm::PropEditColor(ctx, "color F", colorValF);
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
	ui::Color4b colorValB = { 180, 200, 220, 255 };
	ui::Color4f colorValF = { 0.9f, 0.7f, 0.5f, 0.8f };
};
void Test_IMGUI(ui::UIContainer* ctx)
{
	ctx->Make<IMGUITest>();
}


struct TooltipTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		ctx->MakeWithText<ui::Button>("Text-only tooltip") + ui::AddTooltip("Text only");
		ctx->MakeWithText<ui::Button>("Checklist tooltip") + ui::AddTooltip([](ui::UIContainer* ctx)
		{
			bool t = true, f = false;
			ui::imm::PropEditBool(ctx, "Done", t, { ui::Enable(false) });
			ui::imm::PropEditBool(ctx, "Not done", f, { ui::Enable(false) });
		});

		ctx->Make<ui::DefaultOverlayBuilder>();
	}
};
void Test_Tooltip(ui::UIContainer* ctx)
{
	ctx->Make<TooltipTest>();
}


struct DropdownTest : ui::Buildable
{
	static constexpr bool Persistent = true;

	uintptr_t sel3opts = 1;
	uintptr_t selPtr = uintptr_t(&typeid(ui::Buildable));
	const type_info* selPtrReal = &typeid(ui::Buildable);

	void Build(ui::UIContainer* ctx) override
	{
		*this + ui::Set(ui::StackingDirection::LeftToRight);
		ctx->PushBox() + ui::SetWidth(ui::Coord::Percent(33));
		{
			ctx->Make<SpecificDropdownMenu>();

			ctx->Text("[zssl] unlimited options");
			MenuList(ctx, sel3opts, Allocate<ui::ZeroSepCStrOptionList>("First\0Second\0Third\0"));
			ctx->Text("[zssl] limited options");
			MenuList(ctx, sel3opts, Allocate<ui::ZeroSepCStrOptionList>(2, "First\0Second"));

			static const char* options[] = { "First", "Second", "Third", nullptr };

			ctx->Text("[sa] unlimited options");
			MenuList(ctx, sel3opts, Allocate<ui::CStrArrayOptionList>(options));
			ctx->Text("[sa] limited options");
			MenuList(ctx, sel3opts, Allocate<ui::CStrArrayOptionList>(2, options));

			ctx->Text("custom pointer options");
			MenuList(ctx, selPtr, Allocate<TypeInfoOptions>());
		}
		ctx->Pop();

		ctx->PushBox() + ui::SetWidth(ui::Coord::Percent(33));
		{
			ctx->Text("immediate mode");
			ui::imm::DropdownMenuList(ctx, sel3opts, Allocate<ui::ZeroSepCStrOptionList>("First\0Second\0Third\0"));
			ui::imm::DropdownMenuList(ctx, selPtrReal, Allocate<TypeInfoOptions>());
		}
		ctx->Pop();
	}

	void MenuList(ui::UIContainer* ctx, uintptr_t& sel, ui::OptionList* list)
	{
		auto& ddml = ctx->Make<ui::DropdownMenuList>();
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
		void OnBuildButtonContents(ui::UIContainer* ctx) override
		{
			ctx->Text("Menu");
		}
		void OnBuildMenuContents(ui::UIContainer* ctx) override
		{
			static bool flag1, flag2;

			ctx->Push<ui::CheckboxFlagT<bool>>().Init(flag1);
			ctx->Make<ui::CheckboxIcon>();
			ctx->Text("Option 1") + ui::SetPadding(5);
			ctx->Pop();

			ctx->Push<ui::CheckboxFlagT<bool>>().Init(flag2);
			ctx->Make<ui::CheckboxIcon>();
			ctx->Text("Option 2") + ui::SetPadding(5);
			ctx->Pop();
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
		void BuildElement(ui::UIContainer* ctx, const void* ptr, uintptr_t id, bool list)
		{
			ctx->Text(ptr ? static_cast<const type_info*>(ptr)->name() : "<none>");
		}
	};
};
void Test_Dropdown(ui::UIContainer* ctx)
{
	ctx->Make<DropdownTest>();
}

