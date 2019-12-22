
#include "pch.h"

#include "../GUI.h"


struct OpenClose : UINode
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>();

		ctx->Make<ui::Checkbox>()->Init(open)->onChange = [this]() { Rerender(); };

		ctx->MakeWithText<ui::Button>("Open")->onClick = [this]() { open = true; Rerender(); };

		ctx->MakeWithText<ui::Button>("Close")->onClick = [this]() { open = false; Rerender(); };

		if (open)
		{
			ctx->Push<ui::Panel>();
			ctx->Text("It is open!");
			ctx->Pop();
		}

		ctx->Pop();
	}

	bool open = false;
};


static const char* numberNames[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "." };
static const char* opNames[] = { "+", "-", "*", "/" };
struct Calculator : UINode
{
	void Render(UIContainer* ctx) override
	{
		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		{
			ctx->Make<ui::Textbox>()->text = operation;

			ctx->MakeWithText<ui::Button>("<")->onClick = [this]() { if (!operation.empty()) operation.pop_back(); Rerender(); };
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		for (int i = 0; i < 11; i++)
		{
			ctx->MakeWithText<ui::Button>(numberNames[i])->onClick = [this, i]() { AddChar(numberNames[i][0]); };
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		{
			for (int i = 0; i < 4; i++)
			{
				ctx->MakeWithText<ui::Button>(opNames[i])->onClick = [this, i]() { AddChar(opNames[i][0]); };
			}

			ctx->MakeWithText<ui::Button>("=")->onClick = [this]() { operation = ToString(Calculate()); Rerender(); };
		}
		ctx->Pop();

		ctx->Push<ui::Panel>()->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("=" + ToString(Calculate()));
		ctx->Pop();
	}

	void AddChar(char ch)
	{
		if (ch == '.')
		{
			// must be preceded by a number without a dot
			if (operation.empty())
				return;
			for (size_t i = operation.size(); i > 0; )
			{
				i--;

				if (operation[i] == '.')
					return;
				if (!isdigit(operation[i]))
					break;
			}
		}
		else if (ch == '+' || ch == '-' || ch == '*' || ch == '/')
		{
			// must be preceded by a number or a dot
			if (operation.empty())
				return;
			if (!isdigit(operation.back()) && operation.back() != '.')
				return;
		}
		operation.push_back(ch);
		Rerender();
	}

	int precedence(char op)
	{
		if (op == '*' || op == '/')
			return 2;
		if (op == '+' || op == '-')
			return 1;
		return 0;
	}
	void Apply(std::vector<double>& numqueue, char op)
	{
		double b = numqueue.back();
		numqueue.pop_back();
		double& a = numqueue.back();
		switch (op)
		{
		case '+': a += b; break;
		case '-': a -= b; break;
		case '*': a *= b; break;
		case '/': a /= b; break;
		}
	}
	double Calculate()
	{
		std::vector<double> numqueue;
		std::vector<char> opstack;
		bool lastwasop = false;
		for (size_t i = 0; i < operation.size(); i++)
		{
			char c = operation[i];
			if (isdigit(c))
			{
				char* end = nullptr;
				numqueue.push_back(std::strtod(operation.c_str() + i, &end));
				i = end - operation.c_str() - 1;
				lastwasop = false;
			}
			else
			{
				while (!opstack.empty() && precedence(opstack.back()) > precedence(c))
				{
					Apply(numqueue, opstack.back());
					opstack.pop_back();
				}
				opstack.push_back(c);
				lastwasop = true;
			}
		}
		if (lastwasop)
			opstack.pop_back();
		while (!opstack.empty() && numqueue.size() >= 2)
		{
			Apply(numqueue, opstack.back());
			opstack.pop_back();
		}
		return !numqueue.empty() ? numqueue.back() : 0;
	}

	std::string ToString(double val)
	{
		std::string s = std::to_string(val);
		while (!s.empty() && s.back() == '0')
			s.pop_back();
		if (s.empty())
			s.push_back('0');
		else if (s.back() == '.')
			s.pop_back();
		return s;
	}

	std::string operation;
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

#if 0
		auto* nw = ctx->Make<ui::NativeWindowNode>();
		nw->GetWindow()->SetTitle("Subwindow A");
		auto renderFunc = [](UIContainer* ctx)
		{
			ctx->Push<ui::Panel>();
			auto onClick = []()
			{
				ui::NativeDialogWindow dlg;
				auto render = [&dlg](UIContainer* ctx)
				{
					auto s = ctx->Push<ui::Panel>()->GetStyle();
					s.SetLayout(style::Layout::Stack);
					s.SetStackingDirection(style::StackingDirection::RightToLeft);
					ctx->MakeWithText<ui::Button>("X")->onClick = [&dlg]() { dlg.OnClose(); };
					ctx->MakeWithText<ui::Button>("[]")->onClick = [&dlg]() {
						dlg.SetState(dlg.GetState() == ui::WindowState::Maximized ? ui::WindowState::Normal : ui::WindowState::Maximized); };
					ctx->MakeWithText<ui::Button>("_")->onClick = [&dlg]() { dlg.SetState(ui::WindowState::Minimized); };
					ctx->Pop();

					ctx->Push<ui::Panel>();
					ctx->Make<ItemButton>()->Init("Test", [&dlg]() { dlg.OnClose(); });
					ctx->Pop();
				};
				dlg.SetRenderFunc(render);
				dlg.SetTitle("Dialog!");
				dlg.Show();
			};
			ctx->Make<ItemButton>()->Init("Only a button", []() {});
			ctx->Make<ItemButton>()->Init("Only another button (dialog)", onClick);
			ctx->Pop();
		};
		ctx->GetNativeWindow()->SetInnerUIEnabled(false);
		nw->GetWindow()->SetParent(ctx->GetNativeWindow());
		nw->GetWindow()->SetRenderFunc(renderFunc);
#endif

#if 1
		auto* frm = ctx->Make<ui::InlineFrameNode>();
		auto frf = [](UIContainer* ctx)
		{
			ctx->Push<ui::Panel>();
			ctx->MakeWithText<ui::Button>("In-frame button");
			static int cur = 1;
			ctx->Push<ui::RadioButtonT<int>>()->Init(cur, 0);
			ctx->Text("Zero");
			ctx->Pop();
			ctx->Push<ui::RadioButtonT<int>>()->Init(cur, 1)->SetStyle(ui::Theme::current->button);
			ctx->Text("One");
			ctx->Pop();
			ctx->Push<ui::RadioButtonT<int>>()->Init(cur, 2)->SetStyle(ui::Theme::current->button);
			ctx->Text("Two");
			ctx->Pop();
			ctx->Pop();
		};
		frm->CreateFrameContents(frf);
		auto frs = frm->GetStyle();
		frs.SetPadding(4);
		frs.SetMinWidth(100);
		frs.SetMinHeight(100);
#endif

		auto* tg = ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabList>();
			{
				ctx->MakeWithText<ui::TabButton>("First tab");
				ctx->MakeWithText<ui::TabButton>("Second tab");
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>()->SetVisible(tg->active == 0);
			{
				ctx->Text("Contents of the first tab");
			}
			ctx->Pop();

			ctx->Push<ui::TabPanel>()->SetVisible(tg->active == 1);
			{
				ctx->Text("Contents of the second tab");
			}
			ctx->Pop();
		}
		ctx->Pop();

		tg = ctx->Push<ui::TabGroup>();
		{
			ctx->Push<ui::TabList>();
			{
				ctx->MakeWithText<ui::TabButton>("First tab");
				ctx->MakeWithText<ui::TabButton>("Second tab");
			}
			ctx->Pop();

			if (tg->active == 0)
			{
				ctx->Push<ui::TabPanel>()->id = 0;
				{
					ctx->Text("Contents of the first tab");
				}
				ctx->Pop();
			}

			if (tg->active == 1)
			{
				ctx->Push<ui::TabPanel>()->id = 1;
				{
					ctx->Text("Contents of the second tab");
				}
				ctx->Pop();
			}
		}
		ctx->Pop();

		btnGoBack = nullptr;
		tbName = nullptr;
		if (editing == SIZE_MAX)
		{
			ctx->Text("List");
			ctx->Push<ui::Panel>();
			for (size_t i = 0; i < items.size(); i++)
			{
				ctx->Make<ItemButton>()->Init(items[i].name.c_str(), [this, i]() { editing = i; Rerender(); });
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
			auto* b = ctx->PushBox();
			b->GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
			ctx->Text("Item:")->GetStyle().SetPadding(5);
			ctx->Text(items[editing].name.c_str());
			btnGoBack = ctx->Push<ui::Button>();
			ctx->Text("Go back");
			ctx->Pop();
			ctx->Pop();

			Property::Make(ctx, "Name", [&]()
			{
				tbName = ctx->Make<ui::Textbox>();
				tbName->text = items[editing].name;
			});

			Property::Make(ctx, "Enable", [&]()
			{
				ctx->Make<ui::Checkbox>()->Init(items[editing].enable);//->SetInputDisabled(true);
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
	ui::Button* btnGoBack;
	ui::Textbox* tbName;
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
			ctx->Push<ui::RadioButtonT<int>>()->Init(curTest, i)->onChange = [this]() { Rerender(); };
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

	int curTest = 0;
};



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

struct MainWindow : ui::NativeMainWindow
{
	MainWindow()
	{
		SetRenderFunc([](UIContainer* ctx)
		{
			ctx->Make<DataEditor>();
			//ctx->Make<OpenClose>();
			//ctx->Make<TEST>();
		});
	}
};

int uimain(int argc, char* argv[])
{
	EarlyTest();

	ui::Application app(argc, argv);
	MainWindow mw;
	return app.Run();

	//uisys.Build<TEST>();
	//uisys.Build<FileStructureViewer>();
}
