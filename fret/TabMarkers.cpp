
#include "pch.h"
#include "TabMarkers.h"

#include "Workspace.h"


void TabMarkers::Build(ui::UIContainer* ctx)
{
	auto* f = of->ddFile;
	auto& spmkr = ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());

		ctx->Text("Marked items") + ui::Padding(5);
		auto& tv = ctx->Make<ui::TableView>();
		curTable = &tv;
		tv + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
		tv.SetDataSource(&f->mdSrc);
		tv.SetSelectionStorage(&f->mdSrc);
		tv.SetSelectionMode(ui::SelectionMode::Single);
		tv.CalculateColumnWidths();
		tv.HandleEvent(&tv, ui::EventType::Click) = [this, f, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == ui::MouseButton::Left && e.numRepeats == 2)
			{
				Marker& M = f->markerData.markers[row];
				of->hexViewerState.GoToPos(M.at);
			}
		};
		tv.HandleEvent(&tv, ui::EventType::ContextMenu) = [this, f, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX)
			{
				Marker& M = f->markerData.markers[row];

				auto& CM = ui::ContextMenu::Get();
				CM.Add("Go to start", false, false, 0) = [this, &M]() { of->hexViewerState.GoToPos(M.at); };
				CM.Add("Go to end", false, false, 1) = [this, &M]() { of->hexViewerState.GoToPos(M.GetEnd()); };
			}
		};
		tv.HandleEvent(&tv, ui::EventType::SelectionChange) = [](ui::Event& e) { e.current->Rebuild(); };
		tv.HandleEvent(&tv, ui::EventType::KeyAction) = [&tv, f](ui::Event& e)
		{
			if (e.GetKeyAction() == ui::KeyAction::Delete)
			{
				if (f->mdSrc.selected < f->markerData.markers.size())
				{
					f->markerData.markers.erase(f->markerData.markers.begin() + f->mdSrc.selected);
					f->mdSrc.selected = SIZE_MAX;
					e.current->Rebuild();
				}
			}
		};

		ctx->Pop();

		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());
		if (f->mdSrc.selected < f->markerData.markers.size())
		{
			auto& MIE = ctx->Make<MarkedItemEditor>();
			MIE.dataSource = f->dataSource;
			MIE.marker = &f->markerData.markers[f->mdSrc.selected];
		}
		ctx->Pop();
	}
	ctx->Pop();
	spmkr.SetSplits({ 0.6f });
}
