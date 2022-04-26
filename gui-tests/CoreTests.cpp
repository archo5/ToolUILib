
#include "pch.h"
#include <stdarg.h>


struct RenderingPrimitives : ui::Buildable
{
	ui::draw::ImageHandle stretchTestImg;
	ui::draw::ImageHandle fileTestImg;

	RenderingPrimitives()
	{
		ui::Color4b a(35, 100, 200);
		ui::Color4b b(200, 100, 35);
		ui::Color4b cols[4] =
		{
			a, b,
			b, a,
		};
		stretchTestImg = ui::draw::ImageCreateRGBA8(2, 2, cols, ui::draw::TexFlags::Packed);
		fileTestImg = ui::draw::ImageLoadFromFile("gui-theme2.tga");
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Color4b col = ui::Color4f(1, 0.5f, 0);
		ui::Color4b colO = ui::Color4f(0, 1, 0.5f, 0.5f);

		for (int t = 0; t <= 5; t++)
		{
			float w = powf(3.0f, t * 0.2f);
			float xo = t * 74 + 10;
			float x0 = 10 + xo;
			float x1 = 25 + xo;
			float x2 = 45 + xo;
			float x3 = 60 + xo;
			float x4 = 72 + xo;
			for (int i = 0; i < 8; i++)
			{
				if (i % 2 == 0)
				{
					ui::draw::LineCol(x0, 10 + i * 4, x1, 10 + i * 5, w, col);
					ui::draw::AALineCol(x0, 10 + i * 4, x1, 10 + i * 5, w, colO);
					ui::draw::AALineCol(x0, 210 + i * 4, x1, 210 + i * 5, w, col);

					ui::draw::LineCol(x2, 10 + i * 5, x3, 10 + i * 4, w, col);
					ui::draw::AALineCol(x2, 10 + i * 5, x3, 10 + i * 4, w, colO);
					ui::draw::AALineCol(x2, 210 + i * 5, x3, 210 + i * 4, w, col);
				}
				else
				{
					ui::draw::LineCol(x1, 10 + i * 5, x0, 10 + i * 4, w, col);
					ui::draw::AALineCol(x1, 10 + i * 5, x0, 10 + i * 4, w, colO);
					ui::draw::AALineCol(x1, 210 + i * 5, x0, 210 + i * 4, w, col);

					ui::draw::LineCol(x3, 10 + i * 4, x2, 10 + i * 5, w, col);
					ui::draw::AALineCol(x3, 10 + i * 4, x2, 10 + i * 5, w, colO);
					ui::draw::AALineCol(x3, 210 + i * 4, x2, 210 + i * 5, w, col);
				}
			}
			for (int i = 4; i < 12; i++)
			{
				ui::draw::LineCol(x0, 10 + i * 8, x1, 10 + i * 10, w, col);
				ui::draw::AALineCol(x0, 10 + i * 8, x1, 10 + i * 10, w, colO);
				ui::draw::AALineCol(x0, 210 + i * 8, x1, 210 + i * 10, w, col);

				ui::draw::LineCol(x2, 10 + i * 10, x3, 10 + i * 8, w, col);
				ui::draw::AALineCol(x2, 10 + i * 10, x3, 10 + i * 8, w, colO);
				ui::draw::AALineCol(x2, 210 + i * 10, x3, 210 + i * 8, w, col);
			}
			for (int i = 0; i < 4; i++)
			{
				float x = (x1 + x2) / 2;
				float y = 42 + i * 22;
				float y2 = y + 200;
				float angle = 60 + i * 10;
				float c = cosf(angle / 180 * 3.14159f);
				float s = sinf(angle / 180 * 3.14159f);
				float r = 10;

				ui::draw::LineCol(x - c * r, y - s * r, x + c * r, y + s * r, w, col);
				ui::draw::AALineCol(x - c * r, y - s * r, x + c * r, y + s * r, w, colO);
				ui::draw::AALineCol(x - c * r, y2 - s * r, x + c * r, y2 + s * r, w, col);
			}
			{
				ui::Point2f pts[4] =
				{
					{ x4 - 5, 10 },
					{ x4 + 5, 10 },
					{ x4 + 5, 20 },
					{ x4 - 5, 20 },
				};
				ui::draw::LineCol(pts, w, col, true);
				ui::draw::AALineCol(pts, w, colO, true);
				for (auto& p : pts)
					p.y += 200;
				ui::draw::AALineCol(pts, w, col, true);
			}
			for (int i = 0; i < 4; i++)
			{
				ui::Point2f p = { x4, 32.0f + i * 20 };
				int ptcount = i + 3;
				ui::Point2f pts[6];
				for (int j = 0; j < ptcount; j++)
				{
					float a = 3.14159f * 2 * j / float(ptcount);
					pts[j] = p + ui::Point2f{ sinf(a), cosf(a) } *6.0f;
				}

				ui::draw::LineCol(ui::ArrayView<ui::Point2f>(pts, ptcount), w, col, true);
				ui::draw::AALineCol(ui::ArrayView<ui::Point2f>(pts, ptcount), w, colO, true);
				for (int j = 0; j < ptcount; j++)
					pts[j].y += 200;
				ui::draw::AALineCol(ui::ArrayView<ui::Point2f>(pts, ptcount), w, col, true);
			}
			ui::draw::CircleLineCol({ x4, 110 }, 5, w, col);
			ui::draw::AACircleLineCol({ x4, 110 }, 5, w, colO);
			ui::draw::AACircleLineCol({ x4, 110 + 200 }, 5, w, col);
		}

		ui::draw::RectCol(40, 10, 50, 20, col);

		ui::draw::TextLine(ui::GetFont(ui::FONT_FAMILY_SANS_SERIF), 20, 20, 150, "sans-serif w=normal it=0", ui::Color4f(0.9f, 0.8f, 0.6f));
		ui::draw::TextLine(ui::GetFont(ui::FONT_FAMILY_SERIF, ui::FONT_WEIGHT_BOLD), 20, 20, 170, "serif w=bold it=0", ui::Color4f(0.6f, 0.8f, 0.9f));
		ui::draw::TextLine(ui::GetFont(ui::FONT_FAMILY_MONOSPACE, ui::FONT_WEIGHT_NORMAL, true), 20, 20, 190, "monospace w=normal it=1", ui::Color4f(0.7f, 0.9f, 0.6f));

		ui::draw::RectTex(300, 140, 350, 190, stretchTestImg);
		ui::draw::RectTex(350, 140, 450, 190, fileTestImg);
	}
	void Build() override
	{
	}
};
void Test_RenderingPrimitives()
{
	ui::Make<RenderingPrimitives>();
}


