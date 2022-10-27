
#include "pch.h"


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
		virtual void PreviewUI() {}

		ui::Point2f position = {};
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
				newlinks.Append(link);
			else
				delete link;
		}
		std::swap(newlinks, links);
	}
	void AddLink(const LinkEnd& output, const LinkEnd& input)
	{
		UnlinkAll(input);
		links.Append(new Link{ output, input });
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

	ui::Array<Node*> nodes;
	ui::Array<Link*> links;
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
	void GetNodes(ui::Array<Node*>& outNodes) override
	{
		for (auto* n : graph->nodes)
			outNodes.Append(static_cast<Node*>(n));
	}
	void GetLinks(ui::Array<Link>& outLinks) override
	{
		for (auto* l : graph->links)
		{
			outLinks.Append
			({
				static_cast<Node*>(l->output.node),
				uintptr_t(l->output.pin),
				static_cast<Node*>(l->input.node),
				uintptr_t(l->input.pin),
				});
		}
	}
	static Node* AddNode(IProcGraph* graph, Graph::Node* node)
	{
		static_cast<GraphImpl*>(graph)->graph->nodes.Append(node);
		return node;
	}
	void GetAvailableNodeTypes(ui::Array<NodeTypeEntry>& outEntries) override
	{
		outEntries.Append({ "Vector/Make", 0,
			[](IProcGraph* graph, const char*, uintptr_t) -> Node* { return AddNode(graph, new MakeVec); } });
		outEntries.Append({ "Vector/Scale", 0,
			[](IProcGraph* graph, const char*, uintptr_t) -> Node* { return AddNode(graph, new ScaleVec); } });
		outEntries.Append({ "Vector/Dot product", 0,
			[](IProcGraph* graph, const char*, uintptr_t) -> Node* { return AddNode(graph, new DotProduct); } });
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
	void InputPinEditorUI(const Pin& pin) override
	{
		auto* node = static_cast<Graph::Node*>(pin.end.node);
		auto type = node->GetInputType(pin.end.num);
		float* data = node->GetInputDefaultValuePtr(pin.end.num);
		switch (type)
		{
		case Graph::Type::Scalar:
			ui::imm::PropEditFloat("\b=", *data);
			break;
		case Graph::Type::Vector:
			ui::imm::PropEditFloatVec(nullptr, data);
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

	ui::Point2f GetNodePosition(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->position;
	}
	void SetNodePosition(Node* node, const ui::Point2f& pos) override
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
	void PreviewUI(Node*)
	{
		ui::MakeWithText<ui::FrameElement>("Preview").SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
	}

	bool CanDeleteNode(Node*) override { return true; }
	void DeleteNode(Node* node) override
	{
		for (size_t i = 0; i < graph->links.size(); i++)
		{
			auto* L = graph->links[i];
			if (L->input.node == node || L->output.node == node)
			{
				delete L;
				i--;
			}
		}
		graph->nodes.RemoveFirstOf(static_cast<Graph::Node*>(node));
		delete static_cast<Graph::Node*>(node);
	}

	bool CanDuplicateNode(Node*) override { return true; }
	Node* DuplicateNode(Node* node) override
	{
		return static_cast<Graph::Node*>(node)->Clone();
	}

	GraphImpl(Graph* g) : graph(g) {}

	Graph* graph;
};

struct NodeGraphEditorDemo : ui::Buildable
{
	NodeGraphEditorDemo()
	{
		Graph::Node* makeVec1 = new MakeVec();
		Graph::Node* dotProd = new DotProduct();
		Graph::Node* scaleVec = new ScaleVec();

		makeVec1->position = { 50, 50 };
		dotProd->position = { 200, 50 };
		scaleVec->position = { 200, 150 };

		graph.nodes.Append(makeVec1);
		graph.nodes.Append(dotProd);
		graph.nodes.Append(scaleVec);

		graph.links.Append(new Graph::Link{ { makeVec1, 0, true }, { dotProd, 0 } });
		graph.links.Append(new Graph::Link{ { makeVec1, 0, true }, { dotProd, 1 } });
		graph.links.Append(new Graph::Link{ { makeVec1, 0, true }, { scaleVec, 0 } });
	}
	void Build() override
	{
		ui::Make<ui::ProcGraphEditor>().Init(Allocate<GraphImpl>(&graph));
	}

	Graph graph;
};
void Demo_NodeGraphEditor()
{
	ui::Make<NodeGraphEditorDemo>();
}

