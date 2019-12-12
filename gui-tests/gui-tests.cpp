
#include "pch.h"

#include "../GUI.h"


struct OpenClose : UINode
{
	void Render(UIContainer* ctx) override
	{
		ctx->PushBox();

		cb = ctx->Make<UICheckbox>()->SetValue(open);

		openBtn = ctx->Push<UIButton>();
		ctx->Text("Open");
		ctx->Pop();

		closeBtn = ctx->Push<UIButton>();
		ctx->Text("Close");
		ctx->Pop();

		if (open)
		{
			ctx->PushBox();
			ctx->Text("It is open!");
			ctx->Pop();
		}

		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Activate)
		{
			if (e.target == openBtn || e.target == closeBtn)
			{
				open = e.target == openBtn;
				Rerender();
			}
		}
		if (e.type == UIEventType::Change)
		{
			if (e.target == cb)
			{
				open = cb->GetValue();
				Rerender();
			}
		}
	}

	bool open = false;

	UICheckbox* cb;
	UIButton* openBtn;
	UIButton* closeBtn;
};


static const char* numberNames[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
struct Calculator : UINode
{
	void Render(UIContainer* ctx) override
	{
		for (int i = 0; i < 10; i++)
		{
			numberButtons[i] = ctx->Push<UIButton>();
			ctx->Text(numberNames[i]);
			ctx->Pop();
		}
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Activate)
		{
			for (int i = 0; i < 10; i++)
			{
				if (e.target == numberButtons[i])
				{
				}
			}
		}
	}

	UIButton* numberButtons[10];
};


struct DataEditor : UINode
{
	struct Item
	{
		std::string name;
		int value;
		bool enable;
		float value2;
	};

	struct ItemButton : UINode
	{
		ItemButton()
		{
			GetStyle().SetLayout(style::Layout::Stack);
			//GetStyle().SetMargin(32);
		}
		void Render(UIContainer* ctx) override
		{
			static int wat = 0;
			auto* b = ctx->Push<UIButton>();
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
				e.handled = true;
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

	struct Property : UIElement
	{
		Property()
		{
			GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		}
		static void Make(UIContainer* ctx, const char* label, std::function<void()> content)
		{
			ctx->Push<Property>();
			auto s = ctx->Text(label)->GetStyle();
			s.SetPadding(5);
			s.SetBoxSizing(style::BoxSizing::BorderBox);
			s.SetWidth(style::Coord::Percent(40));
			content();
			ctx->Pop();
		}
	};

	struct Checkbox : UINode
	{
		Checkbox* Init(bool& val)
		{
			at = &val;
			return this;
		}
		void Render(UIContainer* ctx) override
		{
			cb = ctx->Make<UICheckbox>()->SetValue(*at);
			cb->SetInputDisabled(IsInputDisabled());
		}
		void OnEvent(UIEvent& e) override
		{
			if (e.type == UIEventType::Change)
			{
				*at = cb->GetValue();
			}
		}
		void SetInputDisabled(bool v)
		{
			UINode::SetInputDisabled(v);
			if (cb)
				cb->SetInputDisabled(v);
		}
		UICheckbox* cb = nullptr;
		bool* at;
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

#if 1
		auto* nw = ctx->Push<ui::NativeWindow>();
		nw->SetTitle("Subwindow A");
		{
			ctx->Make<ItemButton>()->Init("Only a button", []() {});
		}
		ctx->Pop();
#endif

		btnGoBack = nullptr;
		tbName = nullptr;
		if (editing == SIZE_MAX)
		{
			ctx->Text("List");
			ctx->Push<UIPanel>();
			for (size_t i = 0; i < items.size(); i++)
			{
				ctx->Make<ItemButton>()->Init(items[i].name.c_str(), [this, i]() { editing = i; Rerender(); });
			}
			ctx->Pop();

			auto* r1 = ctx->Push<UICollapsibleTreeNode>();
			ctx->Text("root item 1");
			if (r1->open)
			{
				ctx->Text("- data under root item 1");
				auto* r1c1 = ctx->Push<UICollapsibleTreeNode>();
				ctx->Text("child 1");
				if (r1c1->open)
				{
					auto* r1c1c1 = ctx->Push<UICollapsibleTreeNode>();
					ctx->Text("subchild 1");
					ctx->Pop();
				}
				ctx->Pop();

				auto* r1c2 = ctx->Push<UICollapsibleTreeNode>();
				ctx->Text("child 2");
				if (r1c2->open)
				{
					auto* r1c2c1 = ctx->Push<UICollapsibleTreeNode>();
					ctx->Text("subchild 1");
					ctx->Pop();
				}
				ctx->Pop();
			}
			ctx->Pop();

			auto* r2 = ctx->Push<UICollapsibleTreeNode>();
			ctx->Text("root item 2");
			if (r2->open)
			{
				ctx->Text("- data under root item 2");
			}
			ctx->Pop();
		}
		else
		{
			auto* b = ctx->PushBox();
			b->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
			ctx->Text("Item:")->GetStyle().SetPadding(5);
			ctx->Text(items[editing].name.c_str());
			btnGoBack = ctx->Push<UIButton>();
			ctx->Text("Go back");
			ctx->Pop();
			ctx->Pop();

			Property::Make(ctx, "Name", [&]()
			{
				tbName = ctx->Make<UITextbox>();
				tbName->text = items[editing].name;
			});

			Property::Make(ctx, "Enable", [&]()
			{
				ctx->Make<Checkbox>()->Init(items[editing].enable);//->SetInputDisabled(true);
			});
		}
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Activate)
		{
			if (e.target->IsChildOrSame(btnGoBack))
			{
				editing = SIZE_MAX;
				Rerender();
			}
		}
		if (e.type == UIEventType::Commit)
		{
			if (e.target == tbName)
			{
				items[editing].name = tbName->text;
				Rerender();
			}
		}
	}

