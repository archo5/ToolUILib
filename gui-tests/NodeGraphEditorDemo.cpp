
#include "pch.h"


struct IProcGraph
{
	using Node = void;
	struct LinkEnd
	{
		Node* node;
		uintptr_t num;

		bool operator == (const LinkEnd& o) const
		{
			return node == o.node && num == o.num;
		}
	};
	struct Pin
	{
		LinkEnd end;
		bool isOutput;

		bool operator == (const Pin& o) const
		{
			return end == o.end && isOutput == o.isOutput;
		}
	};
	struct Link
	{
		LinkEnd output;
		LinkEnd input;
	};
	struct PinHash
	{
		size_t operator () (const Pin& p) const
		{
			auto hash = std::hash<Node*>()(p.end.node);
			hash *= 121;
			hash = std::hash<int>()(p.end.num);
			hash *= 121;
			hash = std::hash<bool>()(p.isOutput);
			return hash;
		}
	};

	//virtual int GetSupportedFeatures() = 0;

	virtual void GetNodes(std::vector<Node*>& outNodes) = 0;
	virtual void GetLinks(std::vector<Link>& outLinks) = 0;

	virtual StringView GetNodeName(Node*) = 0;
	virtual uintptr_t GetNodeInputCount(Node*) = 0;
	virtual uintptr_t GetNodeOutputCount(Node*) = 0;

	virtual const char* GetPinName(const Pin&) = 0;
	virtual bool IsPinLinked(const Pin&) = 0;
	virtual void UnlinkPin(const Pin& pin) = 0;
	virtual void InputPinEditorUI(const Pin& pin, UIContainer*) {}

	virtual bool LinkExists(const Link&) = 0;
	virtual bool CanLink(const Link&) = 0;
	virtual bool TryLink(const Link&) = 0;
	virtual bool Unlink(const Link&) = 0;

	virtual Point<float> GetNodePosition(Node*) = 0;
	virtual void SetNodePosition(Node*, const Point<float>&) = 0;

	virtual bool HasPreview(Node*) { return false; }
	virtual bool IsPreviewEnabled(Node*) { return false; }
	virtual void SetPreviewEnabled(Node*, bool) {}
	virtual void PreviewUI(Node*, UIContainer*) {}

	virtual bool CanDeleteNode(Node*) { return false; }
	virtual void DeleteNode(Node*) {}

	virtual bool CanDuplicateNode(Node*) { return false; }
	virtual Node* DuplicateNode(Node*) { return nullptr; }
};


struct LinkDragDropData : ui::DragDropData
{
	LinkDragDropData(IProcGraph* graph, const IProcGraph::Pin& pin) :
		DragDropData("link"),
		_graph(graph),
		_pin(pin)
	{}
	bool ShouldRender() override { return false; }
	IProcGraph* _graph;
	IProcGraph::Pin _pin;
};

struct GraphNodePinUI : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		_sel = ctx->Push<ui::Selectable>();
		*_sel + ui::MakeDraggable();
		*_sel + ui::StackingDirection(!_info.isOutput ? style::StackingDirection::LeftToRight : style::StackingDirection::RightToLeft);

		ctx->Text(_graph->GetPinName(_info));

		if (!_info.isOutput && !_graph->IsPinLinked(_info))
		{
			// TODO not implemented right->left
			*_sel + ui::Layout(style::layouts::StackExpand());

			_graph->InputPinEditorUI(_info, ctx);
		}

		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::DragStart)
		{
			_dragged = true;
			_sel->Init(_dragged || _dragHL);
			ui::DragDrop::SetData(new LinkDragDropData(_graph, _info));
		}
		if (e.type == UIEventType::DragEnd)
		{
			_dragged = false;
			_sel->Init(_dragged || _dragHL);
		}
		if (e.type == UIEventType::DragEnter)
		{
			_dragHL = true;
			_sel->Init(_dragged || _dragHL);
		}
		if (e.type == UIEventType::DragLeave)
		{
			_dragHL = false;
			_sel->Init(_dragged || _dragHL);
		}
		if (e.type == UIEventType::DragDrop)
		{
			if (auto* ddd = static_cast<LinkDragDropData*>(ui::DragDrop::GetData("link")))
			{
				if (_graph == ddd->_graph && _info.isOutput != ddd->_pin.isOutput)
				{
					if (_info.isOutput)
						_graph->TryLink({ _info.end, ddd->_pin.end });
					else
						_graph->TryLink({ ddd->_pin.end, _info.end });
				}
			}
		}

		if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Right)
		{
			ui::MenuItem items[] =
			{
				ui::MenuItem("Unlink").Func([this]() { _graph->UnlinkPin(_info); }),
			};
			ui::Menu menu(items);
			menu.Show(this);
		}
	}
	void OnDestroy() override
	{
		UnregisterPin();
	}
	void Init(IProcGraph* graph, IProcGraph::Node* node, uintptr_t pin, bool isOutput)
	{
		_graph = graph;
		_info = { node, pin, isOutput };
		RegisterPin();
	}
	void RegisterPin();
	void UnregisterPin();

	IProcGraph* _graph = nullptr;
	IProcGraph::Pin _info = {};

	bool _dragged = false;
	bool _dragHL = false;
	ui::Selectable* _sel = nullptr;
};

