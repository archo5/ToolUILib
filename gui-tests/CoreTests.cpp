
#include "pch.h"


struct RenderingPrimitives : ui::Node
{
	void OnPaint() override
	{
		ui::Color4b col = ui::Color4f(1, 0.5f, 0);
		ui::Color4b colO = ui::Color4f(0, 1, 0.5f, 0.5f);

		for (int t = 0; t <= 5; t++)
		{
			float w = powf(3.0f, t * 0.2f);
			float xo = t * 70 + 20;
			float x0 = 10 + xo;
			float x1 = 25 + xo;
			float x2 = 45 + xo;
			float x3 = 60 + xo;
			for (int i = 0; i < 8; i++)
			{
				ui::draw::LineCol(x0, 10 + i * 4, x1, 10 + i * 5, w, col);
				ui::draw::AALineCol(x0, 10 + i * 4, x1, 10 + i * 5, w, colO);
				ui::draw::AALineCol(x0, 210 + i * 4, x1, 210 + i * 5, w, col);

				ui::draw::LineCol(x2, 10 + i * 5, x3, 10 + i * 4, w, col);
				ui::draw::AALineCol(x2, 10 + i * 5, x3, 10 + i * 4, w, colO);
				ui::draw::AALineCol(x2, 210 + i * 5, x3, 210 + i * 4, w, col);
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
		}

		ui::draw::RectCol(50, 10, 60, 20, col);

		ui::draw::TextLine(ui::GetFont(ui::FONT_FAMILY_SANS_SERIF), 20, 20, 150, "sans-serif w=normal it=0", ui::Color4f(0.9f, 0.8f, 0.6f));
		ui::draw::TextLine(ui::GetFont(ui::FONT_FAMILY_SERIF, ui::FONT_WEIGHT_BOLD), 20, 20, 170, "serif w=bold it=0", ui::Color4f(0.6f, 0.8f, 0.9f));
		ui::draw::TextLine(ui::GetFont(ui::FONT_FAMILY_MONOSPACE, ui::FONT_WEIGHT_NORMAL, true), 20, 20, 190, "monospace w=normal it=1", ui::Color4f(0.7f, 0.9f, 0.6f));
	}
	void Render(UIContainer* ctx) override
	{
		*this + ui::Width(1000);
		*this + ui::Height(1000);
	}
};
void Test_RenderingPrimitives(UIContainer* ctx)
{
	ctx->Make<RenderingPrimitives>();
}


struct OpenCloseTest : ui::Node
{
	struct AllocTest
	{
		AllocTest(int v) : val(v) { printf("alloc #=%d\n", v); }
		~AllocTest() { printf("free #=%d\n", val); }
		int val;
	};

	void Render(UIContainer* ctx) override
	{
		static int counter = 0;
		Allocate<AllocTest>(++counter);

		ctx->Push<ui::Panel>();

		auto* cb = ctx->Make<ui::Checkbox>()->Init(open);
		cb->HandleEvent(UIEventType::Change) = [this, cb](UIEvent&) { open = !open; Rerender(); };

		ctx->MakeWithText<ui::Button>("Open")->HandleEvent(UIEventType::Activate) = [this](UIEvent&) { open = true; Rerender(); };

		ctx->MakeWithText<ui::Button>("Close")->HandleEvent(UIEventType::Activate) = [this](UIEvent&) { open = false; Rerender(); };

		if (open)
		{
			ctx->Push<ui::Panel>();
			ctx->Text("It is open!");
			auto s = ctx->MakeWithText<ui::BoxElement>("Different text")->GetStyle();
			s.SetFontSize(16);
			s.SetFontWeight(style::FontWeight::Bold);
			s.SetFontStyle(style::FontStyle::Italic);
			s.SetTextColor(ui::Color4f(1.0f, 0.1f, 0.0f));
			ctx->Pop();
		}

		ctx->Pop();
	}
	void OnSerialize(IDataSerializer& s) override
	{
		s << open;
	}

