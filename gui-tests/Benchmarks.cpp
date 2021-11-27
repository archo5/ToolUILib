
#include "pch.h"


struct SubUIBenchmark : ui::Buildable
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
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::PaintInfo info(this);
		ui::Theme::current->GetStyle("textbox")->background_painter->Paint(info);
		auto r = finalRectC;

		for (uint16_t pid = 0; pid < points.size(); pid++)
		{
			auto ddr = ui::UIRect::FromCenterExtents(r.x0 + points[pid].x, r.y0 + points[pid].y, 4);

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
	void OnEvent(ui::Event& e) override
	{
		auto r = finalRectC;

		subui.InitOnEvent(e);
		for (uint16_t pid = 0; pid < points.size(); pid++)
		{
			switch (subui.DragOnEvent(pid, ui::UIRect::FromCenterExtents(r.x0 + points[pid].x, r.y0 + points[pid].y, 4), e))
			{
			case ui::SubUIDragState::Start:
				dragOff = points[pid] - e.position;
				break;
			case ui::SubUIDragState::Move:
				points[pid] = e.position + dragOff;
				break;
			}
		}
	}
	void Build() override
	{
		GetStyle().SetPadding(3);
		GetStyle().SetWidth(820);
		GetStyle().SetHeight(820);
	}

	ui::SubUI<uint16_t> subui;
	std::vector<ui::Point2f> points;
	ui::Vec2f dragOff = { 0, 0 };
};
void Benchmark_SubUI()
{
	ui::Make<SubUIBenchmark>();
}


struct BuildManyElementsBenchmark : ui::Buildable
{
	void Build() override
	{
		for (int i = 0; i < 100000; i++)
		{
			ui::Make<ui::BoxElement>();
		}
	}
};
void Benchmark_BuildManyElements()
{
	ui::Make<BuildManyElementsBenchmark>();
}

