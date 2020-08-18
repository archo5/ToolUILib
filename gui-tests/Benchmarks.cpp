
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


struct RandomNumberDataSource : ui::TableDataSource
{
	size_t GetNumRows() override { return 1000000; }
	size_t GetNumCols() override { return 5; }
	std::string GetRowName(size_t row) override { return std::to_string(row + 1); }
	std::string GetColName(size_t col) override { return std::to_string(col + 1); }
	std::string GetText(size_t row, size_t col) override
	{
		return std::to_string(((unsigned(row) * 5 + unsigned(col)) + 1013904223U) * 1664525U);
	}
}
g_randomNumbers;

struct TableViewBenchmark : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		GetStyle().SetLayout(style::layouts::EdgeSlice());

		auto* tv = ctx->Make<ui::TableView>();
		*tv + ui::Height(style::Coord::Percent(100));
		tv->SetDataSource(&g_randomNumbers);
	}
};
void Benchmark_TableView(UIContainer* ctx)
{
	ctx->Make<TableViewBenchmark>();
}