	bool open = false;
};
void Test_OpenClose(UIContainer* ctx)
{
	ctx->Make<OpenCloseTest>();
}


struct AnimationRequestTest : ui::Node
{
	AnimationRequestTest()
	{
		animReq.callback = [this]() { OnAnimation(); };
	}
	void OnAnimation()
	{
		GetNativeWindow()->InvalidateAll();
	}
	void Render(UIContainer* ctx) override
	{
		*this + ui::Width(200);
		*this + ui::Height(200);
		auto* cb = ctx->Make<ui::Checkbox>()->Init(animReq.IsAnimating());
		cb->HandleEvent(UIEventType::Change) = [this, cb](UIEvent&) { animReq.SetAnimating(!animReq.IsAnimating()); Rerender(); };
	}
	void OnPaint() override
	{
		ui::Node::OnPaint();

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
void Test_AnimationRequest(UIContainer* ctx)
{
	ctx->Make<AnimationRequestTest>();
}


struct ElementResetTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		if (first)
		{
			*ctx->MakeWithText<ui::Button>("First")
				+ ui::Width(300)
				+ ui::EventHandler(UIEventType::Click, [this](UIEvent&) { first = false; Rerender(); });

			auto& tb = *ctx->Make<ui::Textbox>();
			tb + ui::EventHandler(UIEventType::Change, [this, &tb](UIEvent&) { text[0] = tb.GetText(); });
			tb.SetText(text[0]);
		}
		else
		{
			*ctx->MakeWithText<ui::Button>("Second")
				+ ui::Height(30)
				+ ui::EventHandler(UIEventType::Click, [this](UIEvent&) { first = true; Rerender(); });

			auto& tb = *ctx->Make<ui::Textbox>();
			tb + ui::EventHandler(UIEventType::Change, [this, &tb](UIEvent&) { text[1] = tb.GetText(); });
			tb.SetText(text[1]);
		}

		auto& tb = *ctx->Make<ui::Textbox>();
		tb + ui::EventHandler(UIEventType::Change, [this, &tb](UIEvent&) { text[2] = tb.GetText(); });
		tb.SetText(text[2]);
	}
	bool first = true;
	std::string text[3] = { "first", "second", "third" };
};
void Test_ElementReset(UIContainer* ctx)
{
	ctx->Make<ElementResetTest>();
}


struct SubUITest : ui::Node
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
		DrawTextLine(r.x0 + 25 - GetTextWidth("Button") / 2, r.y0 + 25 + GetFontHeight() / 2, "Button", 1, 1, 1);

		auto ddr = UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(1, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		DrawTextLine(r.x0 + draggableX - GetTextWidth("D&D") / 2, r.y0 + draggableY + GetFontHeight() / 2, "D&D", 1, 1, 1);

		ddr = UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(2, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		DrawTextLine(r.x0 + draggableX - GetTextWidth("x") / 2, r.y0 + 5 + GetFontHeight() / 2, "x", 1, 1, 1);

		ddr = UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10);
		ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, PickColor(3, { 0, 1, 1, 0.5f }, { 1, 1, 0, 0.5f }, { 1, 0, 1, 0.5f }));
		DrawTextLine(r.x0 + 5 - GetTextWidth("y") / 2, r.y0 + draggableY + GetFontHeight() / 2, "y", 1, 1, 1);
	}
	void OnEvent(UIEvent& e) override
	{
		auto r = finalRectC;

		subui.InitOnEvent(e);
		if (subui.ButtonOnEvent(0, UIRect{ r.x0, r.y0, r.x0 + 50, r.y0 + 50 }, e))
		{
			puts("0-50 clicked");
		}
		switch (subui.DragOnEvent(1, UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + draggableY, 10), e))
		{
		case ui::SubUIDragState::Start:
			puts("drag start");
			dox = draggableX - e.x;
			doy = draggableY - e.y;
			break;
		case ui::SubUIDragState::Move:
			puts("drag move");
			draggableX = e.x + dox;
			draggableY = e.y + doy;
			break;
		case ui::SubUIDragState::Stop:
			puts("drag stop");
			break;
		}
		switch (subui.DragOnEvent(2, UIRect::FromCenterExtents(r.x0 + draggableX, r.y0 + 5, 10, 5), e))
		{
		case ui::SubUIDragState::Start:
			dox = draggableX - e.x;
			break;
		case ui::SubUIDragState::Move:
			draggableX = e.x + dox;
			break;
		}
		switch (subui.DragOnEvent(3, UIRect::FromCenterExtents(r.x0 + 5, r.y0 + draggableY, 5, 10), e))
		{
		case ui::SubUIDragState::Start:
			doy = draggableY - e.y;
			break;
		case ui::SubUIDragState::Move:
			draggableY = e.y + doy;
			break;
		}

		if (e.type == UIEventType::ButtonDown && !subui.IsAnyPressed())
		{
			subui.DragStart(1);
			dox = 0;
			doy = 0;
		}
	}
	void Render(UIContainer* ctx) override
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
void Test_SubUI(UIContainer* ctx)
{
	ctx->Make<SubUITest>();
}


