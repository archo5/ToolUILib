
#pragma once

#include "../Core/Array.h"
#include "../Core/HashMap.h"
#include "../Model/Objects.h"


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
	struct PinHEC
	{
		static bool AreEqual(const Pin& a, const Pin& b)
		{
			return a == b;
		}
		static size_t GetHash(const Pin& p)
		{
			auto hash = HashValue(p.end.node);
			hash *= 121;
			hash ^= HashValue(p.end.num);
			hash *= 121;
			hash ^= HashValue(p.isOutput);
			return hash;
		}
	};

	//virtual int GetSupportedFeatures() = 0;

	// global lists
	virtual void GetNodes(Array<Node*>& outNodes) = 0;
	virtual void GetLinks(Array<Link>& outLinks) = 0;

	// node types to add
	typedef Node* NodeCreationFunc(IProcGraph*, const char* path, uintptr_t id);
	struct NodeTypeEntry
	{
		const char* path;
		uintptr_t id;
		NodeCreationFunc* func;
	};
	virtual void GetAvailableNodeTypes(Array<NodeTypeEntry>& outEntries) = 0;

	// basic node info
	virtual std::string GetNodeName(Node*) = 0;
	virtual uintptr_t GetNodeInputCount(Node*) = 0;
	virtual uintptr_t GetNodeOutputCount(Node*) = 0;
	virtual void NodePropertyEditorUI(Node*) {}
	virtual void OnNodeContextMenu(Node*, struct MenuItemCollection&) {}

	// basic pin info
	virtual std::string GetPinName(const Pin&) = 0;
	virtual Color4b GetPinColor(const Pin&) { return Color4b(255, 0); }
	virtual void OnPinContextMenu(const Pin&, struct MenuItemCollection&) {}
	virtual void InputPinEditorUI(const Pin&) {}

	// pin linkage info (with default suboptimal implementations)
	virtual bool IsPinLinked(const Pin& pin)
	{
		Array<Link> allLinks;
		GetLinks(allLinks);
		for (const auto& link : allLinks)
		{
			if ((pin.isOutput ? link.output : link.input) == pin.end)
				return true;
		}
		return false;
	}
	virtual void GetPinLinks(const Pin& pin, Array<Link>& outLinks)
	{
		Array<Link> allLinks;
		GetLinks(allLinks);
		for (const auto& link : allLinks)
		{
			if ((pin.isOutput ? link.output : link.input) == pin.end)
				outLinks.Append(link);
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
	virtual Point2f GetNodePosition(Node*) = 0;
	virtual void SetNodePosition(Node*, const Point2f&) = 0;

	// node preview
	virtual bool HasPreview(Node*) { return false; }
	virtual bool IsPreviewEnabled(Node*) { return false; }
	virtual void SetPreviewEnabled(Node*, bool) {}
	virtual void PreviewUI(Node*) {}

	// node editing
	virtual void OnEditNode(Event&, Node*) {}

	virtual bool CanDeleteNode(Node*) { return false; }
	virtual void DeleteNode(Node*) {}

	virtual bool CanDuplicateNode(Node*) { return false; }
	virtual Node* DuplicateNode(Node*) { return nullptr; }
};

extern MulticastDelegate<IProcGraph*> OnProcGraphEdit;
extern MulticastDelegate<IProcGraph*, IProcGraph::Node*> OnProcGraphNodeEdit;


struct ProcGraphLinkDragDropData : DragDropData
{
	static constexpr const char* NAME = "ProcGraphLinkDragDropData";
	ProcGraphLinkDragDropData(IProcGraph* graph, const IProcGraph::Pin& pin) :
		DragDropData(NAME),
		_graph(graph),
		_pin(pin)
	{}
	bool ShouldBuild() override { return false; }
	IProcGraph* _graph;
	IProcGraph::Pin _pin;
};


struct ProcGraphEditor_NodePin : Buildable
{
	ProcGraphEditor_NodePin()
	{
		flags |= UIObject_NeedsTreeUpdates;
	}

	void Build() override;
	void OnEvent(Event& e) override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnExitTree() override;

	void Init(IProcGraph* graph, IProcGraph::Node* node, uintptr_t pin, bool isOutput);

	void _RegisterPin();
	void _UnregisterPin();

	void _TryLink(const IProcGraph::Pin& pin);
	void _UnlinkPin();

	IProcGraph* _graph = nullptr;
	IProcGraph::Pin _pin = {};

	bool _dragged = false;
	bool _dragHL = false;
	struct Selectable* _sel = nullptr;
};

using PinUIMap = HashMap<IProcGraph::Pin, ProcGraphEditor_NodePin*, IProcGraph::PinHEC>;

struct ProcGraphEditor_Node : Buildable
{
	void Build() override;
	void OnReset() override
	{
		Buildable::OnReset();
		flags |= UIObject_SetsChildTextStyle;
	}
	void OnEvent(Event& e) override;

	void Init(IProcGraph* graph, IProcGraph::Node* node, Point2f vOff);

	virtual void OnBuildTitleBar();
	virtual void OnBuildEditor();
	virtual void OnBuildInputPins();
	virtual void OnBuildOutputPins();
	virtual void OnBuildPreview();

	IProcGraph* _graph = nullptr;
	IProcGraph::Node* _node = nullptr;
	bool _isDragging = false;
	Point2f _dragStartPos = {};
	Point2f _dragStartMouse = {};
	Point2f _viewOffset = {};
};

struct ProcGraphEditor : Buildable
{
	void Build() override;
	void OnReset() override;
	void OnEvent(Event& e) override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return Rangef::Exact(containerSize.x); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override { return Rangef::Exact(containerSize.y); }
	void OnPaint(const UIPaintContext& ctx) override;

	void Init(IProcGraph* graph);

	virtual void OnBuildNodes();
	virtual void OnMakeCreationMenu(struct MenuItemCollection& menu);
	virtual void OnDrawCurrentLinks();
	virtual void OnDrawPendingLinks();
	virtual void GetLinkPoints(const IProcGraph::Link& link, Array<Point2f>& outPoints);
	virtual void GetConnectingLinkPoints(const IProcGraph::Pin& pin, Array<Point2f>& outPoints);
	// connecting = 0 (not), 1 (output-*), -1 (*-input)
	virtual void GetLinkPointsRaw(const Point2f& p0, const Point2f& p1, int connecting, Array<Point2f>& outPoints);
	virtual void GetTangents(const Point2f& b0, const Point2f& b3, Point2f& b1, Point2f& b2);
	virtual void GetLinkPointsRawInner(const Point2f& b0, const Point2f& b3, Array<Point2f>& outPoints);
	virtual Point2f GetPinPos(ProcGraphEditor_NodePin* P);
	virtual void OnDrawSingleLink(const Array<Point2f>& points, int connecting, float width, Color4b color);

	IProcGraph* _graph = nullptr;
	PinUIMap pinUIMap;

	Point2f viewOffset = {};

	Point2f origMPos = {};
	Point2f origVPos = {};

	bool drawCurrentLinksOnTop = false;
	bool drawPendingLinksOnTop = true;
	float linkExtension = 8.0f;
};

} // ui
