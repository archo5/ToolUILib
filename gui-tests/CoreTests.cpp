
#include "pch.h"
#include <stdarg.h>


struct RenderingPrimitives : ui::Buildable
{
	void OnPaint() override
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
	}
	void Build(ui::UIContainer* ctx) override
	{
		*this + ui::Width(1000);
		*this + ui::Height(1000);
	}
};
void Test_RenderingPrimitives(ui::UIContainer* ctx)
{
	ctx->Make<RenderingPrimitives>();
}


struct KeyboardEventsTest : ui::Buildable
{
	static constexpr bool Persistent = true;
	static constexpr unsigned MAX_MESSAGES = 50;

	KeyboardEventsTest()
	{
		SetFlag(ui::UIObject_IsFocusable, true);
		SetFlag(ui::UIObject_DB_FocusOnLeftClick, true);
	}
	void OnInit() override
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
	void OnPaint() override
	{
		auto* font = ui::GetFont(ui::FONT_FAMILY_SANS_SERIF);
		for (unsigned i = 0; i < MAX_MESSAGES; i++)
		{
			unsigned idx = (MAX_MESSAGES * 2 + writePos - i - 1) % MAX_MESSAGES;
			ui::draw::TextLine(font, 12, 0, finalRectC.y1 - i * 12, msgBuf[idx], ui::Color4f::White());
		}
	}
	void Build(ui::UIContainer* ctx) override
	{
		*this + ui::Width(style::Coord::Percent(100));
		*this + ui::Height(style::Coord::Percent(100));
	}

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

	std::string msgBuf[MAX_MESSAGES];
	unsigned writePos = 0;
};
void Test_KeyboardEvents(ui::UIContainer* ctx)
{
	ctx->Make<KeyboardEventsTest>();
}


struct OpenCloseTest : ui::Buildable
{
	struct AllocTest
	{
		AllocTest(int v) : val(v) { printf("alloc #=%d\n", v); }
		~AllocTest() { printf("free #=%d\n", val); }
		int val;
	};

	void Build(ui::UIContainer* ctx) override
	{
		static int counter = 0;
		Allocate<AllocTest>(++counter);

		ctx->Push<ui::Panel>();

		auto& cb = ctx->Push<ui::StateToggle>().InitReadOnly(open);
		ctx->Make<ui::CheckboxIcon>();
		ctx->Pop();
		cb.HandleEvent(ui::EventType::Activate) = [this, cb](ui::Event&) { open = !open; Rebuild(); };

		ctx->MakeWithText<ui::Button>("Open").HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { open = true; Rebuild(); };

		ctx->MakeWithText<ui::Button>("Close").HandleEvent(ui::EventType::Activate) = [this](ui::Event&) { open = false; Rebuild(); };

		if (open)
		{
			ctx->Push<ui::Panel>();
			ctx->Text("It is open!");
			auto s = ctx->MakeWithText<ui::BoxElement>("Different text").GetStyle();
			s.SetFontSize(16);
			s.SetFontWeight(style::FontWeight::Bold);
			s.SetFontStyle(style::FontStyle::Italic);
			s.SetTextColor(ui::Color4f(1.0f, 0.1f, 0.0f));
			ctx->Pop();
		}

		ctx->Pop();
	}
	void OnSerialize(ui::IDataSerializer& s) override
	{
		s << open;
	}

