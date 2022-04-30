
#include "pch.h"
#include "ImageEditor.h"


static float hsplitImgEd1[1] = { 0.6f };
static float vsplitImgEd2[1] = { 0.6f };

void ImageEditorWindowNode::Build()
{
	ui::Push<ui::SplitPane>().Init(ui::Direction::Horizontal, hsplitImgEd1);
	{
		ui::Push<ui::SplitPane>().Init(ui::Direction::Vertical, vsplitImgEd2);
		{
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			if (ddiSrc.dataDesc && ddiSrc.dataDesc->curInst)
			{
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::Checkerboard);
				auto& img = ui::Make<ui::ImageElement>();
				img.SetLayoutMode(ui::ImageLayoutMode::Fill);
				img.SetImage(cachedImg.GetImage(ddiSrc.dataDesc->GetInstanceImage(*ddiSrc.dataDesc->curInst)));
				img.SetScaleMode(ui::ScaleMode::Fit);
				ui::Pop();
			}
			ui::Pop();

			ui::Push<ui::StackTopDownLayoutElement>();
			if (structDef)
			{
				EditImageFormat("Format", image->format);
				ui::MakeWithText<ui::LabelFrame>("Conditional format overrides");
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
				ui::Push<ui::StackTopDownLayoutElement>();
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
				ui::Pop();
				ui::imm::PropEditString("Image offset", image->imgOff.expr.c_str(), [this](const char* v) { image->imgOff.SetExpr(v); });
				ui::imm::PropEditString("Palette offset", image->palOff.expr.c_str(), [this](const char* v) { image->palOff.SetExpr(v); });
				ui::imm::PropEditString("Width", image->width.expr.c_str(), [this](const char* v) { image->width.SetExpr(v); });
				ui::imm::PropEditString("Height", image->height.expr.c_str(), [this](const char* v) { image->height.SetExpr(v); });
			}
			ui::Pop();
		}
		ui::Pop();

		ui::Push<ui::FillerElement>();
		if (ddiSrc.dataDesc)
		{
			auto& tv = ui::Make<ui::TableView>();
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
}
