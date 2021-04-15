
#include "pch.h"
#include "../Render/RHI.h"


ui::DataCategoryTag DCT_ItemSelection[1];
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
		ItemButton()
		{
			GetStyle().SetLayout(ui::layouts::Stack());
			//GetStyle().SetMargin(32);
		}
		void Build() override
		{
			static int wat = 0;
			auto& b = ui::Push<ui::Button>();
			b.GetStyle().SetWidth(ui::Coord::Percent(100));
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
		ui::Push<ui::MenuBarElement>();
		{
			ui::Push<ui::MenuItemElement>().SetText("File");
			{
				ui::Make<ui::MenuItemElement>().SetText("New", "Ctrl+N");
				ui::Make<ui::MenuItemElement>().SetText("Open", "Ctrl+O");
				ui::Make<ui::MenuItemElement>().SetText("Save", "Ctrl+S");
				ui::Make<ui::MenuSeparatorElement>();
				ui::Make<ui::MenuItemElement>().SetText("Quit", "Ctrl+Q");
			}
			ui::Pop();
			ui::Push<ui::MenuItemElement>().SetText("Help");
			{
				ui::Make<ui::MenuItemElement>().SetText("Documentation", "F1");
				ui::Make<ui::MenuItemElement>().SetText("About");
			}
			ui::Pop();
		}
		ui::Pop();

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
		auto* tbl = ui::Make<ui::TableView>();
		tbl->SetDataSource(&tblds);
		tbl->GetStyle().SetHeight(90);
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
		auto& trv = ui::Make<ui::TreeView>();
		trv.SetDataSource(&treeds);
		trv.GetStyle().SetHeight(90);
#endif

#if 1
		auto& nw = ui::Make<ui::NativeWindowNode>();
		nw.GetWindow()->SetTitle("Subwindow A");
		auto buildFunc = []()
		{
			ui::Push<ui::Panel>();
			auto onClick = []()
			{
				struct DialogTest : ui::NativeDialogWindow
				{
					DialogTest()
					{
						SetStyle(ui::WS_Resizable);
					}
					void OnBuild() override
					{
						auto s = ui::Push<ui::Panel>().GetStyle();
						s.SetLayout(ui::layouts::Stack());
						s.SetStackingDirection(ui::StackingDirection::RightToLeft);
						if (ui::imm::Button("X"))
							OnClose();
						if (ui::imm::Button("[]"))
							SetState(GetState() == ui::WindowState::Maximized ? ui::WindowState::Normal : ui::WindowState::Maximized);
						if (ui::imm::Button("_"))
							SetState(ui::WindowState::Minimized);
						ui::Pop();

						ui::Push<ui::Panel>();
						ui::Make<ItemButton>().Init("Test", [this]() { OnClose(); });
						ui::Pop();
					}
				} dlg;
				dlg.SetTitle("Dialog!");
				dlg.Show();
			};
			ui::Make<ItemButton>().Init("Only a button", []() {});
			ui::Make<ItemButton>().Init("Only another button (dialog)", onClick);
			ui::Pop();
		};
		nw.GetWindow()->SetBuildFunc(buildFunc);
		nw.GetWindow()->SetVisible(true);
#endif

#if 1
		auto& frm = ui::Make<ui::InlineFrame>();
		auto frf = []()
		{
			ui::Push<ui::Panel>();
			ui::MakeWithText<ui::Button>("In-frame button");
			static int cur = 1;
			BasicRadioButton("Zero", cur, 0);
			BasicRadioButton2("One", cur, 1);
			BasicRadioButton2("Two", cur, 2);
			ui::Pop();
		};
		frm.CreateFrameContents(frf);
		auto frs = frm.GetStyle();
		frs.SetPadding(4);
		frs.SetMinWidth(100);
		frs.SetMinHeight(100);
#endif

		static bool lbsel0 = false;
		static bool lbsel1 = true;
		ui::Push<ui::ListBox>();
		ui::Push<ui::ScrollArea>();
		ui::MakeWithText<ui::Selectable>("Item 1").Init(lbsel0);
		ui::MakeWithText<ui::Selectable>("Item two").Init(lbsel1);
		ui::Pop();
		ui::Pop();

		Subscribe(DCT_ItemSelection);
		if (editing == SIZE_MAX)
		{
			ui::Text("List");
			ui::Push<ui::Panel>();
			for (size_t i = 0; i < items.size(); i++)
			{
				ui::Make<ItemButton>().Init(items[i].name.c_str(), [this, i]() { editing = i; ui::Notify(DCT_ItemSelection); });
			}
			ui::Pop();

			auto& r1 = ui::Push<ui::CollapsibleTreeNode>();
			ui::Text("root item 1");
			if (r1.open)
			{
				ui::Text("- data under root item 1");
				auto& r1c1 = ui::Push<ui::CollapsibleTreeNode>();
				ui::Text("child 1");
				if (r1c1.open)
				{
					auto& r1c1c1 = ui::Push<ui::CollapsibleTreeNode>();
					ui::Text("subchild 1");
					ui::Pop();
				}
				ui::Pop();

				auto& r1c2 = ui::Push<ui::CollapsibleTreeNode>();
				ui::Text("child 2");
				if (r1c2.open)
				{
					auto& r1c2c1 = ui::Push<ui::CollapsibleTreeNode>();
					ui::Text("subchild 1");
					ui::Pop();
				}
				ui::Pop();
			}
			ui::Pop();

			auto& r2 = ui::Push<ui::CollapsibleTreeNode>();
			ui::Text("root item 2");
			if (r2.open)
			{
				ui::Text("- data under root item 2");
			}
			ui::Pop();
		}
		else
		{
			ui::PushBox()
				+ ui::SetLayout(ui::layouts::StackExpand())
				+ ui::Set(ui::StackingDirection::LeftToRight);
			ui::Text("Item:") + ui::SetPadding(5) + ui::SetWidth(ui::Coord::Fraction(0));
			ui::Text(items[editing].name.c_str());
			if (ui::imm::Button("Go back", { ui::SetWidth(ui::Coord::Fraction(0)) }))
			{
				editing = SIZE_MAX;
				ui::Notify(DCT_ItemSelection);
			}
			ui::Pop();

			ui::imm::PropEditString("Name", items[editing].name.c_str(), [&](const char* v) { items[editing].name = v; });
			ui::imm::PropEditBool("Enable", items[editing].enable);
		}
	}

	size_t editing = SIZE_MAX;
	std::vector<Item> items =
	{
		{ "test item 1", 5, true, 36.6f },
		{ "another test item", 123, false, 12.3f },
		{ "third item", 333, true, 3.0f },
	};
	ui::Menu* topMenu;
};


