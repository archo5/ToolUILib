
#include "pch.h"


namespace ui {

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

	// global lists
	virtual void GetNodes(std::vector<Node*>& outNodes) = 0;
	virtual void GetLinks(std::vector<Link>& outLinks) = 0;

	// basic node info
	virtual std::string GetNodeName(Node*) = 0;
	virtual uintptr_t GetNodeInputCount(Node*) = 0;
	virtual uintptr_t GetNodeOutputCount(Node*) = 0;

	// basic pin info
	virtual std::string GetPinName(const Pin&) = 0;
	virtual ui::Color4b GetPinColor(const Pin& pin) { return ui::Color4b(255, 0); }
	virtual void InputPinEditorUI(const Pin& pin, UIContainer*) {}

	// pin linkage info (with default suboptimal implementations)
	virtual bool IsPinLinked(const Pin& pin)
	{
		std::vector<Link> allLinks;
		GetLinks(allLinks);
		for (const auto& link : allLinks)
		{
			if ((pin.isOutput ? link.output : link.input) == pin.end)
				return true;
		}
		return false;
	}
	virtual void GetPinLinks(const Pin& pin, std::vector<Link>& outLinks)
	{
		std::vector<Link> allLinks;
		GetLinks(allLinks);
		for (const auto& link : allLinks)
		{
			if ((pin.isOutput ? link.output : link.input) == pin.end)
				outLinks.push_back(link);
		}
	}
	virtual void UnlinkPin(const Pin& pin) = 0;

	// link info & editing
	virtual bool LinkExists(const Link&) = 0;
	virtual bool CanLink(const Link&) { return true; } // does not have to evaluate all preconditions, used only for highlighting
	virtual bool TryLink(const Link&) = 0;
	virtual bool Unlink(const Link&) = 0;
	virtual Color4b GetLinkColor(const Link&, bool selected, bool hovered)
	{
		return Color4f(0.2f, 0.8f, 0.9f);
	}
	virtual Color4b GetNewLinkColor(const Pin&) { return Color4f(0.9f, 0.8f, 0.2f); }

	// node position state
	virtual Point<float> GetNodePosition(Node*) = 0;
	virtual void SetNodePosition(Node*, const Point<float>&) = 0;

	// node preview
	virtual bool HasPreview(Node*) { return false; }
	virtual bool IsPreviewEnabled(Node*) { return false; }
	virtual void SetPreviewEnabled(Node*, bool) {}
	virtual void PreviewUI(Node*, UIContainer*) {}

	// node editing
	virtual bool CanDeleteNode(Node*) { return false; }
	virtual void DeleteNode(Node*) {}

	virtual bool CanDuplicateNode(Node*) { return false; }
	virtual Node* DuplicateNode(Node*) { return nullptr; }
};


struct ProcGraphLinkDragDropData : DragDropData
{
	static constexpr const char* NAME = "ProcGraphLinkDragDropData";
	ProcGraphLinkDragDropData(IProcGraph* graph, const IProcGraph::Pin& pin) :
		DragDropData(NAME),
		_graph(graph),
		_pin(pin)
	{}
	bool ShouldRender() override { return false; }
	IProcGraph* _graph;
	IProcGraph::Pin _pin;
};

DataCategoryTag DCT_EditProcGraph[1];
DataCategoryTag DCT_EditProcGraphNode[1];

