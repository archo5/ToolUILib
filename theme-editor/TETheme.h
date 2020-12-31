
#pragma once
#include "TENodes.h"


struct TE_IOverrideProvider
{
	virtual const TE_Overrides* GetOverrides() = 0;
};


struct TE_TmplSettings
{
	unsigned w = 64, h = 64;
	unsigned l = 0, t = 0, r = 0, b = 0;
	bool gamma = false;
	TE_LayerNode* layer = nullptr;

	TE_RenderContext MakeRenderContext() const
	{
		return
		{
			w,
			h,
			{ 0, 0, float(w), float(h) },
			gamma,
		};
	}
	void UI(UIContainer* ctx);
	void Load(NamedTextSerializeReader& nts);
	void Save(JSONSerializeWriter& nts);
};


struct TE_Template : ui::IProcGraph, TE_IRenderContextProvider
{
	~TE_Template();
	void Clear();

	TE_Node* CreateNodeFromTypeName(StringView type);

	void Load(NamedTextSerializeReader& nts);
	void Save(JSONSerializeWriter& nts);

	template <class T> T* CreateNode(float x, float y)
	{
		T* node = new T;
		node->id = ++nodeIDAlloc;
		node->position = { x, y };
		nodes.push_back(node);
		return node;
	}
	template<class T> static Node* CreateNode(IProcGraph* pg)
	{
		return (TE_Node*)static_cast<TE_Template*>(pg)->CreateNode<T>(0, 0);
	}

	void GetNodes(std::vector<Node*>& outNodes) override;
	void GetLinks(std::vector<Link>& outLinks) override;
	void GetAvailableNodeTypes(std::vector<NodeTypeEntry>& outEntries) override;

	std::string GetNodeName(Node* node) override { return static_cast<TE_Node*>(node)->GetName(); }
	uintptr_t GetNodeInputCount(Node* node) override { return static_cast<TE_Node*>(node)->GetInputPinCount(); }
	uintptr_t GetNodeOutputCount(Node* node) override { return 1; }
	void NodePropertyEditorUI(Node* node, UIContainer* ctx) { static_cast<TE_Node*>(node)->PropertyUI(ctx); }

	std::string GetPinName(const Pin& pin) override;
	Color4b GetPinColor(const Pin& pin) override;
	void InputPinEditorUI(const Pin& pin, UIContainer* ctx) override;

	void UnlinkPin(const Pin& pin) override;
	bool LinkExists(const Link& link) override;
	bool CanLink(const Link& link) override;
	bool TryLink(const Link& link) override;
	bool Unlink(const Link& link) override;

	Color4b GetLinkColor(const Link& link, bool selected, bool hovered)
	{
		return GetPinColor({ link.output, true });
	}
	Color4b GetNewLinkColor(const Pin& pin) override { return GetPinColor(pin); }

	Point<float> GetNodePosition(Node* node) override { return static_cast<TE_Node*>(node)->position; }
	void SetNodePosition(Node* node, const Point<float>& pos) override { static_cast<TE_Node*>(node)->position = pos; }

	bool HasPreview(Node*) override { return true; }
	bool IsPreviewEnabled(Node* node) override { return static_cast<TE_Node*>(node)->isPreviewEnabled; }
	void SetPreviewEnabled(Node* node, bool v) override { static_cast<TE_Node*>(node)->isPreviewEnabled = v; }
	void PreviewUI(Node* node, UIContainer* ctx) override { static_cast<TE_Node*>(node)->PreviewUI(ctx, this); }

	void OnEditNode(UIEvent& e, Node* node) override;

	void InvalidateNode(TE_Node* n);
	void InvalidateAllNodes();
	void ResolveAllParameters(const TE_RenderContext& rc, const TE_Overrides* ovr);
	void SetCurrentLayerNode(TE_LayerNode* img);

	bool CanDeleteNode(Node*) override { return true; }
	void DeleteNode(Node* node) override;

	bool CanDuplicateNode(Node*) override { return true; }
	Node* DuplicateNode(Node* node) override;

	void UpdateTopoSortedNodes();

	const TE_RenderContext& GetRenderContext() override;

	std::vector<TE_Node*> nodes;
	std::vector<std::shared_ptr<TE_NamedColor>> colors;
	uint32_t nodeIDAlloc = 0;
	std::string name;
	TE_TmplSettings renderSettings;

	// edit-only
	std::vector<TE_Node*> topoSortedNodes;
	TE_IOverrideProvider* ovrProv = nullptr;
};


struct TE_Variation
{
	std::string name;
};

struct TE_Image
{
	bool expanded = true;
	std::string name;
	HashMap<TE_Variation*, std::shared_ptr<TE_Overrides>> overrides;
};


struct TE_Theme : TE_IOverrideProvider
{
	TE_Theme();
	~TE_Theme();
	void Clear();

	void Load(NamedTextSerializeReader& nts);
	void Save(JSONSerializeWriter& nts);

	void LoadFromFile(const char* path);
	void SaveToFile(const char* path);

	void CreateSampleTheme();

	const TE_Overrides* GetOverrides();

	void SetCurPreviewImage(TE_Image* cpi);

	std::vector<TE_Template*> templates;
	std::vector<std::shared_ptr<TE_Variation>> variations;
	std::vector<std::shared_ptr<TE_Image>> images;
	TE_Template* curTemplate = nullptr;
	TE_Variation* curVariation = nullptr;
	TE_Image* curPreviewImage = nullptr;
};
