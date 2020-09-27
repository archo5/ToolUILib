
#include "../GUI.h"

using namespace ui;


enum UserEvents
{
	TEUE_ImageMadeCurrent,
};

static DataCategoryTag DCT_NodeEdited[1];


using AbsRect = AABB<float>;

struct SubRect
{
	AABB<float> anchors = {};
	AABB<float> offsets = {};
};

struct SubPos
{
	Point<float> anchor = {};
	Point<float> offset = {};
};

struct TE_NamedColor
{
	std::string name;
	Color4b color;
};

struct TE_ColorRef
{
	// if name is empty, color is used instead
	std::string name;
	Color4b color;
};

struct CornerRadiuses
{
	CornerRadiuses Eval()
	{
		if (uniform)
			return { uniform, r, r, r, r, r };
		return *this;
	}

	bool uniform = true;
	float r = 0;
	float r00 = 0, r10 = 0, r01 = 0, r11 = 0;
};


float Clamp(float x, float xmin, float xmax)
{
	return x < xmin ? xmin : x > xmax ? xmax : x;
}

float Clamp01(float x)
{
	return x < 0 ? 0 : x > 1 ? 1 : x;
}

float AARect(float x, float y, const AbsRect& rect)
{
	return Clamp01(x - rect.x0 + 0.5f)
		* Clamp01(rect.x1 - x + 0.5f)
		* Clamp01(y - rect.y0 + 0.5f)
		* Clamp01(rect.y1 - y + 0.5f);
}

float AACircle(float x, float y, float cx, float cy, float radius)
{
	float r2 = radius * radius;
	float d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
	float q = radius - sqrtf(d2) + 0.5f;
	return Clamp(q, 0, 1);
}

AbsRect ResolveRect(const SubRect& rect, const AbsRect& frame)
{
	return
	{
		lerp(frame.x0, frame.x1, rect.anchors.x0) + rect.offsets.x0,
		lerp(frame.y0, frame.y1, rect.anchors.y0) + rect.offsets.y0,
		lerp(frame.x0, frame.x1, rect.anchors.x1) + rect.offsets.x1,
		lerp(frame.y0, frame.y1, rect.anchors.y1) + rect.offsets.y1,
	};
}


enum TE_NodeType
{
	TENT_Unknown = 0,
	TENT_Mask,
	TENT_Layer,
	TENT_Image,
};

static Color4b g_nodeTypeColors[] =
{
	// unknown/default
	Color4b(240, 240, 240),
	// mask
	Color4b(120, 120, 120),
	// layer
	Color4b(240, 180, 100),
	// image
	Color4b(120, 180, 240),
};

struct TE_Node
{
	virtual ~TE_Node() {}
	virtual const char* GetName() = 0;
	virtual TE_NodeType GetType() = 0;
	virtual int GetInputPinCount() = 0;
	virtual std::string GetInputPinName(int pin) = 0;
	virtual TE_NodeType GetInputPinType(int pin) = 0;
	virtual TE_Node* GetInputPinValue(int pin) = 0;
	virtual void SetInputPinValue(int pin, TE_Node* value) = 0;
	virtual void InputPinUI(int pin, UIContainer* ctx) = 0;
	virtual void PropertyUI(UIContainer* ctx) = 0;
	virtual void PreviewUI(UIContainer* ctx) = 0;

	Point<float> position = {};
	bool isPreviewEnabled = true;
};

struct TE_MaskNode : TE_Node
{
	TE_NodeType GetType() override { return TENT_Mask; }
	void PreviewUI(UIContainer* ctx)
	{
		// TODO
	}

	virtual float Eval(float x, float y, const AbsRect& frame) = 0;
};

static float EvalAARoundedRectMask(float x, float y, const AbsRect& rr, const CornerRadiuses& cr)
{
	if (x < rr.x0 + cr.r00 &&
		y < rr.y0 + cr.r00)
		return AACircle(x, y, rr.x0 + cr.r00, rr.y0 + cr.r00, cr.r00);
	if (x > rr.x1 - cr.r10 &&
		y < rr.y0 + cr.r10)
		return AACircle(x, y, rr.x1 - cr.r10, rr.y0 + cr.r10, cr.r10);
	if (x < rr.x0 + cr.r01 &&
		y > rr.y1 - cr.r01)
		return AACircle(x, y, rr.x0 + cr.r01, rr.y1 - cr.r01, cr.r01);
	if (x > rr.x1 - cr.r11 &&
		y > rr.y1 - cr.r11)
		return AACircle(x, y, rr.x1 - cr.r11, rr.y1 - cr.r11, cr.r11);
	return AARect(x, y, rr);
}