struct StylePaintingTest : ui::Buildable, ui::AnimationRequester
{
	ui::FrameStyle buttonStyle;
	ui::IconStyle checkboxStyle;
	ui::IconStyle radioBtnStyle;

	void OnReset() override
	{
		buttonStyle = *ui::GetCurrentTheme()->FindStructByName<ui::FrameStyle>("button");
		checkboxStyle = *ui::GetCurrentTheme()->FindStructByName<ui::IconStyle>("checkbox");
		radioBtnStyle = *ui::GetCurrentTheme()->FindStructByName<ui::IconStyle>("radio_button");
	}
	void OnEnable() override
	{
		BeginAnimation();
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::PaintInfo pi(this);
		constexpr int W = 10;
		constexpr int H = 10;
		for (int yi = 0; yi < 100; yi++)
		{
			float y = yi * H;
			for (int xi = 0; xi < 16; xi++)
			{
				float x = xi * W;
				pi.state = xi;
				pi.rect = { x, y, x + W, y + H };
				buttonStyle.backgroundPainter->Paint(pi);
			}
			for (int xi = 0; xi < 16; xi++)
			{
				float x = (xi + 16) * W;
				pi.state = xi;
				pi.rect = { x, y, x + W, y + H };
				checkboxStyle.painter->Paint(pi);
			}
			for (int xi = 0; xi < 16; xi++)
			{
				float x = (xi + 32) * W;
				pi.state = xi;
				pi.rect = { x, y, x + W, y + H };
				radioBtnStyle.painter->Paint(pi);
			}
		}
	}
	void Build() override
	{
	}
	void OnAnimationFrame() override
	{
		GetNativeWindow()->InvalidateAll();
	}
};
void Test_StylePainting()
{
	ui::Make<StylePaintingTest>();
}


