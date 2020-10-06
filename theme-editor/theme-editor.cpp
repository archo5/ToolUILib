
#include "../GUI.h"

using namespace ui;


enum UserEvents
{
	TEUE_ImageMadeCurrent,
};

static DataCategoryTag DCT_NodePreviewInvalidated[1];


static void Color4bLoad(const char* key, Color4b& col, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	col.r = nts.ReadInt("r");
	col.g = nts.ReadInt("g");
	col.b = nts.ReadInt("b");
	col.a = nts.ReadInt("a");
	nts.EndDict();
}

static void Color4bSave(const char* key, Color4b& col, NamedTextSerializeWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteInt("r", col.r);
	nts.WriteInt("g", col.g);
	nts.WriteInt("b", col.b);
	nts.WriteInt("a", col.a);
	nts.EndDict();
}

static void AABBFloatLoad(const char* key, AABB<float>& rect, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	rect.x0 = nts.ReadFloat("x0");
	rect.y0 = nts.ReadFloat("y0");
	rect.x1 = nts.ReadFloat("x1");
	rect.y1 = nts.ReadFloat("y1");
	nts.EndDict();
}

static void AABBFloatSave(const char* key, AABB<float>& rect, NamedTextSerializeWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteFloat("x0", rect.x0);
	nts.WriteFloat("y0", rect.y0);
	nts.WriteFloat("x1", rect.x1);
	nts.WriteFloat("y1", rect.y1);
	nts.EndDict();
}


using AbsRect = AABB<float>;

struct SubRect
{
	AABB<float> anchors = {};
	AABB<float> offsets = {};

	void Load(const char* key, NamedTextSerializeReader& nts)
	{
		nts.BeginDict(key);
		AABBFloatLoad("anchors", anchors, nts);
		AABBFloatLoad("offsets", offsets, nts);
		nts.EndDict();
	}
	void Save(const char* key, NamedTextSerializeWriter& nts)
	{
		nts.BeginDict(key);
		AABBFloatSave("anchors", anchors, nts);
		AABBFloatSave("offsets", offsets, nts);
		nts.EndDict();
	}
};

struct SubPos
{
	Point<float> anchor = {};
	Point<float> offset = {};
};

struct TE_RenderContext
{
	unsigned width;
	unsigned height;
	AbsRect frame;
	bool gamma;
};
static TE_RenderContext g_curImageCtx = { 8, 8, { 0, 0, 8.f, 8.f }, false };

struct TE_NamedColor
{
	std::string name;
	Color4b color;

	TE_NamedColor() {}
	TE_NamedColor(const std::string& n, Color4b c) : name(n), color(c)
	{
	}
	void Load(NamedTextSerializeReader& nts)
	{
		nts.BeginDict("NamedColor");
		name = nts.ReadString("name");
		Color4bLoad("color", color, nts);
		nts.EndDict();
	}
	void Save(NamedTextSerializeWriter& nts)
	{
		nts.BeginDict("NamedColor");
		nts.WriteString("name", name);
		Color4bSave("color", color, nts);
		nts.EndDict();
	}
};

static std::vector<std::shared_ptr<TE_NamedColor>>* g_namedColors;
static void EditNCRef(UIContainer* ctx, std::weak_ptr<TE_NamedColor>& ncref)
{
	// TODO internal selection with color previews
	auto ncr = ncref.lock();
	if (imm::Button(ctx, ncr ? ncr->name.c_str() : "<none>"))
	{
		MenuItemCollection mic;
		mic.Add("<none>", false, false, -1) = [&ncref]() { ncref = {}; };
		for (const auto& nc : *g_namedColors)
		{
			mic.Add(nc->name) = [&ncref, &nc]() { ncref = nc; };
		}
		Menu(mic.Finalize()).Show(ctx->GetCurrentNode());
	}
}

struct TE_ColorRef
{
	bool useRef = true;
	std::weak_ptr<TE_NamedColor> ncref;
	Color4b color;

	Color4b Get()
	{
		if (useRef)
			if (auto ptr = ncref.lock())
				return ptr->color;
		return color;
	}
	void Set(std::weak_ptr<TE_NamedColor> ncr)
	{
		useRef = true;
		ncref = ncr;
	}
	void Set(Color4b c)
	{
		useRef = false;
		color = c;
	}
	void Load(const char* key, NamedTextSerializeReader& nts)
	{
		nts.BeginDict(key);
		useRef = nts.ReadBool("useRef", true);
		auto name = nts.ReadString("name");
		ncref = {};
		for (auto& nc : *g_namedColors)
		{
			if (nc->name == name)
				ncref = nc;
		}
		Color4bLoad("color", color, nts);
		nts.EndDict();
	}
	void Save(const char* key, NamedTextSerializeWriter& nts)
	{
		nts.BeginDict(key);
		nts.WriteBool("useRef", useRef);
		auto r = ncref.lock();
		nts.WriteString("name", r ? r->name : "");
		Color4bSave("color", color, nts);
		nts.EndDict();
	}
	void UI(UIContainer* ctx)
	{
		Property::Scope ps(ctx, "\bColor");
		imm::EditBool(ctx, useRef, useRef ? "N" : "C", {}, imm::ButtonStateToggleSkin());
		if (useRef)
		{
			EditNCRef(ctx, ncref);
		}
		else
		{
			imm::EditColor(ctx, color);
		}
	}
};

