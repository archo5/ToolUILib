
#include "pch.h"
#include "../lib-src/Render/RHI.h"


ui::MulticastDelegate<> ItemSelectionChanged;
struct DataEditor : ui::Buildable
{
	struct Item
	{
		std::string name;
		int value;
		bool enable;
		float value2;
	};

	struct ItemButton : ui::Buildable
	{
		void Build() override
		{
			static int wat = 0;
			auto& b = ui::Push<ui::Button>();
			//b.SetInputDisabled(wat++ % 2);
			ui::Text(name);
			ui::Pop();
		}
		void OnEvent(ui::Event& e) override
		{
			if (e.type == ui::EventType::Activate)
			{
				callback();
			}
			if (e.type == ui::EventType::Click && e.GetButton() == ui::MouseButton::Right)
			{
				ui::MenuItem sm[] =
				{
					ui::MenuItem("Something", "Yeah", true),
					ui::MenuItem("Another thing", "Yup").Func([]() { puts("- yeah!"); }),
					ui::MenuItem("Detailed thing", "Ooh").Func([](ui::Menu* menu, int id)
					{
						while (id >= 0)
						{
							printf("- with %d!\n", id);
							id = menu->GetItems()[id].parent;
						}
					}),
				};

				ui::MenuItem m[] =
				{
					ui::MenuItem("Item 1", "Ctrl+Enter"),
					ui::MenuItem::Separator(),
					ui::MenuItem::Submenu("Submenu", sm),
					ui::MenuItem::Submenu("Submenu 2", sm),
					ui::MenuItem("Item 2", {}, false, true),
				};

				ui::Menu menu(m);
				int ret = menu.Show(this);
				printf("%d\n", ret);
				e.StopPropagation();
			}
		}
		void Init(const char* txt, const std::function<void()> cb)
		{
			name = txt;
			callback = cb;
		}
		std::function<void()> callback;
		const char* name;
	};

#if 0
	void OnInit() override
	{
		static ui::MenuItem smfile[] =
		{
			ui::MenuItem("New"),
			ui::MenuItem("Open", "Ctrl+O"),
			ui::MenuItem("Save", "Ctrl+S"),
			ui::MenuItem("Save As..."),
		};
		static ui::MenuItem smhelp[] =
		{
			ui::MenuItem("About"),
		};
		static ui::MenuItem main[] =
		{
			ui::MenuItem::Submenu("File", smfile),
			ui::MenuItem::Submenu("Help", smhelp),
		};
		topMenu = new ui::Menu(main, true);
		FindParentOfType<ui::NativeWindow>()->SetMenu(topMenu);
	}
	void OnDestroy() override
	{
		FindParentOfType<ui::NativeWindow>()->SetMenu(nullptr);
		delete topMenu;
	}
#endif