struct ImageSetSizingTest : ui::Buildable
{
	ui::draw::ImageSetHandle icon;

	void OnReset() override
	{
		icon = new ui::draw::ImageSet;
		icon->baseSize = { 32, 32 };
		icon->entries.push_back({ ui::draw::ImageLoadFromFile("iss32.png") });
		icon->entries.push_back({ ui::draw::ImageLoadFromFile("iss64.png") });
		icon->entries.push_back({ ui::draw::ImageLoadFromFile("iss96.png") });
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		using namespace ui::draw;
		float y = 0;
		for (auto mode : { ImageSetSizeMode::NearestScaleDown, ImageSetSizeMode::NearestScaleUp, ImageSetSizeMode::NearestNoScale })
		{
			icon->sizeMode = mode;
			float x = 0;
			for (float i = 32; i < 96 + 8; i += 16)
			{
				icon->Draw({ x, y, x + i, y + i });
				x += i;
			}
			y += 96;
		}
	}
	void Build() override
	{
	}
};
void Test_ImageSetSizing()
{
	ui::Make<ImageSetSizingTest>();
}


struct EventsTest : ui::Buildable
{
	static constexpr unsigned MAX_MESSAGES = 50;

	std::string msgBuf[MAX_MESSAGES];
	unsigned writePos = 0;

	void WriteMsg(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		msgBuf[writePos++] = buf;
		writePos %= MAX_MESSAGES;
	}

	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		auto* font = ui::GetFont(ui::FONT_FAMILY_SANS_SERIF);
		for (unsigned i = 0; i < MAX_MESSAGES; i++)
		{
			unsigned idx = (MAX_MESSAGES * 2 + writePos - i - 1) % MAX_MESSAGES;
			ui::draw::TextLine(font, 12, 0, GetFinalRect().y1 - i * 12, msgBuf[idx], ui::Color4f::White());
		}
	}
	void Build() override
	{
	}
};

struct KeyboardEventsTest : EventsTest
{
	void OnReset() override
	{
		SetFlag(ui::UIObject_IsFocusable, true);
		SetFlag(ui::UIObject_DB_FocusOnLeftClick, true);
	}
	void OnEnable() override
	{
		system->eventSystem.SetKeyboardFocus(this);
	}
	void OnEvent(ui::Event& e) override
	{
		if (e.type == ui::EventType::KeyAction)
		{
			WriteMsg("type=KeyAction action=%u numRepeats=%u modifier=%s",
				unsigned(e.GetKeyAction()),
				e.numRepeats,
				e.GetKeyActionModifier() ? "true" : "false");
		}
		else if (e.type == ui::EventType::KeyDown || e.type == ui::EventType::KeyUp)
		{
			WriteMsg("type=Key%s virtual=%u physical=%u mod=%02X numRepeats=%u",
				e.type == ui::EventType::KeyDown ? "Down" : "Up",
				e.longCode,
				e.shortCode,
				e.GetModifierKeys(),
				e.numRepeats);
		}
		else if (e.type == ui::EventType::TextInput)
		{
			char text[5] = {};
			e.GetUTF8Text(text);
			WriteMsg("type=TextInput text=%s (code=%u) mod=%02X",
				text,
				e.GetUTF32Char(),
				e.GetModifierKeys());
		}
	}
};
void Test_KeyboardEvents()
{
	ui::Make<KeyboardEventsTest>();
}

