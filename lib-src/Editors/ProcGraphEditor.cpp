
#include "ProcGraphEditor.h"

#include "../Model/Controls.h"
#include "../Model/Graphics.h"
#include "../Model/ImmediateMode.h"
#include "../Model/Layout.h"
#include "../Layout_PaddingElement.h"
#include "../Model/Menu.h"
#include "../Model/System.h"
#include "../Model/Theme.h"


namespace ui {

MulticastDelegate<IProcGraph*> OnProcGraphEdit;
MulticastDelegate<IProcGraph*, IProcGraph::Node*> OnProcGraphNodeEdit;


void ProcGraphEditor_NodePin::Build()
{
	auto& ple = Push<PlacementLayoutElement>();
	_sel = &Push<Selectable>();
	*this + MakeDraggable();
	*_sel + MakeDraggable();

	if (!_pin.isOutput && !_graph->IsPinLinked(_pin))
	{
		// TODO not implemented right->left
		Push<StackExpandLTRLayoutElement>();

		Text(_graph->GetPinName(_pin));

		_graph->InputPinEditorUI(_pin);

		Pop();
	}
	else
	{
		Text(_graph->GetPinName(_pin));
	}

	Pop();

	auto tmpl = ple.GetSlotTemplate();
	tmpl->measure = false;
	auto* pap = UI_BUILD_ALLOC(PointAnchoredPlacement)();
	pap->pivot = { _pin.isOutput ? 0.5f : 0.5f, 0.5f };
	pap->anchor = { _pin.isOutput ? 1.f : 0.f, 0.5f };
	tmpl->placement = pap;
	auto& cb = Push<ColorBlock>().SetSize(4, 7);
	cb.SetColor(_graph->GetPinColor(_pin));
	Pop();

	Pop();
}

void ProcGraphEditor_NodePin::OnEvent(Event& e)
{
	if (e.type == EventType::DragStart)
	{
		_dragged = true;
		_sel->Init(_dragged || _dragHL);
		DragDrop::SetData(new ProcGraphLinkDragDropData(_graph, _pin));
	}
	if (e.type == EventType::DragEnd)
	{
		_dragged = false;
		_sel->Init(_dragged || _dragHL);
	}
	if (e.type == EventType::DragEnter)
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
	if (e.type == EventType::DragLeave)
	{
		_dragHL = false;
		_sel->Init(_dragged || _dragHL);
	}
	if (e.type == EventType::DragDrop)
	{
		if (auto* ddd = DragDrop::GetData<ProcGraphLinkDragDropData>())
		{
			if (_graph == ddd->_graph && _pin.isOutput != ddd->_pin.isOutput)
				_TryLink(ddd->_pin);
		}
	}

	if (e.type == EventType::ContextMenu)
	{
		auto& CM = ContextMenu::Get();
		CM.Add("Unlink pin") = [this]() { _UnlinkPin(); };
		CM.Add("Link pin to...") = [this]() { DragDrop::SetData(new ProcGraphLinkDragDropData(_graph, _pin)); };

		_graph->OnPinContextMenu(_pin, CM);
	}
}

void ProcGraphEditor_NodePin::OnPaint(const UIPaintContext& ctx)
{
	Buildable::OnPaint(ctx);

#if 0
	auto c = _graph->GetPinColor(_pin);
	if (_pin.isOutput)
		draw::RectCol(finalRectCP.x1 - 2, finalRectCP.y0, finalRectCP.x1, finalRectCP.y1, c);
	else
		draw::RectCol(finalRectCP.x0, finalRectCP.y0, finalRectCP.x0 + 2, finalRectCP.y1, c);
#endif
}

void ProcGraphEditor_NodePin::OnExitTree()
{
	_UnregisterPin();
}

void ProcGraphEditor_NodePin::Init(IProcGraph* graph, IProcGraph::Node* node, uintptr_t pin, bool isOutput)
{
	_graph = graph;
	_pin = { node, pin, isOutput };
	_RegisterPin();
}

void ProcGraphEditor_NodePin::_RegisterPin()
{
	if (auto* p = FindParentOfType<ProcGraphEditor>())
	{
		p->pinUIMap.Insert(_pin, this);
	}
}

void ProcGraphEditor_NodePin::_UnregisterPin()
{
	if (auto* p = FindParentOfType<ProcGraphEditor>())
	{
		p->pinUIMap.Remove(_pin);
	}
}

void ProcGraphEditor_NodePin::_TryLink(const IProcGraph::Pin& pin)
{
	bool success = _pin.isOutput
		? _graph->TryLink({ _pin.end, pin.end })
		: _graph->TryLink({ pin.end, _pin.end });
	if (success)
	{
		OnProcGraphNodeEdit.Call(_graph, _pin.end.node);
		OnProcGraphNodeEdit.Call(_graph, pin.end.node);
	}
}

void ProcGraphEditor_NodePin::_UnlinkPin()
{
	Array<IProcGraph::Link> links;
	_graph->GetPinLinks(_pin, links);

	_graph->UnlinkPin(_pin);

	OnProcGraphNodeEdit.Call(_graph, _pin.end.node);
	for (const auto& link : links)
	{
		OnProcGraphNodeEdit.Call(_graph, (_pin.isOutput ? link.input : link.output).node);
	}
}


void ProcGraphEditor_Node::Build()
{
	BuildMulticastDelegateAdd(OnProcGraphNodeEdit, [this](IProcGraph* g, IProcGraph::Node* n)
	{
		if (g == _graph && n == _node)
			Rebuild();
	});

	Push<FrameElement>().SetDefaultFrameStyle(DefaultFrameStyle::ProcGraphNode);
	Push<StackTopDownLayoutElement>();

	OnBuildTitleBar();
	OnBuildOutputPins();
	OnBuildPreview();
	OnBuildEditor();
	OnBuildInputPins();

	Pop();
	Pop();
}

void ProcGraphEditor_Node::OnEvent(Event& e)
{
	Buildable::OnEvent(e);
	if (e.type == EventType::ContextMenu)
	{
		ContextMenu::Get().Add("Delete node", !_graph->CanDeleteNode(_node)) = [this]()
		{
			_graph->DeleteNode(_node);
			OnProcGraphEdit.Call(_graph);
		};
		ContextMenu::Get().Add("Duplicate node", !_graph->CanDuplicateNode(_node)) = [this]()
		{
			_graph->DuplicateNode(_node);
			OnProcGraphEdit.Call(_graph);
		};
		if (_graph->HasPreview(_node))
		{
			ContextMenu::Get().Add("Show preview", false, _graph->IsPreviewEnabled(_node)) = [this]()
			{
				_graph->SetPreviewEnabled(_node, !_graph->IsPreviewEnabled(_node));
				OnProcGraphNodeEdit.Call(_graph, _node);
			};
		}

		_graph->OnNodeContextMenu(_node, ContextMenu::Get());
	}
	if (e.type == EventType::Change ||
		e.type == EventType::Commit ||
		e.type == EventType::IMChange)
	{
		_graph->OnEditNode(e, _node);
	}
}

void ProcGraphEditor_Node::Init(IProcGraph* graph, IProcGraph::Node* node, Point2f vOff)
{
	_graph = graph;
	_node = node;
	_viewOffset = vOff;
}

void ProcGraphEditor_Node::OnBuildTitleBar()
{
	auto& sel = Push<Selectable>().Init(_isDragging);
	sel
		.SetPadding(0)
		+ MakeDraggable()
		+ AddEventHandler([this](Event& e)
	{
		if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
		{
			_isDragging = true;
			_dragStartMouse = e.context->clickStartPositions[int(MouseButton::Left)];
			_dragStartPos = _graph->GetNodePosition(_node);
		}
		if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
			_isDragging = false;
		if (e.type == EventType::MouseMove && _isDragging)
		{
			Point2f newPos = _dragStartPos + e.position - _dragStartMouse;
			_graph->SetNodePosition(_node, newPos);
			parent->_OnChangeStyle();
		}
	});

	Push<StackExpandLTRLayoutElement>();
	bool hasPreview = _graph->HasPreview(_node);
	if (hasPreview)
	{
		bool showPreview = _graph->IsPreviewEnabled(_node);
		imEditBool(showPreview);
		_graph->SetPreviewEnabled(_node, showPreview);
	}
	auto& lf = MakeWithText<LabelFrame>(_graph->GetNodeName(_node));
	lf.frameStyle.font.weight = FontWeight::Bold;
	lf.frameStyle.font.style = FontStyle::Italic;
	Pop();
	Pop();
}

void ProcGraphEditor_Node::OnBuildEditor()
{
	_graph->NodePropertyEditorUI(_node);
}

void ProcGraphEditor_Node::OnBuildInputPins()
{
	uintptr_t count = _graph->GetNodeInputCount(_node);
	for (uintptr_t i = 0; i < count; i++)
		Make<ProcGraphEditor_NodePin>().Init(_graph, _node, i, false);
}

void ProcGraphEditor_Node::OnBuildOutputPins()
{
	uintptr_t count = _graph->GetNodeOutputCount(_node);
	for (uintptr_t i = 0; i < count; i++)
		Make<ProcGraphEditor_NodePin>().Init(_graph, _node, i, true);
}

void ProcGraphEditor_Node::OnBuildPreview()
{
	bool showPreview = _graph->HasPreview(_node) && _graph->IsPreviewEnabled(_node);
	if (showPreview)
	{
		_graph->PreviewUI(_node);
	}
}


struct ProcGraphLayoutElement : ListLayoutElementBase<ListLayoutSlotBase>
{
	ProcGraphEditor* PGE = nullptr;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{
		return EstSizeRange::SoftExact(containerSize.x);
	}
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		return EstSizeRange::SoftExact(containerSize.y);
	}
	void OnLayout(const UIRect& rect, LayoutInfo info) override
	{
		auto rectSize = rect.GetSize();
		for (auto& slot : _slots)
		{
			auto* N = static_cast<ProcGraphEditor_Node*>(slot._obj);
			auto pos = PGE->_graph->GetNodePosition(N->_node) + PGE->viewOffset + rect.GetMin();
			auto wr = N->CalcEstimatedWidth(rectSize, ui::EstSizeType::Expanding);
			auto hr = N->CalcEstimatedHeight(rectSize, ui::EstSizeType::Expanding);
			N->PerformLayout({ pos.x, pos.y, pos.x + wr.softMin, pos.y + hr.softMin }, info.WithoutAnyFill());
		}
		_finalRect = rect;
	}
};