	void Build() override
	{
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::MakeWithText<ui::ProgressBar>("Processing...").progress = 0.37f;
		static float sldval = 0.63f;
		ui::Make<ui::Slider>().Init(sldval, { 0, 2, 0.1 });

#if 0
		struct TableDS : ui::TableDataSource
		{
			size_t GetNumRows() { return 3; }
			size_t GetNumCols() { return 4; }
			std::string GetRowName(size_t row) { return "Row" + std::to_string(row); }
			std::string GetColName(size_t col) { return "Col" + std::to_string(col); }
			std::string GetText(size_t row, size_t col) { return "R" + std::to_string(row) + "C" + std::to_string(col); }
		};
		static TableDS tblds;
		ui::Push<ui::SizeConstraintElement>().SetHeight(90);
		auto& tbl = ui::Make<ui::TableView>();
		tbl.SetDataSource(&tblds);
		ui::Pop();
#endif

#if 1
		struct TreeDS : ui::TreeDataSource
		{
			size_t GetNumCols() { return 4; }
			std::string GetColName(size_t col) { return "Col" + std::to_string(col); }
			size_t GetChildCount(uintptr_t id)
			{
				if (id == ROOT) return 2;
				if (id == 0) return 1;
				return 0;
			}
			uintptr_t GetChild(uintptr_t id, size_t which)
			{
				if (id == ROOT) return which;
				if (id == 0) return 2;
				return 0;
			}
			std::string GetText(uintptr_t id, size_t col) { return "ID" + std::to_string(id) + "C" + std::to_string(col); }
		};
		static TreeDS treeds;
		ui::Push<ui::SizeConstraintElement>().SetHeight(90);
		//auto& trv = ui::Make<ui::TreeView>();
		//trv.SetDataSource(&treeds);
		ui::Pop();
#endif

#if 1
		auto& nw = ui::Make<ui::NativeWindowNode>();
		nw.GetWindow()->SetTitle("Subwindow A");
		auto buildFunc = []()
		{
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackTopDownLayoutElement>();
			auto onClick = []()
			{
				struct DialogTest : ui::NativeDialogWindow, ui::Buildable
				{
					DialogTest()
					{
						SetStyle(ui::WS_Resizable);
						SetContents(this, false);
					}
					~DialogTest()
					{
						PO_BeforeDelete();
					}
					void Build() override
					{
						ui::Push<ui::EdgeSliceLayoutElement>();

						ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
						ui::Push<ui::StackLTRLayoutElement>(); // TODO RTL
						if (ui::imm::Button(ui::DefaultIconStyle::Close))
							OnClose();
						if (ui::imm::Button("[]"))
							SetState(GetState() == ui::WindowState::Maximized ? ui::WindowState::Normal : ui::WindowState::Maximized);
						if (ui::imm::Button("_"))
							SetState(ui::WindowState::Minimized);
						ui::Pop();
						ui::Pop();

						ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
						ui::Make<ItemButton>().Init("Test", [this]() { OnClose(); });
						ui::Pop();

						ui::Pop();
					}
				} dlg;
				dlg.SetTitle("Dialog!");
				dlg.Show();
			};
			ui::Make<ItemButton>().Init("Only a button", []() {});
			ui::Make<ItemButton>().Init("Only another button (dialog)", onClick);
			ui::Pop();
			ui::Pop();
		};
		auto* bcb = ui::CreateUIObject<ui::BuildCallback>();
		bcb->buildFunc = buildFunc;
		nw.GetWindow()->SetContents(bcb, true);
		nw.GetWindow()->SetVisible(true);
#endif

#if 1
		ui::Push<ui::SizeConstraintElement>().SetMinWidth(100).SetMinHeight(100);
		ui::Push<ui::PaddingElement>().SetPadding(4);
		auto& frm = ui::Make<ui::InlineFrame>();
		auto frf = []()
		{
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackTopDownLayoutElement>();
			ui::MakeWithText<ui::Button>("In-frame button");
			static int cur = 1;
			BasicRadioButton("Zero", cur, 0);
			BasicRadioButton2("One", cur, 1);
			BasicRadioButton2("Two", cur, 2);
			ui::Pop();
			ui::Pop();
		};
		frm.CreateFrameContents(frf);
		ui::Pop();
		ui::Pop();
#endif

		static bool lbsel0 = false;
		static bool lbsel1 = true;
		ui::Push<ui::SizeConstraintElement>().SetHeight(50);
		ui::Push<ui::ListBoxFrame>();
		ui::Push<ui::ScrollArea>();
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::MakeWithText<ui::Selectable>("Item 1").Init(lbsel0);
		ui::MakeWithText<ui::Selectable>("Item two").Init(lbsel1);
		ui::Pop();
		ui::Pop();
		ui::Pop();
		ui::Pop();

		ui::BuildMulticastDelegateAdd(ItemSelectionChanged, [this]() { Rebuild(); });
		if (editing == SIZE_MAX)
		{
			ui::Text("List");
			ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			ui::Push<ui::StackTopDownLayoutElement>();
			for (size_t i = 0; i < items.Size(); i++)
			{
				ui::Make<ItemButton>().Init(items[i].name.c_str(), [this, i]() { editing = i; ItemSelectionChanged.Call(); });
			}
			ui::Pop();
			ui::Pop();
		}
		else
		{
			ui::Push<ui::StackExpandLTRLayoutElement>();
			auto tmpl = ui::StackExpandLTRLayoutElement::GetSlotTemplate();
			tmpl->DisableScaling();
			ui::MakeWithText<ui::LabelFrame>("Item:");
			ui::Text(items[editing].name);
			ui::Push<ui::SizeConstraintElement>().SetWidth(80);
			if (ui::imm::Button("Go back"))
			{
				editing = SIZE_MAX;
				ItemSelectionChanged.Call();
			}
			ui::Pop();
			ui::Pop();

			ui::imm::PropEditString("Name", items[editing].name.c_str(), [&](const char* v) { items[editing].name = v; });
			ui::imm::PropEditBool("Enable", items[editing].enable);
		}

		ui::Pop();
	}

