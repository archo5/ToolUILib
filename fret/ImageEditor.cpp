
#include "pch.h"
#include "ImageEditor.h"


void ImageEditorWindowNode::Build(ui::UIContainer* ctx)
{
	auto& sp1 = ctx->Push<ui::SplitPane>();
	{
		auto& sp2 = ctx->Push<ui::SplitPane>();
		{
			ctx->Push<ui::Panel>();
			if (ddiSrc.dataDesc && ddiSrc.dataDesc->curInst)
			{
				auto& img = ctx->Make<ui::ImageElement>();
				img + ui::SetWidth(ui::Coord::Percent(100));
				img + ui::SetHeight(ui::Coord::Percent(100));
				img.GetStyle().SetPaintFunc([](const ui::PaintInfo& info)
				{
					auto bgr = ui::Theme::current->GetImage(ui::ThemeImage::CheckerboardBackground);

					auto r = info.rect;

					ui::draw::RectTex(r.x0, r.y0, r.x1, r.y1, bgr->_texture, 0, 0, r.GetWidth() / bgr->GetWidth(), r.GetHeight() / bgr->GetHeight());
				});
				img.SetImage(cachedImg.GetImage(ddiSrc.dataDesc->GetInstanceImage(*ddiSrc.dataDesc->curInst)));
				img.SetScaleMode(ui::ScaleMode::Fit);
			}
			ctx->Pop();

			ctx->PushBox();
			if (structDef)
			{
				EditImageFormat(ctx, "Format", image->format);
				ctx->Text("Conditional format overrides") + ui::SetPadding(5);
				ctx->Push<ui::Panel>();
				for (auto& FO : image->formatOverrides)
				{
					EditImageFormat(ctx, "Format", FO.format);
					ui::imm::PropEditString(ctx, "Condition", FO.condition.expr.c_str(), [&FO](const char* v) { FO.condition.SetExpr(v); });
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					image->formatOverrides.push_back({});
				}
				ctx->Pop();
				ui::imm::PropEditString(ctx, "Image offset", image->imgOff.expr.c_str(), [this](const char* v) { image->imgOff.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Palette offset", image->palOff.expr.c_str(), [this](const char* v) { image->palOff.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Width", image->width.expr.c_str(), [this](const char* v) { image->width.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Height", image->height.expr.c_str(), [this](const char* v) { image->height.SetExpr(v); });
			}
			ctx->Pop();
		}
		ctx->Pop();
		sp2.SetDirection(true);
		sp2.SetSplits({ 0.6f });

		ctx->PushBox();
		if (ddiSrc.dataDesc)
		{
			auto& tv = ctx->Make<ui::TableView>();
			tv + ui::SetLayout(ui::layouts::EdgeSlice()) + ui::SetHeight(ui::Coord::Percent(100));
			tv.SetDataSource(&ddiSrc);
			tv.SetSelectionStorage(&ddiSrc);
			tv.SetSelectionMode(ui::SelectionMode::Single);
			ddiSrc.refilter = true;
			tv.CalculateColumnWidths();
			tv.HandleEvent(ui::EventType::SelectionChange) = [this, &tv](ui::Event& e) { e.current->Rebuild(); };
		}
		ctx->Pop();
	}
	ctx->Pop();
	sp1.SetDirection(false);
	sp1.SetSplits({ 0.6f });
}
