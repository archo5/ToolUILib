
#include "pch.h"


struct FileReceiverTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("File receiver");

		ui::Push<ui::ListBoxFrame>().HandleEvent(ui::EventType::DragDrop) = [this](ui::Event& e)
		{
			if (auto* ddd = static_cast<ui::DragDropFiles*>(ui::DragDrop::GetData(ui::DragDropFiles::NAME)))
			{
				filePaths = ddd->paths;
				Rebuild();
			}
		};
		ui::Push<ui::StackTopDownLayoutElement>();
		if (filePaths.IsEmpty())
		{
			ui::Text("Drop files here");
		}
		else
		{
			for (const auto& path : filePaths)
			{
				ui::Text(path);
			}
		}
		ui::Pop();
		ui::Pop();

		WPop();
	}

	ui::Array<std::string> filePaths;
};


struct TransferCountablesTest : ui::Buildable
{
	struct Data : ui::DragDropData
	{
		static constexpr const char* NAME = "slot item";
		Data(int f) : ui::DragDropData(NAME), from(f) {}
		int from;
	};

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		WText("Transfer countables");

		WPush<ui::StackLTRLayoutElement>();
		for (int i = 0; i < 3; i++)
		{
			auto& btn = ui::Push<ui::Button>();
			auto& pad = ui::Push<ui::PaddingElement>();
			if (btn.flags & ui::UIObject_DragHovered)
			{
				pad.SetPadding(4, 4, 4, 10);
			}
			ui::Text("Slot " + std::to_string(i + 1) + ": " + std::to_string(slots[i]));
			ui::Pop();
			ui::Pop();

			btn.SetInputDisabled(slots[i] == 0);
			btn.HandleEvent() = [this, i](ui::Event& e)
			{
				if (e.context->DragCheck(e, ui::MouseButton::Left))
				{
					if (slots[i] > 0)
					{
						e.context->SetKeyboardFocus(nullptr);
						ui::DragDrop::SetData(new Data(i));
					}
				}
				else if (e.type == ui::EventType::DragDrop)
				{
					if (auto* ddd = ui::DragDrop::GetData<Data>())
					{
						slots[i]++;
						slots[ddd->from]--;
						e.StopPropagation();
						Rebuild();
					}
				}
				else if (e.type == ui::EventType::DragEnter)
				{
					Rebuild();
				}
				else if (e.type == ui::EventType::DragLeave)
				{
					Rebuild();
				}
			};
		}
		WPop();

		WPop();
	}

	int slots[3] = { 5, 2, 0 };
};


struct SlideReorderTest : ui::Buildable
{
	struct Data : ui::DragDropData
	{
		static constexpr const char* NAME = "current location";
		Data(int f) : ui::DragDropData(NAME), at(f) {}
		int at;
	};

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Slide-reorder (instant)");

		ui::Push<ui::ListBoxFrame>();
		ui::Push<ui::StackTopDownLayoutElement>();
		for (int i = 0; i < 4; i++)
		{
			bool dragging = false;
			if (auto* dd = ui::DragDrop::GetData<Data>())
				if (dd->at == i)
					dragging = true;
			auto& sel = ui::Push<ui::Selectable>().Init(dragging);
			sel.SetFlag(ui::UIObject_DB_Draggable, true);
			sel.HandleEvent() = [this, i](ui::Event& e)
			{
				if (e.context->DragCheck(e, ui::MouseButton::Left))
				{
					ui::DragDrop::SetData(new Data(i));
					e.context->SetKeyboardFocus(nullptr);
					e.context->ReleaseMouse();
				}
				else if (e.type == ui::EventType::DragEnter)
				{
					if (auto* ddd = ui::DragDrop::GetData<Data>())
					{
						if (ddd->at != i)
						{
							e.context->MoveClickTo(e.current);
						}
						while (ddd->at < i)
						{
							int pos = ddd->at++;
							std::swap(iids[pos], iids[pos + 1]);
						}
						while (ddd->at > i)
						{
							int pos = ddd->at--;
							std::swap(iids[pos], iids[pos - 1]);
						}
					}
					Rebuild();
				}
				else if (e.type == ui::EventType::DragLeave || e.type == ui::EventType::DragEnd)
				{
					Rebuild();
				}
			};
			ui::Push<ui::StackExpandLTRLayoutElement>();
			ui::Textf("item %d%s", iids[i], dragging ? " [dragging]" : "");
			ui::Pop();
			ui::Pop();
		}
		ui::Pop();
		ui::Pop();

