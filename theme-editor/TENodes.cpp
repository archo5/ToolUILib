
#include "TENodes.h"


void TE_Node::Preview::Render(UIContainer* ctx)
{
	Subscribe(DCT_NodePreviewInvalidated);
	Subscribe(DCT_EditProcGraph);
	Subscribe(DCT_EditProcGraphNode);

	*ctx->Make<ImageElement>()
		->SetImage(node->GetImage(rcp))
		->SetScaleMode(ScaleMode::Fit)
		->SetAlphaBackgroundEnabled(true)
		//+ Width(style::Coord::Percent(100)) -- TODO fix
		+ Width(134)
		;
}

void TE_Node::PreviewUI(UIContainer* ctx, TE_IRenderContextProvider* rcp)
{
	auto* preview = ctx->Make<Preview>();
	preview->node = this;
	preview->rcp = rcp;
}

void TE_Node::_LoadBase(NamedTextSerializeReader& nts)
{
	id = nts.ReadUInt("__id");
	position.x = nts.ReadFloat("__x");
	position.y = nts.ReadFloat("__y");
	isPreviewEnabled = nts.ReadBool("__isPreviewEnabled");
}

void TE_Node::_SerializeBase(IObjectIterator& oi)
{
	OnField(oi, "__id", id);
	OnField(oi, "__pos", position);
	OnField(oi, "__isPreviewEnabled", isPreviewEnabled);
}

void TE_Node::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Node");
	if (!oi.IsUnserializer())
	{
		std::string type = GetSysName();
		OnField(oi, "__type", type);
	}
	_SerializeBase(oi);
	Serialize(oi);
	oi.EndObject();
}

Image* TE_Node::GetImage(TE_IRenderContextProvider* rcp)
{
	if (!_image)
	{
		Canvas canvas;
		Render(canvas, rcp->GetRenderContext());
#if 0
			// show when the image is regenerated
		* canvas.GetPixels() = Color4f(Color4b(rand() % 256, 255)).GetColor32();
#endif
		_image = new Image(canvas);
	}
	return _image;
}


HashMap<uint32_t, TE_Node*> g_nodeRefMap;


