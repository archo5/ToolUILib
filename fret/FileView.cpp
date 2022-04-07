
#include "pch.h"
#include "FileView.h"

#include "HexViewer.h"
#include "FileReaders.h"
#include "DataDesc.h"
#include "ImageParsers.h"
#include "Workspace.h"


static bool viewSettingsOpen = false;
void FileView::Build()
{
	ui::Push<ui::EdgeSliceLayoutElement>();

	ui::Push<ui::StackTopDownLayoutElement>();
	{
		char buf[256];
		if (of->hexViewerState.selectionStart != UINT64_MAX)
		{
			int64_t selMin = ui::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
			int64_t selMax = ui::max(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
			int64_t len = selMax - selMin + 1;
			int64_t ciOff = 0, ciSize = 0;
			if (auto* SI = workspace->desc.curInst)
			{
				ciOff = SI->off;
				ciSize = SI->def->size;
			}
			snprintf(buf, 256,
				"Selection: %" PRIu64 " - %" PRIu64 " (%" PRIu64 ") rel: %" PRId64 " fit: %" PRId64 " rem: %" PRId64,
				selMin, selMax, len,
				selMin - ciOff,
				ciSize ? len / ciSize : 0,
				ciSize ? len % ciSize : 0);
		}
		else
			snprintf(buf, 256, "Selection: <none>");
		ui::Text(buf);
	}
	ui::Pop();

	ui::imm::EditBool(viewSettingsOpen, "View settings", {}, ui::imm::TreeStateToggleSkin());

	ui::Push<ui::StackTopDownLayoutElement>(); // tree stabilization box
	if (viewSettingsOpen)
	{
		ui::imm::PropEditInt("Width", of->hexViewerState.byteWidth, {}, {}, { 1, 256 });
		ui::imm::PropEditInt("Position", of->hexViewerState.basePos);
	}
	ui::Pop(); // end tree stabilization box

	auto& hv = ui::Make<HexViewer>();
	curHexViewer = &hv;
	hv.Init(&workspace->desc, of->ddFile, &of->hexViewerState, &of->highlightSettings);
	hv.HandleEvent(ui::EventType::ButtonUp) = [this](ui::Event& e)
	{
		if (e.GetButton() == ui::MouseButton::Right)
		{
			HexViewer_OnRightClick();
		}
	};
	ui::Pop();
}

void FileView::HexViewer_OnRightClick()
{
	auto* ds = of->ddFile->dataSource;
	int64_t pos = of->hexViewerState.hoverByte;
	auto selMin = ui::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
	auto selMax = ui::max(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
	if (selMin != UINT64_MAX && selMax != UINT64_MAX && selMin <= pos && pos <= selMax)
		pos = selMin;

	char txt_pos[64];
	snprintf(txt_pos, 64, "@ %" PRIu64 " (0x%" PRIX64 ")", pos, pos);

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

	int32_t val_int32;
	ds->Read(pos, sizeof(val_int32), &val_int32);

	std::vector<ui::MenuItem> structs;
	{
		structs.push_back(ui::MenuItem("Create a new struct (blank)").Func([this, pos]() { CreateBlankStruct(pos); }));
		structs.push_back(ui::MenuItem("Create a new struct (from markers)", {},
			of->hexViewerState.selectionStart == UINT64_MAX || of->hexViewerState.selectionEnd == UINT64_MAX)
			.Func([this, pos]() { CreateStructFromMarkers(pos); }));
		if (workspace->desc.structs.size())
			structs.push_back(ui::MenuItem::Separator());
	}
	for (auto& s : workspace->desc.structs)
	{
		auto fn = [this, pos, s]()
		{
			workspace->desc.SetCurrentInstance(workspace->desc.AddInstance({ -1LL, &workspace->desc, s.second, of->ddFile, pos, "", CreationReason::UserDefined }));
		};
		structs.push_back(ui::MenuItem(s.first).Func(fn));
	}

	std::vector<ui::MenuItem> images;
	ui::StringView prevCat;
	for (size_t i = 0, count = GetImageFormatCount(); i < count; i++)
	{
		ui::StringView cat = GetImageFormatCategory(i);
		if (cat != prevCat)
		{
			prevCat = cat;
			images.push_back(ui::MenuItem(cat, {}, true));
		}
		ui::StringView name = GetImageFormatName(i);
		images.push_back(ui::MenuItem(name).Func([this, pos, name]() { CreateImage(pos, name); }));
	}

	auto& md = of->ddFile->markerData;
	ui::MenuItem items[] =
	{
		ui::MenuItem(txt_pos, {}, true),
		ui::MenuItem::Submenu("Place struct", structs),
		ui::MenuItem::Submenu("Place image", images),
		ui::MenuItem::Separator(),
		ui::MenuItem("Go to offset (u32)", txt_uint32).Func([this, pos]() { GoToOffset(pos); }),
		ui::MenuItem::Separator(),
		ui::MenuItem("Mark ASCII", txt_ascii).Func([&md, pos]() { md.AddMarker(DT_CHAR, pos, pos + 1); }),
		ui::MenuItem("Mark int8", txt_int8).Func([&md, pos]() { md.AddMarker(DT_I8, pos, pos + 1); }),
		ui::MenuItem("Mark uint8", txt_uint8).Func([&md, pos]() { md.AddMarker(DT_U8, pos, pos + 1); }),
		ui::MenuItem("Mark int16", txt_int16).Func([&md, pos]() { md.AddMarker(DT_I16, pos, pos + 2); }),
		ui::MenuItem("Mark uint16", txt_uint16).Func([&md, pos]() { md.AddMarker(DT_U16, pos, pos + 2); }),
		ui::MenuItem("Mark int32", txt_int32).Func([&md, pos]() { md.AddMarker(DT_I32, pos, pos + 4); }),
		ui::MenuItem("Mark uint32", txt_uint32).Func([&md, pos]() { md.AddMarker(DT_U32, pos, pos + 4); }),
		ui::MenuItem("Mark int64", txt_int64).Func([&md, pos]() { md.AddMarker(DT_I64, pos, pos + 8); }),
		ui::MenuItem("Mark uint64", txt_uint64).Func([&md, pos]() { md.AddMarker(DT_U64, pos, pos + 8); }),
		ui::MenuItem("Mark float32", txt_float32).Func([&md, pos]() { md.AddMarker(DT_F32, pos, pos + 4); }),
		ui::MenuItem("Mark float64", txt_float64).Func([&md, pos]() { md.AddMarker(DT_F64, pos, pos + 8); }),
		ui::MenuItem::Separator(),
		ui::MenuItem("Highlight all int32", txt_int32).Func([this, val_int32]() { of->highlightSettings.AddCustomInt32(val_int32); }),
	};
	ui::Menu menu(items);
	menu.Show(this);
	Rebuild();
}

DDStruct* FileView::CreateBlankStruct(int64_t pos)
{
	auto* ns = new DDStruct;
	do
	{
		ns->name = "struct" + std::to_string(rand() % 10000);
	}
	while (workspace->desc.structs.count(ns->name));
	int64_t off = pos;
	if (of->hexViewerState.selectionStart != UINT64_MAX && of->hexViewerState.selectionEnd != UINT64_MAX)
	{
		off = ui::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
		ns->size = abs(int(of->hexViewerState.selectionEnd - of->hexViewerState.selectionStart)) + 1;
	}
	workspace->desc.structs[ns->name] = ns;
	workspace->desc.SetCurrentInstance(workspace->desc.AddInstance({ -1LL, &workspace->desc, ns, of->ddFile, off, "", CreationReason::UserDefined }));
	return ns;
}

DDStruct* FileView::CreateStructFromMarkers(int64_t pos)
{
	auto* ns = CreateBlankStruct(pos);
	auto selMin = ui::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
	auto selMax = ui::max(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
	int at = 0;
	for (Marker& M : of->ddFile->markerData.markers)
	{
		if (M.at < selMin || M.at > selMax)
			continue;
		for (DDField f;
			f.type = GetDataTypeName(M.type),
			f.name = f.type + "_" + std::to_string(at++),
			f.off = M.at - selMin,
			f.count = M.count,
			ns->fields.push_back(f),
			false;);
	}
	return ns;
}

void FileView::CreateImage(int64_t pos, ui::StringView fmt)
{
	DataDesc::Image img;
	img.userCreated = true;
	img.width = 4;
	img.height = 4;
	img.offImage = pos;
	img.format.assign(fmt.data(), fmt.size());
	img.file = of->ddFile;
	workspace->desc.curImage = workspace->desc.images.size();
	workspace->desc.images.push_back(img);
}

void FileView::GoToOffset(int64_t pos)
{
	uint32_t off = 0;
	of->ddFile->dataSource->Read(pos, sizeof(off), &off);

	of->hexViewerState.GoToPos(off);
}