struct CornerRadiuses
{
	CornerRadiuses Eval()
	{
		if (uniform)
			return { uniform, r, r, r, r, r };
		return *this;
	}
	void Load(const char* key, NamedTextSerializeReader& nts)
	{
		nts.BeginDict(key);
		uniform = nts.ReadBool("uniform", true);
		r = nts.ReadFloat("r");
		r00 = nts.ReadFloat("r00");
		r10 = nts.ReadFloat("r10");
		r01 = nts.ReadFloat("r01");
		r11 = nts.ReadFloat("r11");
		nts.EndDict();
	}
	void Save(const char* key, NamedTextSerializeWriter& nts)
	{
		nts.BeginDict(key);
		nts.WriteBool("uniform", uniform);
		nts.WriteFloat("r", r);
		nts.WriteFloat("r00", r00);
		nts.WriteFloat("r10", r10);
		nts.WriteFloat("r01", r01);
		nts.WriteFloat("r11", r11);
		nts.EndDict();
	}

	bool uniform = true;
	float r = 0;
	float r00 = 0, r10 = 0, r01 = 0, r11 = 0;
};


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
	return Clamp01(q);
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

	TENT__COUNT,
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
	struct Preview : Node
	{
		void Render(UIContainer* ctx) override
		{
			Subscribe(DCT_NodePreviewInvalidated);
			Subscribe(DCT_EditProcGraph);
			Subscribe(DCT_EditProcGraphNode);

			*ctx->Make<ImageElement>()
				->SetImage(node->GetImage())
				->SetScaleMode(ScaleMode::Fit)
				->SetAlphaBackgroundEnabled(true)
				//+ Width(style::Coord::Percent(100)) -- TODO fix
				+ Width(134)
				;
		}
		TE_Node* node;
	};

	virtual ~TE_Node()
	{
		delete _image;
	}
	virtual const char* GetName() = 0;
	virtual const char* GetSysName() = 0;
	virtual TE_NodeType GetType() = 0;
	virtual TE_Node* Clone() = 0;
	virtual int GetInputPinCount() = 0;
	virtual std::string GetInputPinName(int pin) = 0;
	virtual TE_NodeType GetInputPinType(int pin) = 0;
	virtual TE_Node* GetInputPinValue(int pin) = 0;
	virtual void SetInputPinValue(int pin, TE_Node* value) = 0;
	virtual void InputPinUI(int pin, UIContainer* ctx) = 0;
	virtual void PropertyUI(UIContainer* ctx) = 0;
	virtual void Load(NamedTextSerializeReader& nts) = 0;
	virtual void Save(NamedTextSerializeWriter& nts) = 0;
	virtual void Render(Canvas& canvas) = 0;

	void PreviewUI(UIContainer* ctx)
	{
		ctx->Make<Preview>()->node = this;
	}

	void _LoadBase(NamedTextSerializeReader& nts)
	{
		id = nts.ReadUInt("__id");
		position.x = nts.ReadFloat("__x");
		position.y = nts.ReadFloat("__y");
		isPreviewEnabled = nts.ReadBool("__isPreviewEnabled");
	}
	void _SaveBase(NamedTextSerializeWriter& nts)
	{
		nts.WriteInt("__id", id);
		nts.WriteFloat("__x", position.x);
		nts.WriteFloat("__y", position.y);
		nts.WriteBool("__isPreviewEnabled", isPreviewEnabled);
	}

	Image* GetImage()
	{
		if (!_image)
		{
			Canvas canvas;
			Render(canvas);
#if 0
			// show when the image is regenerated
			*canvas.GetPixels() = Color4f(Color4b(rand() % 256, 255)).GetColor32();
#endif
			_image = new Image(canvas);
		}
		return _image;
	}
	void SetDirty()
	{
		delete _image;
		_image = nullptr;
	}
	bool IsDirty()
	{
		return !_image;
	}
	template <class F>
	void IterateDependentNodes(F&& cb)
	{
		for (int i = 0, c = GetInputPinCount(); i < c; i++)
		{
			if (auto* d = GetInputPinValue(i))
				cb(this, d);
		}
	}

	uint32_t id = 0;
	Point<float> position = {};
	bool isPreviewEnabled = true;
	uint8_t _topoState = 0;
	Image* _image = nullptr;
};

static HashMap<uint32_t, TE_Node*> g_nodeRefMap;
template <class T>
static void NodeRefLoad(const char* key, T*& node, NamedTextSerializeReader& nts)
{
	uint32_t id = nts.ReadInt(key);
	auto it = id != 0 ? g_nodeRefMap.find(id) : g_nodeRefMap.end();
	node = it.is_valid() ? static_cast<T*>(it->value) : nullptr;
}
static void NodeRefSave(const char* key, TE_Node* node, NamedTextSerializeWriter& nts)
{
	nts.WriteInt(key, node ? node->id : 0);
}

struct TE_MaskNode : TE_Node
{
	TE_NodeType GetType() override { return TENT_Mask; }

	void Render(Canvas& canvas) override
	{
		TE_RenderContext rc = g_curImageCtx;
		canvas.SetSize(rc.width, rc.height);
		auto* pixels = canvas.GetPixels();
		for (unsigned y = 0; y < rc.height; y++)
		{
			for (unsigned x = 0; x < rc.width; x++)
			{
				pixels[x + y * rc.width] = Color4f(Eval(x + 0.5f, y + 0.5f, rc), 1).GetColor32();
			}
		}
	}

	virtual float Eval(float x, float y, const TE_RenderContext& rc) = 0;
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
	static constexpr const char* NAME = "RECT_MASK";
	const char* GetSysName() override { return NAME; }
	TE_Node* Clone() override { return new TE_RectMask(*this); }
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
	void Load(NamedTextSerializeReader& nts) override
	{
		rect.Load("rect", nts);
		crad.Load("crad", nts);
	}
	void Save(NamedTextSerializeWriter& nts) override
	{
		rect.Save("rect", nts);
		crad.Save("crad", nts);
	}

