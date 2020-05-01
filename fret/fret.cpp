
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
	uint32_t byteWidth = 16;
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
							ctx->Pop();

							ctx->PushBox();
							ctx->Text("Marked items");
							ctx->Pop();

							ctx->PushBox();
							ctx->Make<MarkedItemsList>()->markerData = &f->markerData;
							f->desc.Edit(ctx, f);
							ctx->Pop();
						}
						sp->SetSplit(0, 0.45f);
						sp->SetSplit(1, 0.7f);
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