	size_t editing = SIZE_MAX;
	ui::Array<Item> items =
	{
		{ "test item 1", 5, true, 36.6f },
		{ "another test item", 123, false, 12.3f },
		{ "third item", 333, true, 3.0f },
	};
	ui::Menu* topMenu;
};


void Test_RenderingPrimitives();
void Test_VectorImage();
void Test_BlurMask();
void Test_TextBaseline();
void Test_TextMultiline();
void Test_StylePainting();
void Test_ImageSetSizing();
void Test_AtlasOverflow();
void Test_KeyboardEvents();
void Test_RawMouseEvents();
void Test_OpenClose();
void Test_AppendMix();
void Test_RebuildEvents();
void Test_AnimationRequest();
void Test_ElementReset();
void Test_SubUI();
void Test_HighElementCount();
void Test_ZeroRebuild();
void Test_GlobalEvents();
void Test_Frames();
void Test_DialogWindow();

void Test_EdgeSlice();
void Test_StackingLayoutVariations();
void Test_Size();
void Test_Placement();

void Test_DragDrop();
void Test_StateButtons();
void Test_PropertyList();
void Test_Sliders();
void Test_SplitPane();
void Test_Tabs();
void Test_TransformContainer();
void Test_Scrollbars();
void Test_Tooltip();
void Test_Dropdown();
void Test_Textbox();
void Test_ColorBlock();
void Test_Image();
void Test_ColorPicker();
void Test_IMGUI();
void Test_ModalWindow();
void Test_Docking();

void Test_3DView();
void Test_Gizmo();
void Test_Quaternion();

void Test_BasicEasingAnim();
void Test_VExpandAnim();
void Test_ThreadWorker();
void Test_ThreadedImageRendering();
void Test_Fullscreen();
void Test_OSCommunication();
void Test_SysDirPaths();
void Test_FileSelectionWindow();
void Test_DirectoryChangeWatcher();
void Test_SerializationSpeed();
void Test_ConfigTweakable();
void Test_TweakableValues();

void Test_SequenceEditors();
void Test_TreeEditors();
void Test_MessageLogView();
void Test_CurveEditor();

void Benchmark_SubUI();
void Benchmark_BuildManyElements();
void Test_TableView();
void Test_TreeView();
void Test_FileTreeView();

void Demo_Calculator();
void Demo_SettingsWindow();
void Demo_BasicTreeNodeEdit();
void Demo_CompactTreeNodeEdit();
void Demo_ScriptTree();
void Demo_AnimatedPatternEditor();
void Demo_NodeGraphEditor();
void Demo_TrackEditor();
void Demo_SlidingHighlightAnim();
void Demo_ButtonPressHighlight();
void Demo_FancyButton();


