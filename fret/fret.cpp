
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

struct MainWindowContents : ui::Buildable
{
	void OnEnable() override
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
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);

		if (curFileView && curFileView->curHexViewer && curTable)
		{
			auto hvcr = curFileView->curHexViewer->GetFinalRect();
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
	void Build() override
	{
		curFileView = nullptr;
		curTable = nullptr;

		HandleEvent(ui::UserEvent(GlobalEvent_OpenImage)) = [this](ui::Event& e)
		{
			workspace.curSubtab = SubtabType::Images;
			Rebuild();
		};
		HandleEvent(ui::UserEvent(GlobalEvent_OpenImageRsrcEditor)) = [this](ui::Event& e)
		{
			if (!curImageEditor)
				curImageEditor = new WindowT<ImageEditorWindowNode>();
			curImageEditor->SetVisible(true);
			curImageEditor->rootBuildable->Setup(&workspace.desc);
			curImageEditor->rootBuildable->SetStruct((DDStruct*)e.arg0);
			curImageEditor->rootBuildable->Rebuild();
		};
		HandleEvent(ui::UserEvent(GlobalEvent_OpenMeshRsrcEditor)) = [this](ui::Event& e)
		{
			if (!curMeshEditor)
				curMeshEditor = new WindowT<MeshEditorWindowNode>();
			curMeshEditor->SetVisible(true);
			curMeshEditor->rootBuildable->Setup(&workspace.desc);
			curMeshEditor->rootBuildable->SetStruct((DDStruct*)e.arg0);
			curMeshEditor->rootBuildable->Rebuild();
		};

#if 1
		std::vector<ui::MenuItem> topMenu;
		topMenu.push_back(ui::MenuItem("Save").Func([&]()
		{
			NamedTextSerializeWriter ntsw;
			workspace.Save(ntsw);
			ui::WriteTextFile(CUR_WORKSPACE, ntsw.data);
		}));
		topMenu.push_back(ui::MenuItem("Export script").Func([&]()
		{
			char bfr[256];
			strcpy(bfr, CUR_WORKSPACE);
			*strrchr(bfr, '.') = '\0';
			strcat(bfr, ".py");
			auto scr = ExportPythonScript(&workspace.desc);
			ui::WriteTextFile(bfr, scr);
		}));
		Allocate<ui::TopMenu>(GetNativeWindow(), topMenu);
#else
		ui::Push<ui::MenuBarElement>();
		ui::Make<ui::MenuItemElement>().SetText("Save").Func([&]()
		{
			NamedTextSerializeWriter ntsw;
			workspace.Save(ntsw);
			ui::WriteTextFile(CUR_WORKSPACE, ntsw.data);
		});
		ui::Make<ui::MenuItemElement>().SetText("Export script").Func([&]()
		{
			char bfr[256];
			strcpy(bfr, CUR_WORKSPACE);
			*strrchr(bfr, '.') = '\0';
			strcat(bfr, ".py");
			auto scr = ExportPythonScript(&workspace.desc);
			ui::WriteTextFile(bfr, scr);
		});
		ui::Pop();
#endif

		auto& tpFiles = ui::Push<ui::TabbedPanel>();
		tpFiles.showCloseButton = true;
		{
			int nf = 0;
			for (auto* f : workspace.openedFiles)
			{
				tpFiles.AddTextTab(f->ddFile->name, uintptr_t(nf++));
			}
			tpFiles.SetActiveTabByUID(workspace.curOpenedFile);
			tpFiles.HandleEvent(&tpFiles, ui::EventType::Commit) = [this, &tpFiles](ui::Event&)
			{
				workspace.curOpenedFile = tpFiles.GetCurrentTabUID(0);
			};

			nf = 0;
			for (auto* of : workspace.openedFiles)
			{
				if (workspace.curOpenedFile != nf++)
					continue;

				{
					//ui::Push<ui::FrameElement>().SetDefaultStyle(ui::DefaultFrameStyle::GroupBox);
					{
						//ui::Make<FileStructureViewer2>()->ds = f->ds;
						auto& sp = ui::Push<ui::SplitPane>();
						{
							// left
							auto& fv = ui::Make<FileView>();
							fv.workspace = &workspace;
							fv.of = of;
							curFileView = &fv;

							// right
							auto& tp = ui::Push<ui::TabbedPanel>();
							{
								tp.AddEnumTab("Inspect", SubtabType::Inspect);
								tp.AddEnumTab("Highlights", SubtabType::Highlights);
								tp.AddEnumTab("Markers", SubtabType::Markers);
								tp.AddEnumTab("Structures", SubtabType::Structures);
								tp.AddEnumTab("Images", SubtabType::Images);

								tp.SetActiveTabByUID(uintptr_t(workspace.curSubtab));
								tp.HandleEvent(&tp, ui::EventType::Commit) = [this, &tp](ui::Event&)
								{
									workspace.curSubtab = SubtabType(tp.GetCurrentTabUID(0));
								};

								if (workspace.curSubtab == SubtabType::Inspect)
								{
									ui::Make<TabInspect>().of = of;
								}

								if (workspace.curSubtab == SubtabType::Highlights)
								{
									ui::Make<TabHighlights>().of = of;
								}

								if (workspace.curSubtab == SubtabType::Markers)
								{
									auto& tm = ui::Make<TabMarkers>();
									tm.of = of;
									curTable = &tm;
								}

								if (workspace.curSubtab == SubtabType::Structures)
								{
									auto& ts = ui::Make<TabStructures>();
									ts.workspace = &workspace;
									curTable = &ts;
								}

								if (workspace.curSubtab == SubtabType::Images)
								{
									ui::Make<TabImages>().workspace = &workspace;
								}
							}
							ui::Pop();
						}
						sp.SetSplits({ 0.3f });
						ui::Pop();
					}
					//ui::Pop();
				}
			}
		}
		ui::Pop();
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
	WindowT<MainWindowContents> mw;
	mw.subWindow = false;
	mw.SetVisible(true);
	return app.Run();
}
