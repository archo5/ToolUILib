
#include "pch.h"
#include "TabImages.h"

#include "Workspace.h"


static float hsplitImgTab1[1] = { 0.5f };
static float vsplitImgTab2[1] = { 0.5f };

void TabImages::Build()
{
	ui::Push<ui::SplitPane>().Init(ui::Direction::Horizontal, hsplitImgTab1);
	{
		ui::Push<ui::EdgeSliceLayoutElement>();

		workspace->ddimgSrc.Edit();

		auto& tv = ui::Make<ui::TableView>();
		tv.SetDataSource(&workspace->ddimgSrc);
		tv.SetSelectionStorage(&workspace->ddimgSrc);
		tv.SetSelectionMode(ui::SelectionMode::Single);
		workspace->ddimgSrc.refilter = true;
		tv.CalculateColumnWidths();
		tv.HandleEvent(ui::EventType::SelectionChange) = [this, &tv](ui::Event& e) { e.current->Rebuild(); };
		tv.HandleEvent(&tv, ui::EventType::Click) = [this, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == ui::MouseButton::Left && e.numRepeats == 2)
			{
				auto idx = workspace->ddimgSrc._indices[row];
				auto& IMG = workspace->desc.images[idx];
				// find tab showing this image
				OpenedFile* ofile = nullptr;
				int ofid = -1;
				for (auto* of : workspace->openedFiles)
				{
					ofid++;
					if (of->ddFile != IMG.file)
						continue;
					ofile = of;
					break;
				}
				// TODO open a new tab if not opened already
				if (ofile)
				{
					workspace->curOpenedFile = ofid;
					ofile->hexViewerState.basePos = IMG.offImage;
					tv.Rebuild();
				}
			}
		};
		tv.HandleEvent(&tv, ui::EventType::ButtonUp) = [this, &tv](ui::Event& e)
		{
			size_t row = tv.GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == ui::MouseButton::Right)
			{
				auto idx = workspace->ddimgSrc._indices[row];
				auto& IMG = workspace->desc.images[idx];
				ui::MenuItem items[] =
				{
					ui::MenuItem("Delete").Func([this, idx, &e]() { workspace->desc.DeleteImage(idx); e.current->Rebuild(); }),
					ui::MenuItem("Duplicate").Func([this, idx, &e]() { workspace->desc.curImage = workspace->desc.DuplicateImage(idx); e.current->Rebuild(); }),
				};
				ui::Menu menu(items);
				menu.Show(e.current);
				e.StopPropagation();
			}
		};
		tv.HandleEvent(ui::EventType::KeyAction) = [this, &tv](ui::Event& e)
		{
			if (e.GetKeyAction() == ui::KeyAction::Delete)
			{
				if (workspace->desc.curImage < workspace->desc.images.size())
				{
					workspace->desc.images.erase(workspace->desc.images.begin() + workspace->desc.curImage);
					workspace->desc.curImage = UINT32_MAX;
					workspace->ddimgSrc.refilter = true;
					e.current->Rebuild();
				}
			}
		};

		ui::Pop();
	}
	{
		ui::Push<ui::SplitPane>().Init(ui::Direction::Vertical, vsplitImgTab2);
		{
			if (workspace->desc.curImage < workspace->desc.images.size())
			{
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::Checkerboard);
				auto& img = ui::Make<ui::ImageElement>();
				img.SetLayoutMode(ui::ImageLayoutMode::Fill);
				img.SetImage(workspace->cachedImg.GetImage(workspace->desc.images[workspace->desc.curImage]));
				img.SetScaleMode(ui::ScaleMode::Fit);
				ui::Pop();
				ui::Pop();
			}
			ui::Push<ui::StackTopDownLayoutElement>();
			workspace->desc.EditImageItems();
			ui::Pop();
		}
		ui::Pop();
	}
	ui::Pop();
}