	float Eval(float x, float y, const TE_RenderContext& rc) override
	{
		AbsRect rr = ResolveRect(rect, rc.frame);
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

	float Eval(float x, float y, const TE_RenderContext& rc)
	{
		if (mask)
			return mask->Eval(x, y, rc);
		CornerRadiuses cr;
		cr.r = radius;
		cr = cr.Eval();
		auto rr = rc.frame.ShrinkBy(UIRect::UniformBorder(border));
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

	void Load(const char* key, NamedTextSerializeReader& nts)
	{
		nts.BeginDict(key);
		NodeRefLoad("mask", mask, nts);
		border = nts.ReadUInt("border");
		radius = nts.ReadUInt("radius");
		nts.EndDict();
	}
	void Save(const char* key, NamedTextSerializeWriter& nts)
	{
		nts.BeginDict(key);
		NodeRefSave("mask", mask, nts);
		nts.WriteInt("border", border);
		nts.WriteInt("radius", radius);
		nts.EndDict();
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
	static constexpr const char* NAME = "COMBINE_MASK";
	const char* GetSysName() override { return NAME; }
	TE_Node* Clone() override { return new TE_CombineMask(*this); }
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
	void Load(NamedTextSerializeReader& nts) override
	{
		masks[0].Load("mask0", nts);
		masks[1].Load("mask1", nts);
		mode = (TE_MaskCombineMode)nts.ReadInt("mode", TEMCM_Intersect);
	}
	void Save(NamedTextSerializeWriter& nts) override
	{
		masks[0].Save("mask0", nts);
		masks[1].Save("mask1", nts);
		nts.WriteInt("mode", mode);
	}

	float Eval(float x, float y, const TE_RenderContext& rc) override
	{
		float a = masks[0].Eval(x, y, rc);
		float b = masks[1].Eval(x, y, rc);
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
	void Render(Canvas& canvas)
	{
		TE_RenderContext rc = g_curImageCtx;
		canvas.SetSize(rc.width, rc.height);
		auto* pixels = canvas.GetPixels();
		for (unsigned y = 0; y < rc.height; y++)
		{
			for (unsigned x = 0; x < rc.width; x++)
			{
				Color4f c = Eval(x + 0.5f, y + 0.5f, rc);
				if (rc.gamma)
					c = c.Power(1.0f / 2.2f);
				pixels[x + y * rc.width] = c.GetColor32();
			}
		}
	}

	TE_NodeType GetType() override { return TENT_Layer; }

	virtual Color4f Eval(float x, float y, const TE_RenderContext& rc) = 0;
};

struct TE_SolidColorLayer : TE_LayerNode
{
	const char* GetName() override { return "Solid color layer"; }
	static constexpr const char* NAME = "SOLID_COLOR_LAYER";
	const char* GetSysName() override { return NAME; }
	TE_Node* Clone() override { return new TE_SolidColorLayer(*this); }
	int GetInputPinCount() override { return 1; }
	std::string GetInputPinName(int pin) override { return "Mask"; }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Mask; }
	TE_Node* GetInputPinValue(int pin) override { return mask.mask; }
	void SetInputPinValue(int pin, TE_Node* value) override { mask.mask = dynamic_cast<TE_MaskNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override { mask.UI(ctx); }
	void PropertyUI(UIContainer* ctx) override
	{
		color.UI(ctx);
	}
	void Load(NamedTextSerializeReader& nts) override
	{
		color.Load("color", nts);
		mask.Load("mask", nts);
	}
	void Save(NamedTextSerializeWriter& nts) override
	{
		color.Save("color", nts);
		mask.Save("mask", nts);
	}

	Color4f Eval(float x, float y, const TE_RenderContext& rc)
	{
		float a = mask.Eval(x, y, rc);
		Color4f cc = color.Get();
		if (rc.gamma)
			cc = cc.Power(2.2f);
		cc.a *= a;
		return cc;
	}

	TE_ColorRef color;
	TE_MaskRef mask;
};

struct TE_LayerBlendRef
{
	TE_LayerNode* layer = nullptr;
	bool enabled = true;
	float opacity = 1;

	void Load(const char* key, NamedTextSerializeReader& nts)
	{
		nts.BeginDict(key);
		NodeRefLoad("layer", layer, nts);
		enabled = nts.ReadBool("enabled", true);
		opacity = nts.ReadFloat("opacity", 1);
		nts.EndDict();
	}
	void Save(const char* key, NamedTextSerializeWriter& nts)
	{
		nts.BeginDict(key);
		NodeRefSave("layer", layer, nts);
		nts.WriteBool("enabled", enabled);
		nts.WriteFloat("opacity", opacity);
		nts.EndDict();
	}
};

struct TE_BlendLayer : TE_LayerNode
{
	const char* GetName() override { return "Blend layer"; }
	static constexpr const char* NAME = "BLEND_LAYER";
	const char* GetSysName() override { return NAME; }
	TE_Node* Clone() override { return new TE_BlendLayer(*this); }
	int GetInputPinCount() override { return layers.size(); }
	std::string GetInputPinName(int pin) override { return "Layer " + std::to_string(pin); }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Layer; }
	TE_Node* GetInputPinValue(int pin) override { return layers[pin].layer; }
	void SetInputPinValue(int pin, TE_Node* value) override { layers[pin].layer = dynamic_cast<TE_LayerNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override
	{
		imm::EditBool(ctx, layers[pin].enabled, nullptr);
		imm::PropEditFloat(ctx, "\bO", layers[pin].opacity);
	}
	void PropertyUI(UIContainer* ctx) override
	{
		if (imm::Button(ctx, "Add pin"))
			layers.push_back({});
	}
	void Load(NamedTextSerializeReader& nts) override
	{
		nts.BeginArray("layers");
		for (auto ch : nts.GetCurrentRange())
		{
			nts.BeginEntry(ch);
			TE_LayerBlendRef lbr;
			lbr.Load("", nts);
			layers.push_back(lbr);
			nts.EndEntry();
		}
		nts.EndArray();
	}
	void Save(NamedTextSerializeWriter& nts) override
	{
		nts.BeginArray("layers");
		for (auto& lbr : layers)
			lbr.Save("", nts);
		nts.EndArray();
	}

	Color4f Eval(float x, float y, const TE_RenderContext& rc)
	{
		Color4f ret = Color4f::Zero();
		for (const auto& layer : layers)
		{
			if (layer.layer && layer.enabled)
			{
				ret.BlendOver(layer.layer->Eval(x, y, rc));
			}
		}
		return ret;
	}

	std::vector<TE_LayerBlendRef> layers;
};

struct TE_ImageNode : TE_Node
{
	const char* GetName() override { return "Image"; }
	static constexpr const char* NAME = "IMAGE";
	const char* GetSysName() override { return NAME; }
	TE_NodeType GetType() override { return TENT_Image; }
	TE_Node* Clone() override { return new TE_ImageNode(*this); }
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
		imm::PropEditInt(ctx, "\bLeft", l, {}, imm::DEFAULT_SPEED, 0, 1024);
		imm::PropEditInt(ctx, "\bRight", r, {}, imm::DEFAULT_SPEED, 0, 1024);
		imm::PropEditInt(ctx, "\bTop", t, {}, imm::DEFAULT_SPEED, 0, 1024);
		imm::PropEditInt(ctx, "\bBtm.", b, {}, imm::DEFAULT_SPEED, 0, 1024);
		imm::PropEditBool(ctx, "\bGamma", gamma);
	}
	void Load(NamedTextSerializeReader& nts) override
	{
		w = nts.ReadUInt("w");
		h = nts.ReadUInt("h");
		l = nts.ReadUInt("l");
		t = nts.ReadUInt("t");
		r = nts.ReadUInt("r");
		b = nts.ReadUInt("b");
		gamma = nts.ReadBool("gamma");
		NodeRefLoad("layer", layer, nts);
	}
	void Save(NamedTextSerializeWriter& nts) override
	{
		nts.WriteInt("w", w);
		nts.WriteInt("h", h);
		nts.WriteInt("l", l);
		nts.WriteInt("t", t);
		nts.WriteInt("r", r);
		nts.WriteInt("b", b);
		nts.WriteBool("gamma", gamma);
		NodeRefSave("layer", layer, nts);
	}

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
	void Render(Canvas& canvas) override
	{
		TE_RenderContext rc = MakeRenderContext();
		canvas.SetSize(rc.width, rc.height);
		auto* pixels = canvas.GetPixels();
		for (int y = 0; y < rc.height; y++)
		{
			for (int x = 0; x < rc.width; x++)
			{
				Color4f col = Color4f::Zero();
				if (layer)
					col = layer->Eval(x + 0.5f, y + 0.5f, rc);
				if (rc.gamma)
					col = col.Power(1.0f / 2.2f);
				pixels[x + y * rc.width] = col.GetColor32();
			}
		}
	}

	unsigned w = 64, h = 64;
	unsigned l = 0, t = 0, r = 0, b = 0;
	bool gamma = false;
	TE_LayerNode* layer = nullptr;
};

struct TE_Template : ui::IProcGraph
{
	~TE_Template()
	{
		Clear();
	}

	void Clear()
	{
		for (TE_Node* node : nodes)
			delete node;
		nodes.clear();
		nodeIDAlloc = 0;
	}

	TE_Node* CreateNodeFromTypeName(StringView type)
	{
		if (type == TE_RectMask::NAME) return new TE_RectMask;
		if (type == TE_CombineMask::NAME) return new TE_CombineMask;
		if (type == TE_SolidColorLayer::NAME) return new TE_SolidColorLayer;
		if (type == TE_BlendLayer::NAME) return new TE_BlendLayer;
		if (type == TE_ImageNode::NAME) return new TE_ImageNode;
		return nullptr;
	}
	struct EntryPair
	{
		NamedTextSerializeReader::Entry* entry;
		TE_Node* node;
	};
	void Load(NamedTextSerializeReader& nts)
	{
		Clear();
		nts.BeginDict("template");

		name = nts.ReadString("name");
		nodeIDAlloc = nts.ReadUInt("nodeIDAlloc");

		nts.BeginArray("colors");
		for (auto ch : nts.GetCurrentRange())
		{
			nts.BeginEntry(ch);
			auto color = std::make_shared<TE_NamedColor>();
			color->Load(nts);
			colors.push_back(color);
			nts.EndEntry();
		}
		nts.EndArray();

		auto* oldcol = g_namedColors;
		g_namedColors = &colors;

		nts.BeginArray("nodes");
		// read node base data (at least type, id), create the node placeholders
		std::vector<EntryPair> entryPairs;
		for (auto ch : nts.GetCurrentRange())
		{
			nts.BeginEntry(ch);
			nts.BeginDict("node");
			auto type = nts.ReadString("__type");
			auto* N = CreateNodeFromTypeName(type);
			if (N)
			{
				N->_LoadBase(nts);
				entryPairs.push_back({ ch, N });
				nodes.push_back(N);
				g_nodeRefMap[N->id] = N;
			}
			nts.EndDict();
			nts.EndEntry();
		}
		// load node data
		for (auto ch : entryPairs)
		{
			nts.BeginEntry(ch.entry);
			nts.BeginDict("node");
			ch.node->Load(nts);
			nts.EndDict();
			nts.EndEntry();
		}
		nts.EndArray();

		g_namedColors = oldcol;

		NodeRefLoad("curNode", curNode, nts);
		g_nodeRefMap.clear();

		nts.EndDict();
	}
	void Save(NamedTextSerializeWriter& nts)
	{
		nts.BeginDict("template");

		nts.WriteString("name", name);
		nts.WriteInt("nodeIDAlloc", nodeIDAlloc);

		nts.BeginArray("colors");
		for (const auto& color : colors)
		{
			color->Save(nts);
		}
		nts.EndArray();

		nts.BeginArray("nodes");
		for (auto* N : nodes)
		{
			nts.BeginDict("node");
			nts.WriteString("__type", N->GetSysName());
			N->_SaveBase(nts);
			N->Save(nts);
			nts.EndDict();
		}
		nts.EndArray();

		NodeRefSave("curNode", curNode, nts);

		nts.EndDict();
	}
	template <class T> T* CreateNode(float x, float y)
	{
		T* node = new T;
		node->id = ++nodeIDAlloc;
		node->position = { x, y };
		nodes.push_back(node);
		return node;
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
		return (TE_Node*)static_cast<TE_Template*>(pg)->CreateNode<T>(0, 0);
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
					{
						inNode->SetInputPinValue(i, nullptr);
						InvalidateNode(inNode);
					}
				}
			}
		}
		else
		{
			auto* n = static_cast<TE_Node*>(pin.end.node);
			n->SetInputPinValue(pin.end.num, nullptr);
			InvalidateNode(n);
		}
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
		topoSortedNodes.clear();
		InvalidateNode(inNode);
		return true;
	}
	bool Unlink(const Link& link) override
	{
		static_cast<TE_Node*>(link.input.node)->SetInputPinValue(link.input.num, nullptr);
		InvalidateNode(static_cast<TE_Node*>(link.input.node));
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

	void OnEditNode(UIEvent& e, Node* node) override
	{
		auto* N = static_cast<TE_Node*>(node);
		if (curNode == N)
		{
			g_curImageCtx = static_cast<TE_ImageNode*>(N)->MakeRenderContext();
			InvalidateAllNodes();
			return;
		}

		UpdateTopoSortedNodes();

		InvalidateNode(N);

		for (size_t i = topoSortedNodes.size(); i > 0; )
		{
			i--;
			topoSortedNodes[i]->IterateDependentNodes([this](TE_Node* n, TE_Node* d)
			{
				if (d->IsDirty())
					InvalidateNode(n);
			});
		}
	}

	void InvalidateNode(TE_Node* n)
	{
		n->SetDirty();
		Notify(DCT_NodePreviewInvalidated, n);
	}
	void InvalidateAllNodes()
	{
		for (auto* n : nodes)
			InvalidateNode(n);
	}
	void SetCurrentImageNode(TE_ImageNode* img)
	{
		g_curImageCtx = img->MakeRenderContext();
		curNode = img;
		InvalidateAllNodes();
	}

	bool CanDeleteNode(Node*) override { return true; }
	void DeleteNode(Node* node) override
	{
		UnlinkPin({ { node, 0 }, true });

		auto* N = static_cast<TE_Node*>(node);

		if (curNode == N)
			curNode = nullptr;

		for (size_t i = 0; i < nodes.size(); i++)
			if (nodes[i] == N)
				nodes.erase(nodes.begin() + i);

		for (size_t i = 0; i < topoSortedNodes.size(); i++)
			if (topoSortedNodes[i] == N)
				topoSortedNodes.erase(topoSortedNodes.begin() + i);

		delete N;
	}

	bool CanDuplicateNode(Node*) override { return true; }
	Node* DuplicateNode(Node* node) override
	{
		auto* n = static_cast<TE_Node*>(node)->Clone();
		n->_image = nullptr;
		n->id = ++nodeIDAlloc;
		n->position += { 5, 5 };
		nodes.push_back(n);
		topoSortedNodes.clear();
		return n;
	}

	void UpdateTopoSortedNodes()
	{
		if (!topoSortedNodes.empty())
			return;

		struct TopoSortUtil
		{
			enum
			{
				Unset = 0,
				Temp,
				Perm,
			};
			void Visit(TE_Node* n)
			{
				if (n->_topoState == Perm)
					return;
				assert(n->_topoState != Temp);

				n->_topoState = Temp;

				n->IterateDependentNodes([this](TE_Node*, TE_Node* dep)
				{
					Visit(dep);
				});

				n->_topoState = Perm;
				topoSortedNodes.push_back(n);
			}

			std::vector<TE_Node*> topoSortedNodes;
		};

		TopoSortUtil tsu;
		for (TE_Node* n : nodes)
			n->_topoState = TopoSortUtil::Unset;
		for (TE_Node* n : nodes)
			if (n->_topoState == TopoSortUtil::Unset)
				tsu.Visit(n);
		topoSortedNodes = std::move(tsu.topoSortedNodes);
	}

	std::vector<TE_Node*> nodes;
	std::vector<std::shared_ptr<TE_NamedColor>> colors;
	uint32_t nodeIDAlloc = 0;
	std::string name;
	TE_Node* curNode = nullptr;

	// edit-only
	std::vector<TE_Node*> topoSortedNodes;
};

struct TE_Variation
{
	std::string name;
};

struct TE_ColorOverride
{
	std::weak_ptr<TE_NamedColor> ncref;
	Color4b color;
};

struct TE_Overrides
{
	std::vector<TE_ColorOverride> colorOverrides;
};

struct TE_Image
{
	bool expanded = true;
	std::string name;
	HashMap<TE_Variation*, std::shared_ptr<TE_Overrides>> overrides;
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
		for (TE_Template* tmpl : templates)
			delete tmpl;
		templates.clear();
		curTemplate = nullptr;
		g_namedColors = nullptr;
	}
	void Load(NamedTextSerializeReader& nts)
	{
		Clear();
		nts.BeginDict("theme");

		nts.BeginArray("templates");
		for (auto ch : nts.GetCurrentRange())
		{
			nts.BeginEntry(ch);
			auto* tmpl = new TE_Template;
			tmpl->Load(nts);
			templates.push_back(tmpl);
			nts.EndEntry();
		}
		nts.EndArray();

		int curTemplateNum = nts.ReadInt("curTemplate", -1);
		curTemplate = curTemplateNum >= 0 && curTemplateNum < templates.size() ? templates[curTemplateNum] : nullptr;
		g_namedColors = curTemplate ? &curTemplate->colors : nullptr;

		nts.EndDict();
	}
	void Save(NamedTextSerializeWriter& nts)
	{
		nts.BeginDict("theme");

		nts.BeginArray("templates");
		for (const auto& tmpl : templates)
		{
			tmpl->Save(nts);
		}
		nts.EndArray();

		int curTemplateNum = -1;
		if (curTemplate)
		{
			for (size_t i = 0; i < templates.size(); i++)
			{
				if (templates[i] == curTemplate)
				{
					curTemplateNum = i;
					break;
				}
			}
		}
		nts.WriteInt("curTemplate", curTemplateNum);

		nts.EndDict();
	}
	void LoadFromFile(const char* path)
	{
		FILE* f = fopen(path, "r");
		if (!f)
			return;
		std::string data;
		fseek(f, 0, SEEK_END);
		if (auto s = ftell(f))
		{
			data.resize(s);
			fseek(f, 0, SEEK_SET);
			s = fread(&data[0], 1, s, f);
			data.resize(s);
		}
		fclose(f);
		NamedTextSerializeReader ntsr;
		ntsr.Parse(data);
		Load(ntsr);
	}
	void SaveToFile(const char* path)
	{
		FILE* f = fopen(path, "w");
		if (!f)
			return;
		NamedTextSerializeWriter ntsw;
		Save(ntsw);
		fwrite(ntsw.data.data(), ntsw.data.size(), 1, f);
		fclose(f);
	}
	void CreateSampleTheme()
	{
		Clear();

		TE_Template* tmpl = new TE_Template;
		tmpl->name = "Button";

		tmpl->colors.push_back(std::make_shared<TE_NamedColor>("dkedge", Color4b(0x1a, 0xff)));
		tmpl->colors.push_back(std::make_shared<TE_NamedColor>("ltedge", Color4b(0x4d, 0xff)));
		tmpl->colors.push_back(std::make_shared<TE_NamedColor>("grdtop", Color4b(0x34, 0xff)));
		tmpl->colors.push_back(std::make_shared<TE_NamedColor>("grdbtm", Color4b(0x27, 0xff)));
#if 0
		auto* mr = tmpl->CreateNode<TE_RectMask>(100, 100);
		mr->rect = { { 0, 0, 1, 1 }, { 1, 1, -1, -1 } };
		mr->crad.r = 16;
#endif
		auto* l1 = tmpl->CreateNode<TE_SolidColorLayer>(100, 100);
		l1->color.Set(tmpl->colors[0]);
		l1->mask.border = 0;
		l1->mask.radius = 1;

		auto* l2 = tmpl->CreateNode<TE_SolidColorLayer>(100, 300);
		l2->color.Set(tmpl->colors[1]);
		l2->mask.border = 1;
		l2->mask.radius = 1;

		auto* l3 = tmpl->CreateNode<TE_SolidColorLayer>(300, 300);
		l3->color.Set(tmpl->colors[2]);
		l3->mask.border = 2;
		l3->mask.radius = 1;

		auto* bl = tmpl->CreateNode<TE_BlendLayer>(300, 100);
		bl->layers.push_back({ l1, true, 1 });
		bl->layers.push_back({ l2, true, 1 });
		bl->layers.push_back({ l3, true, 1 });

		auto* img = tmpl->CreateNode<TE_ImageNode>(500, 100);
		img->layer = bl;
		img->w = 10;
		img->h = 20;
		img->l = img->r = img->t = img->b = 5;

		tmpl->SetCurrentImageNode(img);

		templates.push_back(tmpl);

		curTemplate = tmpl;
		g_namedColors = &tmpl->colors;

		auto variation = std::make_shared<TE_Variation>();
		variations.push_back(variation);
		curVariation = variation.get();

		auto overrides = std::make_shared<TE_Overrides>();
		overrides->colorOverrides.push_back({ tmpl->colors[0], Color4b(0x1a, 0xff) });
		overrides->colorOverrides.push_back({ tmpl->colors[1], Color4b(0x4d, 0xff) });
		overrides->colorOverrides.push_back({ tmpl->colors[2], Color4b(0x34, 0xff) });
		overrides->colorOverrides.push_back({ tmpl->colors[3], Color4b(0x27, 0xff) });

		auto image = std::make_shared<TE_Image>();
		image->name = "TE_ButtonNormal";
		image->overrides[variation.get()] = overrides;
		images.push_back(image);

		//SaveToFile("sample.ths");
		//LoadFromFile("sample.ths");
	}