struct TestEntry
{
	const char* name;
	void(*func)();
};
static const TestEntry coreTestEntries[] =
{
	{ "Off", []() {} },
	{},
	{ "- Rendering -" },
	{ "Primitives", Test_RenderingPrimitives },
	{ "Vector image", Test_VectorImage },
	{ "Blur mask", Test_BlurMask },
	{ "Text baseline", Test_TextBaseline },
	{ "Text (multiline)", Test_TextMultiline },
	{ "Style painting", Test_StylePainting },
	{ "Image set sizing", Test_ImageSetSizing },
	{ "Atlas overflow", Test_AtlasOverflow },
	{},
	{ "- Events -" },
	{ "Keyboard", Test_KeyboardEvents },
	{ "[Raw] Mouse", Test_RawMouseEvents },
	{},
	{ "- Basic logic -" },
	{ "Open/Close", Test_OpenClose },
	{ "Append (+mixing/replacing)", Test_AppendMix },
	{ "Rebuild events", Test_RebuildEvents },
	{ "Animation requests", Test_AnimationRequest },
	{ "Element reset", Test_ElementReset },
	{ "SubUI", Test_SubUI },
	{ "High element count", Test_HighElementCount },
	{ "Zero-rebuild", Test_ZeroRebuild },
	{ "Global events", Test_GlobalEvents },
	{},
	{ "- Frames and windows -" },
	{ "Frames", Test_Frames },
	{ "Dialog window", Test_DialogWindow },
};
static const TestEntry layoutTestEntries[] =
{
	{ "Edge slice", Test_EdgeSlice },
	{ "Stacking layout variations", Test_StackingLayoutVariations },
	{ "Sizing", Test_Size },
	{ "Placement", Test_Placement },
};
static const TestEntry compoundTestEntries[] =
{
	{ "- Compound events -" },
	{ "Drag and drop", Test_DragDrop },
	{},
	{ "- Advanced/compound UI -" },
	{ "State buttons", Test_StateButtons },
	{ "Property list", Test_PropertyList },
	{ "Sliders", Test_Sliders },
	{ "Split pane", Test_SplitPane },
	{ "Tabs", Test_Tabs },
	{ "Transform container", Test_TransformContainer },
	{ "Scrollbars", Test_Scrollbars },
	{ "Tooltip", Test_Tooltip },
	{ "Dropdown", Test_Dropdown },
	{ "Textbox", Test_Textbox },
	{},
	{ "Color block", Test_ColorBlock },
	{ "Image", Test_Image },
	{ "Color picker", Test_ColorPicker },
	{},
	{ "IMGUI test", Test_IMGUI },
	{ "Modal window test", Test_ModalWindow },
	{ "Docking test", Test_Docking },
};
static const TestEntry _3dTestEntries[] =
{
	{ "3D view", Test_3DView },
	{ "Gizmo", Test_Gizmo },
	{ "Quaternion", Test_Quaternion },
};
static const TestEntry utilityTestEntries[] =
{
	{ "- Animation -" },
	{ "Basic easing", Test_BasicEasingAnim },
	{ "Vertical expand", Test_VExpandAnim },
	{},
	{ "- Threading -" },
	{ "Thread worker", Test_ThreadWorker },
	{ "Threaded image rendering", Test_ThreadedImageRendering },
	{},
	{ "Fullscreen", Test_Fullscreen },
	{ "OS communication", Test_OSCommunication },
	{ "System directory paths", Test_SysDirPaths },
	{ "File selection window", Test_FileSelectionWindow },
	{ "Directory change watcher", Test_DirectoryChangeWatcher },
	{},
	{ "Serialization speed", Test_SerializationSpeed },
	{ "Config tweakable", Test_ConfigTweakable },
	{ "Tweakable values", Test_TweakableValues },
};
static const TestEntry dseditTestEntries[] =
{
	{ "- Non-virtualized -" },
	{ "Sequence editors", Test_SequenceEditors },
	{ "Tree editors", Test_TreeEditors },
	{},
	{ "- Partially virtualized (real UI, visible items only) -" },
	{ "Node graph", Demo_NodeGraphEditor },
	{},
	{ "- Fully virtualized (single UI element for all items) -" },
	{ "Table view", Test_TableView },
	{ "Tree view", Test_TreeView },
	{ "File tree view", Test_FileTreeView },
	{ "Message log view", Test_MessageLogView },
	{ "Curve editor", Test_CurveEditor },
};
static const TestEntry benchmarkEntries[] =
{
	{ "SubUI", Benchmark_SubUI },
	{ "Build many elements", Benchmark_BuildManyElements },
};
static const TestEntry demoEntries[] =
{
	{ "- Layout -" },
	{ "Calculator (grid layout)", Demo_Calculator },
	{},
	{ "- Editors -" },
	{ "Settings window", Demo_SettingsWindow },
	{ "Tree", Demo_BasicTreeNodeEdit },
	{ "Inline tree", Demo_CompactTreeNodeEdit },
	{ "Script tree", Demo_ScriptTree },
	{ "Animated pattern editor", Demo_AnimatedPatternEditor },
	{ "Track editor demo", Demo_TrackEditor },
	{},
	{ "- Animations -" },
	{ "Sliding highlight anim", Demo_SlidingHighlightAnim },
	{ "Button press highlight", Demo_ButtonPressHighlight },
	{ "Fancy button", Demo_FancyButton },
	// TODO fix/redistribute
	//{ "Data editor", []() { ui::Make<DataEditor>(); } },
};

