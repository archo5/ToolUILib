
#include "pch.h"
#include "FileReaders.h"
#include "FileStructureViewer.h"
#include "DataDesc.h"
#include "HexViewer.h"


struct REFile : FileDataSource
{
	REFile(std::string n)
	{
		path = n;
		name = n;
		highlighter.markerData = &markerData;
		//ds = new FileStructureDataSource(n.c_str());
		fp = fopen(("FRET_Plugins/" + n).c_str(), "rb");
	}
	~REFile()
	{
		fclose(fp);
		//delete ds;
	}

	std::string path;
	std::string name;
	FileStructureDataSource* ds;
	MarkerData markerData;
	uint64_t basePos = 0;
	uint32_t byteWidth = 8;
	Highlighter highlighter;
	DataDesc desc;
};

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetSize(1200, 800);
#if 1
		{
			auto* f = new REFile("loop.wav");

			auto* chunk = new DataDesc::Struct;
			chunk->name = "chunk";
			chunk->serialized = true;
			chunk->fields.push_back({ "char", "type", 0, 4 });
			chunk->fields.push_back({ "u32", "size" });
			chunk->fields.push_back({ "list_data", "list_data" });
			chunk->fields.push_back({ "fmt_data", "fmt_data" });
			chunk->fields.push_back({ "str_data", "str_data" });
			f->desc.structs["chunk"] = chunk;

			auto* list_data = new DataDesc::Struct;
			list_data->name = "list_data";
			list_data->serialized = true;
			list_data->params.push_back({ "size", 0 });
			list_data->fields.push_back({ "char", "subtype", 0, 4 });
			list_data->fields.push_back({ "chunk", "chunks", 0, 0, "size", true });
			f->desc.structs["list_data"] = list_data;

			auto* fmt_data = new DataDesc::Struct;
			fmt_data->name = "fmt_data";
			fmt_data->serialized = true;
			fmt_data->fields.push_back({ "u16", "AudioFormat" });
			fmt_data->fields.push_back({ "u16", "NumChannels" });
			fmt_data->fields.push_back({ "u32", "SampleRate" });
			fmt_data->fields.push_back({ "u32", "ByteRate" });
			fmt_data->fields.push_back({ "u16", "BlockAlign" });
			fmt_data->fields.push_back({ "u16", "BitsPerSample" });
			f->desc.structs["fmt_data"] = fmt_data;

			auto* str_data = new DataDesc::Struct;
			str_data->name = "str_data";
			str_data->serialized = true;
			str_data->params.push_back({ "size", 0 });
			str_data->fields.push_back({ "char", "text", 0, 0, "size" });
			f->desc.structs["str_data"] = str_data;

			f->desc.instances.push_back({ chunk, 0, "RIFF chunk", true });
			f->desc.instances.push_back({ fmt_data, 20, "fmt chunk data", true });
			f->desc.instances.push_back({ str_data, 56, "ICMT data", true, { { "size", 28 } } });

			files.push_back(f);

			f->markerData.markers.push_back({ DT_I32, 8, 1, 1, 0 });
			f->markerData.markers.push_back({ DT_F32, 12, 3, 2, 12 });
		}
#endif
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
				for (auto* f : files)
				{
					ctx->Push<ui::TabButton>();
					ctx->Text(f->name);
					ctx->MakeWithText<ui::Button>("X");
					ctx->Pop();
				}
			}
			ctx->Pop();

			for (auto* f : files)
			{
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
								ui::imm::PropEditInt(ctx, "Width", f->byteWidth, {}, 1, 1, 256);
								ui::imm::PropEditInt(ctx, "Position", f->basePos, {}, 1, 0);
							}
							if (hs->open)
							{
								f->highlighter.EditUI(ctx);
							}
							ctx->Pop(); // end tree stabilization box

							auto* hv = ctx->Make<HexViewer>();
							hv->Init(f, &f->basePos, &f->byteWidth, &f->highlighter);
							hv->HandleEvent(UIEventType::Click) = [f, hv](UIEvent& e)
							{
								if (e.GetButton() == UIMouseButton::Right)
								{
									uint64_t pos = hv->hoverByte;

									char txt_pos[32];
									snprintf(txt_pos, 32, "@ %" PRIu64 " (0x%" PRIX64 ")", pos, pos);

									char txt_int8[32];
									f->GetInt8Text(txt_int8, 32, pos, true);
									char txt_uint8[32];
									f->GetInt8Text(txt_uint8, 32, pos, false);
									char txt_int16[32];
									f->GetInt16Text(txt_int16, 32, pos, true);
									char txt_uint16[32];
									f->GetInt16Text(txt_uint16, 32, pos, false);
									char txt_int32[32];
									f->GetInt32Text(txt_int32, 32, pos, true);
									char txt_uint32[32];
									f->GetInt32Text(txt_uint32, 32, pos, false);
									char txt_int64[32];
									f->GetInt64Text(txt_int64, 32, pos, true);
									char txt_uint64[32];
									f->GetInt64Text(txt_uint64, 32, pos, false);
									char txt_float32[32];
									f->GetFloat32Text(txt_float32, 32, pos);
									char txt_float64[32];
									f->GetFloat64Text(txt_float64, 32, pos);

									std::vector<ui::MenuItem> structs;
									for (auto& s : f->desc.structs)
									{
										auto fn = [f, pos, s]()
										{
											f->desc.curInst = f->desc.instances.size();
											f->desc.instances.push_back({ s.second, pos, "", true });
										};
										structs.push_back(ui::MenuItem(s.first).Func(fn));
									}
									ui::MenuItem items[] =
									{
										ui::MenuItem(txt_pos, {}, true),
										ui::MenuItem::Submenu("Place struct", structs),
										ui::MenuItem::Separator(),
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
							ctx->Text("Marked items");
							auto* tv = ctx->Make<ui::TableView>();
							*tv + ui::Layout(style::layouts::EdgeSlice());
							tv->SetDataSource(&f->markerData);
							tv->CalculateColumnWidths();
							// TODO cannot currently apply event handler to `tv` (node) directly from outside
							ed.HandleEvent(tv, UIEventType::SelectionChange) = [](UIEvent& e) { e.current->RerenderNode(); };
							ctx->Pop();

							ctx->PushBox();
							if (tv->selection.AnySelected())
							{
								size_t pos = tv->selection.GetFirstSelection();
								if (tv->IsValidRow(pos))
									ctx->Make<MarkedItemEditor>()->marker = &f->markerData.markers[pos];
							}
							f->desc.Edit(ctx, f);
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

	std::vector<REFile*> files;
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
