
#include "pch.h"


ui::DataCategoryTag DCT_ItemSelection[1];
struct DataEditor : ui::Node
{
	struct Item
	{
		std::string name;
		int value;
		bool enable;
		float value2;
	};

	struct ItemButton : ui::Node
	{
		ItemButton()
		{
			GetStyle().SetLayout(style::layouts::Stack());
			//GetStyle().SetMargin(32);
		}
		void Render(UIContainer* ctx) override
		{
			static int wat = 0;
			auto* b = ctx->Push<ui::Button>();
			b->GetStyle().SetWidth(style::Coord::Percent(100));
			//b->SetInputDisabled(wat++ % 2);
			ctx->Text(name);
			ctx->Pop();
		}
		void OnEvent(UIEvent& e) override
		{
			if (e.type == UIEventType::Activate)
			{
				callback();
			}
			if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Right)
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

	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::MenuBarElement>();
		{
			ctx->Push<ui::MenuItemElement>()->SetText("File");
			{
				ctx->Make<ui::MenuItemElement>()->SetText("New", "Ctrl+N");
				ctx->Make<ui::MenuItemElement>()->SetText("Open", "Ctrl+O");
				ctx->Make<ui::MenuItemElement>()->SetText("Save", "Ctrl+S");
				ctx->Make<ui::MenuSeparatorElement>();
				ctx->Make<ui::MenuItemElement>()->SetText("Quit", "Ctrl+Q");
			}
			ctx->Pop();
			ctx->Push<ui::MenuItemElement>()->SetText("Help");
			{
				ctx->Make<ui::MenuItemElement>()->SetText("Documentation", "F1");
				ctx->Make<ui::MenuItemElement>()->SetText("About");
			}
			ctx->Pop();
		}
		ctx->Pop();

		ctx->MakeWithText<ui::ProgressBar>("Processing...")->progress = 0.37f;
		static float sldval = 0.63f;
		ctx->Make<ui::Slider>()->Init(sldval, { 0, 2, 0.1 });

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
		auto* tbl = ctx->Make<ui::TableView>();
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
		auto* trv = ctx->Make<ui::TreeView>();
		trv->SetDataSource(&treeds);
		trv->GetStyle().SetHeight(90);
#endif

#if 1
		auto* nw = ctx->Make<ui::NativeWindowNode>();
		nw->GetWindow()->SetTitle("Subwindow A");
		auto renderFunc = [](UIContainer* ctx)
		{
			ctx->Push<ui::Panel>();
			auto onClick = []()
			{
				struct DialogTest : ui::NativeDialogWindow
				{
					DialogTest()
					{
						SetStyle(ui::WS_Resizable);
					}
					void OnRender(UIContainer* ctx) override
					{
						auto s = ctx->Push<ui::Panel>()->GetStyle();
						s.SetLayout(style::layouts::Stack());
						s.SetStackingDirection(style::StackingDirection::RightToLeft);
						if (ui::imm::Button(ctx, "X"))
							OnClose();
						if (ui::imm::Button(ctx, "[]"))
							SetState(GetState() == ui::WindowState::Maximized ? ui::WindowState::Normal : ui::WindowState::Maximized);
						if (ui::imm::Button(ctx, "_"))
							SetState(ui::WindowState::Minimized);
						ctx->Pop();

						ctx->Push<ui::Panel>();
						ctx->Make<ItemButton>()->Init("Test", [this]() { OnClose(); });
						ctx->Pop();
					}
				} dlg;
				dlg.SetTitle("Dialog!");
				dlg.Show();
			};
			ctx->Make<ItemButton>()->Init("Only a button", []() {});
			ctx->Make<ItemButton>()->Init("Only another button (dialog)", onClick);
			ctx->Pop();
		};
		nw->GetWindow()->SetRenderFunc(renderFunc);
		nw->GetWindow()->SetVisible(true);
#endif

#if 1
		auto* frm = ctx->Make<ui::InlineFrameNode>();
		auto frf = [](UIContainer* ctx)
		{
			ctx->Push<ui::Panel>();
			ctx->MakeWithText<ui::Button>("In-frame button");
			static int cur = 1;
			BasicRadioButton(ctx, "Zero", cur, 0);
			BasicRadioButton2(ctx, "One", cur, 1);
			BasicRadioButton2(ctx, "Two", cur, 2);
			ctx->Pop();
		};
		frm->CreateFrameContents(frf);
		auto frs = frm->GetStyle();
		frs.SetPadding(4);
		frs.SetMinWidth(100);
		frs.SetMinHeight(100);
#endif

		static bool lbsel0 = false;
		static bool lbsel1 = true;
		ctx->Push<ui::ListBox>();
		ctx->Push<ui::ScrollArea>();
		ctx->MakeWithText<ui::Selectable>("Item 1")->Init(lbsel0);
		ctx->MakeWithText<ui::Selectable>("Item two")->Init(lbsel1);
		ctx->Pop();
		ctx->Pop();