struct ProcGraphEditor_NodePin : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		_sel = ctx->Push<Selectable>();
		*this + MakeDraggable();
		*_sel + MakeDraggable();
		*_sel + StackingDirection(!_pin.isOutput ? style::StackingDirection::LeftToRight : style::StackingDirection::RightToLeft);

		ctx->Text(_graph->GetPinName(_pin));

		if (!_pin.isOutput && !_graph->IsPinLinked(_pin))
		{
			// TODO not implemented right->left
			*_sel + Layout(style::layouts::StackExpand());

			_graph->InputPinEditorUI(_pin, ctx);
		}

		ctx->Pop();

		auto& cb = *ctx->Make<ColorBlock>();
		cb.SetColor(_graph->GetPinColor(_pin));
		cb + MakeOverlay() + ui::BoxSizing(style::BoxSizing::ContentBox) + Width(4) + Height(6);
		auto* pap = Allocate<style::PointAnchoredPlacement>();
		pap->pivot = { _pin.isOutput ? 0.f : 1.f, 0.5f };
		pap->anchor = { _pin.isOutput ? 1.f : 0.f, 0.5f };
		pap->useContentBox = true;
		cb.GetStyle().SetPlacement(pap);
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::DragStart)
		{
			_dragged = true;
			_sel->Init(_dragged || _dragHL);
			DragDrop::SetData(new ProcGraphLinkDragDropData(_graph, _pin));
		}
		if (e.type == UIEventType::DragEnd)
		{
			_dragged = false;
			_sel->Init(_dragged || _dragHL);
		}
		if (e.type == UIEventType::DragEnter)
		{
			if (auto* ddd = DragDrop::GetData<ProcGraphLinkDragDropData>())
			{
				if (_pin.end.node != ddd->_pin.end.node &&
					_pin.isOutput != ddd->_pin.isOutput)
				{
					_dragHL = _pin.isOutput
						? _graph->CanLink({ _pin.end, ddd->_pin.end })
						: _graph->CanLink({ ddd->_pin.end, _pin.end });
				}
			}
			_sel->Init(_dragged || _dragHL);
		}
		if (e.type == UIEventType::DragLeave)
		{
			_dragHL = false;
			_sel->Init(_dragged || _dragHL);
		}
		if (e.type == UIEventType::DragDrop)
		{
			if (auto* ddd = DragDrop::GetData<ProcGraphLinkDragDropData>())
			{
				if (_graph == ddd->_graph && _pin.isOutput != ddd->_pin.isOutput)
					_TryLink(ddd->_pin);
			}
		}

		if (e.type == UIEventType::ContextMenu)
		{
			auto& CM = ui::ContextMenu::Get();
			CM.Add("Unlink pin") = [this]() { _UnlinkPin(); };
			CM.Add("Link pin to...") = [this]() { DragDrop::SetData(new ProcGraphLinkDragDropData(_graph, _pin)); };
		}
	}
#if 0
	void OnPaint() override
	{
		Node::OnPaint();

		auto c = _graph->GetPinColor(_pin);
		if (_pin.isOutput)
			draw::RectCol(finalRectCPB.x1 - 2, finalRectCPB.y0, finalRectCPB.x1, finalRectCPB.y1, c);
		else
			draw::RectCol(finalRectCPB.x0, finalRectCPB.y0, finalRectCPB.x0 + 2, finalRectCPB.y1, c);
	}
#endif
	void OnDestroy() override
	{
		_UnregisterPin();
	}
	void Init(IProcGraph* graph, IProcGraph::Node* node, uintptr_t pin, bool isOutput)
	{
		_graph = graph;
		_pin = { node, pin, isOutput };
		_RegisterPin();
	}

	void _RegisterPin();
	void _UnregisterPin();

	void _TryLink(const IProcGraph::Pin& pin)
	{
		bool success = _pin.isOutput
			? _graph->TryLink({ _pin.end, pin.end })
			: _graph->TryLink({ pin.end, _pin.end });
		if (success)
		{
			Notify(DCT_EditProcGraphNode, _pin.end.node);
			Notify(DCT_EditProcGraphNode, pin.end.node);
		}
	}
	void _UnlinkPin()
	{
		std::vector<IProcGraph::Link> links;
		_graph->GetPinLinks(_pin, links);

		_graph->UnlinkPin(_pin);

		Notify(DCT_EditProcGraphNode, _pin.end.node);
		for (const auto& link : links)
		{
			Notify(DCT_EditProcGraphNode, (_pin.isOutput ? link.input : link.output).node);
		}
	}

	IProcGraph* _graph = nullptr;
	IProcGraph::Pin _pin = {};

	bool _dragged = false;
	bool _dragHL = false;
	Selectable* _sel = nullptr;
};