struct RawMouseEventsTest : EventsTest, ui::RawMouseInputRequester
{
	void OnRawInputEvent(float dx, float dy) override
	{
		WriteMsg("raw mouse event: dx=%g dy=%g", dx, dy);
	}
};
void Test_RawMouseEvents()
{
	ui::Make<RawMouseEventsTest>();
}


struct OpenCloseTest : ui::Buildable
{
	struct AllocTest
	{
		AllocTest(int v) : val(v) { printf("alloc #=%d\n", v); }
		~AllocTest() { printf("free #=%d\n", val); }
		int val;
	};

	struct EDLogger : ui::UIObjectNoChildren
	{
		void OnEnable() override
		{
			puts("STATE: enabled");
		}
		void OnDisable() override
		{
			puts("STATE: disabled");
		}
	};
	void Build() override
	{
		static int counter = 0;
		Allocate<AllocTest>(++counter);

		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		WPush<ui::StackTopDownLayoutElement>();

		auto& cb = ui::Push<ui::StateToggle>().InitReadOnly(open);
		ui::Make<ui::CheckboxIcon>();
		ui::Pop();
		cb.HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { open = !open; Rebuild(); };

		auto& openBtn = ui::MakeWithText<ui::Button>("Open");
		openBtn.HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { open = true; Rebuild(); };

		auto& closeBtn = ui::MakeWithText<ui::Button>("Close");
		closeBtn.HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { open = false; Rebuild(); };

		(open ? closeBtn : openBtn).frameStyle.font.weight = ui::FontWeight::Bold;
		(open ? closeBtn : openBtn).frameStyle.textColor.SetValue(ui::Color4f(0.1f, 0.9f, 0.5f));
		(!open ? closeBtn : openBtn).frameStyle.textColor.SetValue(ui::Color4f(0.7f, 1.0f));

		if (open)
		{
			ui::Make<EDLogger>();
			WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			WPush<ui::StackTopDownLayoutElement>();
			ui::Text("It is open!");
			auto& fe = ui::MakeWithText<ui::FrameElement>("Different text");
			fe.frameStyle.font.size = 16;
			fe.frameStyle.font.family = ui::FONT_FAMILY_SERIF;
			fe.frameStyle.font.weight = ui::FontWeight::Bold;
			fe.frameStyle.font.style = ui::FontStyle::Italic;
			fe.frameStyle.textColor.SetValue(ui::Color4f(1.0f, 0.1f, 0.0f));
			WPop();
			WPop();
		}

		WPop();
		WPop();
	}

	bool open = false;
};
void Test_OpenClose()
{
	ui::Make<OpenCloseTest>();
}


struct AppendMixTest : ui::Buildable
{
	enum Mode
	{
		Nothing,
		Inline,
		Append1,
		Append2,
	};
	Mode mode = Nothing;

	ui::TextElement* append1;
	ui::TextElement* append2;

	void OnEnable() override
	{
		append1 = ui::CreateUIObject<ui::TextElement>();
		append1->system = system;
		append1->SetText("append one");

		append2 = ui::CreateUIObject<ui::TextElement>();
		append2->system = system;
		append2->SetText("append two");
	}

	void OnDisable() override
	{
		ui::DeleteUIObject(append1);
		ui::DeleteUIObject(append2);
	}