		Subscribe(DCT_ItemSelection);
		if (editing == SIZE_MAX)
		{
			ctx->Text("List");
			ctx->Push<ui::Panel>();
			for (size_t i = 0; i < items.size(); i++)
			{
				ctx->Make<ItemButton>()->Init(items[i].name.c_str(), [this, i]() { editing = i; ui::Notify(DCT_ItemSelection); });
			}
			ctx->Pop();

			auto* r1 = ctx->Push<ui::CollapsibleTreeNode>();
			ctx->Text("root item 1");
			if (r1->open)
			{
				ctx->Text("- data under root item 1");
				auto* r1c1 = ctx->Push<ui::CollapsibleTreeNode>();
				ctx->Text("child 1");
				if (r1c1->open)
				{
					auto* r1c1c1 = ctx->Push<ui::CollapsibleTreeNode>();
					ctx->Text("subchild 1");
					ctx->Pop();
				}
				ctx->Pop();

				auto* r1c2 = ctx->Push<ui::CollapsibleTreeNode>();
				ctx->Text("child 2");
				if (r1c2->open)
				{
					auto* r1c2c1 = ctx->Push<ui::CollapsibleTreeNode>();
					ctx->Text("subchild 1");
					ctx->Pop();
				}
				ctx->Pop();
			}
			ctx->Pop();

			auto* r2 = ctx->Push<ui::CollapsibleTreeNode>();
			ctx->Text("root item 2");
			if (r2->open)
			{
				ctx->Text("- data under root item 2");
			}
			ctx->Pop();
		}
		else
		{
			ctx->PushBox()
				+ ui::Layout(style::layouts::StackExpand())
				+ ui::StackingDirection(style::StackingDirection::LeftToRight);
			ctx->Text("Item:") + ui::Padding(5) + ui::Width(style::Coord::Fraction(0));
			ctx->Text(items[editing].name.c_str());
			if (ui::imm::Button(ctx, "Go back", { ui::Width(style::Coord::Fraction(0)) }))
			{
				editing = SIZE_MAX;
				ui::Notify(DCT_ItemSelection);
			}
			ctx->Pop();

			ui::imm::PropEditString(ctx, "Name", items[editing].name.c_str(), [&](const char* v) { items[editing].name = v; });
			ui::imm::PropEditBool(ctx, "Enable", items[editing].enable);
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


void Test_RenderingPrimitives(UIContainer* ctx);
void Test_OpenClose(UIContainer* ctx);
void Test_AnimationRequest(UIContainer* ctx);
void Test_ElementReset(UIContainer* ctx);
void Test_SubUI(UIContainer* ctx);
void Test_HighElementCount(UIContainer* ctx);
void Test_ZeroRerender(UIContainer* ctx);
void Test_GlobalEvents(UIContainer* ctx);
void Test_DialogWindow(UIContainer* ctx);

void Test_EdgeSlice(UIContainer* ctx);
void Test_LayoutNestCombo(UIContainer* ctx);
void Test_StackingLayoutVariations(UIContainer* ctx);
void Test_Size(UIContainer* ctx);
void Test_Placement(UIContainer* ctx);

void Test_DragDrop(UIContainer* ctx);
void Test_StateButtons(UIContainer* ctx);
void Test_Sliders(UIContainer* ctx);
void Test_SplitPane(UIContainer* ctx);
void Test_Tabs(UIContainer* ctx);
void Test_Scrollbars(UIContainer* ctx);
void Test_ColorBlock(UIContainer* ctx);
void Test_Image(UIContainer* ctx);
void Test_ColorPicker(UIContainer* ctx);
void Test_3DView(UIContainer* ctx);
void Test_IMGUI(UIContainer* ctx);
void Test_Tooltip(UIContainer* ctx);
void Test_Dropdown(UIContainer* ctx);

void Test_BasicEasingAnim(UIContainer* ctx);
void Test_ThreadWorker(UIContainer* ctx);
void Test_ThreadedImageRendering(UIContainer* ctx);
void Test_OSCommunication(UIContainer* ctx);

void Test_SequenceEditors(UIContainer* ctx);
void Test_TreeEditors(UIContainer* ctx);
void Test_MessageLogView(UIContainer* ctx);

void Benchmark_SubUI(UIContainer* ctx);
void Test_TableView(UIContainer* ctx);

void Demo_Calculator(UIContainer* ctx);
void Demo_BasicTreeNodeEdit(UIContainer* ctx);
void Demo_CompactTreeNodeEdit(UIContainer* ctx);
void Demo_ScriptTree(UIContainer* ctx);
void Demo_NodeGraphEditor(UIContainer* ctx);
void Demo_TrackEditor(UIContainer* ctx);
void Demo_SlidingHighlightAnim(UIContainer* ctx);
void Demo_ButtonPressHighlight(UIContainer* ctx);


struct TestEntry
{
	const char* name;
	void(*func)(UIContainer* ctx);
};
static const TestEntry coreTestEntries[] =
{
	{ "Off", [](UIContainer* ctx) {} },
	{},
	{ "- Rendering -" },
	{ "Primitives", Test_RenderingPrimitives },
	{},
	{ "- Basic logic -" },
	{ "Open/Close", Test_OpenClose },
	{ "Animation requests", Test_AnimationRequest },
	{ "Element reset", Test_ElementReset },
	{ "SubUI", Test_SubUI },
	{ "High element count", Test_HighElementCount },
	{ "Zero-rerender", Test_ZeroRerender },
	{ "Global events", Test_GlobalEvents },
	{},
	{ "- Windows -" },
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
	{ "3D view", Test_3DView },
	{},
	{ "IMGUI test", Test_IMGUI },
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
	{ "OS Communication", Test_OSCommunication },
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
	{ "Tree", Demo_BasicTreeNodeEdit },
	{ "Inline tree", Demo_CompactTreeNodeEdit },
	{ "Script tree", Demo_ScriptTree },
	{ "Track editor demo", Demo_TrackEditor },
	{},
	{ "- Animations -" },
	{ "Sliding highlight anim", Demo_SlidingHighlightAnim },
	{ "Button press highlight", Demo_ButtonPressHighlight },
	// TODO fix/redistribute
	//{ "Data editor", [](UIContainer* ctx) { ctx->Make<DataEditor>(); } },
};

struct ExampleGroup
{
	const char* name;
	ArrayView<TestEntry> entries;
};
static ExampleGroup exampleGroups[] =
{
	{ "Core", coreTestEntries },
	{ "Layout", layoutTestEntries },
	{ "Compound", compoundTestEntries },
	{ "Utility", utilityTestEntries },
	{ "DS Edit", dseditTestEntries },
	{ "Benchmark", benchmarkEntries },
	{ "Demo", demoEntries },
};

static bool rerenderAlways;
struct TEST : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::MenuBarElement>();

		for (const auto& group : exampleGroups)
		{
			ctx->Push<ui::MenuItemElement>()->SetText(group.name);
			for (auto& entry : group.entries)
			{
				if (!entry.func)
				{
					if (entry.name)
						ctx->Make<ui::MenuItemElement>()->SetText(entry.name).SetDisabled(true);
					else
						ctx->Make<ui::MenuSeparatorElement>();
					continue;
				}

				auto fn = [this, &entry]()
				{
					curTest = &entry;
					GetNativeWindow()->SetTitle(entry.name);
					Rerender();
				};
				ctx->Make<ui::MenuItemElement>()->SetText(entry.name).SetChecked(curTest == &entry).onActivate = fn;
			}
			ctx->Pop();
		}

		ctx->Push<ui::MenuItemElement>()->SetText("Debug");
		{
			ctx->Make<ui::MenuItemElement>()->SetText("Rerender always").SetChecked(rerenderAlways).onActivate = [this]() { rerenderAlways ^= true; Rerender(); };
			ctx->Make<ui::MenuItemElement>()->SetText("Dump layout").onActivate = [this]() { DumpLayout(lastChild); };
			ctx->Make<ui::MenuItemElement>()->SetText("Draw rectangles").SetChecked(GetNativeWindow()->IsDebugDrawEnabled()).onActivate = [this]() {
				auto* w = GetNativeWindow(); w->SetDebugDrawEnabled(!w->IsDebugDrawEnabled()); Rerender(); };
		}
		ctx->Pop();

		ctx->Pop();

		if (curTest)
			curTest->func(ctx);

		if (rerenderAlways)
			Rerender();
	}