using PinUIMap = HashMap<IProcGraph::Pin, ProcGraphEditor_NodePin*, IProcGraph::PinHash>;

struct ProcGraphEditor_Node : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		Subscribe(DCT_EditProcGraphNode, _node);

		auto s = GetStyle(); // for style only
		s.SetWidth(style::Coord::Undefined());
		s.SetMinWidth(100);

		*ctx->Push<TabPanel>() + Width(style::Coord::Undefined());

		OnBuildTitleBar(ctx);
		OnBuildOutputPins(ctx);
		OnBuildPreview(ctx);
		OnBuildInputPins(ctx);

		ctx->Pop();
	}
	void Init(IProcGraph* graph, IProcGraph::Node* node)
	{
		_graph = graph;
		_node = node;
	}

	void OnEvent(UIEvent& e) override
	{
		Node::OnEvent(e);
		if (e.type == UIEventType::ContextMenu)
		{
			ContextMenu::Get().Add("Delete node", !_graph->CanDeleteNode(_node)) = [this]()
			{
				_graph->DeleteNode(_node);
				Notify(DCT_EditProcGraph, _graph);
			};
			if (_graph->HasPreview(_node))
			{
				ContextMenu::Get().Add("Show preview", false, _graph->IsPreviewEnabled(_node)) = [this]()
				{
					_graph->SetPreviewEnabled(_node, !_graph->IsPreviewEnabled(_node));
					Notify(DCT_EditProcGraphNode, _node);
				};
			}
		}
	}

	virtual void OnBuildTitleBar(UIContainer* ctx)
	{
		auto* placement = Allocate<style::PointAnchoredPlacement>();
		placement->bias = _graph->GetNodePosition(_node);
		GetStyle().SetPlacement(placement);

		*ctx->Push<Selectable>()->Init(_isDragging)
			+ Padding(0)
			+ MakeDraggable()
			+ EventHandler([this, placement](UIEvent& e)
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
		if (hasPreview)
		{
			bool showPreview = _graph->IsPreviewEnabled(_node);
			imm::EditBool(ctx, showPreview);
			_graph->SetPreviewEnabled(_node, showPreview);
		}
		ctx->Text(_graph->GetNodeName(_node)) + Padding(5, hasPreview ? 0 : 5, 5, 5);
		ctx->Pop();
	}

	virtual void OnBuildInputPins(UIContainer* ctx)
	{
		uintptr_t count = _graph->GetNodeInputCount(_node);
		for (uintptr_t i = 0; i < count; i++)
			ctx->Make<ProcGraphEditor_NodePin>()->Init(_graph, _node, i, false);
	}

	virtual void OnBuildOutputPins(UIContainer* ctx)
	{
		uintptr_t count = _graph->GetNodeOutputCount(_node);
		for (uintptr_t i = 0; i < count; i++)
			ctx->Make<ProcGraphEditor_NodePin>()->Init(_graph, _node, i, true);
	}

	virtual void OnBuildPreview(UIContainer* ctx)
	{
		bool showPreview = _graph->HasPreview(_node) && _graph->IsPreviewEnabled(_node);
		if (showPreview)
		{
			_graph->PreviewUI(_node, ctx);
		}
	}

	IProcGraph* _graph = nullptr;
	IProcGraph::Node* _node = nullptr;
	bool _isDragging = false;
	Point<float> _dragStartPos = {};
	Point<float> _dragStartMouse = {};
};

