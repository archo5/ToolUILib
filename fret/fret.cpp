
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
			chunk->serialized = true;
			chunk->fields.push_back({ "char", "type", 4 });
			chunk->fields.push_back({ "u32", "size", 1 });
			f->desc.structs["chunk"] = chunk;

			auto* fmt_data = new DataDesc::Struct;
			fmt_data->serialized = true;
			fmt_data->fields.push_back({ "u16", "AudioFormat", 1 });
			fmt_data->fields.push_back({ "u16", "NumChannels", 1 });
			fmt_data->fields.push_back({ "u32", "SampleRate", 1 });
			fmt_data->fields.push_back({ "u32", "ByteRate", 1 });
			fmt_data->fields.push_back({ "u16", "BlockAlign", 1 });
			fmt_data->fields.push_back({ "u16", "BitsPerSample", 1 });
			f->desc.structs["fmt_data"] = fmt_data;

			f->desc.instances.push_back({ "chunk", 0, "RIFF chunk" });
			f->desc.instances.push_back({ "fmt_data", 20, "fmt chunk data" });

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

									char txt_pos[64];
									snprintf(txt_pos, 32, "@ %" PRIu64 " (0x%" PRIX64 ")", pos, pos);

									char txt_int16[32];
									hv->GetInt16Text(txt_int16, 32, pos, true);
									char txt_uint16[32];
									hv->GetInt16Text(txt_uint16, 32, pos, false);
									char txt_int32[32];
									hv->GetInt32Text(txt_int32, 32, pos, true);
									char txt_uint32[32];
									hv->GetInt32Text(txt_uint32, 32, pos, false);
									char txt_float32[32];
									hv->GetFloat32Text(txt_float32, 32, pos);

									std::vector<ui::MenuItem> structs;
									for (auto& s : f->desc.structs)
									{
										auto fn = [f, pos, s]()
										{
											f->desc.curInst = f->desc.instances.size();
											f->desc.instances.push_back({ s.first, pos, "" });
										};
										structs.push_back(ui::MenuItem(s.first).Func(fn));
									}
									ui::MenuItem items[] =
									{
										ui::MenuItem(txt_pos, {}, true),
										ui::MenuItem::Submenu("Place struct", structs),
										ui::MenuItem::Separator(),
										ui::MenuItem("Mark int16", txt_int16).Func([f, pos]() { f->markerData.AddMarker(DT_I16, pos, pos + 2); }),
										ui::MenuItem("Mark uint16", txt_uint16).Func([f, pos]() { f->markerData.AddMarker(DT_U16, pos, pos + 2); }),
										ui::MenuItem("Mark int32", txt_int32).Func([f, pos]() { f->markerData.AddMarker(DT_I32, pos, pos + 4); }),
										ui::MenuItem("Mark uint32", txt_uint32).Func([f, pos]() { f->markerData.AddMarker(DT_U32, pos, pos + 4); }),
										ui::MenuItem("Mark float32", txt_float32).Func([f, pos]() { f->markerData.AddMarker(DT_F32, pos, pos + 4); }),
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
						sp->SetSplits({ 0.25f, 0.7f });
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
