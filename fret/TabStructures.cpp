
#include "pch.h"
#include "TabStructures.h"

#include "Workspace.h"


void TabStructures::Build(ui::UIContainer* ctx)
{
	if (workspace->ddiSrc.filterFileFollow && workspace->curOpenedFile < (int)workspace->openedFiles.size())
		workspace->ddiSrc.filterFile = workspace->openedFiles[workspace->curOpenedFile]->ddFile;

	auto& spstr = ctx->Push<ui::SplitPane>();
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

		auto& tv = ctx->Make<ui::TableView>();
		curTable = &tv;
		tv + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
		tv.SetDataSource(&workspace->ddiSrc);
		tv.SetSelectionStorage(&workspace->ddiSrc);
		tv.SetSelectionMode(ui::SelectionMode::Single);
		workspace->ddiSrc.refilter = true;
		tv.CalculateColumnWidths();
		tv.HandleEvent(ui::EventType::SelectionChange) = [this, &tv](ui::Event& e) { e.current->Rebuild(); };
		tv.HandleEvent(ui::EventType::Click) = [this, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == ui::MouseButton::Left && e.numRepeats == 2)
			{
				auto idx = workspace->ddiSrc._indices[row];
				auto* SI = workspace->desc.instances[idx];
				// find tab showing this SI
				OpenedFile* ofile = nullptr;
				int ofid = -1;
				for (auto* of : workspace->openedFiles)
				{
					ofid++;
					if (of->ddFile != SI->file)
						continue;
					ofile = of;
					break;
				}
				// TODO open a new tab if not opened already
				if (ofile)
				{
					workspace->curOpenedFile = ofid;
					ofile->hexViewerState.basePos = SI->off;
					tv.Rebuild();
				}
			}
		};
		tv.HandleEvent(ui::EventType::KeyAction) = [this, &tv](ui::Event& e)
		{
			if (e.GetKeyAction() == ui::KeyAction::Delete)
			{
				if (workspace->desc.curInst)
				{
					workspace->desc.DeleteInstance(workspace->desc.curInst);
					workspace->ddiSrc.refilter = true;
					e.current->Rebuild();
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