		WPop();
	}

	int iids[4] = { 1, 2, 3, 4 };
};


struct TreeNodeReorderTest : ui::Buildable
{
	struct Node
	{
		~Node()
		{
			for (Node* ch : children)
				delete ch;
		}
		std::string name;
		bool open = true;
		Node* parent = nullptr;
		ui::Array<Node*> children;
	};
	struct DragDropNodeData : ui::DragDropData
	{
		static constexpr const char* NAME = "node";
		struct Entry
		{
			Node* node;
			size_t childNum;
		};
		DragDropNodeData() : ui::DragDropData(NAME)
		{
		}
		void Build() override
		{
			if (entries.size() == 1)
			{
				ui::Text(entries[0].node->name);
			}
			else
			{
				ui::Textf("%zu items", entries.size());
			}
		}
		ui::Array<Entry> entries;
	};

	TreeNodeReorderTest()
	{
		Node* first = new Node();
		first->name = "first [1]";

		Node* second = new Node();
		second->name = "second [2]";

		Node* third = new Node();
		third->name = "third [3]";

		Node* fourth = new Node();
		fourth->name = "fourth [4]";

		rootNodes.Append(first);
		first->children.Append(second);
		second->parent = first;
		second->children.Append(third);
		third->parent = second;
		rootNodes.Append(fourth);
	}
	~TreeNodeReorderTest()
	{
		for (Node* ch : rootNodes)
			delete ch;
	}

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Tree node reorder with preview");

		ui::Push<ui::ListBoxFrame>();
		ui::Push<ui::StackTopDownLayoutElement>();
		BuildNodeList(nullptr, rootNodes, 0);
		ui::Pop();
		ui::Pop();