	void Build() override
	{
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::Push<ui::RadioButtonT<Mode>>().Init(mode, Nothing).HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { Rebuild(); };
		ui::Push<ui::StackLTRLayoutElement>();
		ui::Make<ui::RadioButtonIcon>();
		ui::Text("Nothing");
		ui::Pop();
		ui::Pop();

		ui::Push<ui::RadioButtonT<Mode>>().Init(mode, Inline).HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { Rebuild(); };
		ui::Push<ui::StackLTRLayoutElement>();
		ui::Make<ui::RadioButtonIcon>();
		ui::Text("Inline");
		ui::Pop();
		ui::Pop();

		ui::Push<ui::RadioButtonT<Mode>>().Init(mode, Append1).HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { Rebuild(); };
		ui::Push<ui::StackLTRLayoutElement>();
		ui::Make<ui::RadioButtonIcon>();
		ui::Text("Append (1)");
		ui::Pop();
		ui::Pop();

		ui::Push<ui::RadioButtonT<Mode>>().Init(mode, Append2).HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { Rebuild(); };
		ui::Push<ui::StackLTRLayoutElement>();
		ui::Make<ui::RadioButtonIcon>();
		ui::Text("Append (2)");
		ui::Pop();
		ui::Pop();

		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);

		switch (mode)
		{
		case Nothing:
			break;
		case Inline:
			ui::Text("inline");
			break;
		case Append1:
			ui::Append(append1);
			break;
		case Append2:
			ui::Append(append2);
			break;
		}

		ui::Pop();
		ui::Pop();
	}
};
void Test_AppendMix()
{
	ui::Make<AppendMixTest>();
}


struct AnimationRequestTest : ui::Buildable
{
	AnimationRequestTest()
	{
		animReq.callback = [this]() { OnAnimation(); };
	}
	void OnAnimation()
	{
		GetNativeWindow()->InvalidateAll();
	}
	void Build() override
	{
		auto& cb = ui::Push<ui::StateToggle>().InitReadOnly(animReq.IsAnimating());
		ui::Make<ui::CheckboxIcon>();
		ui::Pop();
		cb.HandleEvent(ui::EventType::Activate) = [this, &cb](ui::Event&) { animReq.SetAnimating(!animReq.IsAnimating()); Rebuild(); };
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);

		static uint32_t start = ui::platform::GetTimeMs();
		float cx = 100;
		float cy = 100;
		float rad = 50;
		float t = (ui::platform::GetTimeMs() - start) * 0.001f;
		float c = cos(t);
		float s = sin(t);
		ui::draw::AALineCol(cx - rad * c, cy - rad * s, cx + rad * c, cy + rad * s, 2, ui::Color4f(0.6f, 0.7f, 0.9f));
	}

	ui::AnimationCallbackRequester animReq;
};
void Test_AnimationRequest()
{
	ui::Make<AnimationRequestTest>();
}


struct ElementResetTest : ui::Buildable
{
	void Build() override
	{
		if (first)
		{
			ui::Push<ui::SizeConstraintElement>().SetWidth(300);
			ui::MakeWithText<ui::Button>("First")
				+ ui::AddEventHandler(ui::EventType::Click, [this](ui::Event&) { first = false; Rebuild(); });
			ui::Pop();

			auto& tb = ui::Make<ui::Textbox>();
			tb + ui::AddEventHandler(ui::EventType::Change, [this, &tb](ui::Event&) { text[0] = tb.GetText(); });
			tb.SetText(text[0]);
		}
		else
		{
			ui::Push<ui::SizeConstraintElement>().SetHeight(30);
			ui::MakeWithText<ui::Button>("Second")
				+ ui::AddEventHandler(ui::EventType::Click, [this](ui::Event&) { first = true; Rebuild(); });
			ui::Pop();

			auto& tb = ui::Make<ui::Textbox>();
			tb + ui::AddEventHandler(ui::EventType::Change, [this, &tb](ui::Event&) { text[1] = tb.GetText(); });
			tb.SetText(text[1]);
		}

		auto& tb = ui::Make<ui::Textbox>();
		tb + ui::AddEventHandler(ui::EventType::Change, [this, &tb](ui::Event&) { text[2] = tb.GetText(); });
		tb.SetText(text[2]);
	}
	bool first = true;
	std::string text[3] = { "first", "second", "third" };
};
void Test_ElementReset()
{
	ui::Make<ElementResetTest>();
}


