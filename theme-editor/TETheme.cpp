
#include "TETheme.h"


void TE_TmplSettings::UI(UIContainer* ctx)
{
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

void TE_TmplSettings::Load(NamedTextSerializeReader& nts)
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

void TE_TmplSettings::Save(NamedTextSerializeWriter& nts)
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

	nts.BeginDict("renderSettings");
	renderSettings.Load(nts);
	nts.EndDict();

	g_namedColors = oldcol;
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

	nts.BeginDict("renderSettings");
	renderSettings.Save(nts);
	nts.EndDict();

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
	rc = renderSettings.MakeRenderContext();
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

	auto* bl = tmpl->CreateNode<TE_BlendLayer>(600, 100);
	bl->layers.push_back({ l1, true, 1 });
	bl->layers.push_back({ l2, true, 1 });
	bl->layers.push_back({ l3, true, 1 });

	auto& rs = tmpl->renderSettings;
	rs.w = 10;
	rs.h = 32;
	rs.l = rs.r = rs.t = rs.b = 5;
	tmpl->SetCurrentLayerNode(bl);

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