using PinUIMap = HashMap<IProcGraph::Pin, GraphNodePinUI*, IProcGraph::PinHash>;

struct GraphNodeUI : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		auto s = GetStyle(); // for style only
		s.SetWidth(style::Coord::Undefined());
		s.SetMinWidth(100);
		auto* placement = Allocate<style::PointAnchoredPlacement>();
		placement->bias = _graph->GetNodePosition(_node);
		s.SetPlacement(placement);

		*ctx->Push<ui::TabPanel>() + ui::Width(style::Coord::Undefined());
		*ctx->Push<ui::Selectable>()->Init(_isDragging)
			+ ui::Padding(0)
			+ ui::MakeDraggable()
			+ ui::EventHandler([this, placement](UIEvent& e)
		{
			if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
			{
				_isDragging = true;
				_dragStartMouse = e.context->clickStartPositions[int(UIMouseButton::Left)];
				_dragStartPos = _graph->GetNodePosition(_node);
			}
			if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
				_isDragging = false;
			if (e.type == UIEventType::MouseMove && _isDragging)
			{
				Point<float> curMousePos = { e.x, e.y };
				//_node->position = _dragStartPos + curMousePos - _dragStartMouse;
				Point<float> newPos =
				{
					_dragStartPos.x + curMousePos.x - _dragStartMouse.x,
					_dragStartPos.y + curMousePos.y - _dragStartMouse.y,
				};
				placement->bias = newPos;
				_graph->SetNodePosition(_node, newPos);
				_OnChangeStyle();
			}
		});

		bool hasPreview = _graph->HasPreview(_node);
		bool showPreview = false;
		if (hasPreview)
		{
			showPreview = _graph->IsPreviewEnabled(_node);
			ui::imm::EditBool(ctx, showPreview);
			_graph->SetPreviewEnabled(_node, showPreview);
		}
		ctx->Text(_graph->GetNodeName(_node)) + ui::Padding(5, hasPreview ? 0 : 5, 5, 5);
		ctx->Pop();

		uintptr_t count = _graph->GetNodeOutputCount(_node);
		for (uintptr_t i = 0; i < count; i++)
			ctx->Make<GraphNodePinUI>()->Init(_graph, _node, i, true);

		if (showPreview)
		{
			_graph->PreviewUI(_node, ctx);
		}

		count = _graph->GetNodeInputCount(_node);
		for (uintptr_t i = 0; i < count; i++)
			ctx->Make<GraphNodePinUI>()->Init(_graph, _node, i, false);

		ctx->Pop();
	}
	void Init(IProcGraph* graph, IProcGraph::Node* node)
	{
		_graph = graph;
		_node = node;
	}

	IProcGraph* _graph = nullptr;
	IProcGraph::Node* _node = nullptr;
	bool _isDragging = false;
	Point<float> _dragStartPos = {};
	Point<float> _dragStartMouse = {};
};

struct GraphUI : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		*this + ui::Height(style::Coord::Percent(100));
		*ctx->Push<ui::ListBox>() + ui::Height(style::Coord::Percent(100));

		std::vector<IProcGraph::Node*> nodes;
		_graph->GetNodes(nodes);
		for (auto* N : nodes)
		{
			ctx->Make<GraphNodeUI>()->Init(_graph, N);
		}

		ctx->Pop();
	}
	void OnPaint() override
	{
		Node::OnPaint();

		std::vector<IProcGraph::Link> links;
		_graph->GetLinks(links);
		for (const auto& link : links)
		{
			auto A = link.output;
			auto B = link.input;
			if (auto* LA = pinUIMap.get({ A, true }))
			{
				if (auto* LB = pinUIMap.get({ B, false }))
				{
					auto PA = GetPinPos(LA);
					auto PB = GetPinPos(LB);
					ui::draw::AALineCol(
						PA.x, PA.y,
						PB.x, PB.y,
						1, ui::Color4f(0.2f, 0.8f, 0.9f));
				}
			}
		}

		if (auto* ddd = static_cast<LinkDragDropData*>(ui::DragDrop::GetData("link")))
		{
			if (auto* pin = pinUIMap.get(ddd->_pin))
			{
				auto pos = GetPinPos(pin);
				ui::draw::AALineCol(
					pos.x, pos.y,
					system->eventSystem.prevMouseX, system->eventSystem.prevMouseY,
					1, ui::Color4f(0.9f, 0.8f, 0.2f));
			}
		}
	}
	void Init(IProcGraph* graph)
	{
		_graph = graph;
	}

	Point<float> GetPinPos(GraphNodePinUI* P)
	{
		return
		{
			!P->_info.isOutput ? P->finalRectCPB.x0 : P->finalRectCPB.x1,
			(P->finalRectCPB.y0 + P->finalRectCPB.y1) * 0.5f,
		};
	}

	IProcGraph* _graph = nullptr;
	PinUIMap pinUIMap;
};