void Test_RenderingPrimitives();
void Test_KeyboardEvents();
void Test_OpenClose();
void Test_AnimationRequest();
void Test_ElementReset();
void Test_SubUI();
void Test_HighElementCount();
void Test_ZeroRebuild();
void Test_GlobalEvents();
void Test_Frames();
void Test_DialogWindow();

void Test_EdgeSlice();
void Test_LayoutNestCombo();
void Test_StackingLayoutVariations();
void Test_Size();
void Test_Placement();

void Test_DragDrop();
void Test_StateButtons();
void Test_PropertyList();
void Test_Sliders();
void Test_SplitPane();
void Test_Tabs();
void Test_Scrollbars();
void Test_Tooltip();
void Test_Dropdown();
void Test_ColorBlock();
void Test_Image();
void Test_ColorPicker();
void Test_IMGUI();

void Test_3DView();
void Test_Gizmo();
void Test_Quaternion();

void Test_BasicEasingAnim();
void Test_ThreadWorker();
void Test_ThreadedImageRendering();
void Test_OSCommunication();
void Test_FileSelectionWindow();

void Test_SequenceEditors();
void Test_TreeEditors();
void Test_MessageLogView();

void Benchmark_SubUI();
void Test_TableView();

void Demo_Calculator();
void Demo_SettingsWindow();
void Demo_BasicTreeNodeEdit();
void Demo_CompactTreeNodeEdit();
void Demo_ScriptTree();
void Demo_NodeGraphEditor();
void Demo_TrackEditor();
void Demo_SlidingHighlightAnim();
void Demo_ButtonPressHighlight();


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
	{ "- Events -" },
	{ "Keyboard", Test_KeyboardEvents },
	{},
	{ "- Basic logic -" },
	{ "Open/Close", Test_OpenClose },
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
	{ "Layout nesting combos", Test_LayoutNestCombo },
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
	{ "Scrollbars", Test_Scrollbars },
	{ "Tooltip", Test_Tooltip },
	{ "Dropdown", Test_Dropdown },
	{},
	{ "Color block", Test_ColorBlock },
	{ "Image", Test_Image },
	{ "Color picker", Test_ColorPicker },
	{},
	{ "IMGUI test", Test_IMGUI },
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
	{},
	{ "- Threading -" },
	{ "Thread worker", Test_ThreadWorker },
	{ "Threaded image rendering", Test_ThreadedImageRendering },
	{},
	{ "OS communication", Test_OSCommunication },
	{ "File selection window", Test_FileSelectionWindow },
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
	{ "Message log view", Test_MessageLogView },
};
static const TestEntry benchmarkEntries[] =
{
	{ "SubUI benchmark", Benchmark_SubUI },
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
	{ "Track editor demo", Demo_TrackEditor },
	{},
	{ "- Animations -" },
	{ "Sliding highlight anim", Demo_SlidingHighlightAnim },
	{ "Button press highlight", Demo_ButtonPressHighlight },
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
	void Build() override
	{
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

		if (curTest)
			curTest->func();

		if (rebuildAlways)
			Rebuild();
	}

	static const char* cln(const char* s)
	{
		auto* ss = strchr(s, ' ');
		if (ss)
			s = ss + 1;
		if (strncmp(s, "ui::layouts::", sizeof("ui::layouts::") - 1) == 0)
			s += sizeof("ui::layouts::") - 1;
		return s;
	}
	static const ui::ILayout* glo(UIObject* o)
	{
		auto* lo = o->GetStyle().GetLayout();
		return lo ? lo : ui::layouts::Stack();
	}
	static const char* dir(const ui::StyleAccessor& a)
	{
		static const char* names[] = { "-", "Inh", "T-B", "R-L", "B-T", "L-R" };
		return names[(int)a.GetStackingDirection()];
	}
	static const char* ctu(ui::CoordTypeUnit u)
	{
		static const char* names[] = { "undefined", "inherit", "auto", "px", "%", "fr" };
		return names[(int)u];
	}
	static const char* costr(const ui::Coord& c)
	{
		static char buf[128];
		auto* us = ctu(c.unit);
		if (c.unit == ui::CoordTypeUnit::Undefined ||
			c.unit == ui::CoordTypeUnit::Inherit ||
			c.unit == ui::CoordTypeUnit::Auto)
			return us;
		snprintf(buf, 128, "%g%s", c.value, us);
		return buf;
	}
	static void DumpLayout(UIObject* o, int lev = 0)
	{
		for (int i = 0; i < lev; i++)
			printf("  ");
		auto* lo = glo(o);
		printf("%s  -%s", cln(typeid(*o).name()), cln(typeid(*lo).name()));
		if (lo == ui::layouts::Stack() || lo == ui::layouts::StackExpand())
			printf(":%s", dir(o->GetStyle()));
		printf(" w=%s", costr(o->GetStyle().GetWidth()));
		printf(" h=%s", costr(o->GetStyle().GetHeight()));
		auto cr = o->GetContentRect();
		printf(" [%g;%g - %g;%g]", cr.x0, cr.y0, cr.x1, cr.y1);
		puts("");

		for (auto* ch = o->firstChild; ch; ch = ch->next)
		{
			DumpLayout(ch, lev + 1);
		}
	}

	const TestEntry* curTest = nullptr;
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
		PrintPointers("begin frame", ip);
	}
	void OnEndFrame(const ui::rhi::RHIInternalPointers& ip) override
	{
		PrintPointers("end frame", ip);
	}
}
g_rl;



void EarlyTest()
{
	puts("done");
}

struct MainWindow : ui::NativeMainWindow
{
	void OnBuild() override
	{
		ui::Make<TEST>();
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
	//EarlyTest();
	ui::rhi::AttachListener(&g_rl);
	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
