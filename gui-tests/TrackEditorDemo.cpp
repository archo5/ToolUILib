
#include "pch.h"


struct TrackEditorDemo : ui::Buildable
{
	struct TrackEditor : ui::UIElement
	{
		static constexpr float TRACK_HEIGHT = 40;
		struct Item
		{
			float x0, x1, size;
			int track;
			const char* name;
		};

		TrackEditor()
		{
			items.push_back({ 100, 150, 50, 0, "Animation one" });
			items.push_back({ 200, 350, 150, 0, "Sound effect two" });
			items.push_back({ 120, 320, 200, 1, "Camera track" });
		}
		void OnReset() override
		{
			ui::UIElement::OnReset();

			SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			GetStyle().SetHeight(160);
		}
		void OnPaint(const ui::UIPaintContext& ctx) override
		{
			for (int i = 0; i < 4; i++)
			{
				ui::PaintInfo info(this);
				info.rect.y0 += i * TRACK_HEIGHT;
				info.rect.y1 = info.rect.y0 + TRACK_HEIGHT;
				ui::Theme::current->GetStyle("listbox")->background_painter->Paint(info);
			}

			uint32_t id = 0;
			for (Item& item : items)
			{
				ui::PaintInfo info(this);
				info.rect = { item.x0, item.track * TRACK_HEIGHT, item.x1, (item.track + 1) * TRACK_HEIGHT };
				info.state = 0;
				if (subui.IsHovered(id))
					info.state |= ui::PS_Hover;
				if (subui.IsPressed(id))
					info.state |= ui::PS_Down;
				ui::Theme::current->GetStyle("button")->background_painter->Paint(info);
				id++;
			}

			for (Item& item : items)
			{
				ui::UIRect rect = { item.x0, item.track * TRACK_HEIGHT, item.x1, (item.track + 1) * TRACK_HEIGHT };
				ui::draw::TextLine(ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF), 10, rect.x0 + 3 + 1, rect.y1 - 4 + 1, item.name, ui::Color4f(0.0f, 0.5f));
				ui::draw::TextLine(ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF), 10, rect.x0 + 3, rect.y1 - 4, item.name, ui::Color4f(0.9f, 1));
			}
		}
		void OnEvent(ui::Event& e) override
		{
			subui.InitOnEvent(e);
			uint32_t id = 0;
			for (Item& item : items)
			{
				ui::UIRect rect = { item.x0, item.track * TRACK_HEIGHT, item.x1, (item.track + 1) * TRACK_HEIGHT };
				switch (subui.DragOnEvent(id, rect, e))
				{
				case ui::SubUIDragState::Start:
					dx = e.position.x - item.x0;
					dy = e.position.y - item.track * TRACK_HEIGHT;
					break;
				case ui::SubUIDragState::Move:
					item.x0 = e.position.x - dx;
					item.x1 = item.x0 + item.size;
					item.track = round((e.position.y - dy) / TRACK_HEIGHT);
					item.track = std::min(std::max(item.track, 0), 3);
					break;
				}

				ui::UIRect rectL = rect;
				rectL.x1 = rectL.x0 + 8;
				switch (subui.DragOnEvent(id | (1 << 30), rectL, e))
				{
				case ui::SubUIDragState::Start:
					dx = e.position.x - item.x0;
					break;
				case ui::SubUIDragState::Move:
					item.x0 = e.position.x - dx;
					if (item.x0 > item.x1)
						item.x0 = item.x1;
					item.size = item.x1 - item.x0;
					break;
				}

				ui::UIRect rectR = rect;
				rectR.x0 = rectR.x1 - 8;
				switch (subui.DragOnEvent(id | (1 << 31), rectR, e))
				{
				case ui::SubUIDragState::Start:
					dx = e.position.x - item.x1;
					break;
				case ui::SubUIDragState::Move:
					item.x1 = e.position.x - dx;
					if (item.x1 < item.x0)
						item.x1 = item.x0;
					item.size = item.x1 - item.x0;
					break;
				}

				id++;
			}
			if (e.type == ui::EventType::SetCursor)
			{
				if (subui.IsAnyHovered() && subui._hovered >= 0x40000000)
				{
					e.context->SetDefaultCursor(ui::DefaultCursor::ResizeHorizontal);
					e.StopPropagation();
				}
			}
		}

		std::vector<Item> items;
		ui::SubUI<uint32_t> subui;
		float dx, dy;
	};
	void Build() override
	{
		ui::Make<TrackEditor>();
	}
};
void Demo_TrackEditor()
{
	ui::Make<TrackEditorDemo>();
}