void GraphNodePinUI::RegisterPin()
{
	if (auto* p = FindParentOfType<GraphUI>())
	{
		p->pinUIMap.insert(_info, this);
	}
}

void GraphNodePinUI::UnregisterPin()
{
	if (auto* p = FindParentOfType<GraphUI>())
	{
		p->pinUIMap.erase(_info);
	}
}


struct Graph
{
	enum class Type
	{
		Scalar,
		Vector,
	};
	struct Node
	{
		virtual Node* Clone() = 0;
		virtual const char* GetName() = 0;
		virtual int GetNumInputs() = 0;
		virtual int GetNumOutputs() = 0;
		virtual const char* GetInputName(int which) = 0;
		virtual const char* GetOutputName(int which) = 0;
		virtual Type GetInputType(int which) = 0;
		virtual Type GetOutputType(int which) = 0;
		virtual float* GetInputDefaultValuePtr(int which) = 0;

		virtual bool HasPreview() = 0;
		virtual void PreviewUI(UIContainer* ctx) {}

		Point<float> position = {};
		bool showPreview = true;
	};
	struct LinkEnd
	{
		Node* node;
		int pin;
		bool isOutput;

		bool operator == (const LinkEnd& o) const
		{
			return node == o.node && pin == o.pin && isOutput == o.isOutput;
		}
		bool operator != (const LinkEnd& o) const { return !(*this == o); }
	};
	struct Link
	{
		LinkEnd output;
		LinkEnd input;
	};

	bool IsLinked(const LinkEnd& end)
	{
		for (auto* link : links)
		{
			if (link->input == end ||
				link->output == end)
				return true;
		}
		return false;
	}
	void UnlinkAll(const LinkEnd& end)
	{
		decltype(links) newlinks;
		for (auto* link : links)
		{
			if (link->input != end &&
				link->output != end)
				newlinks.push_back(link);
			else
				delete link;
		}
		std::swap(newlinks, links);
	}
	void AddLink(const LinkEnd& output, const LinkEnd& input)
	{
		UnlinkAll(input);
		links.push_back(new Link{ output, input });
	}

	bool TryLink(const LinkEnd& A, const LinkEnd& B)
	{
		if (A.isOutput == B.isOutput || A.node == B.node)
			return false;
		auto typeA = A.isOutput ? A.node->GetOutputType(A.pin) : A.node->GetInputType(A.pin);
		auto typeB = B.isOutput ? B.node->GetOutputType(B.pin) : B.node->GetInputType(B.pin);
		if (typeA != typeB)
			return false;

		if (B.isOutput)
		{
			AddLink(B, A);
		}
		else
		{
			AddLink(A, B);
		}
		return true;
	}

	std::vector<Node*> nodes;
	std::vector<Link*> links;
};

struct MakeVec : Graph::Node
{
	Node* Clone() override { return new MakeVec(*this); }
	const char* GetName() override { return "Make vector"; }
	int GetNumInputs() override { return 3; }
	int GetNumOutputs() override { return 1; }
	const char* GetInputName(int which) override { return which == 0 ? "X" : which == 1 ? "Y" : "Z"; }
	const char* GetOutputName(int which) override { return "Vector"; }
	Graph::Type GetInputType(int) override { return Graph::Type::Scalar; }
	Graph::Type GetOutputType(int) override { return Graph::Type::Vector; }
	float* GetInputDefaultValuePtr(int which) override { return &defInputs[which]; }
	bool HasPreview() override { return false; }

	float defInputs[3] = {};
};