	std::vector<TE_Template*> templates;
	std::vector<std::shared_ptr<TE_Variation>> variations;
	std::vector<std::shared_ptr<TE_Image>> images;
	TE_Template* curTemplate = nullptr;
	TE_Variation* curVariation = nullptr;
};

enum TE_PreviewMode
{
	TEPM_Original,
	TEPM_Sliced,
};
static TE_PreviewMode g_previewMode = TEPM_Original;
static ScaleMode g_previewScaleMode = ScaleMode::None;
static unsigned g_previewZoomPercent = 100;
static unsigned g_previewSlicedWidth = 128;
static unsigned g_previewSlicedHeight = 32;

struct TE_SlicedImageElement : UIElement
{
	void OnPaint() override
	{
		UIRect r = GetContentRect();
		float iw = _image->GetWidth();
		float ih = _image->GetHeight();
		float x0 = roundf((r.x0 + r.x1 - _width) * 0.5f);
		float y0 = roundf((r.y0 + r.y1 - _height) * 0.5f);
		UIRect outer = { x0, y0, x0 + _width, y0 + _height };
		UIRect inner = outer.ShrinkBy({ float(_left), float(_top), float(_right), float(_bottom) });
		UIRect texinner = { invlerp(0, iw, _left), invlerp(0, ih, _top), invlerp(iw, 0, _right), invlerp(ih, 0, _bottom) };
		draw::RectColTex9Slice(outer, inner, ui::Color4b::White(), _image->_texture, { 0, 0, 1, 1 }, texinner);
	}
	TE_SlicedImageElement* SetImage(Image* img)
	{
		_image = img;
		return this;
	}
	TE_SlicedImageElement* SetTargetSize(int w, int h)
	{
		_width = w;
		_height = h;
		return this;
	}
	TE_SlicedImageElement* SetBorderSizes(int l, int t, int r, int b)
	{
		_left = l;
		_top = t;
		_right = r;
		_bottom = b;
		return this;
	}