struct SubUITest : ui::Buildable
{
	ui::Color4b PickColor(uint8_t id, ui::Color4f normal, ui::Color4f hovered, ui::Color4f pressed)
	{
		if (subui.IsPressed(id))
			return pressed;
		if (subui.IsHovered(id))
			return hovered;
		return normal;
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::PaintInfo info(this);
		ui::GetCurrentTheme()->FindStructByName<ui::FrameStyle>("textbox")->backgroundPainter->Paint(info);
		auto r = GetFinalRect();

		auto* font = ui::GetFont(ui::FONT_FAMILY_SANS_SERIF);
		const int size = 12;

		ui::draw::RectCol(r.x0, r.y0, r.x0 + 50, r.y0 + 50, PickColor(0, { 0, 0, 1, 0.5f }, { 0, 1, 0, 0.5f }, { 1, 0, 0, 0.5f }));
		ui::draw::TextLine(font, size, r.x0 + 25 - ui::GetTextWidth(font, size, "Button") / 2, r.y0 + 25 + size / 2, "Button", ui::Color4b::White());

		auto ddr = ui::UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(1, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		ui::draw::TextLine(font, size, r.x0 + draggableX - ui::GetTextWidth(font, size, "D&D") / 2, r.y0 + draggableY + size / 2, "D&D", ui::Color4b::White());

		ddr = ui::UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(2, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		ui::draw::TextLine(font, size, r.x0 + draggableX - ui::GetTextWidth(font, size, "x") / 2, r.y0 + 5 + size / 2, "x", ui::Color4b::White());

		ddr = ui::UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(3, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		ui::draw::TextLine(font, size, r.x0 + 5 - ui::GetTextWidth(font, size, "y") / 2, r.y0 + draggableY + size / 2, "y", ui::Color4b::White());
	}
	void OnEvent(ui::Event& e) override
	{
		auto r = GetFinalRect();

		subui.InitOnEvent(e);
		if (subui.ButtonOnEvent(0, ui::UIRect{ r.x0, r.y0, r.x0 + 50, r.y0 + 50 }, e))
		{
			puts("0-50 clicked");
		}
		switch (subui.DragOnEvent(1, ui::UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10), e))
		{
		case ui::SubUIDragState::Start:
			puts("drag start");
			dox = draggableX - e.position.x;
			doy = draggableY - e.position.y;
			break;
		case ui::SubUIDragState::Move:
			puts("drag move");
			draggableX = e.position.x + dox;
			draggableY = e.position.y + doy;
			break;
		case ui::SubUIDragState::Stop:
			puts("drag stop");
			break;
		}
		switch (subui.DragOnEvent(2, ui::UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5), e))
		{
		case ui::SubUIDragState::Start:
			dox = draggableX - e.position.x;
			break;
		case ui::SubUIDragState::Move:
			draggableX = e.position.x + dox;
			break;
		}
		switch (subui.DragOnEvent(3, ui::UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10), e))
		{
		case ui::SubUIDragState::Start:
			doy = draggableY - e.position.y;
			break;
		case ui::SubUIDragState::Move:
			draggableY = e.position.y + doy;
			break;
		}

		if (e.type == ui::EventType::ButtonDown && !subui.IsAnyPressed())
		{
			subui.DragStart(1);
			dox = 0;
			doy = 0;
		}
	}
	void Build() override
	{
	}

	ui::SubUI<uint8_t> subui;
	float draggableX = 75;
	float draggableY = 75;
	float dox = 0, doy = 0;
};
void Test_SubUI()
{
	ui::Make<SubUITest>();
}