struct DotProduct : Graph::Node
{
	Node* Clone() override { return new DotProduct(*this); }
	const char* GetName() override { return "Dot product"; }
	int GetNumInputs() override { return 2; }
	int GetNumOutputs() override { return 1; }
	const char* GetInputName(int which) override { return which == 0 ? "A" : "B"; }
	const char* GetOutputName(int which) override { return "Dot product"; }
	Graph::Type GetInputType(int) override { return Graph::Type::Vector; }
	Graph::Type GetOutputType(int) override { return Graph::Type::Scalar; }
	float* GetInputDefaultValuePtr(int which) override { return defInputs[which]; }
	bool HasPreview() override { return false; }

	float defInputs[2][3] = {};
};


struct GraphImpl : IProcGraph
{
	void GetNodes(std::vector<Node*>& outNodes) override
	{
		for (auto* n : graph->nodes)
			outNodes.push_back(static_cast<Node*>(n));
	}
	void GetLinks(std::vector<Link>& outLinks) override
	{
		for (auto* l : graph->links)
		{
			outLinks.push_back
			({
				static_cast<Node*>(l->output.node),
				uintptr_t(l->output.pin),
				static_cast<Node*>(l->input.node),
				uintptr_t(l->input.pin),
			});
		}
	}

	StringView GetNodeName(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->GetName();
	}
	uintptr_t GetNodeInputCount(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->GetNumInputs();
	}
	uintptr_t GetNodeOutputCount(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->GetNumOutputs();
	}

	const char* GetPinName(const Pin& pin) override
	{
		auto* node = static_cast<Graph::Node*>(pin.end.node);
		return pin.isOutput ? node->GetOutputName(pin.end.num) : node->GetInputName(pin.end.num);
	}
	bool IsPinLinked(const Pin& pin) override
	{
		Graph::LinkEnd gle = { static_cast<Graph::Node*>(pin.end.node), int(pin.end.num), pin.isOutput };
		return graph->IsLinked(gle);
	}
	void UnlinkPin(const Pin& pin) override
	{
		Graph::LinkEnd gle = { static_cast<Graph::Node*>(pin.end.node), int(pin.end.num), pin.isOutput };
		graph->UnlinkAll(gle);
	}
	void InputPinEditorUI(const Pin& pin, UIContainer* ctx) override
	{
		auto* node = static_cast<Graph::Node*>(pin.end.node);
		auto type = node->GetInputType(pin.end.num);
		float* data = node->GetInputDefaultValuePtr(pin.end.num);
		switch (type)
		{
		case Graph::Type::Scalar:
			ui::imm::PropEditFloat(ctx, "\b=", *data);
			break;
		case Graph::Type::Vector:
			ui::imm::PropEditFloatVec(ctx, nullptr, data);
			break;
		}
	}

	bool LinkExists(const Link& link) override
	{
		return true;
	}
	bool CanLink(const Link&) override
	{
		return true;
	}
	bool TryLink(const Link& link) override
	{
		return graph->TryLink(
			{ static_cast<Graph::Node*>(link.output.node), int(link.output.num), true },
			{ static_cast<Graph::Node*>(link.input.node), int(link.input.num), false }
		);
	}
	bool Unlink(const Link&) override
	{
		return false;
	}

	Point<float> GetNodePosition(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->position;
	}
	void SetNodePosition(Node* node, const Point<float>& pos) override
	{
		static_cast<Graph::Node*>(node)->position = pos;
	}

	bool CanDeleteNode(Node*) override { return true; }
	void DeleteNode(Node* node) override
	{
		for (auto it = graph->nodes.begin(), itend = graph->nodes.end(); it != itend; it++)
		{
			if (*it == node)
			{
				graph->nodes.erase(it);
				delete node;
				break;
			}
		}
	}

	bool CanDuplicateNode(Node*) override { return true; }
	Node* DuplicateNode(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->Clone();
	}

	GraphImpl(Graph* g) : graph(g) {}

	Graph* graph;
};

struct NodeGraphEditorDemo : ui::Node
{
	static constexpr bool Persistent = true;

	NodeGraphEditorDemo()
	{
		Graph::Node* makeVec1 = new MakeVec();
		Graph::Node* dotProd = new DotProduct();
		makeVec1->position = { 100, 100 };
		dotProd->position = { 300, 100 };
		graph.nodes.push_back(makeVec1);
		graph.nodes.push_back(dotProd);
		graph.links.push_back(new Graph::Link{ { makeVec1, 0, true }, { dotProd, 0 } });
		graph.links.push_back(new Graph::Link{ { makeVec1, 0, true }, { dotProd, 1 } });
	}
	void Render(UIContainer* ctx) override
	{
		*this + ui::Height(style::Coord::Percent(100));
		ctx->Make<GraphUI>()->Init(Allocate<GraphImpl>(&graph));
	}

	Graph graph;
};
void Demo_NodeGraphEditor(UIContainer* ctx)
{
	ctx->Make<NodeGraphEditorDemo>();
}