void ProcGraphEditor::Build()
{
	BuildMulticastDelegateAdd(OnProcGraphEdit, [this](IProcGraph* g)
	{
		if (g == _graph)
			Rebuild();
	});

	Push<ProcGraphLayoutElement>().PGE = this;
	OnBuildNodes();
	Pop();
}

void ProcGraphEditor::OnReset()
{
	Buildable::OnReset();

	_graph = nullptr;
	drawCurrentLinksOnTop = false;
	drawPendingLinksOnTop = true;
	linkExtension = 8.0f;
}

void ProcGraphEditor::OnEvent(Event& e)
{
	Buildable::OnEvent(e);

	if (e.type == EventType::ContextMenu)
	{
		if (!ContextMenu::Get().HasAny())
			OnMakeCreationMenu(ContextMenu::Get());
	}

	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Middle)
	{
		origMPos = e.position;
		origVPos = viewOffset;
	}
	if (e.type == EventType::MouseMove && HasFlags(UIObject_IsClickedM))
	{
		viewOffset = origVPos + e.position - origMPos;
		Rebuild();
	}
}

void ProcGraphEditor::OnPaint(const UIPaintContext& ctx)
{
	if (!drawCurrentLinksOnTop)
		OnDrawCurrentLinks();
	if (!drawPendingLinksOnTop)
		OnDrawPendingLinks();

	Buildable::OnPaint(ctx);

	if (drawCurrentLinksOnTop)
		OnDrawCurrentLinks();
	if (drawPendingLinksOnTop)
		OnDrawPendingLinks();
}

