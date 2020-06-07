
#include "pch.h"
#include "TabStructures.h"

#include "Workspace.h"


void TabStructures::Render(UIContainer* ctx)
{
	auto& spstr = *ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());

		workspace->ddiSrc.Edit(ctx);

		ui::Property::Begin(ctx);
		ctx->Text("Instances") + ui::Padding(5);
		if (ui::imm::Button(ctx, "Expand all instances"))
		{
			workspace->desc.ExpandAllInstances(workspace->ddiSrc.filterFile);
		}
		if (ui::imm::Button(ctx, "Delete auto-created"))
		{
			workspace->desc.DeleteAllInstances(workspace->ddiSrc.filterFile, workspace->ddiSrc.filterStruct);
		}
		ui::Property::End(ctx);

		auto* tv = ctx->Make<ui::TableView>();
		curTable = tv;
		*tv + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
		tv->SetDataSource(&workspace->ddiSrc);
		workspace->ddiSrc.refilter = true;
		tv->CalculateColumnWidths();
		tv->HandleEvent(UIEventType::SelectionChange) = [this, tv](UIEvent& e)
		{
			auto sel = tv->selection.GetFirstSelection();
			if (tv->IsValidRow(sel))
				workspace->desc.curInst = workspace->ddiSrc._indices[sel];
			e.current->RerenderNode();
		};
		tv->HandleEvent(UIEventType::Click) = [this, tv](UIEvent& e)
		{
			size_t row = tv->GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == UIMouseButton::Left && e.numRepeats == 2)
			{
				auto idx = workspace->ddiSrc._indices[row];
				auto& SI = workspace->desc.instances[idx];
				// find tab showing this SI
				OpenedFile* ofile = nullptr;
				int ofid = -1;
				for (auto* of : workspace->openedFiles)
				{
					ofid++;
					if (of->ddFile != SI.file)
						continue;
					ofile = of;
					break;
				}
				// TODO open a new tab if not opened already
				if (ofile)
				{
					workspace->curOpenedFile = ofid;
					ofile->hexViewerState.basePos = SI.off;
					tv->RerenderNode();
				}
			}
		};
		tv->HandleEvent(UIEventType::KeyAction) = [this, tv](UIEvent& e)
		{
			if (e.GetKeyAction() == UIKeyAction::Delete && tv->selection.AnySelected())
			{
				size_t pos = tv->selection.GetFirstSelection();
				if (tv->IsValidRow(pos))
				{
					if (pos == workspace->desc.curInst)
						workspace->desc.curInst = 0;
					workspace->desc.instances.erase(workspace->desc.instances.begin() + pos);
					workspace->ddiSrc.refilter = true;
					e.current->RerenderNode();
				}
			}
		};

		ctx->Pop();
	}
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());
		workspace->desc.EditStructuralItems(ctx);
		ctx->Pop();
	}
	ctx->Pop();
	spstr.SetSplits({ 0.6f });
}