
#include "pch.h"
#include "TabMarkers.h"

#include "Workspace.h"


void TabMarkers::Render(UIContainer* ctx)
{
	auto* f = of->ddFile;
	auto& spmkr = *ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());

		ctx->Text("Marked items") + ui::Padding(5);
		auto* tv = ctx->Make<ui::TableView>();
		curTable = tv;
		*tv + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
		tv->SetDataSource(&f->mdSrc);
		tv->SetSelectionStorage(&f->mdSrc);
		tv->SetSelectionMode(ui::SelectionMode::Single);
		tv->CalculateColumnWidths();
		tv->HandleEvent(tv, UIEventType::Click) = [this, f, tv](UIEvent& e)
		{
			size_t row = tv->GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == UIMouseButton::Left && e.numRepeats == 2)
			{
				Marker& M = f->markerData.markers[row];
				of->hexViewerState.GoToPos(M.at);
			}
		};
		tv->HandleEvent(tv, UIEventType::Click) = [this, f, tv](UIEvent& e)
		{
			size_t row = tv->GetHoverRow();
			if (e.GetButton() == UIMouseButton::Right && row != SIZE_MAX)
			{
				Marker& M = f->markerData.markers[row];

				ui::MenuItem items[] =
				{
					ui::MenuItem("Go to start").Func([this, &M]() { of->hexViewerState.GoToPos(M.at); }),
					ui::MenuItem("Go to end").Func([this, &M]() { of->hexViewerState.GoToPos(M.GetEnd()); }),
				};
				ui::Menu(items).Show(this);
			}
		};
		tv->HandleEvent(tv, UIEventType::SelectionChange) = [](UIEvent& e) { e.current->RerenderNode(); };
		tv->HandleEvent(tv, UIEventType::KeyAction) = [tv, f](UIEvent& e)
		{
			if (e.GetKeyAction() == UIKeyAction::Delete)
			{
				if (f->mdSrc.selected < f->markerData.markers.size())
				{
					f->markerData.markers.erase(f->markerData.markers.begin() + f->mdSrc.selected);
					f->mdSrc.selected = SIZE_MAX;
					e.current->RerenderNode();
				}
			}
		};

		ctx->Pop();

		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());
		if (f->mdSrc.selected < f->markerData.markers.size())
		{
			auto* MIE = ctx->Make<MarkedItemEditor>();
			MIE->dataSource = f->dataSource;
			MIE->marker = &f->markerData.markers[f->mdSrc.selected];
		}
		ctx->Pop();
	}
	ctx->Pop();
	spmkr.SetSplits({ 0.6f });
}