void ProcGraphEditor::Init(IProcGraph* graph)
{
	_graph = graph;
}

void ProcGraphEditor::OnBuildNodes()
{
	Array<IProcGraph::Node*> nodes;
	_graph->GetNodes(nodes);
	for (auto* N : nodes)
	{
		Make<ProcGraphEditor_Node>().Init(_graph, N, viewOffset);
	}
}

void ProcGraphEditor::OnMakeCreationMenu(MenuItemCollection& menu)
{
	Array<IProcGraph::NodeTypeEntry> nodes;
	_graph->GetAvailableNodeTypes(nodes);

	auto& CM = ContextMenu::Get();
	for (auto& e : nodes)
	{
		CM.Add(e.path) = [this, graph{ _graph }, e{ e }]()
		{
			auto* node = e.func(graph, e.path, e.id);
			graph->SetNodePosition(node, system->eventSystem.clickStartPositions[1]);
			OnProcGraphEdit.Call(graph);
		};
	}

	if (menu.HasAny())
	{
		ContextMenu::Get().Add("- Create a new node -", true, false, MenuItemCollection::MIN_SAFE_PRIORITY);
	}
}

void ProcGraphEditor::OnDrawCurrentLinks()
{
	Array<IProcGraph::Link> links;
	_graph->GetLinks(links);

	Array<Point2f> points;
	for (const auto& link : links)
	{
		points.Clear();
		GetLinkPoints(link, points);
		OnDrawSingleLink(points, 0, 1, _graph->GetLinkColor(link, false, false));
	}
}