struct HighElementCountTest : ui::Buildable
{
	struct DummyElement : ui::UIObjectNoChildren
	{
		void OnPaint(const ui::UIPaintContext& ctx) override
		{
			auto r = GetFinalRect();
			ui::draw::RectCol(r.x0, r.y0, r.x1, r.y1, ui::Color4f(fmodf(uintptr_t(this) / (8 * 256.0f), 1.0f), 0.0f, 0.0f));
		}
		ui::Rangef CalcEstimatedWidth(const ui::Size2f& containerSize, ui::EstSizeType type) override
		{
			return ui::Rangef::Exact(100);
		}
		ui::Rangef CalcEstimatedHeight(const ui::Size2f& containerSize, ui::EstSizeType type) override
		{
			return ui::Rangef::Exact(1);
		}
	};
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		for (int i = 0; i < 1000; i++)
		{
			auto& el = ui::Make<DummyElement>();
		}

		WPop();
	}
	int styleMode;
};
void Test_HighElementCount()
{
	ui::Make<HighElementCountTest>();
}


struct ZeroRebuildTest : ui::Buildable
{
	bool first = true;
	void Build() override
	{
		if (first)
			first = false;
		else
			puts("Should not happen!");

		ui::LabeledProperty::Begin("Show?");
		cbShow = &ui::Push<ui::StateToggle>().InitEditable(show);
		ui::Make<ui::CheckboxIcon>();
		ui::Pop();
		ui::LabeledProperty::End();

		*cbShow + ui::AddEventHandler(ui::EventType::Change, [this](ui::Event& e) { show = cbShow->GetState(); OnShowChange(); });
		ui::MakeWithText<ui::Button>("Show")
			+ ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event& e) { show = true; OnShowChange(); });
		ui::MakeWithText<ui::Button>("Hide")
			+ ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event& e) { show = false; OnShowChange(); });

		showable = &ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::Push<ui::StackTopDownLayoutElement>();
		contentLabel = &ui::Text("Contents: " + text);
		tbText = &ui::Make<ui::Textbox>().SetText(text);
		*tbText + ui::AddEventHandler(ui::EventType::Change, [this](ui::Event& e)
		{
			text = tbText->GetText();
			contentLabel->SetText("Contents: " + text);
		});
		ui::Pop();
		ui::Pop();

		OnShowChange();
	}

	void OnShowChange()
	{
		cbShow->SetState(show);
		showable->SetVisible(show);
	}

	ui::StateToggle* cbShow;
	UIObject* showable;
	ui::TextElement* contentLabel;
	ui::Textbox* tbText;

	bool show = false;
	std::string text;
};
void Test_ZeroRebuild()
{
	ui::Make<ZeroRebuildTest>();
}


struct GlobalEventsTest : ui::Buildable
{
	struct EventTest : ui::Buildable
	{
		void Build() override
		{
			Subscribe(dct);
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackTopDownLayoutElement>();
			count++;
			char bfr[64];
			snprintf(bfr, 64, "%s: %d", name, count);
			ui::Text(bfr);
			infofn(bfr);
			ui::Text(bfr);
			ui::Pop();
			ui::Pop();
		}
		const char* name;
		ui::DataCategoryTag* dct;
		std::function<void(char*)> infofn;
		int count = -1;
	};

