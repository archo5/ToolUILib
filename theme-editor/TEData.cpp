
#include "TEData.h"


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

void TE_Node::_SaveBase(NamedTextSerializeWriter& nts)
{
	nts.WriteInt("__id", id);
	nts.WriteFloat("__x", position.x);
	nts.WriteFloat("__y", position.y);
	nts.WriteBool("__isPreviewEnabled", isPreviewEnabled);
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

void TE_RectMask::Save(NamedTextSerializeWriter& nts)
{
	rect.Save("rect", nts);
	crad.Save("crad", nts);
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

void TE_MaskRef::Save(const char* key, NamedTextSerializeWriter& nts)
{
	nts.BeginDict(key);
	NodeRefSave("mask", mask, nts);
	nts.WriteInt("border", border);
	nts.WriteInt("radius", radius);
	nts.WriteInt("vbias", vbias);
	nts.EndDict();
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

void TE_CombineMask::Save(NamedTextSerializeWriter& nts)
{
	masks[0].Save("mask0", nts);
	masks[1].Save("mask1", nts);
	nts.WriteInt("mode", mode);
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

void TE_SolidColorLayer::Save(NamedTextSerializeWriter& nts)
{
	color.Save("color", nts);
	mask.Save("mask", nts);
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

void TE_2ColorLinearGradientColorLayer::Save(NamedTextSerializeWriter& nts)
{
	color0.Save("color0", nts);
	color1.Save("color1", nts);
	pos0.Save("pos0", nts);
	pos1.Save("pos1", nts);
	mask.Save("mask", nts);
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

void TE_LayerBlendRef::Save(const char* key, NamedTextSerializeWriter& nts)
{
	nts.BeginDict(key);
	NodeRefSave("layer", layer, nts);
	nts.WriteBool("enabled", enabled);
	nts.WriteFloat("opacity", opacity);
	nts.EndDict();
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

void TE_BlendLayer::Save(NamedTextSerializeWriter& nts)
{
	nts.BeginArray("layers");
	for (auto& lbr : layers)
		lbr.Save("", nts);
	nts.EndArray();
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


TE_Template::~TE_Template()
{
	Clear();
}

void TE_Template::Clear()
{
	for (TE_Node* node : nodes)
		delete node;
	nodes.clear();
	nodeIDAlloc = 0;
}

TE_Node* TE_Template::CreateNodeFromTypeName(StringView type)
{
	if (type == TE_RectMask::NAME) return new TE_RectMask;
	if (type == TE_CombineMask::NAME) return new TE_CombineMask;
	if (type == TE_SolidColorLayer::NAME) return new TE_SolidColorLayer;
	if (type == TE_2ColorLinearGradientColorLayer::NAME) return new TE_2ColorLinearGradientColorLayer;
	if (type == TE_BlendLayer::NAME) return new TE_BlendLayer;
	if (type == TE_ImageNode::NAME) return new TE_ImageNode;
	return nullptr;
}

struct EntryPair
{
	NamedTextSerializeReader::Entry* entry;
	TE_Node* node;
};

void TE_Template::Load(NamedTextSerializeReader& nts)
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

void TE_Template::Save(NamedTextSerializeWriter& nts)
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

void TE_Template::GetNodes(std::vector<Node*>& outNodes)
{
	for (TE_Node* node : nodes)
		outNodes.push_back(node);
}

void TE_Template::GetLinks(std::vector<Link>& outLinks)
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

void TE_Template::GetAvailableNodeTypes(std::vector<NodeTypeEntry>& outEntries)
{
	outEntries.push_back({ "Masks/Rectangle", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_RectMask>(pg); } });
	outEntries.push_back({ "Masks/Combine", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_CombineMask>(pg); } });
	outEntries.push_back({ "Layers/Solid color", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_SolidColorLayer>(pg); } });
	outEntries.push_back({ "Layers/2 color linear gradient", 0,
		[](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_2ColorLinearGradientColorLayer>(pg); } });
	outEntries.push_back({ "Layers/Blend", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_BlendLayer>(pg); } });
	outEntries.push_back({ "Image", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_ImageNode>(pg); } });
}

std::string TE_Template::GetPinName(const Pin& pin)
{
	if (pin.isOutput)
		return "Output";
	return static_cast<TE_Node*>(pin.end.node)->GetInputPinName(pin.end.num);
}

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

static Color4b GetColorByType(TE_NodeType t)
{
	return g_nodeTypeColors[t];
}

Color4b TE_Template::GetPinColor(const Pin& pin)
{
	if (pin.isOutput)
		return GetColorByType(static_cast<TE_Node*>(pin.end.node)->GetType());
	else
		return GetColorByType(static_cast<TE_Node*>(pin.end.node)->GetInputPinType(pin.end.num));
}

void TE_Template::InputPinEditorUI(const Pin& pin, UIContainer* ctx)
{
	if (pin.isOutput)
		return;
	static_cast<TE_Node*>(pin.end.node)->InputPinUI(pin.end.num, ctx);
}

void TE_Template::UnlinkPin(const Pin& pin)
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

bool TE_Template::LinkExists(const Link& link)
{
	return static_cast<TE_Node*>(link.input.node)->GetInputPinValue(link.input.num) == link.output.node;
}

bool TE_Template::CanLink(const Link& link)
{
	auto* inNode = static_cast<TE_Node*>(link.input.node);
	auto* outNode = static_cast<TE_Node*>(link.output.node);
	return inNode->GetInputPinType(link.input.num) == outNode->GetType();
}

bool TE_Template::TryLink(const Link& link)
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

bool TE_Template::Unlink(const Link& link)
{
	static_cast<TE_Node*>(link.input.node)->SetInputPinValue(link.input.num, nullptr);
	InvalidateNode(static_cast<TE_Node*>(link.input.node));
	return true;
}

void TE_Template::OnEditNode(UIEvent& e, Node* node)
{
	auto* N = static_cast<TE_Node*>(node);
	if (curNode == N)
	{
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

void TE_Template::InvalidateNode(TE_Node* n)
{
	n->SetDirty();
	Notify(DCT_NodePreviewInvalidated, n);
}

void TE_Template::InvalidateAllNodes()
{
	for (auto* n : nodes)
		InvalidateNode(n);
}

void TE_Template::ResolveAllParameters(const TE_RenderContext& rc, const TE_Overrides* ovr)
{
	for (auto* n : nodes)
		if (n->IsDirty())
			n->ResolveParameters(rc, ovr);
}

void TE_Template::SetCurrentImageNode(TE_ImageNode* img)
{
	curNode = img;
	InvalidateAllNodes();
}

void TE_Template::DeleteNode(Node* node)
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

TE_Template::Node* TE_Template::DuplicateNode(Node* node)
{
	auto* n = static_cast<TE_Node*>(node)->Clone();
	n->_image = nullptr;
	n->id = ++nodeIDAlloc;
	n->position += { 5, 5 };
	nodes.push_back(n);
	topoSortedNodes.clear();
	return n;
}

void TE_Template::UpdateTopoSortedNodes()
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

const TE_RenderContext& TE_Template::GetRenderContext()
{
	static TE_RenderContext rc;
	rc = static_cast<TE_ImageNode*>(curNode)->MakeRenderContext();
	ResolveAllParameters(rc, ovrProv->GetOverrides());
	return rc;
}


TE_Theme::TE_Theme()
{
	CreateSampleTheme();
}

TE_Theme::~TE_Theme()
{
	Clear();
}

void TE_Theme::Clear()
{
	for (TE_Template* tmpl : templates)
		delete tmpl;
	templates.clear();
	curTemplate = nullptr;
	g_namedColors = nullptr;
}

void TE_Theme::Load(NamedTextSerializeReader& nts)
{
	Clear();
	nts.BeginDict("theme");

	nts.BeginArray("templates");
	for (auto ch : nts.GetCurrentRange())
	{
		nts.BeginEntry(ch);
		auto* tmpl = new TE_Template;
		tmpl->ovrProv = this;
		tmpl->Load(nts);
		templates.push_back(tmpl);
		nts.EndEntry();
	}
	nts.EndArray();

	int curTemplateNum = nts.ReadInt("curTemplate", -1);
	curTemplate = curTemplateNum >= 0 && curTemplateNum < int(templates.size()) ? templates[curTemplateNum] : nullptr;
	g_namedColors = curTemplate ? &curTemplate->colors : nullptr;

	nts.EndDict();
}

void TE_Theme::Save(NamedTextSerializeWriter& nts)
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

void TE_Theme::LoadFromFile(const char* path)
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

void TE_Theme::SaveToFile(const char* path)
{
	FILE* f = fopen(path, "w");
	if (!f)
		return;
	NamedTextSerializeWriter ntsw;
	Save(ntsw);
	fwrite(ntsw.data.data(), ntsw.data.size(), 1, f);
	fclose(f);
}

void TE_Theme::CreateSampleTheme()
{
	Clear();

	TE_Template* tmpl = new TE_Template;
	tmpl->ovrProv = this;
	tmpl->name = "Button";

	tmpl->colors.push_back(std::make_shared<TE_NamedColor>("dkedge", Color4b(0x00, 0xff)));
	tmpl->colors.push_back(std::make_shared<TE_NamedColor>("ltedge", Color4b(0xcd, 0xff)));
	tmpl->colors.push_back(std::make_shared<TE_NamedColor>("grdtop", Color4b(0x94, 0xff)));
	tmpl->colors.push_back(std::make_shared<TE_NamedColor>("grdbtm", Color4b(0x47, 0xff)));
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

	auto* l3 = tmpl->CreateNode<TE_2ColorLinearGradientColorLayer>(300, 300);
	l3->color0.Set(tmpl->colors[2]);
	l3->color1.Set(tmpl->colors[3]);
	l3->pos0.offset = { 0, 2.5f };
	l3->pos1.offset = { 0, -2.5f };
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

	SetCurPreviewImage(image.get());

	//SaveToFile("sample.ths");
	//LoadFromFile("sample.ths");
}

const TE_Overrides* TE_Theme::GetOverrides()
{
	if (!curPreviewImage)
		return nullptr;
	return curPreviewImage->overrides[curVariation].get();
}

void TE_Theme::SetCurPreviewImage(TE_Image* cpi)
{
	curTemplate->InvalidateAllNodes();
	curPreviewImage = cpi;
	Notify(DCT_ChangeActiveImage);
}