		WPop();
	}
	void BuildNodeList(Node* cont, ui::Array<Node*>& nodes, int level)
	{
		for (size_t i = 0; i < nodes.size(); i++)
		{
			Node* N = nodes[i];
			ui::Push<ui::StackTopDownLayoutElement>();

			ui::Push<ui::Selectable>().Init(selectedNodes.Contains(N))
				.SetPadding(0)
				+ ui::MakeDraggable()
				+ ui::AddEventHandler([this, cont, N, i, &nodes, level](ui::Event& e)
			{
				if (e.type == ui::EventType::DragStart)
				{
					if (selectedNodes.size())
					{
						auto* ddd = new DragDropNodeData();
						ddd->entries.Append({ N, i });
						ui::DragDrop::SetData(ddd);
					}
				}
				if (e.type == ui::EventType::DragMove)
				{
					Tree_OnDragMove(cont, nodes, level, i, e);
				}
				if (e.type == ui::EventType::DragDrop)
				{
					Tree_OnDragDrop();
				}
				if (e.type == ui::EventType::DragLeave)
				{
					hasDragTarget = false;
				}
				if (e.type == ui::EventType::Activate)
				{
					selectedNodes.Clear();
					selectedNodes.Insert(N);
					Rebuild();
				}
			});
			ui::Push<ui::StackExpandLTRLayoutElement>();

			ui::Make<ui::SizeConstraintElement>().SetWidth(level * 12);

			ui::Push<ui::CheckboxFlagT<bool>>().Init(N->open) + ui::RebuildOnChange();
			ui::Make<ui::TreeExpandIcon>();
			ui::Pop();

			ui::Text(N->name);
			ui::Pop();
			ui::Pop();

			if (N->open)
			{
				ui::Push<ui::StackTopDownLayoutElement>();
				BuildNodeList(N, N->children, level + 1);
				ui::Pop();
			}
			ui::Pop();
		}
	}
	void Tree_OnDragMove(Node* cont, ui::Array<Node*>& nodes, int level, size_t i, ui::Event& e)
	{
		Node* N = nodes[i];

		hasDragTarget = true;
		const ui::UIRect R = e.current->GetFinalRect();
		if (e.position.y < ui::lerp(R.y0, R.y1, 0.25f))
		{
			// above
			dragTargetLine = ui::UIRect{ R.x0 + level * 12, R.y0, R.x1, R.y0 }.ExtendBy(1);
			dragTargetArr = &nodes;
			dragTargetCont = cont;
			dragTargetInsertBefore = i;
		}
		else if (e.position.y > ui::lerp(R.y0, R.y1, 0.75f))
		{
			// below
			dragTargetLine = ui::UIRect{ R.x0, R.y1, R.x1, R.y1 }.ExtendBy(1);
			if (N->open && N->children.size())
			{
				// first child
				dragTargetArr = &N->children;
				dragTargetCont = N;
				dragTargetInsertBefore = 0;
				dragTargetLine.x0 += (level + 1) * 12;
			}
			else
			{
				int ll = level - 1;
				auto* PP = N;
				auto* P = N->parent;
				bool foundplace = false;
				for (; P; P = P->parent, ll--)
				{
					for (size_t i = 0; i < P->children.size(); i++)
					{
						if (P->children[i] == PP && i + 1 < P->children.size())
						{
							dragTargetArr = &P->children;
							dragTargetCont = P;
							dragTargetInsertBefore = i + 1;
							dragTargetLine.x0 += ll * 12;
							foundplace = true;
							break;
						}
					}
					if (foundplace)
						break;
					PP = P;
				}
				if (!foundplace)
				{
					for (size_t i = 0; i < rootNodes.size(); i++)
					{
						if (rootNodes[i] == PP)
						{
							dragTargetArr = &rootNodes;
							dragTargetCont = nullptr;
							dragTargetInsertBefore = i + 1;
							foundplace = true;
							break;
						}
					}
				}
			}
		}
		else
		{
			// in
			dragTargetLine = ui::UIRect{ R.x0 + level * 12, ui::lerp(R.y0, R.y1, 0.25f), R.x1, ui::lerp(R.y0, R.y1, 0.75f) };
			dragTargetArr = &N->children;
			dragTargetCont = N;
			dragTargetInsertBefore = N->children.size();
		}
	}
	void Tree_OnDragDrop()
	{
		if (auto* ddd = ui::DragDrop::GetData<DragDropNodeData>())
		{
			printf("target cont=%p name=%s ins.before=%zu\n", dragTargetCont, dragTargetCont ? dragTargetCont->name.c_str() : "-", dragTargetInsertBefore);

			// validate
			bool canDrop = true;
			for (auto* P = dragTargetCont; P; P = P->parent)
			{
				bool found = false;
				for (auto& E : ddd->entries)
				{
					if (E.node == P)
					{
						found = true;
						break;
					}
				}
				if (found)
				{
					canDrop = false;
					break;
				}
			}
			printf("can drop: %c\n", "NY"[canDrop]);

			if (canDrop)
			{
				// unlink from parents
				for (auto& E : ddd->entries)
				{
					auto& arr = E.node->parent ? E.node->parent->children : rootNodes;
					size_t i = arr.IndexOf(E.node);
					if (E.node->parent == dragTargetCont && i < ptrdiff_t(dragTargetInsertBefore))
					{
						dragTargetInsertBefore--;
					}
					arr.RemoveAt(i);
				}

				// reparent
				for (auto& E : ddd->entries)
				{
					E.node->parent = dragTargetCont;
					dragTargetArr->InsertAt(dragTargetInsertBefore, E.node);
				}

				Rebuild();
			}
		}
	}
	void OnPaint(const ui::UIPaintContext& ctx)
	{
		ui::Buildable::OnPaint(ctx);

		if (hasDragTarget)
		{
			auto r = dragTargetLine;
			ui::draw::RectCol(r.x0, r.y0, r.x1, r.y1, ui::Color4f(0.1f, 0.7f, 0.9f, 0.6f));
		}
	}

	ui::Array<Node*> rootNodes;
	ui::HashSet<Node*> selectedNodes;

	bool hasDragTarget = false;
	ui::UIRect dragTargetLine = {};
	size_t dragTargetInsertBefore = 0;
	Node* dragTargetCont = nullptr;
	ui::Array<Node*>* dragTargetArr = nullptr;
};