struct TE_RectMask : TE_MaskNode
{
	const char* GetName() override { return "Rectangle mask"; }
	int GetInputPinCount() override { return 0; }
	std::string GetInputPinName(int pin) override { return nullptr; }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Unknown; }
	TE_Node* GetInputPinValue(int pin) override { return nullptr; }
	void SetInputPinValue(int pin, TE_Node* value) override {}
	void InputPinUI(int pin, UIContainer* ctx) override {}
	void PropertyUI(UIContainer* ctx) override
	{
		{
			ctx->Text("Anchors");
			imm::PropEditFloatVec(ctx, "\bMin", &rect.anchors.x0, "XY", { MinWidth(20) }, 0.01f);
			imm::PropEditFloatVec(ctx, "\bMax", &rect.anchors.x1, "XY", { MinWidth(20) }, 0.01f);
		}
		{
			ctx->Text("Offsets");
			imm::PropEditFloatVec(ctx, "\bMin", &rect.offsets.x0, "XY", { MinWidth(20) });
			imm::PropEditFloatVec(ctx, "\bMax", &rect.offsets.x1, "XY", { MinWidth(20) });
		}
		{
			ctx->Text("Radius");
			imm::PropEditBool(ctx, "\bUniform", crad.uniform);
			if (crad.uniform)
			{
				imm::PropEditFloat(ctx, "\bRadius", crad.r);
			}
			else
			{
				imm::PropEditFloat(ctx, "\bR(top-left)", crad.r00);
				imm::PropEditFloat(ctx, "\bR(top-right)", crad.r10);
				imm::PropEditFloat(ctx, "\bR(bottom-left)", crad.r01);
				imm::PropEditFloat(ctx, "\bR(bottom-right)", crad.r11);
			}
		}
	}

	float Eval(float x, float y, const AbsRect& frame) override
	{
		AbsRect rr = ResolveRect(rect, frame);
		auto cr = crad.Eval();
		return EvalAARoundedRectMask(x, y, rr, cr);
	}

	SubRect rect;
	CornerRadiuses crad;
};

struct TE_MaskRef
{
	TE_MaskNode* mask = nullptr;
	unsigned border = 0;
	unsigned radius = 0;

	float Eval(float x, float y, const AbsRect& frame)
	{
		if (mask)
			return mask->Eval(x, y, frame);
		CornerRadiuses cr;
		cr.r = radius;
		cr = cr.Eval();
		auto rr = frame.ShrinkBy(UIRect::UniformBorder(border));
		return EvalAARoundedRectMask(x, y, rr, cr);
	}
	void UI(UIContainer* ctx)
	{
		if (mask)
			return;
		ctx->PushBox();
		imm::PropEditInt(ctx, "\bBorder", border, { MinWidth(20) });
		imm::PropEditInt(ctx, "\bRadius", radius, { MinWidth(20) });
		ctx->Pop();
	}
};

enum TE_MaskCombineMode
{
	TEMCM_Intersect,
	TEMCM_Union,
	TEMCM_SoftDifference,
	TEMCM_HardDifference,
	TEMCM_AMinusB,
	TEMCM_BMinusA,
};

