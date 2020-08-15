
#include "pch.h"


struct TrackEditorDemo : ui::Node
{
	struct TrackEditor : UIElement
	{
		static constexpr float TRACK_HEIGHT = 40;
		struct Item
		{
			float x0, x1, size;
			int track;
		};

		TrackEditor()
		{
			SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
			items.push_back({ 100, 150, 50, 0 });
			items.push_back({ 200, 350, 150, 0 });
			items.push_back({ 120, 320, 200, 1 });
			GetStyle().SetHeight(160);
		}
		void OnPaint() override
		{
			for (int i = 0; i < 4; i++)
			{
				style::PaintInfo info(this);
				info.rect.y0 += i * TRACK_HEIGHT;
				info.rect.y1 = info.rect.y0 + TRACK_HEIGHT;
				ui::Theme::current->listBox->paint_func(info);
			}

			uint32_t id = 0;
			for (Item& item : items)
			{
				style::PaintInfo info(this);
				info.rect = { item.x0, item.track * TRACK_HEIGHT, item.x1, (item.track + 1) * TRACK_HEIGHT };
				info.state = 0;
				if (subui.IsHovered(id))
					info.state |= style::PS_Hover;
				if (subui.IsPressed(id))
					info.state |= style::PS_Down;
				ui::Theme::current->button->paint_func(info);
				id++;
			}
		}
		void OnEvent(UIEvent& e) override
		{
			subui.InitOnEvent(e);
			uint32_t id = 0;
			for (Item& item : items)
			{
				UIRect rect = { item.x0, item.track * TRACK_HEIGHT, item.x1, (item.track + 1) * TRACK_HEIGHT };
				switch (subui.DragOnEvent(id, rect, e))
				{
				case ui::SubUIDragState::Start:
					dx = e.x - item.x0;
					dy = e.y - item.track * TRACK_HEIGHT;
					break;
				case ui::SubUIDragState::Move:
					item.x0 = e.x - dx;
					item.x1 = item.x0 + item.size;
					item.track = round((e.y - dy) / TRACK_HEIGHT);
					item.track = std::min(std::max(item.track, 0), 3);
					break;
				}

				UIRect rectL = rect;
				rectL.x1 = rectL.x0 + 8;
				switch (subui.DragOnEvent(id | (1 << 30), rectL, e))
				{
				case ui::SubUIDragState::Start:
					dx = e.x - item.x0;
					break;
				case ui::SubUIDragState::Move:
					item.x0 = e.x - dx;
					if (item.x0 > item.x1)
						item.x0 = item.x1;
					item.size = item.x1 - item.x0;
					break;
				}

				UIRect rectR = rect;
				rectR.x0 = rectR.x1 - 8;
				switch (subui.DragOnEvent(id | (1 << 31), rectR, e))
				{
				case ui::SubUIDragState::Start:
					dx = e.x - item.x1;
					break;
				case ui::SubUIDragState::Move:
					item.x1 = e.x - dx;
					if (item.x1 < item.x0)
						item.x1 = item.x0;
					item.size = item.x1 - item.x0;
					break;
				}

				id++;
			}
			if (e.type == UIEventType::SetCursor)
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
	void Render(UIContainer* ctx) override
	{
		ctx->Make<TrackEditor>();
	}
};
void Demo_TrackEditor(UIContainer* ctx)
{
	ctx->Make<TrackEditorDemo>();
}

