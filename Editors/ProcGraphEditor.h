
#pragma once

#include "../Core/HashTable.h"
#include "../Model/Objects.h"


namespace ui {

extern DataCategoryTag DCT_EditProcGraph[1];
extern DataCategoryTag DCT_EditProcGraphNode[1];

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

	// node types to add
	typedef Node* NodeCreationFunc(IProcGraph*, const char* path, uintptr_t id);
	struct NodeTypeEntry
	{
		const char* path;
		uintptr_t id;
		NodeCreationFunc* func;
	};
	virtual void GetAvailableNodeTypes(std::vector<NodeTypeEntry>& outEntries) = 0;

	// basic node info
	virtual std::string GetNodeName(Node*) = 0;
	virtual uintptr_t GetNodeInputCount(Node*) = 0;
	virtual uintptr_t GetNodeOutputCount(Node*) = 0;
	virtual void NodePropertyEditorUI(Node*, UIContainer*) {}
	virtual void OnNodeContextMenu(Node*, struct MenuItemCollection&) {}

	// basic pin info
	virtual std::string GetPinName(const Pin&) = 0;
	virtual Color4b GetPinColor(const Pin&) { return Color4b(255, 0); }
	virtual void OnPinContextMenu(const Pin&, struct MenuItemCollection&) {}
	virtual void InputPinEditorUI(const Pin&, UIContainer*) {}

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
	virtual void OnEditNode(UIEvent&, Node*) {}

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


struct ProcGraphEditor_NodePin : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;
	void OnDestroy() override;

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

using PinUIMap = HashMap<IProcGraph::Pin, ProcGraphEditor_NodePin*, IProcGraph::PinHash>;

struct ProcGraphEditor_Node : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;

	void Init(IProcGraph* graph, IProcGraph::Node* node, Point<float> vOff);

	virtual void OnBuildTitleBar(UIContainer* ctx);
	virtual void OnBuildEditor(UIContainer* ctx);
	virtual void OnBuildInputPins(UIContainer* ctx);
	virtual void OnBuildOutputPins(UIContainer* ctx);
	virtual void OnBuildPreview(UIContainer* ctx);

	IProcGraph* _graph = nullptr;
	IProcGraph::Node* _node = nullptr;
	bool _isDragging = false;
	Point<float> _dragStartPos = {};
	Point<float> _dragStartMouse = {};
	Point<float> _viewOffset = {};
};

struct ProcGraphEditor : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;

	void Init(IProcGraph* graph);

	virtual void OnBuildNodes(UIContainer* ctx);
	virtual void OnMakeCreationMenu(struct MenuItemCollection& menu);
	virtual void OnDrawCurrentLinks();
	virtual void OnDrawPendingLinks();
	virtual void GetLinkPoints(const IProcGraph::Link& link, std::vector<Point<float>>& outPoints);
	virtual void GetConnectingLinkPoints(const IProcGraph::Pin& pin, std::vector<Point<float>>& outPoints);
	// connecting = 0 (not), 1 (output-*), -1 (*-input)
	virtual void GetLinkPointsRaw(const Point<float>& p0, const Point<float>& p1, int connecting, std::vector<Point<float>>& outPoints);
	virtual void GetTangents(const Point<float>& b0, const Point<float>& b3, Point<float>& b1, Point<float>& b2);
	virtual void GetLinkPointsRawInner(const Point<float>& b0, const Point<float>& b3, std::vector<Point<float>>& outPoints);
	virtual Point<float> GetPinPos(ProcGraphEditor_NodePin* P);
	virtual void OnDrawSingleLink(const std::vector<Point<float>>& points, int connecting, float width, Color4b color);

	IProcGraph* _graph = nullptr;
	PinUIMap pinUIMap;

	Point<float> viewOffset = {};

	Point<float> origMPos = {};
	Point<float> origVPos = {};

	bool drawCurrentLinksOnTop = false;
	bool drawPendingLinksOnTop = true;
	float linkExtension = 8.0f;
};

} // ui
