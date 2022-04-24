
#include "TENodes.h"


void TE_Node::Preview::Build()
{
	Subscribe(DCT_NodePreviewInvalidated);
	Subscribe(DCT_EditProcGraph);
	Subscribe(DCT_EditProcGraphNode);

	Make<ImageElement>()
		.SetImage(node->GetImage(rcp))
		.SetScaleMode(ScaleMode::Fit)
		.SetAlphaBackgroundEnabled(true)
		.SetLayoutMode(ImageLayoutMode::PreferredMin);
}

void TE_Node::PreviewUI(TE_IRenderContextProvider* rcp)
{
	auto& preview = Make<Preview>();
	preview.node = this;
	preview.rcp = rcp;
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

draw::ImageHandle TE_Node::GetImage(TE_IRenderContextProvider* rcp)
{
	if (!_image)
	{
		Canvas canvas;
		Render(canvas, rcp->GetRenderContext());
#if 0
			// show when the image is regenerated
		* canvas.GetPixels() = Color4f(Color4b(rand() % 256, 255)).GetColor32();
#endif
		_image = draw::ImageCreateFromCanvas(canvas);
	}
	return _image;
}


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


void TE_RectMask::PropertyUI()
{
	{
		Text("Anchors");
		imm::PropEditFloatVec("\bMin", &rect.anchors.x0, "XY", { SetMinWidth(20) }, 0.01f);
		imm::PropEditFloatVec("\bMax", &rect.anchors.x1, "XY", { SetMinWidth(20) }, 0.01f);
	}
	{
		Text("Offsets");
		imm::PropEditFloatVec("\bMin", &rect.offsets.x0, "XY", { SetMinWidth(20) }, 0.5f);
		imm::PropEditFloatVec("\bMax", &rect.offsets.x1, "XY", { SetMinWidth(20) }, 0.5f);
	}
	{
		Text("Radius");
		imm::PropEditBool("\bUniform", crad.uniform);
		if (crad.uniform)
		{
			imm::PropEditFloat("\bRadius", crad.r);
		}
		else
		{
			imm::PropEditFloat("\bR(top-left)", crad.r00);
			imm::PropEditFloat("\bR(top-right)", crad.r10);
			imm::PropEditFloat("\bR(bottom-left)", crad.r01);
			imm::PropEditFloat("\bR(bottom-right)", crad.r11);
		}
	}
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

void TE_MaskRef::UI()
{
	if (mask)
		return;
	auto& pl = Push<PropertyList>();
	pl.splitPos = Coord::Percent(30);
	pl.minSplitPos = 50;
	Push<StackTopDownLayoutElement>();
	imm::PropEditInt("Border", border, { SetMinWidth(20) });
	imm::PropEditInt("Radius", radius, { SetMinWidth(20) });
	imm::PropEditInt("V.bias", vbias, { SetMinWidth(20) });
	Pop();
	Pop();
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


void TE_CombineMask::PropertyUI()
{
	imm::RadioButton(mode, TEMCM_Intersect, "Intersect");
	imm::RadioButton(mode, TEMCM_Union, "Union");
	imm::RadioButton(mode, TEMCM_SoftDifference, "Difference (soft)");
	imm::RadioButton(mode, TEMCM_HardDifference, "Difference (hard)");
	imm::RadioButton(mode, TEMCM_AMinusB, "A-B");
	imm::RadioButton(mode, TEMCM_BMinusA, "B-A");
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


void TE_SolidColorLayer::PropertyUI()
{
	color.UI();
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


void TE_2ColorLinearGradientColorLayer::PropertyUI()
{
	color0.UI();
	color1.UI();
	{
		Text("Start pos.");
		imm::PropEditFloatVec("Anchor", &pos0.anchor.x, "XY", { SetMinWidth(20) }, 0.01f);
		imm::PropEditFloatVec("Offset", &pos0.offset.x, "XY", { SetMinWidth(20) }, 0.5f);
	}
	{
		Text("End pos.");
		imm::PropEditFloatVec("Anchor", &pos1.anchor.x, "XY", { SetMinWidth(20) }, 0.01f);
		imm::PropEditFloatVec("Offset", &pos1.offset.x, "XY", { SetMinWidth(20) }, 0.5f);
	}
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


void TE_LayerBlendRef::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "LayerBlendRef");
	OnNodeRefField(oi, "layer", layer);
	OnField(oi, "enabled", enabled);
	OnField(oi, "opacity", opacity);
	oi.EndObject();
}


void TE_BlendLayer::InputPinUI(int pin)
{
	imm::EditBool(layers[pin].enabled, nullptr);
	imm::PropEditFloat("\bO", layers[pin].opacity);
}

void TE_BlendLayer::PropertyUI()
{
	if (imm::Button("Add pin"))
		layers.push_back({});
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