	static const char* cln(const char* s)
	{
		auto* ss = strchr(s, ' ');
		if (ss)
			s = ss + 1;
		if (strncmp(s, "style::layouts::", sizeof("style::layouts::") - 1) == 0)
			s += sizeof("style::layouts::") - 1;
		return s;
	}
	static const style::Layout* glo(UIObject* o)
	{
		auto* lo = o->GetStyle().GetLayout();
		return lo ? lo : style::layouts::Stack();
	}
	static const char* dir(const style::Accessor& a)
	{
		static const char* names[] = { "-", "Inh", "T-B", "R-L", "B-T", "L-R" };
		return names[(int)a.GetStackingDirection()];
	}
	static const char* ctu(style::CoordTypeUnit u)
	{
		static const char* names[] = { "undefined", "inherit", "auto", "px", "%", "fr" };
		return names[(int)u];
	}
	static const char* costr(const style::Coord& c)
	{
		static char buf[128];
		auto* us = ctu(c.unit);
		if (c.unit == style::CoordTypeUnit::Undefined ||
			c.unit == style::CoordTypeUnit::Inherit ||
			c.unit == style::CoordTypeUnit::Auto)
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
		if (lo == style::layouts::Stack() || lo == style::layouts::StackExpand())
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



void EarlyTest()
{
	puts("done");
}

struct MainWindow : ui::NativeMainWindow
{
	void OnRender(UIContainer* ctx)
	{
		ctx->Make<TEST>();
	}
};

int uimain(int argc, char* argv[])
{
	//EarlyTest();

	ui::Application app(argc, argv);
	MainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