struct ProcGraphEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		Subscribe(DCT_EditProcGraph, _graph);

		*this + Height(style::Coord::Percent(100));
		//*ctx->Push<ListBox>() + Height(style::Coord::Percent(100));

		OnBuildNodes(ctx);

		//ctx->Pop();
	}
	void OnPaint() override
	{
		styleProps->paint_func(this);

		if (!drawCurrentLinksOnTop)
			OnDrawCurrentLinks();
		if (!drawPendingLinksOnTop)
			OnDrawPendingLinks();

		PaintChildren();

		if (drawCurrentLinksOnTop)
			OnDrawCurrentLinks();
		if (drawPendingLinksOnTop)
			OnDrawPendingLinks();
	}
	void Init(IProcGraph* graph)
	{
		_graph = graph;
	}

	virtual void OnBuildNodes(UIContainer* ctx)
	{
		std::vector<IProcGraph::Node*> nodes;
		_graph->GetNodes(nodes);
		for (auto* N : nodes)
		{
			ctx->Make<ProcGraphEditor_Node>()->Init(_graph, N);
		}
	}

	virtual void OnDrawCurrentLinks()
	{
		std::vector<IProcGraph::Link> links;
		_graph->GetLinks(links);

		std::vector<Point<float>> points;
		for (const auto& link : links)
		{
			points.clear();
			GetLinkPoints(link, points);
			OnDrawSingleLink(points, 0, 1, _graph->GetLinkColor(link, false, false));
		}
	}

	virtual void OnDrawPendingLinks()
	{
		if (auto* ddd = DragDrop::GetData<ProcGraphLinkDragDropData>())
		{
			if (ddd->_graph == _graph)
			{
				std::vector<Point<float>> points;
				GetConnectingLinkPoints(ddd->_pin, points);
				OnDrawSingleLink(points, ddd->_pin.isOutput ? 1 : -1, 1, _graph->GetNewLinkColor(ddd->_pin));
			}
		}
	}

	virtual void GetLinkPoints(const IProcGraph::Link& link, std::vector<Point<float>>& outPoints)
	{
		auto A = link.output;
		auto B = link.input;
		if (auto* LA = pinUIMap.get({ A, true }))
		{
			if (auto* LB = pinUIMap.get({ B, false }))
			{
				auto PA = GetPinPos(LA);
				auto PB = GetPinPos(LB);
				GetLinkPointsRaw(PA, PB, 0, outPoints);
			}
		}
	}

	virtual void GetConnectingLinkPoints(const IProcGraph::Pin& pin, std::vector<Point<float>>& outPoints)
	{
		if (auto* P = pinUIMap.get(pin))
		{
			auto p0 = GetPinPos(P);
			Point<float> p1 = { system->eventSystem.prevMouseX, system->eventSystem.prevMouseY };
			if (!pin.isOutput)
				std::swap(p0, p1);
			GetLinkPointsRaw(p0, p1, pin.isOutput ? 1 : -1, outPoints);
		}
	}

	// connecting = 0 (not), 1 (output-*), -1 (*-input)
	virtual void GetLinkPointsRaw(const Point<float>& p0, const Point<float>& p1, int connecting, std::vector<Point<float>>& outPoints)
	{
		if (linkExtension)
		{
			outPoints.push_back(p0);
			Point<float> b0 = { p0.x + linkExtension, p0.y };
			Point<float> b3 = { p1.x - linkExtension, p1.y };
			GetLinkPointsRawInner(b0, b3, outPoints);
			outPoints.push_back(p1);
		}
		else
			GetLinkPointsRawInner(p0, p1, outPoints);
	}
	virtual void GetTangents(const Point<float>& b0, const Point<float>& b3, Point<float>& b1, Point<float>& b2)
	{
		float tanLen = fabsf(b0.x - b3.x) * 0.5f;
		if (tanLen < 4)
			tanLen = 4;
		b1 = { b0.x + tanLen, b0.y };
		b2 = { b3.x - tanLen, b3.y };
	}
	virtual void GetLinkPointsRawInner(const Point<float>& b0, const Point<float>& b3, std::vector<Point<float>>& outPoints)
	{
		Point<float> b1, b2;
		GetTangents(b0, b3, b1, b2);

		// TODO adaptive
		for (int i = 0; i < 30; i++)
		{
			float q = i / 29.f;
			outPoints.push_back(BezierPoint(b0, b1, b2, b3, q));
		}
	}

	static Point<float> Lerp(const Point<float>& a, const Point<float>& b, float q)
	{
		return { lerp(a.x, b.x, q), lerp(a.y, b.y, q) };
	}
	static Point<float> BezierPoint(const Point<float>& b0, const Point<float>& b1, const Point<float>& b2, const Point<float>& b3, float q)
	{
		auto b01 = Lerp(b0, b1, q);
		auto b12 = Lerp(b1, b2, q);
		auto b23 = Lerp(b2, b3, q);
		auto b012 = Lerp(b01, b12, q);
		auto b123 = Lerp(b12, b23, q);
		return Lerp(b012, b123, q);
	}

	virtual Point<float> GetPinPos(ProcGraphEditor_NodePin* P)
	{
		return
		{
			!P->_pin.isOutput ? P->finalRectCPB.x0 : P->finalRectCPB.x1,
			(P->finalRectCPB.y0 + P->finalRectCPB.y1) * 0.5f,
		};
	}

	virtual void OnDrawSingleLink(const std::vector<Point<float>>& points, int connecting, float width, Color4b color)
	{
		// TODO polyline
		for (size_t i = 0; i + 1 < points.size(); i++)
		{
			const auto& PA = points[i];
			const auto& PB = points[i + 1];
			draw::AALineCol(
				PA.x, PA.y,
				PB.x, PB.y,
				width + 2, Color4b::Black());
		}
		for (size_t i = 0; i + 1 < points.size(); i++)
		{
			const auto& PA = points[i];
			const auto& PB = points[i + 1];
			draw::AALineCol(
				PA.x, PA.y,
				PB.x, PB.y,
				width, color);
		}
	}

	IProcGraph* _graph = nullptr;
	PinUIMap pinUIMap;

	bool drawCurrentLinksOnTop = false;
	bool drawPendingLinksOnTop = true;
	float linkExtension = 8.0f;
};

