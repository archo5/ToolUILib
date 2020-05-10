
#include "pch.h"
#include "FileReaders.h"
#include "FileStructureViewer.h"
#include "DataDesc.h"
#include "HexViewer.h"


struct OpenedFile
{
	void Load(NamedTextSerializeReader& r)
	{
		r.BeginDict("");
		fileID = r.ReadUInt64("fileID");
		basePos = r.ReadUInt64("basePos");
		byteWidth = r.ReadUInt("byteWidth");
		highlightSettings.Load("highlighter", r);
		r.EndDict();
	}
	void Save(NamedTextSerializeWriter& w)
	{
		w.BeginDict("");
		w.WriteInt("fileID", fileID);
		w.WriteInt("basePos", basePos);
		w.WriteInt("byteWidth", byteWidth);
		highlightSettings.Save("highlighter", w);
		w.EndDict();
	}

	DataDesc::File* ddFile = nullptr;
	uint64_t fileID = 0;
	uint64_t basePos = 0;
	uint32_t byteWidth = 8;
	HighlightSettings highlightSettings;
};

struct Workspace
{
	Workspace()
	{
		ddiSrc.dataDesc = &desc;
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
			F->dataSource = new FileDataSource(("FRET_Plugins/" + F->name).c_str());
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

	std::vector<OpenedFile*> openedFiles;
	DataDesc desc;
	DataDescInstanceSource ddiSrc;
};

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetSize(1200, 800);
		FileDataSource fds("FRET_Plugins/wav.bdaw");
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
		auto s = ctx->Push<ui::TabGroup>()->GetStyle();
		s.SetLayout(style::layouts::EdgeSlice());
		s.SetHeight(style::Coord::Percent(100));
		{
			ctx->Push<ui::TabList>();
			{
				for (auto* f : workspace.openedFiles)
				{
					ctx->Push<ui::TabButton>();
					ctx->Text(f->ddFile->name);
					ctx->MakeWithText<ui::Button>("X");
					ctx->Pop();
				}
			}
			ctx->Pop();

			for (auto* of : workspace.openedFiles)
			{
				DataDesc::File* f = of->ddFile;
				IDataSource* ds = f->dataSource;

				auto* p = ctx->Push<ui::TabPanel>();
				{
					auto s = p->GetStyle();
					s.SetLayout(style::layouts::EdgeSlice());
					s.SetHeight(style::Coord::Percent(100));
					//s.SetBoxSizing(style::BoxSizing::BorderBox);
					*ctx->Push<ui::Panel>() + ui::BoxSizing(style::BoxSizing::BorderBox) + ui::Height(style::Coord::Percent(100));
					{
						//ctx->Make<FileStructureViewer2>()->ds = f->ds;
						auto* sp = ctx->Push<ui::SplitPane>();
						{
							ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice());

							ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
							auto* vs = ctx->MakeWithText<ui::CollapsibleTreeNode>("View settings");
							auto* hs = ctx->MakeWithText<ui::CollapsibleTreeNode>("Highlight settings");
							ctx->Pop();

							ctx->PushBox(); // tree stabilization box
							if (vs->open)
							{
								ui::imm::PropEditInt(ctx, "Width", of->byteWidth, {}, 1, 1, 256);
								ui::imm::PropEditInt(ctx, "Position", of->basePos, {}, 1, 0);
							}
							if (hs->open)
							{
								of->highlightSettings.EditUI(ctx);
							}
							ctx->Pop(); // end tree stabilization box

							auto* hv = ctx->Make<HexViewer>();
							hv->Init(&workspace.desc, f, &of->basePos, &of->byteWidth, &of->highlightSettings);
							hv->HandleEvent(UIEventType::Click) = [this, f, ds, hv](UIEvent& e)
							{
								if (e.GetButton() == UIMouseButton::Right)
								{
									int64_t pos = hv->hoverByte;
									auto selMin = std::min(hv->selectionStart, hv->selectionEnd);
									if (selMin != UINT64_MAX)
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
										auto createBlank = [this, f, hv, pos]()
										{
											auto* ns = new DataDesc::Struct;
											do
											{
												ns->name = "struct" + std::to_string(rand() % 10000);
											} while (workspace.desc.structs.count(ns->name));
											int64_t off = pos;
											if (hv->selectionStart != UINT64_MAX && hv->selectionEnd != UINT64_MAX)
											{
												off = std::min(hv->selectionStart, hv->selectionEnd);
												ns->size = abs(int(hv->selectionEnd - hv->selectionStart)) + 1;
											}
											workspace.desc.structs[ns->name] = ns;
											workspace.desc.curInst = workspace.desc.AddInst({ ns, f, off, "", true });
											return ns;
										};
										auto createBlankOpt = [createBlank]() { createBlank(); };
										auto createFromMarkersOpt = [createBlank, f, hv]()
										{
											auto* ns = createBlank();
											auto selMin = std::min(hv->selectionStart, hv->selectionEnd);
											auto selMax = std::max(hv->selectionStart, hv->selectionEnd);
											int at = 0;
											for (Marker& M : f->markerData.markers)
											{
												if (M.at < selMin || M.at > selMax)
													continue;
												for (DataDesc::Field f;
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
											hv->selectionStart == UINT64_MAX || hv->selectionEnd == UINT64_MAX).Func(createFromMarkersOpt));
										if (workspace.desc.structs.size())
											structs.push_back(ui::MenuItem::Separator());
									}
									for (auto& s : workspace.desc.structs)
									{
										auto fn = [this, f, pos, s]()
										{
											workspace.desc.curInst = workspace.desc.AddInst({ s.second, f, pos, "", true });
										};
										structs.push_back(ui::MenuItem(s.first).Func(fn));
									}
									ui::MenuItem items[] =
									{
										ui::MenuItem(txt_pos, {}, true),
										ui::MenuItem::Submenu("Place struct", structs),
										ui::MenuItem::Separator(),
										ui::MenuItem("Mark ASCII", txt_ascii).Func([f, pos]() { f->markerData.AddMarker(DT_CHAR, pos, pos + 1); }),
										ui::MenuItem("Mark int8", txt_int8).Func([f, pos]() { f->markerData.AddMarker(DT_I8, pos, pos + 1); }),
										ui::MenuItem("Mark uint8", txt_uint8).Func([f, pos]() { f->markerData.AddMarker(DT_U8, pos, pos + 1); }),
										ui::MenuItem("Mark int16", txt_int16).Func([f, pos]() { f->markerData.AddMarker(DT_I16, pos, pos + 2); }),
										ui::MenuItem("Mark uint16", txt_uint16).Func([f, pos]() { f->markerData.AddMarker(DT_U16, pos, pos + 2); }),
										ui::MenuItem("Mark int32", txt_int32).Func([f, pos]() { f->markerData.AddMarker(DT_I32, pos, pos + 4); }),
										ui::MenuItem("Mark uint32", txt_uint32).Func([f, pos]() { f->markerData.AddMarker(DT_U32, pos, pos + 4); }),
										ui::MenuItem("Mark int64", txt_int64).Func([f, pos]() { f->markerData.AddMarker(DT_I64, pos, pos + 8); }),
										ui::MenuItem("Mark uint64", txt_uint64).Func([f, pos]() { f->markerData.AddMarker(DT_U64, pos, pos + 8); }),
										ui::MenuItem("Mark float32", txt_float32).Func([f, pos]() { f->markerData.AddMarker(DT_F32, pos, pos + 4); }),
										ui::MenuItem("Mark float64", txt_float64).Func([f, pos]() { f->markerData.AddMarker(DT_F64, pos, pos + 8); }),
									};
									ui::Menu menu(items);
									menu.Show(hv);
									hv->RerenderNode();
								}
							};
							ctx->Pop();

							auto& ed = ctx->PushBox();
#if 1
							ctx->Text("Instances") + ui::Padding(5);
							workspace.ddiSrc.Edit(ctx);
							auto* tv = ctx->Make<ui::TableView>();
							*tv + ui::Layout(style::layouts::EdgeSlice());
							tv->SetDataSource(&workspace.ddiSrc);
							workspace.ddiSrc.refilter = true;
							tv->CalculateColumnWidths();
							ed.HandleEvent(tv, UIEventType::SelectionChange) = [this, tv](UIEvent& e)
							{
								auto sel = tv->selection.GetFirstSelection();
								if (tv->IsValidRow(sel))
									workspace.desc.curInst = workspace.ddiSrc._indices[sel];
								e.current->RerenderNode();
							};
#else
							ctx->Text("Marked items");
							auto* tv = ctx->Make<ui::TableView>();
							*tv + ui::Layout(style::layouts::EdgeSlice());
							tv->SetDataSource(&f->markerData);
							tv->CalculateColumnWidths();
							// TODO cannot currently apply event handler to `tv` (node) directly from outside
							ed.HandleEvent(tv, UIEventType::SelectionChange) = [](UIEvent& e) { e.current->RerenderNode(); };
#endif
							ctx->Pop();

							ctx->PushBox();
#if 0
							if (tv->selection.AnySelected())
							{
								size_t pos = tv->selection.GetFirstSelection();
								if (tv->IsValidRow(pos))
									ctx->Make<MarkedItemEditor>()->marker = &f->markerData.markers[pos];
							}
#endif
							workspace.desc.Edit(ctx);
							ctx->Pop();
						}
						sp->SetSplits({ 0.3f, 0.7f });
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
