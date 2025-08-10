
#include "TETheme.h"


void TE_TmplSettings::UI()
{
	{
		LabeledProperty::Scope ps("Size");
		imLabel("\bW"), imMinWidth(20), imEditInt(w, {}, { 1, 1024 });
		imLabel("\bH"), imMinWidth(20), imEditInt(h, {}, { 1, 1024 });
	}
	imLabel("Left"), imEditInt(l, {}, { 0, 1024 });
	imLabel("Right"), imEditInt(r, {}, { 0, 1024 });
	imLabel("Top"), imEditInt(t, {}, { 0, 1024 });
	imLabel("Bottom"), imEditInt(b, {}, { 0, 1024 });
	imLabel("Gamma"), imEditBool(gamma);
}

void TE_TmplSettings::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "TmplSettings");
	OnField(oi, "w", w);
	OnField(oi, "h", h);
	OnField(oi, "l", l);
	OnField(oi, "t", t);
	OnField(oi, "r", r);
	OnField(oi, "b", b);
	OnField(oi, "gamma", gamma);
	OnNodeRefField(oi, "layer", layer);
	oi.EndObject();
}


void TE_Image::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Image");
	OnField(oi, "name", name);
	OnField(oi, "expanded", expanded);

	size_t sz = oi.BeginArray(overrides.size(), "overrides");
	if (oi.IsUnserializer())
	{
		overrides.Clear();
		overrides.Reserve(sz);
		while (oi.HasMoreArrayElements())
		{
			oi.BeginObject({}, "Override");
			std::string variation;
			auto v_overrides = std::make_shared<TE_Overrides>();
			OnField(oi, "variation", variation);
			OnField(oi, "colorOverrides", v_overrides->colorOverrides);
			if (auto* v = oi.GetUnserializeStorage<TE_IUnserializeStorage>()->FindVariation(variation))
				overrides[v] = v_overrides;
			oi.EndObject();
		}
	}
	else
	{
		for (auto kvp : overrides)
		{
			oi.BeginObject({}, "Override");
			OnField(oi, "variation", kvp.key->name);
			OnField(oi, "colorOverrides", kvp.value->colorOverrides);
			oi.EndObject();
		}
	}
	oi.EndArray();

	oi.EndObject();
}


TE_Template::~TE_Template()
{
	Clear();
}

void TE_Template::Clear()
{
	for (TE_Node* node : nodes)
		delete node;
	nodes.Clear();
	nodeIDAlloc = 0;
}

TE_Node* TE_Template::CreateNodeFromTypeName(StringView type)
{
	if (type == TE_RectMask::NAME) return new TE_RectMask;
	if (type == TE_CombineMask::NAME) return new TE_CombineMask;
	if (type == TE_SolidColorLayer::NAME) return new TE_SolidColorLayer;
	if (type == TE_2ColorLinearGradientColorLayer::NAME) return new TE_2ColorLinearGradientColorLayer;
	if (type == TE_BlendLayer::NAME) return new TE_BlendLayer;
	return nullptr;
}

void TE_Template::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Template");

	OnField(oi, "name", name);
	OnField(oi, "nodeIDAlloc", nodeIDAlloc);

	OnFieldPtrArray(oi, "colors", colors,
		[](auto& v) -> TE_NamedColor& { return *v; },
		[]() { return std::make_shared<TE_NamedColor>(); });

	auto* US = oi.GetUnserializeStorage<TE_IUnserializeStorage>();
	if (oi.IsUnserializer())
	{
		US->curNamedColors = &colors;
		US->curNodes.Clear();
	}

	OnFieldPtrArray(oi, "images", images,
		[](auto& v) -> TE_Image& { return *v; },
		[]() { return std::make_shared<TE_Image>(); });

	OnFieldArrayValIndex(oi, "curPreviewImage", curPreviewImage, images, [](auto& v) -> TE_Image* { return v.get(); });

	if (oi.IsUnserializer())
	{
		oi.BeginArray(0, "nodes");
		nodes.Clear();
		// read node base data (at least type, id), create the node placeholders
		while (oi.HasMoreArrayElements())
		{
			std::string type;
			oi.BeginObject(FI, "Node");
			OnField(oi, "__type", type);
			uint32_t id = 0;
			OnField(oi, "__id", id);
			oi.EndObject();
			auto* N = CreateNodeFromTypeName(type);
			nodes.Append(N);
			US->curNodes[id] = N;
		}
		oi.EndArray();
	}
	oi.BeginArray(nodes.size(), "nodes");
	for (TE_Node* node : nodes)
		OnField(oi, {}, *node);
	oi.EndArray();

	OnField(oi, "renderSettings", renderSettings);

	oi.EndObject();
}

