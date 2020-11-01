
#include "ProcGraphEditor.h"

#include "../Model/Controls.h"
#include "../Model/Graphics.h"
#include "../Model/ImmediateMode.h"
#include "../Model/Menu.h"
#include "../Model/System.h"
#include "../Model/Theme.h"

namespace ui {

DataCategoryTag DCT_EditProcGraph[1];
DataCategoryTag DCT_EditProcGraphNode[1];


void ProcGraphEditor_NodePin::Render(UIContainer* ctx)
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
	cb + BoxSizing(style::BoxSizing::ContentBox) + Width(4) + Height(6);
	auto* pap = Allocate<style::PointAnchoredPlacement>();
	pap->pivot = { _pin.isOutput ? 0.f : 1.f, 0.5f };
	pap->anchor = { _pin.isOutput ? 1.f : 0.f, 0.5f };
	pap->useContentBox = true;
	cb.GetStyle().SetPlacement(pap);
}

void ProcGraphEditor_NodePin::OnEvent(UIEvent& e)
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
		auto& CM = ContextMenu::Get();
		CM.Add("Unlink pin") = [this]() { _UnlinkPin(); };
		CM.Add("Link pin to...") = [this]() { DragDrop::SetData(new ProcGraphLinkDragDropData(_graph, _pin)); };

		_graph->OnPinContextMenu(_pin, CM);
	}
}

void ProcGraphEditor_NodePin::OnPaint()
{
	Node::OnPaint();

#if 0
	auto c = _graph->GetPinColor(_pin);
	if (_pin.isOutput)
		draw::RectCol(finalRectCPB.x1 - 2, finalRectCPB.y0, finalRectCPB.x1, finalRectCPB.y1, c);
	else
		draw::RectCol(finalRectCPB.x0, finalRectCPB.y0, finalRectCPB.x0 + 2, finalRectCPB.y1, c);
#endif
}

void ProcGraphEditor_NodePin::OnDestroy()
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

void ProcGraphEditor_NodePin::_TryLink(const IProcGraph::Pin& pin)
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

void ProcGraphEditor_NodePin::_UnlinkPin()
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


void ProcGraphEditor_Node::Render(UIContainer* ctx)
{
	Subscribe(DCT_EditProcGraphNode, _node);

	auto s = GetStyle(); // for style only
	s.SetWidth(style::Coord::Undefined());
	s.SetMinWidth(100);

	*ctx->Push<TabPanel>() + Width(style::Coord::Undefined()) + Margin(0);

	OnBuildTitleBar(ctx);
	OnBuildOutputPins(ctx);
	OnBuildPreview(ctx);
	OnBuildEditor(ctx);
	OnBuildInputPins(ctx);

	ctx->Pop();
}

void ProcGraphEditor_Node::OnEvent(UIEvent& e)
{
	Node::OnEvent(e);
	if (e.type == UIEventType::ContextMenu)
	{
		ContextMenu::Get().Add("Delete node", !_graph->CanDeleteNode(_node)) = [this]()
		{
			_graph->DeleteNode(_node);
			Notify(DCT_EditProcGraph, _graph);
		};
		ContextMenu::Get().Add("Duplicate node", !_graph->CanDuplicateNode(_node)) = [this]()
		{
			_graph->DuplicateNode(_node);
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

		_graph->OnNodeContextMenu(_node, ContextMenu::Get());
	}
	if (e.type == UIEventType::Change ||
		e.type == UIEventType::Commit ||
		e.type == UIEventType::IMChange)
	{
		_graph->OnEditNode(e, _node);
	}
}

void ProcGraphEditor_Node::Init(IProcGraph* graph, IProcGraph::Node* node, Point<float> vOff)
{
	_graph = graph;
	_node = node;
	_viewOffset = vOff;
}

void ProcGraphEditor_Node::OnBuildTitleBar(UIContainer* ctx)
{
	auto* placement = Allocate<style::PointAnchoredPlacement>();
	placement->bias = _graph->GetNodePosition(_node) + _viewOffset;
	GetStyle().SetPlacement(placement);

	auto* sel = ctx->Push<Selectable>()->Init(_isDragging);
	sel->GetStyle().SetFontWeight(style::FontWeight::Bold);
	sel->GetStyle().SetFontStyle(style::FontStyle::Italic);
	*sel
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
			Point<float> newPos =
			{
				_dragStartPos.x + curMousePos.x - _dragStartMouse.x,
				_dragStartPos.y + curMousePos.y - _dragStartMouse.y,
			};
			placement->bias = newPos + _viewOffset;
			_graph->SetNodePosition(_node, newPos);
			_OnChangeStyle();
		}
	});

	bool hasPreview = _graph->HasPreview(_node);
	if (hasPreview)
	{
		bool showPreview = _graph->IsPreviewEnabled(_node);
		imm::EditBool(ctx, showPreview, nullptr);
		_graph->SetPreviewEnabled(_node, showPreview);
	}
	ctx->Text(_graph->GetNodeName(_node)) + Padding(5, hasPreview ? 0 : 5, 5, 5);
	ctx->Pop();
}