	Image* _image = nullptr;
	int _width = 0;
	int _height = 0;
	int _left = 0;
	int _top = 0;
	int _right = 0;
	int _bottom = 0;
};

struct TE_MainPreviewNode : Node
{
	void Render(UIContainer* ctx) override
	{
		Subscribe(DCT_NodePreviewInvalidated);
		Subscribe(DCT_EditProcGraph);
		Subscribe(DCT_EditProcGraphNode);

		ctx->Text("Preview") + Padding(5);

		if (tmpl->curNode)
		{
			ctx->PushBox() + Layout(style::layouts::EdgeSlice());
			{
				ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
				{
					imm::RadioButton(ctx, g_previewMode, TEPM_Original, "Original", {}, imm::ButtonStateToggleSkin());
					imm::RadioButton(ctx, g_previewMode, TEPM_Sliced, "Sliced", {}, imm::ButtonStateToggleSkin());
				}
				ctx->Pop();
				if (g_previewMode == TEPM_Original)
				{
					ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
					{
						imm::RadioButton(ctx, g_previewScaleMode, ScaleMode::None, "No scaling", {}, imm::ButtonStateToggleSkin());
						imm::RadioButton(ctx, g_previewScaleMode, ScaleMode::Fit, "Fit", {}, imm::ButtonStateToggleSkin());
					}
					ctx->Pop();
				}
				if (g_previewMode == TEPM_Sliced)
				{
					ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
					{
						imm::PropEditInt(ctx, "\bWidth", g_previewSlicedWidth, { MinWidth(20) });
						imm::PropEditInt(ctx, "\bHeight", g_previewSlicedHeight, { MinWidth(20) });
					}
					ctx->Pop();
				}

				*ctx->Push<ListBox>() + Height(style::Coord::Percent(95)); // TODO

				auto* imgNode = static_cast<TE_ImageNode*>(tmpl->curNode);
				Canvas canvas;
				imgNode->Render(canvas);
				auto* img = Allocate<Image>(canvas);
				if (g_previewMode == TEPM_Original)
				{
					*ctx->Make<ImageElement>()
						->SetImage(img)
						->SetScaleMode(g_previewScaleMode)
						+ ui::Width(style::Coord::Percent(100))
						+ ui::Height(style::Coord::Percent(100));
				}
				if (g_previewMode == TEPM_Sliced)
				{
					*ctx->Make<TE_SlicedImageElement>()
						->SetImage(img)
						->SetTargetSize(g_previewSlicedWidth, g_previewSlicedHeight)
						->SetBorderSizes(imgNode->l, imgNode->t, imgNode->r, imgNode->b)
						+ ui::Width(style::Coord::Percent(100))
						+ ui::Height(style::Coord::Percent(100));
				}

				ctx->Pop();
			}
			ctx->Pop();
		}
	}