struct HighElementCountTest : ui::Node
{
	struct DummyElement : UIElement
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
	void Render(UIContainer* ctx) override
	{
		ctx->PushBox();// + ui::StackingDirection(style::StackingDirection::LeftToRight); TODO FIX
		ctx->MakeWithText<ui::RadioButtonT<int>>("no styles")->Init(styleMode, 0)->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
		ctx->MakeWithText<ui::RadioButtonT<int>>("same style")->Init(styleMode, 1)->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
		ctx->MakeWithText<ui::RadioButtonT<int>>("different styles")->Init(styleMode, 2)->HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
		ctx->Pop();

		for (int i = 0; i < 1000; i++)
		{
			auto* el = ctx->Make<DummyElement>();
			switch (styleMode)
			{
			case 0: break;
			case 1: *el + ui::Width(200); break;
			case 2: *el + ui::Width(100 + i * 0.02f); break;
			}
		}

		printf("# blocks: %d\n", style::g_numBlocks);
	}
	int styleMode;
};
void Test_HighElementCount(UIContainer* ctx)
{
}


struct ZeroRerenderTest : ui::Node
{
	bool first = true;
	void Render(UIContainer* ctx) override
	{
		if (first)
			first = false;
		else
			puts("Should not happen!");

		ui::Property::Begin(ctx, "Show?");
		cbShow = ctx->Make<ui::CheckboxBoolState>()->SetChecked(show);
		ui::Property::End(ctx);

		*cbShow + ui::EventHandler(UIEventType::Change, [this](UIEvent& e) { show = cbShow->GetChecked(); OnShowChange(); });
		*ctx->MakeWithText<ui::Button>("Show")
			+ ui::EventHandler(UIEventType::Activate, [this](UIEvent& e) { show = true; OnShowChange(); });
		*ctx->MakeWithText<ui::Button>("Hide")
			+ ui::EventHandler(UIEventType::Activate, [this](UIEvent& e) { show = false; OnShowChange(); });

		showable = ctx->Push<ui::Panel>();
		contentLabel = &ctx->Text("Contents: " + text);
		tbText = &ctx->Make<ui::Textbox>()->SetText(text);
		*tbText + ui::EventHandler(UIEventType::Change, [this](UIEvent& e)
		{
			text = tbText->GetText();
			contentLabel->SetText("Contents: " + text);
		});
		ctx->Pop();

		OnShowChange();
	}

	void OnShowChange()
	{
		cbShow->SetChecked(show);
		showable->GetStyle().SetHeight(show ? style::Coord::Undefined() : style::Coord(0));
	}

	ui::CheckboxBoolState* cbShow;
	UIObject* showable;
	ui::TextElement* contentLabel;
	ui::Textbox* tbText;

	bool show = false;
	std::string text;
};
void Test_ZeroRerender(UIContainer* ctx)
{
	ctx->Make<ZeroRerenderTest>();
}


struct GlobalEventsTest : ui::Node
{
	struct EventTest : ui::Node
	{
		static constexpr bool Persistent = true;
		void Render(UIContainer* ctx) override
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

	void Render(UIContainer* ctx) override
	{
		for (auto* e = ctx->Make<EventTest>();
			e->name = "Mouse moved",
			e->dct = ui::DCT_MouseMoved,
			e->infofn = [this](char* bfr) { snprintf(bfr, 64, "mouse pos: %g; %g", system->eventSystem.prevMouseX, system->eventSystem.prevMouseY); },
			0;);

		for (auto* e = ctx->Make<EventTest>();
			e->name = "Resize window",
			e->dct = ui::DCT_ResizeWindow,
			e->infofn = [this](char* bfr) { auto ws = GetNativeWindow()->GetSize(); snprintf(bfr, 64, "window size: %dx%d", ws.x, ws.y); },
			0;);

		for (auto* e = ctx->Make<EventTest>();
			e->name = "Drag/drop data changed",
			e->dct = ui::DCT_DragDropDataChanged,
			e->infofn = [this](char* bfr) { auto* ddd = ui::DragDrop::GetData(); snprintf(bfr, 64, "drag data: %s", !ddd ? "<none>" : ddd->type.c_str()); },
			0;);

		for (auto* e = ctx->Make<EventTest>();
			e->name = "Tooltip changed",
			e->dct = ui::DCT_TooltipChanged,
			e->infofn = [this](char* bfr) { snprintf(bfr, 64, "tooltip: %s", ui::Tooltip::IsSet() ? "set" : "<none>"); },
			0;);

		*ctx->MakeWithText<ui::Button>("Draggable") + ui::MakeDraggable([]()
		{
			ui::DragDrop::SetData(new ui::DragDropText("test", "text"));
		});
		*ctx->MakeWithText<ui::Button>("Tooltip") + ui::AddTooltip("Tooltip");
		ctx->Make<ui::DefaultOverlayRenderer>();
	}
};
void Test_GlobalEvents(UIContainer* ctx)
{
	ctx->Make<GlobalEventsTest>();
}

