
#include "pch.h"
#include "FileReaders.h"
#include "FileStructureViewer.h"
#include "DataDesc.h"
#include "ExportScript.h"
#include "HexViewer.h"
#include "ImageParsers.h"
#include "Workspace.h"
#include "FileView.h"
#include "ImageEditor.h"
#include "MeshEditor.h"

#include "TableWithOffsets.h"
#include "TabInspect.h"
#include "TabHighlights.h"
#include "TabMarkers.h"
#include "TabStructures.h"
#include "TabImages.h"


#define CUR_WORKSPACE "FRET_Plugins/wav.bdaw"

struct MainWindowNode : ui::Node
{
	void OnInit() override
	{
		GetNativeWindow()->SetTitle("Binary Data Analysis Tool");
		GetNativeWindow()->SetSize(1200, 800);
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
	void OnPaint() override
	{
		ui::Node::OnPaint();

		if (curFileView && curFileView->curHexViewer && curTable)
		{
			auto hvcr = curFileView->curHexViewer->GetContentRect();
			ConnectOffset cobuf[128];
			int count = curTable->GetOffsets(128, cobuf);
			for (int i = 0; i < count; i++)
			{
				auto co = cobuf[i];
				if (co.off < curFileView->curHexViewer->GetBasePos())
					continue;
				auto br = curFileView->curHexViewer->GetByteRect(co.off);
				if (!hvcr.Contains(br.x1, br.y1))
					continue;

				ui::Color4f colLine(1, 0.5f, 0);
				ui::draw::LineCol(br.x0, br.y0, br.x0, br.y1, 1, colLine);
				ui::draw::LineCol(br.x0, br.y1, br.x1, br.y1, 1, colLine);
				ui::draw::LineCol(br.x1, br.y1, co.tablePos.x, co.tablePos.y, 1, colLine);
			}
		}
	}
	void Render(UIContainer* ctx) override
	{
		curFileView = nullptr;
		curTable = nullptr;

		HandleEvent(UserEvent(GlobalEvent_OpenImage)) = [this](UIEvent& e)
		{
			workspace.curSubtab = SubtabType::Images;
			Rerender();
		};
		HandleEvent(UserEvent(GlobalEvent_OpenImageRsrcEditor)) = [this](UIEvent& e)
		{
			if (!curImageEditor)
				curImageEditor = new WindowT<ImageEditorWindowNode>();
			curImageEditor->SetVisible(true);
			curImageEditor->rootNode->Setup(&workspace.desc);
			curImageEditor->rootNode->SetStruct((DDStruct*)e.arg0);
			curImageEditor->rootNode->Rerender();
		};
		HandleEvent(UserEvent(GlobalEvent_OpenMeshRsrcEditor)) = [this](UIEvent& e)
		{
			if (!curMeshEditor)
				curMeshEditor = new WindowT<MeshEditorWindowNode>();
			curMeshEditor->SetVisible(true);
			curMeshEditor->rootNode->Setup(&workspace.desc);
			curMeshEditor->rootNode->SetStruct((DDStruct*)e.arg0);
			curMeshEditor->rootNode->Rerender();
		};

		ctx->Push<ui::MenuBarElement>();
		ctx->Make<ui::MenuItemElement>()->SetText("Save").Func([&]()
		{
			NamedTextSerializeWriter ntsw;
			workspace.Save(ntsw);
			ui::WriteTextFile(CUR_WORKSPACE, ntsw.data);
		});
		ctx->Make<ui::MenuItemElement>()->SetText("Export script").Func([&]()
		{
			char bfr[256];
			strcpy(bfr, CUR_WORKSPACE);
			*strrchr(bfr, '.') = '\0';
			strcat(bfr, ".py");
			auto scr = ExportPythonScript(&workspace.desc);
			ui::WriteTextFile(bfr, scr);
		});
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
							curFileView = fv;

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
									auto* tm = ctx->Make<TabMarkers>();
									tm->of = of;
									curTable = tm;
								}

								if (workspace.curSubtab == SubtabType::Structures)
								{
									auto* ts = ctx->Make<TabStructures>();
									ts->workspace = &workspace;
									curTable = ts;
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

	FileView* curFileView = nullptr;
	TableWithOffsets* curTable = nullptr;
	WindowT<ImageEditorWindowNode>* curImageEditor = nullptr;
	WindowT<MeshEditorWindowNode>* curMeshEditor = nullptr;
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	WindowT<MainWindowNode> mw;
	mw.SetVisible(true);
	return app.Run();
}