struct ExampleGroup
{
	const char* name;
	ui::ArrayView<TestEntry> entries;
};
static ExampleGroup exampleGroups[] =
{
	{ "Core", coreTestEntries },
	{ "Layout", layoutTestEntries },
	{ "Compound", compoundTestEntries },
	{ "3D", _3dTestEntries },
	{ "Utility", utilityTestEntries },
	{ "DS Edit", dseditTestEntries },
	{ "Benchmark", benchmarkEntries },
	{ "Demo", demoEntries },
};

static bool rebuildAlways;
struct TEST : ui::Buildable
{
	decltype(ui::OnWindowKeyEvent)::Entry* wkeRef;
	TEST()
	{
		wkeRef = ui::OnWindowKeyEvent.Add([this](const ui::WindowKeyEvent& e) { OnKeyEvent(e); });
	}
	~TEST()
	{
		ui::OnWindowKeyEvent.Remove(wkeRef);
	}
	void OnKeyEvent(const ui::WindowKeyEvent& e)
	{
		if (e.down)
		{
			if (e.physicalKey == ui::KSC_F6)
			{
				scalePercent += 10;
				enableScale = true;
				Rebuild();
			}
			if (e.physicalKey == ui::KSC_F7)
			{
				if (scalePercent > 10)
					scalePercent -= 10;
				enableScale = true;
				Rebuild();
			}
			if (e.physicalKey == ui::KSC_F8)
			{
				enableMenu ^= true;
				Rebuild();
			}
		}
	}
	void Build() override
	{
#if 1
		ui::Array<ui::MenuItem> rootMenu;

		for (const auto& group : exampleGroups)
		{
			rootMenu.Append(ui::MenuItem(group.name));
			for (auto& entry : group.entries)
			{
				if (!entry.func)
				{
					if (entry.name)
						rootMenu.Last().submenu.Append(ui::MenuItem(entry.name, {}, true));
					else
						rootMenu.Last().submenu.Append(ui::MenuItem::Separator());
					continue;
				}

				auto fn = [this, &entry]()
				{
					curTest = &entry;
					GetNativeWindow()->SetTitle(entry.name);
					Rebuild();
				};
				rootMenu.Last().submenu.Append(ui::MenuItem(entry.name, {}, false, curTest == &entry).Func(fn));
			}
		}

		rootMenu.Append(ui::MenuItem("Debug"));
		{
			rootMenu.Last().submenu.Append(ui::MenuItem("Rebuild always", {}, false, rebuildAlways).Func([this]() { rebuildAlways ^= true; Rebuild(); }));
			rootMenu.Last().submenu.Append(ui::MenuItem("Add wrappers", {}, false, GetWrapperSetting()).Func([this]() { GetWrapperSetting() ^= true; Rebuild(); }));
			rootMenu.Last().submenu.Append(ui::MenuItem("Draw rectangles", {}, false, GetNativeWindow()->IsDebugDrawEnabled()).Func([this]() {
				auto* w = GetNativeWindow(); w->SetDebugDrawEnabled(!w->IsDebugDrawEnabled()); Rebuild(); }));

			rootMenu.Last().submenu.Append(ui::MenuItem::Separator());

			rootMenu.Last().submenu.Append(ui::MenuItem("Enable scale", {}, false, enableScale).Func([this]() {
				enableScale = !enableScale; Rebuild(); }));
			rootMenu.Last().submenu.Append(ui::MenuItem(ui::Format("Scale: %d%%", scalePercent), {}, true));
			rootMenu.Last().submenu.Append(ui::MenuItem("Increase scale by 10%", "F6").Func([this]() {
				scalePercent += 10; enableScale = true; Rebuild(); }));
			rootMenu.Last().submenu.Append(ui::MenuItem("Decrease scale by 10%", "F7").Func([this]() {
				if (scalePercent > 10) scalePercent -= 10; enableScale = true; Rebuild(); }));
		}

		if (enableMenu)
			UI_BUILD_ALLOC(ui::TopMenu)(GetNativeWindow(), rootMenu);
		else
			GetNativeWindow()->SetMenu(nullptr);
#else
		ui::Push<ui::MenuBarElement>();

		for (const auto& group : exampleGroups)
		{
			ui::Push<ui::MenuItemElement>().SetText(group.name);
			for (auto& entry : group.entries)
			{
				if (!entry.func)
				{
					if (entry.name)
						ui::Make<ui::MenuItemElement>().SetText(entry.name).SetDisabled(true);
					else
						ui::Make<ui::MenuSeparatorElement>();
					continue;
				}

				auto fn = [this, &entry]()
				{
					curTest = &entry;
					GetNativeWindow()->SetTitle(entry.name);
					Rebuild();
				};
				ui::Make<ui::MenuItemElement>().SetText(entry.name).SetChecked(curTest == &entry).onActivate = fn;
			}
			ui::Pop();
		}

		ui::Push<ui::MenuItemElement>().SetText("Debug");
		{
			ui::Make<ui::MenuItemElement>().SetText("Rebuild always").SetChecked(rebuildAlways).onActivate = [this]() { rebuildAlways ^= true; Rebuild(); };
			ui::Make<ui::MenuItemElement>().SetText("Dump layout").onActivate = [this]() { DumpLayout(lastChild); };
			ui::Make<ui::MenuItemElement>().SetText("Draw rectangles").SetChecked(GetNativeWindow()->IsDebugDrawEnabled()).onActivate = [this]() {
				auto* w = GetNativeWindow(); w->SetDebugDrawEnabled(!w->IsDebugDrawEnabled()); Rebuild(); };
		}
		ui::Pop();

		ui::Pop();
#endif

		if (enableScale)
		{
			auto& csoe = ui::Push<ui::ChildScaleOffsetElement>();
			csoe.transform.scale = scalePercent / 100.0f;
		}

		if (curTest)
			curTest->func();

		if (enableScale)
			ui::Pop();

		if (rebuildAlways)
			Rebuild();
	}