struct DragElementTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Drag element");

		ui::Push<ui::SizeConstraintElement>().SetHeight(100);
		ui::Push<ui::ListBoxFrame>();
		auto& ple = ui::Push<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		drelPlacement = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
		drelPlacement->bias = drelPos;
		tmpl->placement = drelPlacement;
		auto& tp = ui::Push<ui::FrameElement>();
		tp.SetDefaultFrameStyle(ui::DefaultFrameStyle::ProcGraphNode);
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::MakeWithText<ui::Selectable>("draggable area").Init(drelIsDragging) + ui::MakeDraggable() + ui::AddEventHandler([this, &tp](ui::Event& e)
		{
			if (e.type == ui::EventType::ButtonDown && e.GetButton() == ui::MouseButton::Left)
			{
				drelIsDragging = true;
				drelDragStartMouse = e.context->clickStartPositions[int(ui::MouseButton::Left)];
				drelDragStartPos = drelPos;
			}
			if (e.type == ui::EventType::ButtonUp && e.GetButton() == ui::MouseButton::Left)
				drelIsDragging = false;
			if (e.type == ui::EventType::MouseMove && drelIsDragging)
			{
				//drelPos = drelDragStartPos + e.position - drelDragStartMouse;
				drelPos.x = drelDragStartPos.x + e.position.x - drelDragStartMouse.x;
				drelPos.y = drelDragStartPos.y + e.position.y - drelDragStartMouse.y;
				drelPlacement->bias = drelPos;
				tp._OnChangeStyle(); // TODO?
			}
		});
		ui::MakeWithText<ui::LabelFrame>("the other part");
		ui::Pop(); // StackTopDownLayoutElement
		ui::Pop(); // FrameElement(ProcGraphNode)

		ui::Pop(); // PlacementLayoutElement
		ui::Pop(); // ListBoxFrame
		ui::Pop(); // SizeConstraintElement

		WPop();
	}

	ui::PointAnchoredPlacement* drelPlacement = nullptr;
	ui::Point2f drelPos = { 25, 15 };
	ui::Point2f drelDragStartMouse = {};
	ui::Point2f drelDragStartPos = {};
	bool drelIsDragging = false;
};


struct DragConnectTest : ui::Buildable
{
	static int CombineLinkableIDs(int a, int b)
	{
		if (a > b)
			std::swap(a, b);
		return (a << 16) | b;
	}

	struct LinkDragDropData : ui::DragDropData
	{
		static constexpr const char* NAME = "link";
		LinkDragDropData(int linkID) : DragDropData(NAME), _linkID(linkID) {}
		bool ShouldBuild() override { return false; }
		int _linkID;
	};

	DragConnectTest()
	{
		links.Insert(CombineLinkableIDs(0, 3));
	}

	struct Linkable : ui::Selectable
	{
		Linkable()
		{
			flags |= ui::UIObject_NeedsTreeUpdates;
		}

		void OnReset() override
		{
			ui::Selectable::OnReset();

			*this + ui::MakeDraggable();
		}
		void OnEvent(ui::Event& e) override
		{
			ui::Selectable::OnEvent(e);

			if (e.type == ui::EventType::DragStart)
			{
				dragged = true;
				ui::Selectable::Init(dragged || dragHL);
				ui::DragDrop::SetData(new LinkDragDropData(_which));
			}
			if (e.type == ui::EventType::DragEnd)
			{
				dragged = false;
				ui::Selectable::Init(dragged || dragHL);
			}
			if (e.type == ui::EventType::DragEnter)
			{
				dragHL = true;
				ui::Selectable::Init(dragged || dragHL);
			}
			if (e.type == ui::EventType::DragLeave)
			{
				dragHL = false;
				ui::Selectable::Init(dragged || dragHL);
			}
			if (e.type == ui::EventType::DragDrop)
			{
				if (auto* ddd = ui::DragDrop::GetData<LinkDragDropData>())
				{
					if (auto* p = FindParentOfType<DragConnectTest>())
					{
						if (auto* ol = p->linkables.GetValueOrDefault(ddd->_linkID))
						{
							if (ol->_right != _right)
							{
								p->links.Insert(CombineLinkableIDs(ddd->_linkID, _which));
							}
						}
					}
				}
			}

			if (e.type == ui::EventType::ContextMenu)
			{
				ui::ContextMenu::Get().Add("Unlink") = [this]() { UnlinkAll(); };
			}
		}
		void OnExitTree() override
		{
			if (auto* p = FindParentOfType<DragConnectTest>())
			{
				p->linkables.Remove(_which);
			}
		}
		Linkable& Init(bool right, int which)
		{
			_right = right;
			_which = which;
			if (auto* p = FindParentOfType<DragConnectTest>())
			{
				p->linkables.Insert(_which, this);
			}
			return *this;
		}
		void UnlinkAll()
		{
			if (auto* p = FindParentOfType<DragConnectTest>())
			{
				decltype(p->links) newlinks;
				for (int link : p->links)
				{
					if ((link >> 16) != _which &&
						(link & 0xffff) != _which)
						newlinks.Insert(link);
				}
				std::swap(newlinks, p->links);
			}
		}