	size_t editing = SIZE_MAX;
	std::vector<Item> items =
	{
		{ "test item 1", 5, true, 36.6f },
		{ "another test item", 123, false, 12.3f },
		{ "third item", 333, true, 3.0f },
	};
	UIButton* btnGoBack;
	UITextbox* tbName;
	ui::Menu* topMenu;
};


static const char* testNames[] =
{
	"Test: Off",
	"Test: Open/Close",
	"Test: Calculator",
};
struct TEST : UINode
{
	void Render(UIContainer* ctx) override
	{
		ctx->PushBox();

		for (int i = 0; i < 3; i++)
		{
			testBtns[i] = ctx->Push<UIButton>();
			ctx->Text(testNames[i]);
			ctx->Pop();
		}

		ctx->Text("----------------");

		switch (curTest)
		{
		case 0: break;
		case 1: ctx->Make<OpenClose>(); break;
		case 2: ctx->Make<Calculator>(); break;
		}

		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Activate)
		{
			for (int i = 0; i < 3; i++)
			{
				if (e.target == testBtns[i])
				{
					curTest = i;
					Rerender();
					break;
				}
			}
		}
	}

	UIButton* testBtns[3];
	int curTest = 0;
};








size_t numAllocs = 0, numNew = 0, numDelete = 0;
void* operator new(size_t s)
{
	numAllocs++;
	numNew++;
	return malloc(s);
}
void operator delete(void* p)
{
	numAllocs--;
	numDelete++;
	free(p);
}

void dumpallocinfo()
{
	printf("- allocs:%u new:%u delete:%u\n", (unsigned)numAllocs, (unsigned)numNew, (unsigned)numDelete);
}



struct CSSBuildCallback : style::IErrorCallback
{
	void OnError(const char* msg, int line, int pos) override
	{
		printf("CSS BUILD ERROR [%d:%d]: %s\n", line, pos, msg);
	}
};

void EarlyTest()
{
	style::Sheet sh;
	CSSBuildCallback cb;
	sh.Compile("a#foo.bar:first-child, b:hover:active { display: inline; }", &cb);
	puts("done");
}

int uimain(int argc, char* argv[])
{
	EarlyTest();

#define DEFER(f) struct _defer_##__LINE__ { ~_defer_##__LINE__() { f; } } _defer_inst_##__LINE__
	DEFER(dumpallocinfo());

	ui::Application app(argc, argv);
	app.BuildWithWindow<DataEditor>();
	return app.Run();

	//uisys.Build<TEST>();
	//uisys.Build<DataEditor>();
	//uisys.Build<FileStructureViewer>();
}