struct TE_CombineMask : TE_MaskNode
{
	const char* GetName() override { return "Combine mask"; }
	int GetInputPinCount() override { return 2; }
	std::string GetInputPinName(int pin) override { return pin == 0 ? "Mask A" : "Mask B"; }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Mask; }
	TE_Node* GetInputPinValue(int pin) override { return masks[pin].mask; }
	void SetInputPinValue(int pin, TE_Node* value) override { masks[pin].mask = dynamic_cast<TE_MaskNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override { masks[pin].UI(ctx); }
	void PropertyUI(UIContainer* ctx) override
	{
		imm::RadioButton(ctx, mode, TEMCM_Intersect, "Intersect");
		imm::RadioButton(ctx, mode, TEMCM_Union, "Union");
		imm::RadioButton(ctx, mode, TEMCM_SoftDifference, "Difference (soft)");
		imm::RadioButton(ctx, mode, TEMCM_HardDifference, "Difference (hard)");
		imm::RadioButton(ctx, mode, TEMCM_AMinusB, "A-B");
		imm::RadioButton(ctx, mode, TEMCM_BMinusA, "B-A");
	}

	float Eval(float x, float y, const AbsRect& frame) override
	{
		float a = masks[0].Eval(x, y, frame);
		float b = masks[1].Eval(x, y, frame);
		return Combine(a, b);
	}
	float Combine(float a, float b)
	{
		switch (mode)
		{
		case TEMCM_Intersect: return a * b;
		case TEMCM_Union: return 1 - (1 - a) * (1 - b);
		case TEMCM_SoftDifference: return a + b - 2 * a * b;
		case TEMCM_HardDifference: return fabsf(a - b);
		case TEMCM_AMinusB: return a * (1 - b);
		case TEMCM_BMinusA: return b * (1 - a);
		default: return 1;
		}
	}

	TE_MaskRef masks[2];
	TE_MaskCombineMode mode = TEMCM_Intersect;
};

struct TE_LayerNode : TE_Node
{
	TE_NodeType GetType() override { return TENT_Layer; }
	void PreviewUI(UIContainer* ctx)
	{
		// TODO
	}

	virtual Color4f Eval(float x, float y, const AbsRect& frame) = 0;
};

struct TE_SolidColorLayer : TE_LayerNode
{
	const char* GetName() override { return "Solid color layer"; }
	int GetInputPinCount() override { return 1; }
	std::string GetInputPinName(int pin) override { return "Mask"; }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Mask; }
	TE_Node* GetInputPinValue(int pin) override { return mask.mask; }
	void SetInputPinValue(int pin, TE_Node* value) override { mask.mask = dynamic_cast<TE_MaskNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override { mask.UI(ctx); }
	void PropertyUI(UIContainer* ctx) override
	{
		imm::PropEditColor(ctx, "\bColor", color);
	}

	Color4f Eval(float x, float y, const AbsRect& frame)
	{
		float a = opacity * mask.Eval(x, y, frame);
		auto cc = color;
		cc.a *= a;
		return cc;
	}

	Color4f color = Color4f::White();
	float opacity = 1;
	TE_MaskRef mask;
};

struct TE_LayerBlendRef
{
	TE_LayerNode* layer = nullptr;
	bool enabled = true;
	float opacity = 1;
};

struct TE_BlendLayer : TE_LayerNode
{
	const char* GetName() override { return "Blend layer"; }
	int GetInputPinCount() override { return layers.size(); }
	std::string GetInputPinName(int pin) override { return "Layer " + std::to_string(pin); }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Layer; }
	TE_Node* GetInputPinValue(int pin) override { return layers[pin].layer; }
	void SetInputPinValue(int pin, TE_Node* value) override { layers[pin].layer = dynamic_cast<TE_LayerNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override
	{
		imm::EditBool(ctx, layers[pin].enabled);
		imm::PropEditFloat(ctx, "\bO", layers[pin].opacity);
	}
	void PropertyUI(UIContainer* ctx) override
	{
		if (imm::Button(ctx, "Add pin"))
			layers.push_back({});
	}

	Color4f Eval(float x, float y, const AbsRect& frame)
	{
		Color4f ret = Color4f::Zero();
		for (const auto& layer : layers)
		{
			if (layer.layer && layer.enabled)
			{
				ret.BlendOver(layer.layer->Eval(x, y, frame));
			}
		}
		return ret;
	}

	std::vector<TE_LayerBlendRef> layers;
};

struct TE_ImageNode : TE_Node
{
	struct Preview : Node
	{
		void Render(UIContainer* ctx) override
		{
			Subscribe(DCT_NodeEdited);
			Canvas canvas;
			node->Render(canvas);
			*ctx->Make<ImageElement>()
				->SetImage(ctx->GetCurrentNode()->Allocate<Image>(canvas))
				->SetScaleMode(ScaleMode::Fit, 0.5f, 0.5f)
				->SetAlphaBackgroundEnabled(true)
				//+ Width(style::Coord::Percent(100)) -- TODO fix
				+ Width(134)
				;
		}
		TE_ImageNode* node;
	};

