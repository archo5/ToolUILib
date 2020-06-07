
#include "pch.h"
#include "TabImages.h"

#include "Workspace.h"


void TabImages::Render(UIContainer* ctx)
{
	auto& spstr = *ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox();

		workspace->ddimgSrc.Edit(ctx);

		auto* tv = ctx->Make<ui::TableView>();
		*tv + ui::Layout(style::layouts::EdgeSlice());
		tv->SetDataSource(&workspace->ddimgSrc);
		workspace->ddimgSrc.refilter = true;
		tv->CalculateColumnWidths();
		tv->HandleEvent(UIEventType::SelectionChange) = [this, tv](UIEvent& e)
		{
			auto sel = tv->selection.GetFirstSelection();
			if (tv->IsValidRow(sel))
				workspace->desc.curImage = workspace->ddimgSrc._indices[sel];
			e.current->RerenderNode();
		};
		tv->HandleEvent(tv, UIEventType::Click) = [this, tv](UIEvent& e)
		{
			size_t row = tv->GetHoverRow();
			if (row != SIZE_MAX && e.GetButton() == UIMouseButton::Left && e.numRepeats == 2)
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
					tv->RerenderNode();
				}
			}
			if (row != SIZE_MAX && e.GetButton() == UIMouseButton::Right)
			{
				auto idx = workspace->ddimgSrc._indices[row];
				auto& IMG = workspace->desc.images[idx];
				ui::MenuItem items[] =
				{
					ui::MenuItem("Delete").Func([this, idx, &e]() { workspace->desc.DeleteImage(idx); e.current->RerenderNode(); }),
					ui::MenuItem("Duplicate").Func([this, idx, &e]() { workspace->desc.curImage = workspace->desc.DuplicateImage(idx); e.current->RerenderNode(); }),
				};
				ui::Menu menu(items);
				menu.Show(e.current);
				e.handled = true;
			}
		};
		tv->HandleEvent(UIEventType::KeyAction) = [this, tv](UIEvent& e)
		{
			if (e.GetKeyAction() == UIKeyAction::Delete && tv->selection.AnySelected())
			{
				size_t pos = tv->selection.GetFirstSelection();
				if (tv->IsValidRow(pos))
				{
					if (pos == workspace->desc.curImage)
						workspace->desc.curImage = 0;
					workspace->desc.images.erase(workspace->desc.images.begin() + pos);
					workspace->ddimgSrc.refilter = true;
					e.current->RerenderNode();
				}
			}
		};

		ctx->Pop();
	}
	{
		ctx->PushBox();
		if (workspace->desc.curImage < workspace->desc.images.size())
		{
			ctx->Push<ui::Panel>();
			auto* img = ctx->Make<ui::ImageElement>();
			*img + ui::Width(style::Coord::Percent(100));
			*img + ui::Height(200);
			img->GetStyle().SetPaintFunc([](const style::PaintInfo& info)
			{
				auto bgr = ui::Theme::current->GetImage(ui::ThemeImage::CheckerboardBackground);

				GL::BatchRenderer br;
				auto r = info.rect;

				GL::SetTexture(bgr->_texture);
				br.Begin();
				br.SetColor(1, 1, 1, 1);
				br.Quad(r.x0, r.y0, r.x1, r.y1, 0, 0, r.GetWidth() / bgr->GetWidth(), r.GetHeight() / bgr->GetHeight());
				br.End();
			});
			img->SetImage(workspace->cachedImg.GetImage(workspace->desc.images[workspace->desc.curImage]));
			img->SetScaleMode(ui::ScaleMode::Fit);
			ctx->Pop();
		}
		workspace->desc.EditImageItems(ctx);
		ctx->Pop();
	}
	ctx->Pop();
	spstr.SetSplits({ 0.5f });
}