void ProcGraphEditor_NodePin::_RegisterPin()
{
	if (auto* p = FindParentOfType<ProcGraphEditor>())
	{
		p->pinUIMap.insert(_pin, this);
	}
}

void ProcGraphEditor_NodePin::_UnregisterPin()
{
	if (auto* p = FindParentOfType<ProcGraphEditor>())
	{
		p->pinUIMap.erase(_pin);
	}
}

} // ui


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

	bool CanLink(const LinkEnd& eout, const LinkEnd& ein)
	{
		assert(eout.isOutput && !ein.isOutput);
		if (eout.node == ein.node)
			return false;
		auto typeout = eout.node->GetOutputType(eout.pin);
		auto typein = ein.node->GetInputType(ein.pin);
		if (typeout != typein)
			return false;
		return true;
	}
	bool TryLink(const LinkEnd& eout, const LinkEnd& ein)
	{
		if (!CanLink(eout, ein))
			return false;
		AddLink(eout, ein);
		return true;
	}
	Type GetPinType(const LinkEnd& e)
	{
		return e.isOutput ? e.node->GetOutputType(e.pin) : e.node->GetInputType(e.pin);
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

struct ScaleVec : Graph::Node
{
	Node* Clone() override { return new ScaleVec(*this); }
	const char* GetName() override { return "Scale vector"; }
	int GetNumInputs() override { return 2; }
	int GetNumOutputs() override { return 1; }
	const char* GetInputName(int which) override { return which == 0 ? "Vector" : "Scale"; }
	const char* GetOutputName(int which) override { return "Vector"; }
	Graph::Type GetInputType(int which) override { return which == 0 ? Graph::Type::Vector : Graph::Type::Scalar; }
	Graph::Type GetOutputType(int) override { return Graph::Type::Vector; }
	float* GetInputDefaultValuePtr(int which) override { return which == 0 ? defInputV : &defInputS; }
	bool HasPreview() override { return true; }

	float defInputV[3] = {};
	float defInputS = 1;
};

struct DotProduct : Graph::Node
{
	Node* Clone() override { return new DotProduct(*this); }
	const char* GetName() override { return "Dot product"; }
	int GetNumInputs() override { return 2; }
	int GetNumOutputs() override { return 1; }
	const char* GetInputName(int which) override { return which == 0 ? "A" : "B"; }
	const char* GetOutputName(int which) override { return "Output"; }
	Graph::Type GetInputType(int) override { return Graph::Type::Vector; }
	Graph::Type GetOutputType(int) override { return Graph::Type::Scalar; }
	float* GetInputDefaultValuePtr(int which) override { return defInputs[which]; }
	bool HasPreview() override { return false; }

	float defInputs[2][3] = {};
};


static ui::Color4b g_graphColors[] = 
{
	ui::Color4f(0.9f, 0.1f, 0),
	ui::Color4f(0.3f, 0.6f, 0.9f),
};

struct GraphImpl : ui::IProcGraph
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

	std::string GetNodeName(Node* node) override
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

	std::string GetPinName(const Pin& pin) override
	{
		auto* node = static_cast<Graph::Node*>(pin.end.node);
		return pin.isOutput ? node->GetOutputName(pin.end.num) : node->GetInputName(pin.end.num);
	}
	ui::Color4b GetPinColor(const Pin& pin) override
	{
		return g_graphColors[int(graph->GetPinType({ static_cast<Graph::Node*>(pin.end.node), int(pin.end.num), pin.isOutput }))];
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
	bool CanLink(const Link& link) override
	{
		return graph->CanLink(
			{ static_cast<Graph::Node*>(link.output.node), int(link.output.num), true },
			{ static_cast<Graph::Node*>(link.input.node), int(link.input.num), false }
		);
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
	ui::Color4b GetLinkColor(const Link& link, bool selected, bool hovered) override
	{
		return GetPinColor({ link.output, true });
	}
	virtual ui::Color4b GetNewLinkColor(const Pin& pin)
	{
		return GetPinColor(pin);
	}

	Point<float> GetNodePosition(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->position;
	}
	void SetNodePosition(Node* node, const Point<float>& pos) override
	{
		static_cast<Graph::Node*>(node)->position = pos;
	}

	bool HasPreview(Node* node)
	{
		return static_cast<Graph::Node*>(node)->HasPreview();
	}
	bool IsPreviewEnabled(Node* node)
	{
		return static_cast<Graph::Node*>(node)->showPreview;
	}
	void SetPreviewEnabled(Node* node, bool enabled)
	{
		static_cast<Graph::Node*>(node)->showPreview = enabled;
	}
	void PreviewUI(Node*, UIContainer* ctx)
	{
		ctx->MakeWithText<ui::Panel>("Preview");
	}

	bool CanDeleteNode(Node*) override { return true; }
	void DeleteNode(Node* node) override
	{
		for (auto it = graph->links.begin(), itend = graph->links.end(); it != itend; )
		{
			if ((*it)->input.node == node || (*it)->output.node == node)
			{
				delete *it;
				it = graph->links.erase(it);
			}
			else
				it++;
		}
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
		Graph::Node* scaleVec = new ScaleVec();

		makeVec1->position = { 50, 50 };
		dotProd->position = { 200, 50 };
		scaleVec->position = { 200, 150 };

		graph.nodes.push_back(makeVec1);
		graph.nodes.push_back(dotProd);
		graph.nodes.push_back(scaleVec);

		graph.links.push_back(new Graph::Link{ { makeVec1, 0, true }, { dotProd, 0 } });
		graph.links.push_back(new Graph::Link{ { makeVec1, 0, true }, { dotProd, 1 } });
		graph.links.push_back(new Graph::Link{ { makeVec1, 0, true }, { scaleVec, 0 } });
	}
	void Render(UIContainer* ctx) override
	{
		*this + ui::Height(style::Coord::Percent(100));
		ctx->Make<ui::ProcGraphEditor>()->Init(Allocate<GraphImpl>(&graph));
	}

	Graph graph;
};
void Demo_NodeGraphEditor(UIContainer* ctx)
{
	ctx->Make<NodeGraphEditorDemo>();
}