void TE_Template::GetNodes(Array<Node*>& outNodes)
{
	for (TE_Node* node : nodes)
		outNodes.Append(node);
}

void TE_Template::GetLinks(Array<Link>& outLinks)
{
	for (TE_Node* inNode : nodes)
	{
		uintptr_t num = inNode->GetInputPinCount();
		for (uintptr_t i = 0; i < num; i++)
		{
			TE_Node* outNode = inNode->GetInputPinValue(i);
			if (outNode)
				outLinks.Append({ { outNode, 0 }, { inNode, i } });
		}
	}
}

void TE_Template::GetAvailableNodeTypes(Array<NodeTypeEntry>& outEntries)
{
	outEntries.Append({ "Masks/Rectangle", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_RectMask>(pg); } });
	outEntries.Append({ "Masks/Combine", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_CombineMask>(pg); } });
	outEntries.Append({ "Layers/Solid color", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_SolidColorLayer>(pg); } });
	outEntries.Append({ "Layers/2 color linear gradient", 0,
		[](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_2ColorLinearGradientColorLayer>(pg); } });
	outEntries.Append({ "Layers/Blend", 0, [](IProcGraph* pg, const char*, uintptr_t) { return CreateNode<TE_BlendLayer>(pg); } });
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

void TE_Template::InputPinEditorUI(const Pin& pin)
{
	if (pin.isOutput)
		return;
	static_cast<TE_Node*>(pin.end.node)->InputPinUI(pin.end.num);
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
	topoSortedNodes.Clear();
	InvalidateNode(inNode);
	return true;
}

bool TE_Template::Unlink(const Link& link)
{
	static_cast<TE_Node*>(link.input.node)->SetInputPinValue(link.input.num, nullptr);
	InvalidateNode(static_cast<TE_Node*>(link.input.node));
	return true;
}

void TE_Template::OnEditNode(Event& e, Node* node)
{
	auto* N = static_cast<TE_Node*>(node);
	if (renderSettings.layer == N)
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
	OnNodePreviewInvalidated.Call(n);
}

void TE_Template::InvalidateAllNodes()
{
	for (TE_Node* n : nodes)
		InvalidateNode(n);
}

void TE_Template::ResolveAllParameters(const TE_RenderContext& rc, const TE_Overrides* ovr)
{
	for (TE_Node* n : nodes)
		if (n->IsDirty())
			n->ResolveParameters(rc, ovr);
}

void TE_Template::SetCurrentLayerNode(TE_LayerNode* img)
{
	renderSettings.layer = img;
	InvalidateAllNodes();
}

void TE_Template::DeleteNode(Node* node)
{
	UnlinkPin({ { node, 0 }, true });

	auto* N = static_cast<TE_Node*>(node);

	if (renderSettings.layer == N)
		renderSettings.layer = nullptr;

	nodes.RemoveFirstOf(N);
	topoSortedNodes.RemoveFirstOf(N);

	delete N;
}

TE_Template::Node* TE_Template::DuplicateNode(Node* node)
{
	auto* n = static_cast<TE_Node*>(node)->Clone();
	n->_image = nullptr;
	n->id = ++nodeIDAlloc;
	n->position += { 5, 5 };
	nodes.Append(n);
	topoSortedNodes.Clear();
	return n;
}

void TE_Template::UpdateTopoSortedNodes()
{
	if (topoSortedNodes.NotEmpty())
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
			topoSortedNodes.Append(n);
		}

		Array<TE_Node*> topoSortedNodes;
	};

	TopoSortUtil tsu;
	for (TE_Node* n : nodes)
		n->_topoState = TopoSortUtil::Unset;
	for (TE_Node* n : nodes)
		if (n->_topoState == TopoSortUtil::Unset)
			tsu.Visit(n);
	topoSortedNodes = Move(tsu.topoSortedNodes);
}

const TE_RenderContext& TE_Template::GetRenderContext()
{
	static TE_RenderContext rc;
	rc = renderSettings.MakeRenderContext();
	auto* ovr = curPreviewImage ? curPreviewImage->overrides[varProv->GetVariation()].get() : nullptr;
	ResolveAllParameters(rc, ovr);
	return rc;
}

