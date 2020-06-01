
#include "pch.h"
#include "FileReaders.h"
#include "FileStructureViewer.h"
#include "DataDesc.h"
#include "HexViewer.h"
#include "ImageParsers.h"


struct OpenedFile
{
	void Load(NamedTextSerializeReader& r)
	{
		r.BeginDict("");
		fileID = r.ReadUInt64("fileID");
		hexViewerState.basePos = r.ReadUInt64("basePos");
		hexViewerState.byteWidth = r.ReadUInt("byteWidth");
		highlightSettings.Load("highlighter", r);
		r.EndDict();
	}
	void Save(NamedTextSerializeWriter& w)
	{
		w.BeginDict("");
		w.WriteInt("fileID", fileID);
		w.WriteInt("basePos", hexViewerState.basePos);
		w.WriteInt("byteWidth", hexViewerState.byteWidth);
		highlightSettings.Save("highlighter", w);
		w.EndDict();
	}

	DDFile* ddFile = nullptr;
	uint64_t fileID = 0;
	HexViewerState hexViewerState;
	HighlightSettings highlightSettings;
};

struct TabInspect : ui::Node
{
	void Render(UIContainer* ctx) override
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

	OpenedFile* of = nullptr;
};

struct TabHighlights : ui::Node
{
	void Render(UIContainer* ctx) override
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

	OpenedFile* of = nullptr;
};

struct TabMarkers : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		auto* f = of->ddFile;
		auto& spmkr = *ctx->Push<ui::SplitPane>();
		{
			ctx->PushBox();

			ctx->Text("Marked items") + ui::Padding(5);
			auto* tv = ctx->Make<ui::TableView>();
			*tv + ui::Layout(style::layouts::EdgeSlice());
			tv->SetDataSource(&f->markerData);
			tv->CalculateColumnWidths();
			tv->HandleEvent(tv, UIEventType::Click) = [this, f, tv](UIEvent& e)
			{
				size_t row = tv->GetHoverRow();
				if (row != SIZE_MAX && e.GetButton() == UIMouseButton::Left && e.numRepeats == 2)
				{
					Marker& M = f->markerData.markers[row];
					of->hexViewerState.basePos = M.at;
					tv->RerenderNode();
				}
			};
			tv->HandleEvent(tv, UIEventType::SelectionChange) = [](UIEvent& e) { e.current->RerenderNode(); };
			tv->HandleEvent(tv, UIEventType::KeyAction) = [tv, f](UIEvent& e)
			{
				if (e.GetKeyAction() == UIKeyAction::Delete && tv->selection.AnySelected())
				{
					size_t pos = tv->selection.GetFirstSelection();
					if (tv->IsValidRow(pos))
					{
						f->markerData.markers.erase(f->markerData.markers.begin() + pos);
						e.current->RerenderNode();
					}
				}
			};

			ctx->Pop();

			ctx->PushBox();
			if (tv->selection.AnySelected())
			{
				size_t pos = tv->selection.GetFirstSelection();
				if (tv->IsValidRow(pos))
				{
					auto* MIE = ctx->Make<MarkedItemEditor>();
					MIE->dataSource = f->dataSource;
					MIE->marker = &f->markerData.markers[pos];
				}
			}
			ctx->Pop();
		}
		ctx->Pop();
		spmkr.SetSplits({ 0.6f });
	}

	OpenedFile* of = nullptr;
};

enum class SubtabType
{
	Inspect = 0,
	Highlights = 1,
	Markers = 2,
	Structures = 3,
	Images = 4,
};

struct Workspace
{
	Workspace()
	{
		ddiSrc.dataDesc = &desc;
		ddimgSrc.dataDesc = &desc;
	}
	~Workspace()
	{
		Clear();
	}

	void Clear()
	{
		desc.Clear();
		for (auto* F : openedFiles)
			delete F;
		openedFiles.clear();
	}

