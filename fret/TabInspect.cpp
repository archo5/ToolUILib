
#include "pch.h"
#include "TabInspect.h"

#include "HexViewer.h"
#include "Workspace.h"


void TabInspect::Render(UIContainer* ctx)
{
	Subscribe(DCT_HexViewerState, &of->hexViewerState);

	auto& spmkr = *ctx->Push<ui::SplitPane>();
	{
		ctx->PushBox();

		auto pos = of->hexViewerState.hoverByte;
		if (of->hexViewerState.selectionStart != UINT64_MAX)
			pos = std::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);

		auto* ds = of->ddFile->dataSource;

		char txt_ascii[32];
		ds->GetASCIIText(txt_ascii, 32, pos);

		char txt_int8[32];
		ds->GetInt8Text(txt_int8, 32, pos, true);
		char txt_uint8[32];
		ds->GetInt8Text(txt_uint8, 32, pos, false);
		char txt_int16[32];
		ds->GetInt16Text(txt_int16, 32, pos, true);
		char txt_uint16[32];
		ds->GetInt16Text(txt_uint16, 32, pos, false);
		char txt_int32[32];
		ds->GetInt32Text(txt_int32, 32, pos, true);
		char txt_uint32[32];
		ds->GetInt32Text(txt_uint32, 32, pos, false);
		char txt_int64[32];
		ds->GetInt64Text(txt_int64, 32, pos, true);
		char txt_uint64[32];
		ds->GetInt64Text(txt_uint64, 32, pos, false);
		char txt_float32[32];
		ds->GetFloat32Text(txt_float32, 32, pos);
		char txt_float64[32];
		ds->GetFloat64Text(txt_float64, 32, pos);

		ui::imm::PropText(ctx, "i8", txt_int8);
		ui::imm::PropText(ctx, "u8", txt_uint8);
		ui::imm::PropText(ctx, "i16", txt_int16);
		ui::imm::PropText(ctx, "u16", txt_uint16);
		ui::imm::PropText(ctx, "i32", txt_int32);
		ui::imm::PropText(ctx, "u32", txt_uint32);
		ui::imm::PropText(ctx, "i64", txt_int64);
		ui::imm::PropText(ctx, "u64", txt_uint64);
		ui::imm::PropText(ctx, "f32", txt_float32);
		ui::imm::PropText(ctx, "f64", txt_float64);
		ui::imm::PropText(ctx, "ASCII", txt_ascii);

		ctx->Pop();

		ctx->PushBox();
		ctx->Text("Settings") + ui::Padding(5);
		ctx->Pop();
	}
	ctx->Pop();
	spmkr.SetSplits({ 0.6f });
}
