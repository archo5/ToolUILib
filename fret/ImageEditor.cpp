
#include "pch.h"
#include "ImageEditor.h"


void ImageEditorWindowNode::Build()
{
	auto& sp1 = ui::Push<ui::SplitPane>();
	{
		auto& sp2 = ui::Push<ui::SplitPane>();
		{
			ui::Push<ui::Panel>();
			if (ddiSrc.dataDesc && ddiSrc.dataDesc->curInst)
			{
				auto& img = ui::Make<ui::ImageElement>();
				img + ui::SetWidth(ui::Coord::Percent(100));
				img + ui::SetHeight(ui::Coord::Percent(100));
				img.GetStyle().SetBackgroundPainter(ui::CheckerboardPainter::Get());
				img.SetImage(cachedImg.GetImage(ddiSrc.dataDesc->GetInstanceImage(*ddiSrc.dataDesc->curInst)));
				img.SetScaleMode(ui::ScaleMode::Fit);
			}
			ui::Pop();

			ui::PushBox();
			if (structDef)
			{
				EditImageFormat("Format", image->format);
				ui::Text("Conditional format overrides") + ui::SetPadding(5);
				ui::Push<ui::Panel>();
				for (auto& FO : image->formatOverrides)
				{
					EditImageFormat("Format", FO.format);
					ui::imm::PropEditString("Condition", FO.condition.expr.c_str(), [&FO](const char* v) { FO.condition.SetExpr(v); });
				}
				if (ui::imm::Button("Add"))
				{
					image->formatOverrides.push_back({});
				}
				ui::Pop();
				ui::imm::PropEditString("Image offset", image->imgOff.expr.c_str(), [this](const char* v) { image->imgOff.SetExpr(v); });
				ui::imm::PropEditString("Palette offset", image->palOff.expr.c_str(), [this](const char* v) { image->palOff.SetExpr(v); });
				ui::imm::PropEditString("Width", image->width.expr.c_str(), [this](const char* v) { image->width.SetExpr(v); });
				ui::imm::PropEditString("Height", image->height.expr.c_str(), [this](const char* v) { image->height.SetExpr(v); });
			}
			ui::Pop();
		}
		ui::Pop();
		sp2.SetDirection(true);
		sp2.SetSplits({ 0.6f });

		ui::PushBox();
		if (ddiSrc.dataDesc)
		{
			auto& tv = ui::Make<ui::TableView>();
			tv + ui::SetLayout(ui::layouts::EdgeSlice()) + ui::SetHeight(ui::Coord::Percent(100));
			tv.SetDataSource(&ddiSrc);
			tv.SetSelectionStorage(&ddiSrc);
			tv.SetSelectionMode(ui::SelectionMode::Single);
			ddiSrc.refilter = true;
			tv.CalculateColumnWidths();
			tv.HandleEvent(ui::EventType::SelectionChange) = [this, &tv](ui::Event& e) { e.current->Rebuild(); };
		}
		ui::Pop();
	}
	ui::Pop();
	sp1.SetDirection(false);
	sp1.SetSplits({ 0.6f });
}
