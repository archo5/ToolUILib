
#include "pch.h"
#include "TabImages.h"

#include "Workspace.h"


void TabImages::Build(ui::UIContainer* ctx)
{
	auto& spstr = ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox();

		workspace->ddimgSrc.Edit(ctx);

		auto& tv = ctx->Make<ui::TableView>();
		tv + ui::SetLayout(ui::layouts::EdgeSlice());
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

		ctx->Pop();
	}
	{
		auto& spvert = ctx->Push<ui::SplitPane>();
		{
			if (workspace->desc.curImage < workspace->desc.images.size())
			{
				ctx->Push<ui::Panel>();
				auto& img = ctx->Make<ui::ImageElement>();
				img + ui::SetWidth(ui::Coord::Percent(100));
				img + ui::SetHeight(ui::Coord::Percent(100));
				img.GetStyle().SetPaintFunc([](const ui::PaintInfo& info)
				{
					auto bgr = ui::Theme::current->GetImage(ui::ThemeImage::CheckerboardBackground);

					auto r = info.rect;

					ui::draw::RectTex(r.x0, r.y0, r.x1, r.y1, bgr->_texture, 0, 0, r.GetWidth() / bgr->GetWidth(), r.GetHeight() / bgr->GetHeight());
				});
				img.SetImage(workspace->cachedImg.GetImage(workspace->desc.images[workspace->desc.curImage]));
				img.SetScaleMode(ui::ScaleMode::Fit);
				ctx->Pop();
			}
			ctx->PushBox();
			workspace->desc.EditImageItems(ctx);
			ctx->Pop();
		}
		ctx->Pop();
		spvert.SetDirection(true);
		spvert.SetSplits({ 0.5f });
	}
	ctx->Pop();
	spstr.SetSplits({ 0.5f });
}