	const char* GetName() override { return "Image"; }
	TE_NodeType GetType() override { return TENT_Image; }
	int GetInputPinCount() override { return 1; }
	std::string GetInputPinName(int pin) override { return "Layer"; }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Layer; }
	TE_Node* GetInputPinValue(int pin) override { return layer; }
	void SetInputPinValue(int pin, TE_Node* value) override { layer = dynamic_cast<TE_LayerNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override {}
	void PropertyUI(UIContainer* ctx) override
	{
		if (imm::Button(ctx, "Make current"))
		{
			ctx->GetCurrentNode()->SendUserEvent(TEUE_ImageMadeCurrent, uintptr_t(this));
		}
		{
			Property::Scope ps(ctx, "\bSize");
			imm::PropEditInt(ctx, "\bW", w, { MinWidth(20) }, imm::DEFAULT_SPEED, 1, 1024);
			imm::PropEditInt(ctx, "\bH", h, { MinWidth(20) }, imm::DEFAULT_SPEED, 1, 1024);
		}
	}
	void PreviewUI(UIContainer* ctx)
	{
		ctx->Make<Preview>()->node = this;
	}

	void Render(Canvas& outCanvas)
	{
		outCanvas.SetSize(w, h);
		auto* pixels = outCanvas.GetPixels();
		AbsRect frame = { 0, 0, w, h };
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				Color4f col = Color4f::Zero();
#if 0
				for (TE_LayerNode* layer : rootLayers)
				{
					if (!layer->enabled)
						continue;
					auto cc = layer->Eval(x + 0.5f, y + 0.5f, frame);
					col.BlendOver(cc);
				}
#endif
				if (layer)
					col = layer->Eval(x + 0.5f, y + 0.5f, frame);
				pixels[x + y * w] = col.GetColor32();
			}
		}
	}

	int w = 64, h = 64;
	int l = 0, t = 0, r = 0, b = 0;
	TE_LayerNode* layer = nullptr;
};

struct TE_Page : ui::IProcGraph
{
	~TE_Page()
	{
		for (TE_Node* node : nodes)
			delete node;
	}