void ProcGraphEditor::OnDrawPendingLinks()
{
	if (auto* ddd = DragDrop::GetData<ProcGraphLinkDragDropData>())
	{
		if (ddd->_graph == _graph)
		{
			Array<Point2f> points;
			GetConnectingLinkPoints(ddd->_pin, points);
			OnDrawSingleLink(points, ddd->_pin.isOutput ? 1 : -1, 1, _graph->GetNewLinkColor(ddd->_pin));
		}
	}
}

void ProcGraphEditor::GetLinkPoints(const IProcGraph::Link& link, Array<Point2f>& outPoints)
{
	auto A = link.output;
	auto B = link.input;
	if (auto* LA = pinUIMap.GetValueOrDefault({ A, true }))
	{
		if (auto* LB = pinUIMap.GetValueOrDefault({ B, false }))
		{
			auto PA = GetPinPos(LA);
			auto PB = GetPinPos(LB);
			GetLinkPointsRaw(PA, PB, 0, outPoints);
		}
	}
}

void ProcGraphEditor::GetConnectingLinkPoints(const IProcGraph::Pin& pin, Array<Point2f>& outPoints)
{
	if (auto* P = pinUIMap.GetValueOrDefault(pin))
	{
		auto p0 = GetPinPos(P);
		Point2f p1 = system->eventSystem.prevMousePos;
		if (!pin.isOutput)
			std::swap(p0, p1);
		GetLinkPointsRaw(p0, p1, pin.isOutput ? 1 : -1, outPoints);
	}
}

void ProcGraphEditor::GetLinkPointsRaw(const Point2f& p0, const Point2f& p1, int connecting, Array<Point2f>& outPoints)
{
	if (linkExtension)
	{
		outPoints.Append(p0);
		Point2f b0 = { p0.x + linkExtension, p0.y };
		Point2f b3 = { p1.x - linkExtension, p1.y };
		GetLinkPointsRawInner(b0, b3, outPoints);
		outPoints.Append(p1);
	}
	else
		GetLinkPointsRawInner(p0, p1, outPoints);
}

void ProcGraphEditor::GetTangents(const Point2f& b0, const Point2f& b3, Point2f& b1, Point2f& b2)
{
	float tanLen = fabsf(b0.x - b3.x) * 0.5f;
	if (tanLen < 4)
		tanLen = 4;
	b1 = { b0.x + tanLen, b0.y };
	b2 = { b3.x - tanLen, b3.y };
}

static Point2f Lerp(const Point2f& a, const Point2f& b, float q)
{
	return { lerp(a.x, b.x, q), lerp(a.y, b.y, q) };
}

static Point2f BezierPoint(const Point2f& b0, const Point2f& b1, const Point2f& b2, const Point2f& b3, float q)
{
	auto b01 = Lerp(b0, b1, q);
	auto b12 = Lerp(b1, b2, q);
	auto b23 = Lerp(b2, b3, q);
	auto b012 = Lerp(b01, b12, q);
	auto b123 = Lerp(b12, b23, q);
	return Lerp(b012, b123, q);
}

void ProcGraphEditor::GetLinkPointsRawInner(const Point2f& b0, const Point2f& b3, Array<Point2f>& outPoints)
{
	Point2f b1, b2;
	GetTangents(b0, b3, b1, b2);

	// TODO adaptive
	for (int i = 0; i < 30; i++)
	{
		float q = i / 29.f;
		outPoints.Append(BezierPoint(b0, b1, b2, b3, q));
	}
}

Point2f ProcGraphEditor::GetPinPos(ProcGraphEditor_NodePin* P)
{
	auto frP = P->GetFinalRect();
	return
	{
		!P->_pin.isOutput ? frP.x0 : frP.x1,
		(frP.y0 + frP.y1) * 0.5f,
	};
}

void ProcGraphEditor::OnDrawSingleLink(const Array<Point2f>& points, int connecting, float width, Color4b color)
{
	draw::AALineCol(points, width + 2, Color4b::Black(), false);
	draw::AALineCol(points, width, color, false);
}

} // ui
