
#include "pch.h"
#include "TabHighlights.h"

#include "Workspace.h"


static float hsplitHighlightsTab1[1] = { 0.6f };

void TabHighlights::Build()
{
	ui::Push<ui::SplitPane>().Init(ui::Direction::Horizontal, hsplitHighlightsTab1);
	{
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::MakeWithText<ui::LabelFrame>("Highlighted items");

		ui::Pop();

		ui::Push<ui::StackTopDownLayoutElement>();
		of->highlightSettings.EditUI();
		ui::Pop();
	}
	ui::Pop();
}