	bool open = false;
};
void Test_OpenClose(ui::UIContainer* ctx)
{
	ctx->Make<OpenCloseTest>();
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
	void Build(ui::UIContainer* ctx) override
	{
		*this + ui::Width(200);
		*this + ui::Height(200);
		auto& cb = ctx->Push<ui::StateToggle>().InitReadOnly(animReq.IsAnimating());
		ctx->Make<ui::CheckboxIcon>();
		ctx->Pop();
		cb.HandleEvent(ui::EventType::Activate) = [this, &cb](ui::Event&) { animReq.SetAnimating(!animReq.IsAnimating()); Rebuild(); };
	}
	void OnPaint() override
	{
		ui::Buildable::OnPaint();

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
void Test_AnimationRequest(ui::UIContainer* ctx)
{
	ctx->Make<AnimationRequestTest>();
}


struct ElementResetTest : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override
	{
		if (first)
		{
			ctx->MakeWithText<ui::Button>("First")
				+ ui::Width(300)
				+ ui::EventHandler(ui::EventType::Click, [this](ui::Event&) { first = false; Rebuild(); });

			auto& tb = ctx->Make<ui::Textbox>();
			tb + ui::EventHandler(ui::EventType::Change, [this, &tb](ui::Event&) { text[0] = tb.GetText(); });
			tb.SetText(text[0]);
		}
		else
		{
			ctx->MakeWithText<ui::Button>("Second")
				+ ui::Height(30)
				+ ui::EventHandler(ui::EventType::Click, [this](ui::Event&) { first = true; Rebuild(); });

			auto& tb = ctx->Make<ui::Textbox>();
			tb + ui::EventHandler(ui::EventType::Change, [this, &tb](ui::Event&) { text[1] = tb.GetText(); });
			tb.SetText(text[1]);
		}

		auto& tb = ctx->Make<ui::Textbox>();
		tb + ui::EventHandler(ui::EventType::Change, [this, &tb](ui::Event&) { text[2] = tb.GetText(); });
		tb.SetText(text[2]);
	}
	bool first = true;
	std::string text[3] = { "first", "second", "third" };
};
void Test_ElementReset(ui::UIContainer* ctx)
{
	ctx->Make<ElementResetTest>();
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
	void OnPaint() override
	{
		style::PaintInfo info(this);
		ui::Theme::current->textBoxBase->paint_func(info);
		auto r = finalRectC;

		ui::draw::RectCol(r.x0, r.y0, r.x0 + 50, r.y0 + 50, PickColor(0, { 0, 0, 1, 0.5f }, { 0, 1, 0, 0.5f }, { 1, 0, 0, 0.5f }));
		ui::DrawTextLine(r.x0 + 25 - ui::GetTextWidth("Button") / 2, r.y0 + 25 + ui::GetFontHeight() / 2, "Button", 1, 1, 1);

		auto ddr = ui::UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(1, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		ui::DrawTextLine(r.x0 + draggableX - ui::GetTextWidth("D&D") / 2, r.y0 + draggableY + ui::GetFontHeight() / 2, "D&D", 1, 1, 1);

		ddr = ui::UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(2, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		ui::DrawTextLine(r.x0 + draggableX - ui::GetTextWidth("x") / 2, r.y0 + 5 + ui::GetFontHeight() / 2, "x", 1, 1, 1);

		ddr = ui::UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(3, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		ui::DrawTextLine(r.x0 + 5 - ui::GetTextWidth("y") / 2, r.y0 + draggableY + ui::GetFontHeight() / 2, "y", 1, 1, 1);
	}
	void OnEvent(ui::Event& e) override
	{
		auto r = finalRectC;

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
	void Build(ui::UIContainer* ctx) override
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
void Test_SubUI(ui::UIContainer* ctx)
{
	ctx->Make<SubUITest>();
}


struct HighElementCountTest : ui::Buildable
{
	struct DummyElement : ui::UIElement
	{
		void OnPaint() override
		{
			auto r = GetContentRect();
			ui::draw::RectCol(r.x0, r.y0, r.x1, r.y1, ui::Color4f(fmodf(uintptr_t(this) / (8 * 256.0f), 1.0f), 0.0f, 0.0f));
		}
		void GetSize(style::Coord& outWidth, style::Coord& outHeight) override
		{
			outWidth = 100;
			outHeight = 1;
		}
	};
	void Build(ui::UIContainer* ctx) override
	{
		ctx->PushBox();// + ui::StackingDirection(style::StackingDirection::LeftToRight); TODO FIX
		BasicRadioButton(ctx, "no styles", styleMode, 0) + ui::RebuildOnChange();
		BasicRadioButton(ctx, "same style", styleMode, 1) + ui::RebuildOnChange();
		BasicRadioButton(ctx, "different styles", styleMode, 2) + ui::RebuildOnChange();
		ctx->Pop();

		for (int i = 0; i < 1000; i++)
		{
			auto& el = ctx->Make<DummyElement>();
			switch (styleMode)
			{
			case 0: break;
			case 1: el + ui::Width(200); break;
			case 2: el + ui::Width(100 + i * 0.02f); break;
			}
		}

		printf("# blocks: %d\n", style::g_numBlocks);
	}
	int styleMode;
};
void Test_HighElementCount(ui::UIContainer* ctx)
{
	ctx->Make<HighElementCountTest>();
}


struct ZeroRebuildTest : ui::Buildable
{
	bool first = true;
	void Build(ui::UIContainer* ctx) override
	{
		if (first)
			first = false;
		else
			puts("Should not happen!");

		ui::Property::Begin(ctx, "Show?");
		cbShow = &ctx->Push<ui::StateToggle>().InitEditable(show);
		ctx->Make<ui::CheckboxIcon>();
		ctx->Pop();
		ui::Property::End(ctx);

		*cbShow + ui::EventHandler(ui::EventType::Change, [this](ui::Event& e) { show = cbShow->GetState(); OnShowChange(); });
		ctx->MakeWithText<ui::Button>("Show")
			+ ui::EventHandler(ui::EventType::Activate, [this](ui::Event& e) { show = true; OnShowChange(); });
		ctx->MakeWithText<ui::Button>("Hide")
			+ ui::EventHandler(ui::EventType::Activate, [this](ui::Event& e) { show = false; OnShowChange(); });

		showable = &ctx->Push<ui::Panel>();
		contentLabel = &ctx->Text("Contents: " + text);
		tbText = &ctx->Make<ui::Textbox>().SetText(text);
		*tbText + ui::EventHandler(ui::EventType::Change, [this](ui::Event& e)
		{
			text = tbText->GetText();
			contentLabel->SetText("Contents: " + text);
		});
		ctx->Pop();

		OnShowChange();
	}

	void OnShowChange()
	{
		cbShow->SetState(show);
		showable->GetStyle().SetHeight(show ? style::Coord::Undefined() : style::Coord(0));
	}

	ui::StateToggle* cbShow;
	UIObject* showable;
	ui::TextElement* contentLabel;
	ui::Textbox* tbText;

	bool show = false;
	std::string text;
};
void Test_ZeroRebuild(ui::UIContainer* ctx)
{
	ctx->Make<ZeroRebuildTest>();
}


struct GlobalEventsTest : ui::Buildable
{
	struct EventTest : ui::Buildable
	{
		static constexpr bool Persistent = true;
		void Build(ui::UIContainer* ctx) override
		{
			Subscribe(dct);
			ctx->Push<ui::Panel>();
			count++;
			char bfr[64];
			snprintf(bfr, 64, "%s: %d", name, count);
			ctx->Text(bfr);
			infofn(bfr);
			ctx->Text(bfr);
			ctx->Pop();
		}
		const char* name;
		ui::DataCategoryTag* dct;
		std::function<void(char*)> infofn;
		int count = -1;
	};

	void Build(ui::UIContainer* ctx) override
	{
		for (auto& e = ctx->Make<EventTest>();
			e.name = "Mouse moved",
			e.dct = ui::DCT_MouseMoved,
			e.infofn = [this](char* bfr) { snprintf(bfr, 64, "mouse pos: %g; %g", system->eventSystem.prevMousePos.x, system->eventSystem.prevMousePos.y); },
			0;);

		for (auto& e = ctx->Make<EventTest>();
			e.name = "Resize window",
			e.dct = ui::DCT_ResizeWindow,
			e.infofn = [this](char* bfr) { auto ws = GetNativeWindow()->GetSize(); snprintf(bfr, 64, "window size: %dx%d", ws.x, ws.y); },
			0;);

		for (auto& e = ctx->Make<EventTest>();
			e.name = "Drag/drop data changed",
			e.dct = ui::DCT_DragDropDataChanged,
			e.infofn = [this](char* bfr) { auto* ddd = ui::DragDrop::GetData(); snprintf(bfr, 64, "drag data: %s", !ddd ? "<none>" : ddd->type.c_str()); },
			0;);

		for (auto& e = ctx->Make<EventTest>();
			e.name = "Tooltip changed",
			e.dct = ui::DCT_TooltipChanged,
			e.infofn = [this](char* bfr) { snprintf(bfr, 64, "tooltip: %s", ui::Tooltip::IsSet() ? "set" : "<none>"); },
			0;);

		ctx->MakeWithText<ui::Button>("Draggable") + ui::MakeDraggable([]()
		{
			ui::DragDrop::SetData(new ui::DragDropText("test", "text"));
		});
		ctx->MakeWithText<ui::Button>("Tooltip") + ui::AddTooltip("Tooltip");
		ctx->Make<ui::DefaultOverlayBuilder>();
	}
};
void Test_GlobalEvents(ui::UIContainer* ctx)
{
	ctx->Make<GlobalEventsTest>();
}


struct FrameTest : ui::Buildable
{
	struct Frame : ui::Buildable
	{
		void Build(ui::UIContainer* ctx) override
		{
			ctx->Text(name);
		}
		const char* name = "unknown";
	};

	void OnInit() override
	{
		frameContents[0] = new ui::FrameContents;
		{
			frameContents[0]->AllocRoot<Frame>()->name = "Frame A";
			frameContents[0]->BuildRoot();
		}
		frameContents[1] = new ui::FrameContents;
		{
			frameContents[1]->AllocRoot<Frame>()->name = "Frame B";
			frameContents[1]->BuildRoot();
		}
	}
	void OnDestroy() override
	{
		delete frameContents[0];
		delete frameContents[1];
	}
	void Build(ui::UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>();
		inlineFrames[0] = &ctx->Make<ui::InlineFrame>();
		ctx->Pop();

		ctx->MakeWithText<ui::Button>("Place 1 in 1") + ui::EventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(0, 0); });
		ctx->MakeWithText<ui::Button>("Place 2 in 1") + ui::EventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(1, 0); });

		ctx->Push<ui::Panel>();
		inlineFrames[1] = &ctx->Make<ui::InlineFrame>();
		ctx->Pop();

		for (int i = 0; i < 2; i++)
			*inlineFrames[i] + ui::Height(32);

		ctx->MakeWithText<ui::Button>("Place 1 in 2") + ui::EventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(0, 1); });
		ctx->MakeWithText<ui::Button>("Place 2 in 2") + ui::EventHandler(ui::EventType::Activate, [this](ui::Event&) { Set(1, 1); });
	}
	void Set(int contID, int frameID)
	{
		inlineFrames[frameID]->SetFrameContents(frameContents[contID]);
	}

	ui::FrameContents* frameContents[2] = {};
	ui::InlineFrame* inlineFrames[2] = {};
};
void Test_Frames(ui::UIContainer* ctx)
{
	ctx->Make<FrameTest>();
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
		void OnBuild(ui::UIContainer* ctx) override
		{
			ctx->Text("This is a basic dialog window");
			BasicRadioButton(ctx, "option 0", choice, 0);
			BasicRadioButton(ctx, "option 1", choice, 1);
			ctx->MakeWithText<ui::Button>("Close").HandleEvent(ui::EventType::Activate) = [this](ui::Event&)
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
	void Build(ui::UIContainer* ctx) override
	{
		ctx->MakeWithText<ui::Button>("Open dialog")
			+ ui::EventHandler(ui::EventType::Activate, [this](ui::Event&)
		{
			puts("start showing window");
			BasicDialog bdw;
			bdw.Show();
			printf("done showing window, choice: %d\n", bdw.choice);
		});
	}
};
void Test_DialogWindow(ui::UIContainer* ctx)
{
	ctx->Make<DialogWindowTest>();
}

