
#include "pch.h"
#include "TabHighlights.h"

#include "Workspace.h"


void TabHighlights::Render(UIContainer* ctx)
{
	auto& spmkr = *ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox();

		ctx->Text("Highlighted items") + ui::Padding(5);

		ctx->Pop();

		ctx->PushBox();
		of->highlightSettings.EditUI(ctx);
		ctx->Pop();
	}
	ctx->Pop();
	spmkr.SetSplits({ 0.6f });
}