void TE_Template::SetCurPreviewImage(TE_Image* cpi)
{
	InvalidateAllNodes();
	curPreviewImage = cpi;
	OnActiveImageChanged.Call();
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
	templates.Clear();
	curTemplate = nullptr;
	g_namedColors = nullptr;
}

TE_Variation* TE_Theme::FindVariation(const std::string& name)
{
	for (auto& v : variations)
	{
		if (v->name == name)
			return v.get();
	}
	return nullptr;
}

void TE_Theme::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Theme");

	OnFieldPtrArray(oi, "variations", variations,
		[](auto& v) -> TE_Variation& { return *v; },
		[this]() { return std::make_shared<TE_Variation>(); });

	OnFieldPtrArray(oi, "templates", templates,
		[](auto& v) -> TE_Template& { return *v; },
		[this]() { return new TE_Template(this); });

	OnFieldArrayValIndex(oi, "curTemplate", curTemplate, templates);
	OnFieldArrayValIndex(oi, "curVariation", curVariation, variations, [](auto& v) -> TE_Variation* { return v.get(); });

	g_namedColors = curTemplate ? &curTemplate->colors : nullptr;

	oi.EndObject();
}

void TE_Theme::LoadFromFile(const char* path)
{
	auto frr = ReadTextFile(path);
	if (!frr.data)
	{
		platform::ShowErrorMessage("Theme Editor", Format("Failed to load theme data from %s", path));
		return;
	}

	JSONUnserializerObjectIterator r;
	r.unserializeStorage = this;
	if (!r.Parse(frr.data->GetStringView()))
	{
		platform::ShowErrorMessage("Theme Editor", Format("Failed to read theme data in %s, it may be corrupted", path));
		return;
	}

	Clear();
	OnSerialize(r, "theme");
}

void TE_Theme::SaveToFile(const char* path)
{
	JSONSerializerObjectIterator w;
	OnSerialize(w, "theme");
	if (!WriteTextFile(path, w.GetData()))
		platform::ShowErrorMessage("Theme Editor", Format("Failed to save theme data to %s", path));
}

void TE_Theme::CreateSampleTheme()
{
	Clear();

	TE_Template* tmpl = new TE_Template(this);
	tmpl->name = "Button";

	tmpl->colors.Append(std::make_shared<TE_NamedColor>("dkedge", Color4b(0x00, 0xff)));
	tmpl->colors.Append(std::make_shared<TE_NamedColor>("ltedge", Color4b(0xcd, 0xff)));
	tmpl->colors.Append(std::make_shared<TE_NamedColor>("grdtop", Color4b(0x94, 0xff)));
	tmpl->colors.Append(std::make_shared<TE_NamedColor>("grdbtm", Color4b(0x47, 0xff)));
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

	auto* bl = tmpl->CreateNode<TE_BlendLayer>(600, 100);
	bl->layers.Append({ l1, true, 1 });
	bl->layers.Append({ l2, true, 1 });
	bl->layers.Append({ l3, true, 1 });

	auto& rs = tmpl->renderSettings;
	rs.w = 10;
	rs.h = 32;
	rs.l = rs.r = rs.t = rs.b = 5;
	tmpl->SetCurrentLayerNode(bl);

	templates.Append(tmpl);

	curTemplate = tmpl;
	g_namedColors = &tmpl->colors;

	auto variation = std::make_shared<TE_Variation>();
	variations.Append(variation);
	curVariation = variation.get();

	auto overrides = std::make_shared<TE_Overrides>();
	overrides->colorOverrides.Append({ tmpl->colors[0], Color4b(0x1a, 0xff) });
	overrides->colorOverrides.Append({ tmpl->colors[1], Color4b(0x4d, 0xff) });
	overrides->colorOverrides.Append({ tmpl->colors[2], Color4b(0x34, 0xff) });
	overrides->colorOverrides.Append({ tmpl->colors[3], Color4b(0x27, 0xff) });

	auto image = std::make_shared<TE_Image>();
	image->name = "TE_ButtonNormal";
	image->overrides[variation.get()] = overrides;
	tmpl->images.Append(image);

	tmpl->SetCurPreviewImage(image.get());

	//SaveToFile("sample.ths");
	//LoadFromFile("sample.ths");
}

TE_Variation* TE_Theme::GetVariation()
{
	return curVariation;
}
