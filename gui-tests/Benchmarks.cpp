
#include "pch.h"


struct SubUIBenchmark : ui::Node
{
	SubUIBenchmark()
	{
		for (int y = 0; y < 60; y++)
		{
			for (int x = 0; x < 60; x++)
			{
				points.push_back({ x * 12.f + 10.f, y * 12.f + 10.f });
			}
		}
	}
	void OnPaint() override
	{
		style::PaintInfo info(this);
		ui::Theme::current->textBoxBase->paint_func(info);
		auto r = finalRectC;

		for (uint16_t pid = 0; pid < points.size(); pid++)
		{
			auto ddr = UIRect::FromCenterExtents(r.x0 + points[pid].x, r.y0 + points[pid].y, 4);

			ui::Color4f col;
			if (subui.IsPressed(pid))
				col = { 1, 0, 1, 0.5f };
			else if (subui.IsHovered(pid))
				col = { 1, 1, 0, 0.5f };
			else
				col = { 0, 1, 1, 0.5f };
			ui::draw::RectCol(ddr.x0, ddr.y0, ddr.x1, ddr.y1, col);
		}
	}
	void OnEvent(UIEvent& e) override
	{
		auto r = finalRectC;

		subui.InitOnEvent(e);
		for (uint16_t pid = 0; pid < points.size(); pid++)
		{
			switch (subui.DragOnEvent(pid, UIRect::FromCenterExtents(r.x0 + points[pid].x, r.y0 + points[pid].y, 4), e))
			{
			case ui::SubUIDragState::Start:
				dox = points[pid].x - e.x;
				doy = points[pid].y - e.y;
				break;
			case ui::SubUIDragState::Move:
				points[pid].x = e.x + dox;
				points[pid].y = e.y + doy;
				break;
			}
		}
	}
	void Render(UIContainer* ctx) override
	{
		GetStyle().SetPadding(3);
		GetStyle().SetWidth(820);
		GetStyle().SetHeight(820);
	}

	ui::SubUI<uint16_t> subui;
	std::vector<Point<float>> points;
	float dox = 0, doy = 0;
};
void Benchmark_SubUI(UIContainer* ctx)
{
	ctx->Make<SubUIBenchmark>();
}