	void Load(NamedTextSerializeReader& r)
	{
		Clear();
		r.BeginDict("workspace");

		desc.Load("desc", r);
		for (auto* F : desc.files)
		{
			std::string path = F->name;
			if (path.size() < 2 || path[1] != ':')
				path = "FRET_Plugins/" + path;
			F->dataSource = new FileDataSource(path.c_str());
		}

		r.BeginArray("openedFiles");
		for (auto E : r.GetCurrentRange())
		{
			r.BeginEntry(E);

			auto* F = new OpenedFile;
			F->Load(r);
			F->ddFile = desc.FindFileByID(F->fileID);
			openedFiles.push_back(F);

			r.EndEntry();
		}
		r.EndArray();

		r.EndDict();
	}
	void Save(NamedTextSerializeWriter& w)
	{
		w.BeginDict("workspace");
		w.WriteString("version", "1");

		desc.Save("desc", w);

		w.BeginArray("openedFiles");
		for (auto* F : openedFiles)
			F->Save(w);
		w.EndArray();

		w.EndDict();
	}
	ui::Image* GetImage(const DataDesc::Image& imgDesc)
	{
		if (curImg)
		{
			if (imgDesc.file == curImgDesc.file &&
				imgDesc.offImage == curImgDesc.offImage &&
				imgDesc.offPalette == curImgDesc.offPalette &&
				imgDesc.format == curImgDesc.format &&
				imgDesc.width == curImgDesc.width &&
				imgDesc.height == curImgDesc.height)
			{
				return curImg;
			}
		}

		delete curImg;
		curImg = CreateImageFrom(imgDesc.file->dataSource, imgDesc.format.c_str(), { imgDesc.offImage, imgDesc.offPalette, imgDesc.width, imgDesc.height });
		curImgDesc = imgDesc;
		return curImg;
	}

	std::vector<OpenedFile*> openedFiles;
	int curOpenedFile = 0;
	SubtabType curSubtab = SubtabType::Markers;
	DataDesc desc;
	DataDescInstanceSource ddiSrc;
	DataDescImageSource ddimgSrc;

	// runtime cache
	ui::Image* curImg = nullptr;
	DataDesc::Image curImgDesc;
};

struct TabStructures : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		auto& spstr = *ctx->Push<ui::SplitPane>();
		{
			ctx->PushBox();

			workspace->ddiSrc.Edit(ctx);

			ui::Property::Begin(ctx);
			ctx->Text("Instances") + ui::Padding(5);
			if (ui::imm::Button(ctx, "Expand all instances"))
			{
				workspace->desc.ExpandAllInstances(workspace->ddiSrc.filterFile);
			}
			if (ui::imm::Button(ctx, "Delete auto-created"))
			{
				workspace->desc.DeleteAllInstances(workspace->ddiSrc.filterFile, workspace->ddiSrc.filterStruct);
			}
			ui::Property::End(ctx);

			auto* tv = ctx->Make<ui::TableView>();
			*tv + ui::Layout(style::layouts::EdgeSlice());
			tv->SetDataSource(&workspace->ddiSrc);
			workspace->ddiSrc.refilter = true;
			tv->CalculateColumnWidths();
			tv->HandleEvent(UIEventType::SelectionChange) = [this, tv](UIEvent& e)
			{
				auto sel = tv->selection.GetFirstSelection();
				if (tv->IsValidRow(sel))
					workspace->desc.curInst = workspace->ddiSrc._indices[sel];
				e.current->RerenderNode();
			};
			tv->HandleEvent(UIEventType::Click) = [this, tv](UIEvent& e)
			{
				size_t row = tv->GetHoverRow();
				if (row != SIZE_MAX && e.GetButton() == UIMouseButton::Left && e.numRepeats == 2)
				{
					auto idx = workspace->ddiSrc._indices[row];
					auto& SI = workspace->desc.instances[idx];
					// find tab showing this SI
					OpenedFile* ofile = nullptr;
					int ofid = -1;
					for (auto* of : workspace->openedFiles)
					{
						ofid++;
						if (of->ddFile != SI.file)
							continue;
						ofile = of;
						break;
					}
					// TODO open a new tab if not opened already
					if (ofile)
					{
						workspace->curOpenedFile = ofid;
						ofile->hexViewerState.basePos = SI.off;
						tv->RerenderNode();
					}
				}
			};
			tv->HandleEvent(UIEventType::KeyAction) = [this, tv](UIEvent& e)
			{
				if (e.GetKeyAction() == UIKeyAction::Delete && tv->selection.AnySelected())
				{
					size_t pos = tv->selection.GetFirstSelection();
					if (tv->IsValidRow(pos))
					{
						if (pos == workspace->desc.curInst)
							workspace->desc.curInst = 0;
						workspace->desc.instances.erase(workspace->desc.instances.begin() + pos);
						workspace->ddiSrc.refilter = true;
						e.current->RerenderNode();
					}
				}
			};

			ctx->Pop();
		}
		{
			ctx->PushBox();
			workspace->desc.EditStructuralItems(ctx);
			ctx->Pop();
		}
		ctx->Pop();
		spstr.SetSplits({ 0.6f });
	}

	Workspace* workspace = nullptr;
};