	void Build() override
	{
		for (auto& e = ui::Make<EventTest>();
			e.name = "Mouse moved",
			e.dct = ui::DCT_MouseMoved,
			e.infofn = [this](char* bfr) { snprintf(bfr, 64, "mouse pos: %g; %g", system->eventSystem.prevMousePos.x, system->eventSystem.prevMousePos.y); },
			0;);

		for (auto& e = ui::Make<EventTest>();
			e.name = "Resize window",
			e.dct = ui::DCT_ResizeWindow,
			e.infofn = [this](char* bfr) { auto ws = GetNativeWindow()->GetSize(); snprintf(bfr, 64, "window size: %dx%d", ws.x, ws.y); },
			0;);

		for (auto& e = ui::Make<EventTest>();
			e.name = "Drag/drop data changed",
			e.dct = ui::DCT_DragDropDataChanged,
			e.infofn = [this](char* bfr) { auto* ddd = ui::DragDrop::GetData(); snprintf(bfr, 64, "drag data: %s", !ddd ? "<none>" : ddd->type.c_str()); },
			0;);

		for (auto& e = ui::Make<EventTest>();
			e.name = "Tooltip changed",
			e.dct = ui::DCT_TooltipChanged,
			e.infofn = [this](char* bfr) { snprintf(bfr, 64, "tooltip: %s", ui::Tooltip::IsSet() ? "set" : "<none>"); },
			0;);

		ui::MakeWithText<ui::Button>("Draggable") + ui::MakeDraggable([]()
		{
			ui::DragDrop::SetData(new ui::DragDropText("test", "text"));
		});
		ui::MakeWithText<ui::Button>("Tooltip") + ui::AddTooltip("Tooltip");
		ui::Make<ui::DefaultOverlayBuilder>();
	}
};
void Test_GlobalEvents()
{
	ui::Make<GlobalEventsTest>();
}


struct FrameTest : ui::Buildable
{
	struct Frame : ui::Buildable
	{
		void Build() override
		{
			ui::Text(name);
		}
		const char* name = "unknown";
	};

	void OnEnable() override
	{
		frameContents[0] = new ui::FrameContents;
		{
			auto* f = ui::CreateUIObject<Frame>();
			f->name = "Frame A";
			frameContents[0]->BuildRoot(f);
		}
		frameContents[1] = new ui::FrameContents;
		{
			auto* f = ui::CreateUIObject<Frame>();
			f->name = "Frame B";
			frameContents[1]->BuildRoot(f);
		}
	}
	void OnDisable() override
	{
		delete frameContents[0];
		delete frameContents[1];
	}
	void Build() override
	{
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::Push<ui::SizeConstraintElement>().SetHeight(32);
		inlineFrames[0] = &ui::Make<ui::InlineFrame>();
		ui::Pop();
		ui::Pop();

		ui::MakeWithText<ui::Button>("Place 1 in 1") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(0, 0); });
		ui::MakeWithText<ui::Button>("Place 2 in 1") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(1, 0); });

		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::Push<ui::SizeConstraintElement>().SetHeight(32);
		inlineFrames[1] = &ui::Make<ui::InlineFrame>();
		ui::Pop();
		ui::Pop();

		ui::MakeWithText<ui::Button>("Place 1 in 2") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(0, 1); });
		ui::MakeWithText<ui::Button>("Place 2 in 2") + ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(1, 1); });

		ui::Pop();
	}
	void Set(int contID, int frameID)
	{
		inlineFrames[frameID]->SetFrameContents(frameContents[contID]);
	}

	ui::FrameContents* frameContents[2] = {};
	ui::InlineFrame* inlineFrames[2] = {};
};
void Test_Frames()
{
	ui::Make<FrameTest>();
}


struct DialogWindowTest : ui::Buildable
{
	struct BasicDialog : ui::NativeDialogWindow
	{
		BasicDialog()
		{
			SetTitle("Basic dialog window");
			SetSize(200, 100);
		}
		void OnBuild() override
		{
			ui::Text("This is a basic dialog window");
			BasicRadioButton("option 0", choice, 0);
			BasicRadioButton("option 1", choice, 1);
			ui::MakeWithText<ui::Button>("Close").HandleEvent(ui::EventType::Activate) = [this](ui::Event&)
			{
				OnClose();
			};
		}
		void OnClose() override
		{
			puts("closing basic dialog window");
			ui::NativeDialogWindow::OnClose();
		}
		int choice = -1;
	};
	void Build() override
	{
		ui::MakeWithText<ui::Button>("Open dialog")
			+ ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&)
		{
			puts("start showing window");
			BasicDialog bdw;
			bdw.Show();
			printf("done showing window, choice: %d\n", bdw.choice);
		});
	}
};
void Test_DialogWindow()
{
	ui::Make<DialogWindowTest>();
}