void ProcGraphEditor_Node::OnBuildEditor(UIContainer* ctx)
{
	_graph->NodePropertyEditorUI(_node, ctx);
}

void ProcGraphEditor_Node::OnBuildInputPins(UIContainer* ctx)
{
	uintptr_t count = _graph->GetNodeInputCount(_node);
	for (uintptr_t i = 0; i < count; i++)
		ctx->Make<ProcGraphEditor_NodePin>()->Init(_graph, _node, i, false);
}

void ProcGraphEditor_Node::OnBuildOutputPins(UIContainer* ctx)
{
	uintptr_t count = _graph->GetNodeOutputCount(_node);
	for (uintptr_t i = 0; i < count; i++)
		ctx->Make<ProcGraphEditor_NodePin>()->Init(_graph, _node, i, true);
}

void ProcGraphEditor_Node::OnBuildPreview(UIContainer* ctx)
{
	bool showPreview = _graph->HasPreview(_node) && _graph->IsPreviewEnabled(_node);
	if (showPreview)
	{
		_graph->PreviewUI(_node, ctx);
	}
}


void ProcGraphEditor::Render(UIContainer* ctx)
{
	Subscribe(DCT_EditProcGraph, _graph);

	*this + Height(style::Coord::Percent(100));
	//*ctx->Push<ListBox>() + Height(style::Coord::Percent(100));

	OnBuildNodes(ctx);

	//ctx->Pop();
}

void ProcGraphEditor::OnEvent(UIEvent& e)
{
	Node::OnEvent(e);

	if (e.type == UIEventType::ContextMenu)
	{
		if (!ContextMenu::Get().HasAny())
			OnMakeCreationMenu(ContextMenu::Get());
	}

	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Middle)
	{
		origMPos = { e.x, e.y };
		origVPos = viewOffset;
	}
	if (e.type == UIEventType::MouseMove && HasFlags(UIObject_IsClickedM))
	{
		Point<float> mpos = { e.x, e.y };
		viewOffset = origVPos + mpos - origMPos;
		Rerender();
	}
}

void ProcGraphEditor::OnPaint()
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

void ProcGraphEditor::Init(IProcGraph* graph)
{
	_graph = graph;
}

void ProcGraphEditor::OnBuildNodes(UIContainer* ctx)
{
	std::vector<IProcGraph::Node*> nodes;
	_graph->GetNodes(nodes);
	for (auto* N : nodes)
	{
		ctx->Make<ProcGraphEditor_Node>()->Init(_graph, N, viewOffset);
	}
}

void ProcGraphEditor::OnMakeCreationMenu(MenuItemCollection& menu)
{
	std::vector<IProcGraph::NodeTypeEntry> nodes;
	_graph->GetAvailableNodeTypes(nodes);

	auto& CM = ContextMenu::Get();
	for (auto& e : nodes)
	{
		CM.Add(e.path) = [this, graph{ _graph }, e{ e }]()
		{
			auto* node = e.func(graph, e.path, e.id);
			graph->SetNodePosition(node, system->eventSystem.clickStartPositions[1]);
			Notify(DCT_EditProcGraph, graph);
		};
	}

	if (menu.HasAny())
	{
		ContextMenu::Get().Add("- Create a new node -", true, false, MenuItemCollection::MIN_SAFE_PRIORITY);
	}
}

void ProcGraphEditor::OnDrawCurrentLinks()
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

void ProcGraphEditor::OnDrawPendingLinks()
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

void ProcGraphEditor::GetLinkPoints(const IProcGraph::Link& link, std::vector<Point<float>>& outPoints)
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

void ProcGraphEditor::GetConnectingLinkPoints(const IProcGraph::Pin& pin, std::vector<Point<float>>& outPoints)
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

void ProcGraphEditor::GetLinkPointsRaw(const Point<float>& p0, const Point<float>& p1, int connecting, std::vector<Point<float>>& outPoints)
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

void ProcGraphEditor::GetTangents(const Point<float>& b0, const Point<float>& b3, Point<float>& b1, Point<float>& b2)
{
	float tanLen = fabsf(b0.x - b3.x) * 0.5f;
	if (tanLen < 4)
		tanLen = 4;
	b1 = { b0.x + tanLen, b0.y };
	b2 = { b3.x - tanLen, b3.y };
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

void ProcGraphEditor::GetLinkPointsRawInner(const Point<float>& b0, const Point<float>& b3, std::vector<Point<float>>& outPoints)
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

Point<float> ProcGraphEditor::GetPinPos(ProcGraphEditor_NodePin* P)
{
	return
	{
		!P->_pin.isOutput ? P->finalRectCPB.x0 : P->finalRectCPB.x1,
		(P->finalRectCPB.y0 + P->finalRectCPB.y1) * 0.5f,
	};
}

void ProcGraphEditor::OnDrawSingleLink(const std::vector<Point<float>>& points, int connecting, float width, Color4b color)
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

} // ui