	const TestEntry* curTest = nullptr;

	bool enableScale = false;
	int scalePercent = 100;
	bool enableMenu = true;
};



struct RHIListener : ui::rhi::IRHIListener
{
	static void PrintPointers(const char* when, const ui::rhi::RHIInternalPointers& ip)
	{
		printf("RHI %s device=%p context=%p window=%p swapChain=%p RTV=%p DSV=%p\n",
			when, ip.device, ip.context, ip.window, ip.swapChain, ip.renderTargetView, ip.depthStencilView);
	}

	void OnAttach(const ui::rhi::RHIInternalPointers& ip) override
	{
		PrintPointers("attach", ip);
	}
	void OnDetach(const ui::rhi::RHIInternalPointers& ip) override
	{
		PrintPointers("detach", ip);
	}
	void OnAfterInitSwapChain(const ui::rhi::RHIInternalPointers& ip) override
	{
		PrintPointers("after init swapchain", ip);
	}
	void OnBeforeFreeSwapChain(const ui::rhi::RHIInternalPointers& ip) override
	{
		PrintPointers("before free swapchain", ip);
	}
	void OnBeginFrame(const ui::rhi::RHIInternalPointers& ip) override
	{
		//PrintPointers("begin frame", ip);
	}
	void OnEndFrame(const ui::rhi::RHIInternalPointers& ip) override
	{
		//PrintPointers("end frame", ip);
	}
}
g_rl;


struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetContents(ui::CreateUIObject<TEST>(), true);
	}
	void OnFocusReceived() override
	{
		puts("FOCUS - received");
	}
	void OnFocusLost() override
	{
		puts("FOCUS - lost");
	}
};

int uimain(int argc, char* argv[])
{
	ui::LOG_UISYS.level = ui::LogLevel::All;

	ui::rhi::AttachListener(&g_rl);
	ui::Application app(argc, argv);
	ui::FSGetDefault()->fileSystems.Append(ui::CreateFileSystemSource("gui-tests/rsrc"));
	MainWindow mw;
	mw.SetInnerSize(640, 480);
	mw.SetVisible(true);
	return app.Run();
}