		bool _right = false;
		int _which = 0;
		bool dragged = false;
		bool dragHL = false;
	};
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Drag connect");

		ui::Push<ui::SizeConstraintElement>().SetHeight(60);
		ui::Push<ui::ListBoxFrame>();

		ui::Push<ui::EdgeSliceLayoutElement>();
		auto tmpl = ui::EdgeSliceLayoutElement::GetSlotTemplate();

		tmpl->edge = ui::Edge::Left;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::MakeWithText<Linkable>("left A").Init(false, 0);
		ui::MakeWithText<Linkable>("left B").Init(false, 1);
		ui::Pop();

		tmpl->edge = ui::Edge::Right;
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::MakeWithText<Linkable>("right A").Init(true, 2);
		ui::MakeWithText<Linkable>("right B").Init(true, 3);
		ui::Pop();

		ui::Pop();

		ui::Pop();
		ui::Pop();

		WPop();
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);

		for (int link : links)
		{
			int A = link >> 16;
			int B = link & 0xffff;
			if (auto* LA = linkables.GetValueOrDefault(A))
			{
				if (auto* LB = linkables.GetValueOrDefault(B))
				{
					auto PA = GetLinkablePos(LA);
					auto PB = GetLinkablePos(LB);
					ui::draw::AALineCol(
						PA.x, PA.y,
						PB.x, PB.y,
						1, ui::Color4f(0.2f, 0.8f, 0.9f));
				}
			}
		}

		if (auto* ddd = ui::DragDrop::GetData<LinkDragDropData>())
		{
			if (auto* linkable = linkables.GetValueOrDefault(ddd->_linkID))
			{
				auto pos = GetLinkablePos(linkable);
				ui::draw::AALineCol(
					pos.x, pos.y,
					system->eventSystem.prevMousePos.x, system->eventSystem.prevMousePos.y,
					1, ui::Color4f(0.9f, 0.8f, 0.2f));
			}
		}
	}

	ui::Point2f GetLinkablePos(Linkable* L)
	{
		auto frL = L->GetFinalRect();
		return
		{
			L->_right ? frL.x0 : frL.x1,
			(frL.y0 + frL.y1) * 0.5f,
		};
	}

	ui::HashMap<int, Linkable*> linkables;
	ui::HashSet<int> links; // smallest first
};


struct DragDropTest : ui::Buildable
{
	ui::RectAnchoredPlacement parts[2];

	void Build() override
	{
		for (int i = 0; i < 2; i++)
		{
			parts[i].anchor.x0 = i / 2.f;
			parts[i].anchor.x1 = (i + 1) / 2.f;
		}

		auto& ple = WPush<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		WMake<ui::FillerElement>();

		tmpl->placement = &parts[0];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		WPush<ui::StackTopDownLayoutElement>();

		ui::Make<FileReceiverTest>();
		ui::Make<TransferCountablesTest>();
		ui::Make<SlideReorderTest>();
		ui::Make<TreeNodeReorderTest>();

		WPop(); // StackTopDownLayoutElement
		WPop(); // FrameElement
		WPop(); // StackTopDownLayoutElement

		tmpl->placement = &parts[1];
		tmpl->measure = false;
		WPush<ui::StackTopDownLayoutElement>();
		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		WPush<ui::StackTopDownLayoutElement>();

		ui::Make<DragElementTest>();
		ui::Make<DragConnectTest>();

		WPop(); // StackTopDownLayoutElement
		WPop(); // FrameElement
		WMake<ui::DefaultOverlayBuilder>(); // TODO: hack, do we need these at all? maybe they should work differently?
		WPop(); // StackTopDownLayoutElement

		WPop(); // PlacementLayoutElement
	}
};
void Test_DragDrop()
{
	ui::Make<DragDropTest>();
}

