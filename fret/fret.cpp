
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


static float hsplitHexView[1] = { 0.3f };

struct MainWindow : ui::NativeMainWindow
{
	struct ConnectionPainter : ui::FillerElement
	{
		MainWindow* W;

		void OnPaint(const ui::UIPaintContext& ctx) override
		{
			ui::FillerElement::OnPaint(ctx);

			if (W->curFileView && W->curFileView->curHexViewer && W->curTable)
			{
				auto hvcr = W->curFileView->curHexViewer->GetFinalRect();
				ConnectOffset cobuf[128];
				int count = W->curTable->GetOffsets(128, cobuf);
				for (int i = 0; i < count; i++)
				{
					auto co = cobuf[i];
					if (co.off < W->curFileView->curHexViewer->GetBasePos())
						continue;
					auto br = W->curFileView->curHexViewer->GetByteRect(co.off);
					if (!hvcr.Contains(br.x1, br.y1))
						continue;

					ui::Color4f colLine(1, 0.5f, 0);
					ui::draw::LineCol(br.x0, br.y0, br.x0, br.y1, 1, colLine);
					ui::draw::LineCol(br.x0, br.y1, br.x1, br.y1, 1, colLine);
					ui::draw::LineCol(br.x1, br.y1, co.tablePos.x, co.tablePos.y, 1, colLine);
				}
			}
		}
	};

	void UpdateTitle()
	{
		std::string title;
		if (curWorkspacePath.empty())
			title += "<untitled>";
		else
			title += curWorkspacePath;
		title += " - Binary Data Analysis Tool";
		SetTitle(title);
	}
	MainWindow()
	{
		fileSel.defaultExt = "bdaw";
		fileSel.filters.push_back({ "Binary Data Analysis Tool workspace files (*.bdaw)", "*.bdaw" });

		SetInnerSize(1200, 800);
		UpdateTitle();
	}
	void OnBuild() override
	{
		curFileView = nullptr;
		curTable = nullptr;

		auto* N = ui::GetCurrentBuildable();

		N->HandleEvent(ui::UserEvent(GlobalEvent_OpenImage)) = [this](ui::Event& e)
		{
			workspace.curSubtab = SubtabType::Images;
			Rebuild();
		};
		N->HandleEvent(ui::UserEvent(GlobalEvent_OpenImageRsrcEditor)) = [this](ui::Event& e)
		{
			if (!curImageEditor)
				curImageEditor = new WindowT<ImageEditorWindowNode>();
			curImageEditor->SetVisible(true);
			curImageEditor->rootBuildable->Setup(&workspace.desc);
			curImageEditor->rootBuildable->SetStruct((DDStruct*)e.arg0);
			curImageEditor->rootBuildable->Rebuild();
		};
		N->HandleEvent(ui::UserEvent(GlobalEvent_OpenMeshRsrcEditor)) = [this](ui::Event& e)
		{
			if (!curMeshEditor)
				curMeshEditor = new WindowT<MeshEditorWindowNode>();
			curMeshEditor->SetVisible(true);
			curMeshEditor->rootBuildable->Setup(&workspace.desc);
			curMeshEditor->rootBuildable->SetStruct((DDStruct*)e.arg0);
			curMeshEditor->rootBuildable->Rebuild();
		};

		std::vector<ui::MenuItem> topMenu;
		{
			topMenu.push_back(ui::MenuItem("File"));
			auto& fileMenu = topMenu.back().submenu;

			fileMenu.push_back(ui::MenuItem("New").Func([this, N]() { OnNew(); N->Rebuild(); }));
			fileMenu.push_back(ui::MenuItem("Open...").Func([this, N]() { OnOpen(); N->Rebuild(); }));
			fileMenu.push_back(ui::MenuItem("Save").Func([this, N]() { OnSave(); N->Rebuild(); }));
			fileMenu.push_back(ui::MenuItem("Save As...").Func([this, N]() { OnSaveAs(); N->Rebuild(); }));

			fileMenu.push_back(ui::MenuItem::Separator());

			fileMenu.push_back(ui::MenuItem("Export script").Func([&]()
			{
				std::string path = ui::to_string(ui::StringView(curWorkspacePath).until_last("."), ".py");
				auto scr = ExportPythonScript(&workspace.desc);
				ui::WriteTextFile(path, scr);
			}));

			fileMenu.push_back(ui::MenuItem::Separator());

			fileMenu.push_back(ui::MenuItem("Quit").Func([this]() { OnQuit(); }));
		}
		ui::BuildAlloc<ui::TopMenu>(this, topMenu);

		ui::Push<ConnectionPainter>().W = this;
		ui::Push<ui::EdgeSliceLayoutElement>();
		ui::Make<ui::DefaultOverlayBuilder>();

		auto& tpFiles = ui::Push<ui::TabbedPanel>();
		tpFiles.showCloseButton = true;
		{
			int nf = 0;
			for (auto* f : workspace.openedFiles)
			{
				tpFiles.AddTextTab(f->ddFile->name, uintptr_t(nf++));
			}
			tpFiles.SetActiveTabByUID(workspace.curOpenedFile);
			tpFiles.HandleEvent(&tpFiles, ui::EventType::SelectionChange) = [this, &tpFiles](ui::Event&)
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
						ui::Push<ui::SplitPane>().Init(ui::Direction::Horizontal, hsplitHexView);
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
								tp.HandleEvent(&tp, ui::EventType::SelectionChange) = [this, &tp](ui::Event&)
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
						ui::Pop();
					}
					//ui::Pop();
				}
			}
		}
		ui::Pop();

		ui::Pop(); // EdgeSliceLayoutElement
		ui::Pop(); // ConnectionPainter
	}

	void LoadFile(const std::string& path)
	{
		if (workspace.LoadFromFile(path))
		{
			curWorkspacePath = path;
			UpdateTitle();
		}
	}
	void OnNew()
	{
		curWorkspacePath.clear();
		UpdateTitle();
	}
	void OnOpen()
	{
		if (fileSel.Show(false))
		{
			LoadFile(fileSel.currentDir + "/" + fileSel.selectedFiles[0]);
		}
	}
	void OnSave()
	{
		if (curWorkspacePath.empty())
			OnSaveAs();
		else
			workspace.SaveToFile(curWorkspacePath);
	}
	void OnSaveAs()
	{
		if (fileSel.Show(true))
		{
			auto path = fileSel.currentDir + "/" + fileSel.selectedFiles[0];
			if (workspace.SaveToFile(path))
			{
				curWorkspacePath = path;
				UpdateTitle();
			}
		}
	}
	void OnQuit()
	{
		OnClose();
	}

	Workspace workspace;
	ui::FileSelectionWindow fileSel;
	std::string curWorkspacePath;

	FileView* curFileView = nullptr;
	TableWithOffsets* curTable = nullptr;
	WindowT<ImageEditorWindowNode>* curImageEditor = nullptr;
	WindowT<MeshEditorWindowNode>* curMeshEditor = nullptr;
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	MainWindow mw;
	if (argc == 2)
	{
		mw.LoadFile(argv[1]);
	}
	mw.SetVisible(true);
	return app.Run();
}