void TE_MaskNode::Render(Canvas& canvas, const TE_RenderContext& rc)
{
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


void TE_RectMask::PropertyUI(UIContainer* ctx)
{
	{
		ctx->Text("Anchors");
		imm::PropEditFloatVec(ctx, "\bMin", &rect.anchors.x0, "XY", { MinWidth(20) }, 0.01f);
		imm::PropEditFloatVec(ctx, "\bMax", &rect.anchors.x1, "XY", { MinWidth(20) }, 0.01f);
	}
	{
		ctx->Text("Offsets");
		imm::PropEditFloatVec(ctx, "\bMin", &rect.offsets.x0, "XY", { MinWidth(20) }, 0.5f);
		imm::PropEditFloatVec(ctx, "\bMax", &rect.offsets.x1, "XY", { MinWidth(20) }, 0.5f);
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

void TE_RectMask::Load(NamedTextSerializeReader& nts)
{
	rect.Load("rect", nts);
	crad.Load("crad", nts);
}

void TE_RectMask::Serialize(IObjectIterator& oi)
{
	OnField(oi, "rect", rect);
	OnField(oi, "crad", crad);
}

float TE_RectMask::Eval(float x, float y, const TE_RenderContext& rc)
{
	AbsRect rr = rect.Resolve(rc.frame);
	auto cr = crad.Eval();
	return EvalAARoundedRectMask(x, y, rr, cr);
}


float TE_MaskRef::Eval(float x, float y, const TE_RenderContext& rc)
{
	if (mask)
		return mask->Eval(x, y, rc);
	CornerRadiuses cr;
	cr.r = radius;
	cr = cr.Eval();
	auto rr = rc.frame.ShrinkBy(UIRect::UniformBorder(border));
	if (vbias < 0)
		rr.y1 += vbias;
	else
		rr.y0 += vbias;
	return EvalAARoundedRectMask(x, y, rr, cr);
}

void TE_MaskRef::UI(UIContainer* ctx)
{
	if (mask)
		return;
	ctx->PushBox();
	imm::PropEditInt(ctx, "\bBorder", border, { MinWidth(20) });
	imm::PropEditInt(ctx, "\bRadius", radius, { MinWidth(20) });
	imm::PropEditInt(ctx, "\bV.bias", vbias, { MinWidth(20) });
	ctx->Pop();
}

void TE_MaskRef::Load(const char* key, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	NodeRefLoad("mask", mask, nts);
	border = nts.ReadUInt("border");
	radius = nts.ReadUInt("radius");
	vbias = nts.ReadUInt("vbias");
	nts.EndDict();
}

void TE_MaskRef::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "MaskRef");
	OnNodeRefField(oi, "mask", mask);
	OnField(oi, "border", border);
	OnField(oi, "radius", radius);
	OnField(oi, "vbias", vbias);
	oi.EndObject();
}


void TE_CombineMask::PropertyUI(UIContainer* ctx)
{
	imm::RadioButton(ctx, mode, TEMCM_Intersect, "Intersect");
	imm::RadioButton(ctx, mode, TEMCM_Union, "Union");
	imm::RadioButton(ctx, mode, TEMCM_SoftDifference, "Difference (soft)");
	imm::RadioButton(ctx, mode, TEMCM_HardDifference, "Difference (hard)");
	imm::RadioButton(ctx, mode, TEMCM_AMinusB, "A-B");
	imm::RadioButton(ctx, mode, TEMCM_BMinusA, "B-A");
}

void TE_CombineMask::Load(NamedTextSerializeReader& nts)
{
	masks[0].Load("mask0", nts);
	masks[1].Load("mask1", nts);
	mode = (TE_MaskCombineMode)nts.ReadInt("mode", TEMCM_Intersect);
}

void TE_CombineMask::Serialize(IObjectIterator& oi)
{
	OnField(oi, "mask0", masks[0]);
	OnField(oi, "mask1", masks[1]);
	OnFieldEnumInt(oi, "mode", mode);
}

float TE_CombineMask::Eval(float x, float y, const TE_RenderContext& rc)
{
	float a = masks[0].Eval(x, y, rc);
	float b = masks[1].Eval(x, y, rc);
	return Combine(a, b);
}

float TE_CombineMask::Combine(float a, float b)
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


void TE_LayerNode::Render(Canvas& canvas, const TE_RenderContext& rc)
{
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


void TE_SolidColorLayer::PropertyUI(UIContainer* ctx)
{
	color.UI(ctx);
}

void TE_SolidColorLayer::Load(NamedTextSerializeReader& nts)
{
	color.Load("color", nts);
	mask.Load("mask", nts);
}

void TE_SolidColorLayer::Serialize(IObjectIterator& oi)
{
	OnField(oi, "color", color);
	OnField(oi, "mask", mask);
}

void TE_SolidColorLayer::ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr)
{
	color.Resolve(rc, ovr);
}

Color4f TE_SolidColorLayer::Eval(float x, float y, const TE_RenderContext& rc)
{
	float a = mask.Eval(x, y, rc);
	Color4f cc = color.resolvedColor;
	cc.a *= a;
	return cc;
}


void TE_2ColorLinearGradientColorLayer::PropertyUI(UIContainer* ctx)
{
	color0.UI(ctx);
	color1.UI(ctx);
	{
		ctx->Text("Start pos.");
		imm::PropEditFloatVec(ctx, "\bAnchor", &pos0.anchor.x, "XY", { MinWidth(20) }, 0.01f);
		imm::PropEditFloatVec(ctx, "\bOffset", &pos0.offset.x, "XY", { MinWidth(20) }, 0.5f);
	}
	{
		ctx->Text("End pos.");
		imm::PropEditFloatVec(ctx, "\bAnchor", &pos1.anchor.x, "XY", { MinWidth(20) }, 0.01f);
		imm::PropEditFloatVec(ctx, "\bOffset", &pos1.offset.x, "XY", { MinWidth(20) }, 0.5f);
	}
}

void TE_2ColorLinearGradientColorLayer::Load(NamedTextSerializeReader& nts)
{
	color0.Load("color0", nts);
	color1.Load("color1", nts);
	pos0.Load("pos0", nts);
	pos1.Load("pos1", nts);
	mask.Load("mask", nts);
}

void TE_2ColorLinearGradientColorLayer::Serialize(IObjectIterator& oi)
{
	OnField(oi, "color0", color0);
	OnField(oi, "color1", color1);
	OnField(oi, "pos0", pos0);
	OnField(oi, "pos1", pos1);
	OnField(oi, "mask", mask);
}

void TE_2ColorLinearGradientColorLayer::ResolveParameters(const TE_RenderContext& rc, const TE_Overrides* ovr)
{
	color0.Resolve(rc, ovr);
	color1.Resolve(rc, ovr);
}

Color4f TE_2ColorLinearGradientColorLayer::Eval(float x, float y, const TE_RenderContext& rc)
{
	float a = mask.Eval(x, y, rc);
	auto p0 = pos0.Resolve(rc.frame);
	auto p1 = pos1.Resolve(rc.frame);
	auto dir = p1 - p0;
	float dc = x * dir.x + y * dir.y;
	float d0 = p0.x * dir.y + p0.y * dir.y;
	float d1 = p1.x * dir.y + p1.y * dir.y;
	float q = invlerp(d0, d1, dc);
	Color4f cc = Color4fLerp(color0.resolvedColor, color1.resolvedColor, Clamp01(q));
	cc.a *= a;
	return cc;
}


void TE_LayerBlendRef::Load(const char* key, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	NodeRefLoad("layer", layer, nts);
	enabled = nts.ReadBool("enabled", true);
	opacity = nts.ReadFloat("opacity", 1);
	nts.EndDict();
}

void TE_LayerBlendRef::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "LayerBlendRef");
	OnNodeRefField(oi, "layer", layer);
	OnField(oi, "enabled", enabled);
	OnField(oi, "opacity", opacity);
	oi.EndObject();
}


void TE_BlendLayer::InputPinUI(int pin, UIContainer* ctx)
{
	imm::EditBool(ctx, layers[pin].enabled, nullptr);
	imm::PropEditFloat(ctx, "\bO", layers[pin].opacity);
}

void TE_BlendLayer::PropertyUI(UIContainer* ctx)
{
	if (imm::Button(ctx, "Add pin"))
		layers.push_back({});
}

void TE_BlendLayer::Load(NamedTextSerializeReader& nts)
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

void TE_BlendLayer::Serialize(IObjectIterator& oi)
{
	OnField(oi, "layers", layers);
}

Color4f TE_BlendLayer::Eval(float x, float y, const TE_RenderContext& rc)
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