	TE_Template* tmpl;
};

struct TE_TemplateEditorNode : Node
{
	void Render(UIContainer* ctx) override
	{
		auto* hsp = ctx->Push<SplitPane>();
		{
			ctx->Push<ListBox>();
			{
				auto* pge = ctx->Make<ProcGraphEditor>();
				*pge + Height(style::Coord::Percent(100));
				pge->Init(tmpl);
			}
			ctx->Pop();

			ctx->PushBox();
			{
				auto* vsp = ctx->Push<SplitPane>();
				{
					ctx->Make<TE_MainPreviewNode>()->tmpl = tmpl;

					ctx->PushBox();
					{
						ctx->Text("Colors") + Padding(5);
						auto* ced = ctx->Make<SequenceEditor>();
						*ced + Height(style::Coord::Percent(100));
						ced->SetSequence(Allocate<StdSequence<decltype(tmpl->colors)>>(tmpl->colors));
						ced->itemUICallback = [](UIContainer* ctx, SequenceEditor* se, ISequenceIterator* it)
						{
							auto& NC = se->GetSequence()->GetValue<std::shared_ptr<TE_NamedColor>>(it);
							imm::EditColor(ctx, NC->color);
							imm::EditString(ctx, NC->name.c_str(), [&NC](const char* v) { NC->name = v; });
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
			tmpl->SetCurrentImageNode(reinterpret_cast<TE_ImageNode*>(e.arg0));
			Rerender();
		}
	}

	TE_Theme* theme;
	TE_Template* tmpl;
};

struct TE_ImageEditorNode : Node
{
	static constexpr bool Persistent = true;

	TE_Theme* theme;

	void Render(UIContainer* ctx) override
	{
		ctx->Text("Images") + Padding(5);

		auto* imged = ctx->Make<SequenceEditor>();
		*imged + Height(style::Coord::Percent(100));
		imged->showDeleteButton = false;
		imged->SetSequence(Allocate<StdSequence<decltype(theme->images)>>(theme->images));
		imged->itemUICallback = [this](UIContainer* ctx, SequenceEditor* se, ISequenceIterator* it)
		{
			EditImage(ctx, se->GetSequence()->GetValue<std::shared_ptr<TE_Image>>(it).get(), se, it);
		};
	}

	void EditImage(UIContainer* ctx, TE_Image* img, SequenceEditor* se, ISequenceIterator* it)
	{
		ctx->PushBox();
		{
			ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
			{
				imm::EditBool(ctx, img->expanded, nullptr, { Width(style::Coord::Fraction(0)) }, imm::TreeStateToggleSkin());
				imm::EditString(ctx, img->name.c_str(), [&img](const char* v) { img->name = v; });
				se->OnBuildDeleteButton(ctx, it);
			}
			ctx->Pop();

			if (img->expanded)
			{
				ctx->Push<Panel>();
				{
					auto& ovr = img->overrides[theme->curVariation];
					if (!ovr)
						ovr = std::make_shared<TE_Overrides>();

					auto* coed = ctx->Make<SequenceEditor>();
					coed->SetSequence(Allocate<StdSequence<decltype(ovr->colorOverrides)>>(ovr->colorOverrides));
					coed->itemUICallback = [](UIContainer* ctx, SequenceEditor* se, ISequenceIterator* it)
					{
						auto& co = se->GetSequence()->GetValue<TE_ColorOverride>(it);
						if (auto ncr = co.ncref.lock())
							*ctx->MakeWithText<BoxElement>(ncr->name) + Padding(5);
						else
							EditNCRef(ctx, co.ncref);
						imm::EditColor(ctx, co.color, { Width(40) });
					};

					if (imm::Button(ctx, "Add override"))
					{
					}
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
	}
};

struct TE_ThemeEditorNode : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		ctx->Push<MenuBarElement>();
		{
			ctx->Push<MenuItemElement>()->SetText("File");
			{
				ctx->Make<MenuItemElement>()->SetText("New").Func([this]() { theme->Clear(); Rerender(); });
				ctx->Make<MenuItemElement>()->SetText("Open").Func([this]() { theme->LoadFromFile("sample.ths"); Rerender(); });
				ctx->Make<MenuItemElement>()->SetText("Save").Func([this]() { theme->SaveToFile("sample.ths"); Rerender(); });
			}
			ctx->Pop();
		}
		ctx->Pop();

		auto* hsp = ctx->Push<SplitPane>();
		{
			ctx->Make<TE_ImageEditorNode>()->theme = theme;

			*ctx->Push<TabGroup>() + Height(style::Coord::Percent(100)) + Layout(style::layouts::EdgeSlice());
			{
				ctx->Push<TabButtonList>();
				{
					for (TE_Template* tmpl : theme->templates)
					{
						ctx->Push<TabButtonT<TE_Template*>>()->Init(theme->curTemplate, tmpl);
						if (editNameTemplate != tmpl)
						{
							ctx->Text(tmpl->name).HandleEvent(UIEventType::Click) = [this, tmpl](UIEvent& e)
							{
								if (e.numRepeats == 2)
								{
									editNameTemplate = tmpl;
									Rerender();
								}
							};
						}
						else
						{
							auto efn = [this](UIEvent& e)
							{
								if (e.type == UIEventType::LostFocus)
								{
									editNameTemplate = nullptr;
									Rerender();
								}
							};
							imm::EditString(
								ctx,
								tmpl->name.c_str(),
								[tmpl](const char* v) { tmpl->name = v; },
								{ Width(100), EventHandler(efn) });
						}
						ctx->Pop();
					}
				}
				if (imm::Button(ctx, "+"))
				{
					auto* p = new TE_Template;
					p->name = "<name>";
					theme->templates.push_back(p);
				}
				ctx->Pop();

				if (theme->curTemplate)
				{
					*ctx->Push<TabPanel>() + Height(style::Coord::Percent(100));
					{
						auto* pen = ctx->Make<TE_TemplateEditorNode>();
						*pen + Height(style::Coord::Percent(100));
						pen->theme = theme;
						pen->tmpl = theme->curTemplate;
					}
					ctx->Pop();
				}
			}
			ctx->Pop();
		}
		ctx->Pop();
		hsp->SetSplits({ 0.2f });
	}

	TE_Theme* theme;
	TE_Template* editNameTemplate = nullptr;
};

struct ThemeEditorMainWindow : NativeMainWindow
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
		ten->theme = &theme;

		ctx->Make<DefaultOverlayRenderer>();
	}

	TE_Theme theme;
};

int uimain(int argc, char* argv[])
{
	Application app(argc, argv);
	ThemeEditorMainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
