
#pragma once
#include "TECore.h"


enum TE_NodeType
{
	TENT_Unknown = 0,
	TENT_Mask,
	TENT_Layer,

	TENT__COUNT,
};

struct TE_Node
{
	struct Preview : Node
	{
		void Render(UIContainer* ctx) override;

		TE_Node* node;
		TE_IRenderContextProvider* rcp;
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
	virtual void Serialize(IObjectIterator& oi) = 0;
	virtual void Render(Canvas& canvas, const TE_RenderContext& rc) = 0;
	virtual void ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr) = 0;

	void PreviewUI(UIContainer* ctx, TE_IRenderContextProvider* rcp);

	void _LoadBase(NamedTextSerializeReader& nts);
	void _SerializeBase(IObjectIterator& oi);
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);

	Image* GetImage(TE_IRenderContextProvider* rcp);

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


extern HashMap<uint32_t, TE_Node*> g_nodeRefMap;

template <class T>
inline void NodeRefLoad(const char* key, T*& node, NamedTextSerializeReader& nts)
{
	uint32_t id = nts.ReadInt(key);
	auto it = id != 0 ? g_nodeRefMap.find(id) : g_nodeRefMap.end();
	node = it.is_valid() ? static_cast<T*>(it->value) : nullptr;
}

template <class T>
inline void OnNodeRefField(IObjectIterator& oi, const FieldInfo& FI, T*& node)
{
	uint32_t id = node ? node->id : 0;
	OnField(oi, FI, id);
	if (oi.IsUnserializer())
	{
		auto it = id != 0 ? g_nodeRefMap.find(id) : g_nodeRefMap.end();
		node = it.is_valid() ? static_cast<T*>(it->value) : nullptr;
	}
}


struct TE_MaskNode : TE_Node
{
	TE_NodeType GetType() override { return TENT_Mask; }

	void Render(Canvas& canvas, const TE_RenderContext& rc) override;

	void ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr) override {}

	virtual float Eval(float x, float y, const TE_RenderContext& rc) = 0;
};


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

	void PropertyUI(UIContainer* ctx) override;
	void Load(NamedTextSerializeReader& nts) override;
	void Serialize(IObjectIterator& oi) override;

	float Eval(float x, float y, const TE_RenderContext& rc) override;

	SubRect rect;
	CornerRadiuses crad;
};


struct TE_MaskRef
{
	TE_MaskNode* mask = nullptr;
	unsigned border = 0;
	unsigned radius = 0;
	int vbias = 0;

	float Eval(float x, float y, const TE_RenderContext& rc);
	void UI(UIContainer* ctx);

	void Load(const char* key, NamedTextSerializeReader& nts);
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
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

	void PropertyUI(UIContainer* ctx) override;
	void Load(NamedTextSerializeReader& nts) override;
	void Serialize(IObjectIterator& oi) override;

	float Eval(float x, float y, const TE_RenderContext& rc) override;

	float Combine(float a, float b);

	TE_MaskRef masks[2];
	TE_MaskCombineMode mode = TEMCM_Intersect;
};


struct TE_LayerNode : TE_Node
{
	void Render(Canvas& canvas, const TE_RenderContext& rc);

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

	void PropertyUI(UIContainer* ctx) override;
	void Load(NamedTextSerializeReader& nts) override;
	void Serialize(IObjectIterator& oi) override;
	void ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr) override;

	Color4f Eval(float x, float y, const TE_RenderContext& rc);

	TE_ColorRef color;
	TE_MaskRef mask;
};


struct TE_2ColorLinearGradientColorLayer : TE_LayerNode
{
	const char* GetName() override { return "2-color linear gradient layer"; }
	static constexpr const char* NAME = "2COL_LINEAR_GRAD_COLOR_LAYER";
	const char* GetSysName() override { return NAME; }
	TE_Node* Clone() override { return new TE_2ColorLinearGradientColorLayer(*this); }
	int GetInputPinCount() override { return 1; }
	std::string GetInputPinName(int pin) override { return "Mask"; }
	TE_NodeType GetInputPinType(int pin) override { return TENT_Mask; }
	TE_Node* GetInputPinValue(int pin) override { return mask.mask; }
	void SetInputPinValue(int pin, TE_Node* value) override { mask.mask = dynamic_cast<TE_MaskNode*>(value); }
	void InputPinUI(int pin, UIContainer* ctx) override { mask.UI(ctx); }

	void PropertyUI(UIContainer* ctx) override;
	void Load(NamedTextSerializeReader& nts) override;
	void Serialize(IObjectIterator& oi) override;
	void ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr) override;

	Color4f Eval(float x, float y, const TE_RenderContext& rc) override;

	TE_ColorRef color0;
	TE_ColorRef color1;
	SubPos pos0 = {};
	SubPos pos1 = { { 0, 1 }, { 0, 0 } };
	TE_MaskRef mask;
};


struct TE_LayerBlendRef
{
	TE_LayerNode* layer = nullptr;
	bool enabled = true;
	float opacity = 1;

	void Load(const char* key, NamedTextSerializeReader& nts);
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
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

	void InputPinUI(int pin, UIContainer* ctx) override;
	void PropertyUI(UIContainer* ctx) override;
	void Load(NamedTextSerializeReader& nts) override;
	void Serialize(IObjectIterator& oi) override;
	void ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr) override {}

	Color4f Eval(float x, float y, const TE_RenderContext& rc) override;

	std::vector<TE_LayerBlendRef> layers;
};