	void GetNodes(std::vector<Node*>& outNodes) override
	{
		for (TE_Node* node : nodes)
			outNodes.push_back(node);
	}
	void GetLinks(std::vector<Link>& outLinks) override
	{
		for (TE_Node* inNode : nodes)
		{
			uintptr_t num = inNode->GetInputPinCount();
			for (uintptr_t i = 0; i < num; i++)
			{
				TE_Node* outNode = inNode->GetInputPinValue(i);
				if (outNode)
					outLinks.push_back({ { outNode, 0 }, { inNode, i } });
			}
		}
	}
	template<class T> static Node* CreateNode(IProcGraph* pg)
	{
		TE_Node* n = new T;
		static_cast<TE_Page*>(pg)->nodes.push_back(n);
		return n;
	}
	void GetAvailableNodeTypes(std::vector<NodeTypeEntry>& outEntries) override
	{
		outEntries.push_back({ "Masks/Rectangle", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_RectMask>(pg); } });
		outEntries.push_back({ "Masks/Combine", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_CombineMask>(pg); } });
		outEntries.push_back({ "Layers/Solid color", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_SolidColorLayer>(pg); } });
		outEntries.push_back({ "Layers/Blend", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_BlendLayer>(pg); } });
		outEntries.push_back({ "Image", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_ImageNode>(pg); } });
	}

	std::string GetNodeName(Node* node) override { return static_cast<TE_Node*>(node)->GetName(); }
	uintptr_t GetNodeInputCount(Node* node) override { return static_cast<TE_Node*>(node)->GetInputPinCount(); }
	uintptr_t GetNodeOutputCount(Node* node) override { return static_cast<TE_Node*>(node)->GetType() == TENT_Image ? 0 : 1; }
	void NodePropertyEditorUI(Node* node, UIContainer* ctx) { static_cast<TE_Node*>(node)->PropertyUI(ctx); }

	std::string GetPinName(const Pin& pin) override
	{
		if (pin.isOutput)
			return "Output";
		return static_cast<TE_Node*>(pin.end.node)->GetInputPinName(pin.end.num);
	}
	static Color4b GetColorByType(TE_NodeType t) { return g_nodeTypeColors[t]; }
	Color4b GetPinColor(const Pin& pin) override
	{
		if (pin.isOutput)
			return GetColorByType(static_cast<TE_Node*>(pin.end.node)->GetType());
		else
			return GetColorByType(static_cast<TE_Node*>(pin.end.node)->GetInputPinType(pin.end.num));
	}
	void InputPinEditorUI(const Pin& pin, UIContainer* ctx)
	{
		if (pin.isOutput)
			return;
		static_cast<TE_Node*>(pin.end.node)->InputPinUI(pin.end.num, ctx);
	}

	void UnlinkPin(const Pin& pin) override
	{
		if (pin.isOutput)
		{
			for (TE_Node* inNode : nodes)
			{
				uintptr_t num = inNode->GetInputPinCount();
				for (uintptr_t i = 0; i < num; i++)
				{
					TE_Node* outNode = inNode->GetInputPinValue(i);
					if (outNode == pin.end.node)
						inNode->SetInputPinValue(i, nullptr);
				}
			}
		}
		else
			static_cast<TE_Node*>(pin.end.node)->SetInputPinValue(pin.end.num, nullptr);
	}

	bool LinkExists(const Link& link) override
	{
		return static_cast<TE_Node*>(link.input.node)->GetInputPinValue(link.input.num) == link.output.node;
	}
	bool CanLink(const Link& link) override
	{
		auto* inNode = static_cast<TE_Node*>(link.input.node);
		auto* outNode = static_cast<TE_Node*>(link.output.node);
		return inNode->GetInputPinType(link.input.num) == outNode->GetType();
	}
	bool TryLink(const Link& link) override
	{
		auto* inNode = static_cast<TE_Node*>(link.input.node);
		auto* outNode = static_cast<TE_Node*>(link.output.node);
		if (inNode->GetInputPinType(link.input.num) != outNode->GetType())
			return false;
		inNode->SetInputPinValue(link.input.num, outNode);
		return true;
	}
	bool Unlink(const Link& link) override
	{
		static_cast<TE_Node*>(link.input.node)->SetInputPinValue(link.input.num, nullptr);
		return true;
	}
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
	void PreviewUI(Node* node, UIContainer* ctx) override { static_cast<TE_Node*>(node)->PreviewUI(ctx); }

	bool CanDeleteNode(Node*) override { return true; }
	void DeleteNode(Node* node) override
	{
		UnlinkPin({ { node, 0 }, true });
		auto* N = static_cast<TE_Node*>(node);
		for (size_t i = 0; i < nodes.size(); i++)
			if (nodes[i] == N)
				nodes.erase(nodes.begin() + i);
		delete N;
	}

	bool CanDuplicateNode(Node*) override { return false; }
	Node* DuplicateNode(Node* node) override
	{
		// TODO
		return nullptr;
	}

	std::vector<TE_Node*> nodes;

	// editing state
	std::string name;
	TE_Node* curNode = nullptr;
};

struct TE_Theme
{
	TE_Theme()
	{
		CreateSampleTheme();
	}
	~TE_Theme()
	{
		Clear();
	}
	void Clear()
	{
		colors.clear();
		for (TE_Page* page : pages)
			delete page;
		pages.clear();
	}
	void CreateSampleTheme()
	{
		Clear();

		colors.push_back({ "border", Color4f(0.6f, 0.8f, 0.9f) });
		colors.push_back({ "middle", Color4f(0.1f, 0.15f, 0.2f) });

		TE_Page* page = new TE_Page;
		page->name = "Frame";

		auto* mr = new TE_RectMask;
		mr->position = { 100, 100 };
		mr->rect = { { 0, 0, 1, 1 }, { 1, 1, -1, -1 } };
		mr->crad.r = 16;
		page->nodes.push_back(mr);

		auto* l1 = new TE_SolidColorLayer;
		l1->position = { 300, 100 };
		l1->color = Color4f(0.6f, 0.8f, 0.9f);
		//l1->mask.mask = mr;
		l1->mask.border = 1;
		l1->mask.radius = 15;
		page->nodes.push_back(l1);

		auto* l2 = new TE_SolidColorLayer;
		l2->position = { 300, 300 };
		l2->color = Color4f(0.1f, 0.15f, 0.2f);
		//l2->mask.mask = mr;
		l2->mask.border = 2;
		l2->mask.radius = 14;
		page->nodes.push_back(l2);

		auto* bl = new TE_BlendLayer;
		bl->position = { 500, 100 };
		bl->layers.push_back({ l1, true, 1 });
		bl->layers.push_back({ l2, true, 1 });
		page->nodes.push_back(bl);

		auto* img = new TE_ImageNode;
		img->position = { 700, 100 };
		img->layer = bl;
		page->nodes.push_back(img);

		curPage = page;
		pages.push_back(page);
	}

