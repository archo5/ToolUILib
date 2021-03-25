
#include "pch.h"
#include "TabHighlights.h"

#include "Workspace.h"


void TabHighlights::Build()
{
	auto& spmkr = ui::Push<ui::SplitPane>();
	{
		ui::PushBox();

		ui::Text("Highlighted items") + ui::SetPadding(5);

		ui::Pop();

		ui::PushBox();
		of->highlightSettings.EditUI();
		ui::Pop();
	}
	ui::Pop();
	spmkr.SetSplits({ 0.6f });
}