struct TabImages : ui::Node
{
	void Render(UIContainer* ctx) override
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
				img->SetImage(workspace->GetImage(workspace->desc.images[workspace->desc.curImage]));
				img->SetScaleMode(ui::ScaleMode::Fit);
				ctx->Pop();
			}
			workspace->desc.EditImageItems(ctx);
			ctx->Pop();
		}
		ctx->Pop();
		spstr.SetSplits({ 0.5f });
	}

	Workspace* workspace = nullptr;
};

struct FileView : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());

		ctx->PushBox();
		{
			char buf[256];
			if (of->hexViewerState.selectionStart != UINT64_MAX)
			{
				int64_t selMin = std::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
				int64_t selMax = std::max(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
				int64_t len = selMax - selMin + 1;
				int64_t ciOff = 0, ciSize = 0;
				if (workspace->desc.curInst < workspace->desc.instances.size())
				{
					auto& SI = workspace->desc.instances[workspace->desc.curInst];
					ciOff = SI.off;
					ciSize = SI.def->size;
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
			ctx->Text(buf);
		}
		ctx->Pop();

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		auto* vs = ctx->MakeWithText<ui::CollapsibleTreeNode>("View settings");
		ctx->Pop();

		ctx->PushBox(); // tree stabilization box
		if (vs->open)
		{
			ui::imm::PropEditInt(ctx, "Width", of->hexViewerState.byteWidth, {}, 1, 1, 256);
			ui::imm::PropEditInt(ctx, "Position", of->hexViewerState.basePos, {}, 1, 0);
		}
		ctx->Pop(); // end tree stabilization box

		auto* hv = ctx->Make<HexViewer>();
		hv->Init(&workspace->desc, of->ddFile, &of->hexViewerState, &of->highlightSettings);
		hv->HandleEvent(UIEventType::Click) = [this, hv](UIEvent& e)
		{
			if (e.GetButton() == UIMouseButton::Right)
			{
				auto* ds = of->ddFile->dataSource;
				int64_t pos = of->hexViewerState.hoverByte;
				auto selMin = std::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
				auto selMax = std::max(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
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

				std::vector<ui::MenuItem> structs;
				{
					auto createBlank = [this, hv, pos]()
					{
						auto* ns = new DDStruct;
						do
						{
							ns->name = "struct" + std::to_string(rand() % 10000);
						} while (workspace->desc.structs.count(ns->name));
						int64_t off = pos;
						if (of->hexViewerState.selectionStart != UINT64_MAX && of->hexViewerState.selectionEnd != UINT64_MAX)
						{
							off = std::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
							ns->size = abs(int(of->hexViewerState.selectionEnd - of->hexViewerState.selectionStart)) + 1;
						}
						workspace->desc.structs[ns->name] = ns;
						workspace->desc.curInst = workspace->desc.AddInst({ ns, of->ddFile, off, "", true });
						return ns;
					};
					auto createBlankOpt = [createBlank]() { createBlank(); };
					auto createFromMarkersOpt = [this, createBlank, hv]()
					{
						auto* ns = createBlank();
						auto selMin = std::min(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
						auto selMax = std::max(of->hexViewerState.selectionStart, of->hexViewerState.selectionEnd);
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
					};
					structs.push_back(ui::MenuItem("Create a new struct (blank)").Func(createBlankOpt));
					structs.push_back(ui::MenuItem("Create a new struct (from markers)", {},
						of->hexViewerState.selectionStart == UINT64_MAX || of->hexViewerState.selectionEnd == UINT64_MAX).Func(createFromMarkersOpt));
					if (workspace->desc.structs.size())
						structs.push_back(ui::MenuItem::Separator());
				}
				for (auto& s : workspace->desc.structs)
				{
					auto fn = [this, pos, s]()
					{
						workspace->desc.curInst = workspace->desc.AddInst({ s.second, of->ddFile, pos, "", true });
					};
					structs.push_back(ui::MenuItem(s.first).Func(fn));
				}

				std::vector<ui::MenuItem> images;
				auto createImageOpt = [this, hv, pos](StringView fmt)
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
				};
				StringView prevCat;
				for (size_t i = 0, count = GetImageFormatCount(); i < count; i++)
				{
					StringView cat = GetImageFormatCategory(i);
					if (cat != prevCat)
					{
						prevCat = cat;
						images.push_back(ui::MenuItem(cat, {}, true));
					}
					StringView name = GetImageFormatName(i);
					images.push_back(ui::MenuItem(name).Func([createImageOpt, name]() { createImageOpt(name); }));
				}

				auto& md = of->ddFile->markerData;
				ui::MenuItem items[] =
				{
					ui::MenuItem(txt_pos, {}, true),
					ui::MenuItem::Submenu("Place struct", structs),
					ui::MenuItem::Submenu("Place image", images),
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
				};
				ui::Menu menu(items);
				menu.Show(hv);
				hv->RerenderNode();
			}
		};
		ctx->Pop();
	}

	Workspace* workspace = nullptr;
	OpenedFile* of = nullptr;
};

#define CUR_WORKSPACE "FRET_Plugins/wav.bdaw"

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetTitle("Binary Data Analysis Tool");
		SetSize(1200, 800);
		FileDataSource fds(CUR_WORKSPACE);
		std::string wsdata;
		wsdata.resize(fds.GetSize());
		fds.Read(0, fds.GetSize(), &wsdata[0]);
		NamedTextSerializeReader ntsr;
		printf("parsed: %s\n", ntsr.Parse(wsdata) ? "yes" : "no");
		workspace.Load(ntsr);
		//files.push_back(new REFile("tree.mesh"));
		//files.push_back(new REFile("arch.tar"));
	}
	void OnRender(UIContainer* ctx) override
	{
		ctx->Push<ui::MenuBarElement>();
		ctx->Push<ui::MenuItemElement>()->SetText("Save").Func([&]()
		{
			NamedTextSerializeWriter ntsw;
			workspace.Save(ntsw);
			for (FILE* f = fopen(CUR_WORKSPACE, "w");
				fwrite(ntsw.data.data(), ntsw.data.size(), 1, f),
				fclose(f),
				false;);
		});
		ctx->Pop();
		ctx->Pop();

		*ctx->Push<ui::TabGroup>()
			+ ui::Layout(style::layouts::EdgeSlice())
			+ ui::Height(style::Coord::Percent(100));
		{
			ctx->Push<ui::TabButtonList>();
			{
				int nf = 0;
				for (auto* f : workspace.openedFiles)
				{
					ctx->Push<ui::TabButtonT<int>>()->Init(workspace.curOpenedFile, nf++);
					ctx->Text(f->ddFile->name);
					ctx->MakeWithText<ui::Button>("X");
					ctx->Pop();
				}
			}
			ctx->Pop();

			int nf = 0;
			for (auto* of : workspace.openedFiles)
			{
				if (workspace.curOpenedFile != nf++)
					continue;

				*ctx->Push<ui::TabPanel>() + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
				{
					*ctx->Push<ui::Panel>() + ui::BoxSizing(style::BoxSizing::BorderBox) + ui::Height(style::Coord::Percent(100));
					{
						//ctx->Make<FileStructureViewer2>()->ds = f->ds;
						auto* sp = ctx->Push<ui::SplitPane>();
						{
							// left
							auto* fv = ctx->Make<FileView>();
							fv->workspace = &workspace;
							fv->of = of;

							// right
							*ctx->Push<ui::TabGroup>()
								+ ui::Layout(style::layouts::EdgeSlice())
								+ ui::Height(style::Coord::Percent(100));
							{
								ctx->Push<ui::TabButtonList>();
								ctx->MakeWithText<ui::TabButtonT<SubtabType>>("Inspect")->Init(workspace.curSubtab, SubtabType::Inspect);
								ctx->MakeWithText<ui::TabButtonT<SubtabType>>("Highlights")->Init(workspace.curSubtab, SubtabType::Highlights);
								ctx->MakeWithText<ui::TabButtonT<SubtabType>>("Markers")->Init(workspace.curSubtab, SubtabType::Markers);
								ctx->MakeWithText<ui::TabButtonT<SubtabType>>("Structures")->Init(workspace.curSubtab, SubtabType::Structures);
								ctx->MakeWithText<ui::TabButtonT<SubtabType>>("Images")->Init(workspace.curSubtab, SubtabType::Images);
								ctx->Pop();

								*ctx->Push<ui::TabPanel>()
									+ ui::Layout(style::layouts::EdgeSlice())
									+ ui::Height(style::Coord::Percent(100));

								if (workspace.curSubtab == SubtabType::Inspect)
								{
									ctx->Make<TabInspect>()->of = of;
								}

								if (workspace.curSubtab == SubtabType::Highlights)
								{
									ctx->Make<TabHighlights>()->of = of;
								}

								if (workspace.curSubtab == SubtabType::Markers)
								{
									ctx->Make<TabMarkers>()->of = of;
								}

								if (workspace.curSubtab == SubtabType::Structures)
								{
									ctx->Make<TabStructures>()->workspace = &workspace;
								}

								if (workspace.curSubtab == SubtabType::Images)
								{
									ctx->Make<TabImages>()->workspace = &workspace;
								}

								// tab panel
								ctx->Pop();
							}
							ctx->Pop();
						}
						sp->SetSplits({ 0.3f });
						ctx->Pop();
					}
					ctx->Pop();
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
	}

	Workspace workspace;
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