	std::vector<TE_NamedColor> colors;
	std::vector<TE_Page*> pages;

	// editing state
	TE_Page* curPage = nullptr;
}
g_Theme;

struct TE_PageEditorNode : Node
{
	void Render(UIContainer* ctx) override
	{
		auto* hsp = ctx->Push<ui::SplitPane>();
		{
			ctx->Push<ListBox>();
			{
				auto* pge = ctx->Make<ProcGraphEditor>();
				*pge + Height(style::Coord::Percent(100));
				pge->Init(page);
			}
			ctx->Pop();

			ctx->PushBox();
			{
				auto* vsp = ctx->Push<ui::SplitPane>();
				{
					ctx->PushBox();
					{
						ctx->Text("Preview") + ui::Padding(5);

						if (page->curNode)
						{
							ctx->Text(page->curNode->GetName());
						}
					}
					ctx->Pop();

					ctx->PushBox();
					{
						ctx->Text("Colors") + ui::Padding(5);
						auto* ced = ctx->Make<SequenceEditor>();
						*ced + Height(style::Coord::Percent(100));
						ced->SetSequence(Allocate<StdSequence<decltype(theme->colors)>>(theme->colors));
						ced->itemUICallback = [](UIContainer* ctx, SequenceEditor* se, ISequenceIterator* it)
						{
							auto& NC = se->GetSequence()->GetValue<TE_NamedColor>(it);
							imm::EditColor(ctx, NC.color);
							imm::EditString(ctx, NC.name.c_str(), [&NC](const char* v) { NC.name = v; });
						};
					}
					ctx->Pop();
				}
				ctx->Pop();
				vsp->SetDirection(true);
				vsp->SetSplits({ 0.5f });
			}
			ctx->Pop();
		}
		ctx->Pop();
		hsp->SetSplits({ 0.8f });
	}
	void OnEvent(UIEvent& e) override
	{
		Node::OnEvent(e);
		if (e.type == UserEvent(TEUE_ImageMadeCurrent))
		{
			page->curNode = (TE_ImageNode*)e.arg0;
			Rerender();
		}
		if (e.type == UIEventType::Change || e.type == UIEventType::Commit)
		{
			Notify(DCT_NodeEdited);
		}
	}

	TE_Theme* theme;
	TE_Page* page;
};

struct TE_ThemeEditorNode : Node
{
	void Render(UIContainer* ctx) override
	{
		*ctx->Push<ui::TabGroup>() + Height(style::Coord::Percent(100)) + Layout(style::layouts::EdgeSlice());
		{
			ctx->Push<ui::TabButtonList>();
			{
				for (TE_Page* page : theme->pages)
				{
					ctx->Push<ui::TabButtonT<TE_Page*>>()->Init(theme->curPage, page);
					ctx->Text(page->name);
					ctx->Pop();
				}
			}
			ctx->Pop();

			if (theme->curPage)
			{
				*ctx->Push<ui::TabPanel>() + Height(style::Coord::Percent(100));
				{
					auto* pen = ctx->Make<TE_PageEditorNode>();
					*pen + Height(style::Coord::Percent(100));
					pen->theme = theme;
					pen->page = theme->curPage;
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
	}

	TE_Theme* theme;
};

struct ThemeEditorMainWindow : ui::NativeMainWindow
{
	ThemeEditorMainWindow()
	{
		SetTitle("Theme Editor");
		SetSize(1280, 720);
	}
	void OnRender(UIContainer* ctx)
	{
		auto* ten = ctx->Make<TE_ThemeEditorNode>();
		*ten + Height(style::Coord::Percent(100));
		ten->theme = &g_Theme;

		ctx->Make<DefaultOverlayRenderer>();
	}
};

int uimain(int argc, char* argv[])
{
	ui::Application app(argc, argv);
	ThemeEditorMainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
