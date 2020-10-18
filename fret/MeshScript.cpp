
#include "pch.h"
#include "MeshScript.h"


ui::DataCategoryTag DCT_MeshScriptChanged[1];


typedef MSNode* CreateNodeFn();
struct NodeCreationEntry
{
	StringView name;
	const char* menuName;
	CreateNodeFn* fn;
};
#define DEF_NODE(name, menuName) { #name, menuName, []() -> MSNode* { return new MSN_##name; } }
static NodeCreationEntry g_nodeTypes[] =
{
	DEF_NODE(NewPrimitive, "New primitive"),
	DEF_NODE(VertexData, "Vertex data"),
	DEF_NODE(IndexData, "Index data"),
};

static MSNode* CreateNode(StringView name)
{
	for (auto& nt : g_nodeTypes)
		if (nt.name == name)
			return nt.fn();
	return nullptr;
}


void MSContext::Error(std::string&& text)
{
	printf("MeshScript ERROR: %s\n", text.c_str());
	errors.push_back({ curNode, std::move(text) });
}

MSNode* MSNode::Clone()
{
	MSNode* tmp = CloneBase();
	for (auto& n : tmp->children)
		n.reset(n->Clone());
	return tmp;
}

MeshScript::~MeshScript()
{
	Clear();
}

void MeshScript::Clear()
{
	rootNodes.clear();
}

void MeshScript::Load(NamedTextSerializeReader& nts)
{
	LoadNodeArr("rootNodes", rootNodes, nts);
}

void MeshScript::Save(NamedTextSerializeWriter& nts)
{
	SaveNodeArr("rootNodes", rootNodes, nts);
}

MSNode::Ptr MeshScript::LoadNode(NamedTextSerializeReader& nts)
{
	nts.BeginDict("node");

	auto type = nts.ReadString("__type");
	MSNode* node = CreateNode(type);
	if (node)
	{
		node->LoadProps(nts);
		LoadNodeArr("children", node->children, nts);
	}

	nts.EndDict();
	return MSNode::Ptr(node);
}

void MeshScript::SaveNode(const MSNode::Ptr& src, NamedTextSerializeWriter& nts)
{
	nts.BeginDict("node");

	nts.WriteString("__type", src->GetName());
	src->SaveProps(nts);
	SaveNodeArr("children", src->children, nts);

	nts.EndDict();
}

void MeshScript::LoadNodeArr(const char* key, std::vector<MSNode::Ptr>& arr, NamedTextSerializeReader& nts)
{
	arr.clear();
	nts.BeginArray(key);
	for (auto e : nts.GetCurrentRange())
	{
		nts.BeginEntry(e);
		arr.push_back(LoadNode(nts));
		nts.EndEntry();
	}
	nts.EndArray();
}

void MeshScript::SaveNodeArr(const char* key, std::vector<MSNode::Ptr>& arr, NamedTextSerializeWriter& nts)
{
	nts.BeginArray(key);
	for (auto& n : arr)
	{
		SaveNode(n, nts);
	}
	nts.EndArray();
}

MSData MeshScript::RunScript(IDataSource* src, IVariableSource* instCtx)
{
	MSData ret;
	MSContext ctx = { &ret, src, instCtx };
	for (auto& r : rootNodes)
		r->Do(ctx);

	// remove primitives w/o any verts
	for (size_t i = 0; i < ret.primitives.size(); i++)
		if (ret.primitives[i].positions.size() == 0)
			ret.primitives.erase(ret.primitives.begin() + i--);

	for (auto& P : ret.primitives)
	{
		if (P.normals.size() > 0 && P.normals.size() != P.positions.size())
			ctx.Error("normal count doesn't match");
		if (P.texcoords.size() > 0 && P.texcoords.size() != P.positions.size())
			ctx.Error("texcoord count doesn't match");
		if (P.colors.size() > 0 && P.colors.size() != P.positions.size())
			ctx.Error("color count doesn't match");

		P.convVerts.reserve(P.positions.size());
		for (size_t i = 0; i < P.positions.size(); i++)
		{
			MSVert vert;
			vert.pos = P.positions[i];

			if (i < P.normals.size())
				vert.nrm = P.normals[i];
			else
				vert.nrm = {};

			if (i < P.texcoords.size())
				vert.tex = P.texcoords[i];
			else
				vert.tex = {};

			if (i < P.colors.size())
				vert.col = P.colors[i];
			else
				vert.col = {};

			P.convVerts.push_back(vert);
		}

		if (P.type == MSPrimType::Quads)
		{
			// cannot universally render quads
			if (P.indices.empty())
			{
				auto verts = std::move(P.convVerts);
				P.convVerts.reserve(verts.size() / 4 * 6);
				for (size_t i = 0; i + 3 < verts.size(); i += 4)
				{
					P.convVerts.push_back(verts[i + 0]);
					P.convVerts.push_back(verts[i + 1]);
					P.convVerts.push_back(verts[i + 2]);

					P.convVerts.push_back(verts[i + 2]);
					P.convVerts.push_back(verts[i + 3]);
					P.convVerts.push_back(verts[i + 0]);
				}
			}
			else
			{
				auto indices = std::move(P.indices);
				P.indices.reserve(indices.size() / 4 * 6);
				for (size_t i = 0; i + 3 < indices.size(); i += 4)
				{
					P.indices.push_back(P.indices[i + 0]);
					P.indices.push_back(P.indices[i + 1]);
					P.indices.push_back(P.indices[i + 2]);

					P.indices.push_back(P.indices[i + 2]);
					P.indices.push_back(P.indices[i + 3]);
					P.indices.push_back(P.indices[i + 0]);
				}
			}

			P.type = MSPrimType::Triangles;
		}

		bool idxErrorShown = false;
		for (auto& idx : P.indices)
		{
			if (idx >= P.convVerts.size())
			{
				if (!idxErrorShown)
				{
					idxErrorShown = true;
					ctx.Error("out-of-bounds indices detected");
				}
				idx = 0;
			}
		}
	}

	return ret;
}

void MeshScript::EditUI(UIContainer* ctx)
{
	auto* sp1 = ctx->Push<ui::SplitPane>();
	{
		auto* tree = ctx->Make<ui::TreeEditor>();
		*tree + ui::Width(style::Coord::Percent(100));
		*tree + ui::Height(style::Coord::Percent(100));
		tree->SetTree(this);
		tree->itemUICallback = [this](UIContainer* ctx, ui::TreeEditor*, ui::TreePathRef path, void* data)
		{
			ui::Property::Scope ps(ctx);
			FindNode(path).Get()->InlineEditUI(ctx);
		};

		ctx->PushBox();
		if (auto selNode = selected.lock())
		{
			selNode->FullEditUI(ctx);
		}
		ctx->Pop();
	}
	ctx->Pop();
	sp1->SetDirection(false);
	sp1->SetSplits({ 0.5f });
	auto* cn = ctx->GetCurrentNode();
	cn->Subscribe(DCT_MeshScriptChanged, this);
	sp1->HandleEvent() = [cn](UIEvent& e)
	{
		if (e.type == UIEventType::IMChange ||
			e.type == UIEventType::SelectionChange)
			cn->Rerender();
	};
}

MeshScript::NodeLoc MeshScript::FindNode(ui::TreePathRef path)
{
	if (path.size() == 1)
		return { &rootNodes, path.last() };
	else
	{
		auto parent = FindNode(path.without_last());
		return { &(*parent.arr)[parent.idx]->children, path.last() };
	}
}

void MeshScript::IterateChildren(ui::TreePathRef path, IterationFunc&& fn)
{
	if (path.empty())
	{
		for (auto& node : rootNodes)
			fn(node.get());
	}
	else
	{
		auto loc = FindNode(path);
		for (auto& node : loc.Get()->children)
			fn(node.get());
	}
}

bool MeshScript::HasChildren(ui::TreePathRef path)
{
	return GetChildCount(path) != 0;
}

size_t MeshScript::GetChildCount(ui::TreePathRef path)
{
	if (path.empty())
		return rootNodes.size();
	auto loc = FindNode(path);
	return loc.Get()->children.size();
}

void MeshScript::FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path)
{
	lastVer = mic.GetVersion();
	GenerateInsertMenu(mic, path);
}

void MeshScript::FillListContextMenu(ui::MenuItemCollection& mic)
{
	if (lastVer != mic.GetVersion())
		GenerateInsertMenu(mic, {});
}

void MeshScript::GenerateInsertMenu(ui::MenuItemCollection& mic, ui::TreePathRef path)
{
	mic.basePriority += ui::MenuItemCollection::BASE_ADVANCE;
	for (auto& nt : g_nodeTypes)
		mic.Add(nt.menuName) = [this, path, &nt]() { Insert(path, nt.fn()); };
}

void MeshScript::Insert(ui::TreePathRef path, MSNode* node)
{
	if (path.empty())
		rootNodes.push_back(MSNode::Ptr(node));
	else
		FindNode(path).Get()->children.push_back(MSNode::Ptr(node));
	ui::Notify(DCT_MeshScriptChanged, this);
}

void MeshScript::ClearSelection()
{
	selected.reset();
}

bool MeshScript::GetSelectionState(ui::TreePathRef path)
{
	auto loc = FindNode(path);
	return loc.Get() == selected.lock();
}

void MeshScript::SetSelectionState(ui::TreePathRef path, bool sel)
{
	auto loc = FindNode(path);
	if (sel)
		selected = loc.Get();
}

void MeshScript::Remove(ui::TreePathRef path)
{
	auto loc = FindNode(path);
	loc.arr->erase(loc.arr->begin() + loc.idx);
}

void MeshScript::Duplicate(ui::TreePathRef path)
{
	auto loc = FindNode(path);
	loc.arr->push_back(MSNode::Ptr(loc.Get()->Clone()));
}

void MeshScript::MoveTo(ui::TreePathRef node, ui::TreePathRef dest)
{
	auto srcLoc = FindNode(node);
	auto srcNode = srcLoc.Get();
	srcLoc.arr->erase(srcLoc.arr->begin() + srcLoc.idx);

	auto destLoc = FindNode(dest);
	destLoc.arr->insert(destLoc.arr->begin() + destLoc.idx, srcNode);
}


void MSN_NewPrimitive::Do(MSContext& C)
{
	MSPrimitive P;
	P.type = type;
	P.texInstID = texInst.Evaluate(*C.vs);
	C.data->primitives.push_back(P);
}

static const char* g_primTypeNames[] =
{
	"Points",
	"Lines",
	"Line strip",
	"Triangles",
	"Triangle strip",
	"Quads",
};

void MSN_NewPrimitive::InlineEditUI(UIContainer* ctx)
{
	char bfr[64];
	snprintf(bfr, 64, "New primitive (%s)", g_primTypeNames[int(type)]);
	ctx->Text(bfr) + ui::Padding(5);
}

void MSN_NewPrimitive::FullEditUI(UIContainer* ctx)
{
	if (ui::imm::PropButton(ctx, "Type", g_primTypeNames[int(type)]))
	{
		ui::MenuItem items[6];
		for (int i = 0; i < 6; i++)
			items[i].text = g_primTypeNames[i];
		int ret = ui::Menu(items).Show(ctx->GetCurrentNode());
		if (ret >= 0)
			type = MSPrimType(ret);
	}

	ui::imm::PropEditString(ctx, "Tex.inst.", texInst.expr.c_str(), [this](const char* v) { texInst.SetExpr(v); });
}


static int g_destComps[4] = { 3, 3, 2, 4 };

template <class T> static void ReadVertexDataT(std::vector<float>& ret, IDataSource* src, int destcomp, int readcomp, int64_t off, int64_t count, int64_t stride)
{
	T buf[4];

	for (int64_t i = 0; i < count; i++, off += stride)
	{
		src->Read(off, sizeof(T) * readcomp, &buf);
		for (int j = 0; j < readcomp; j++)
			ret[i * destcomp + j] = float(buf[j]);
	}
}
typedef void ReadVertexDataFn(std::vector<float>& ret, IDataSource* src, int destcomp, int readcomp, int64_t off, int64_t count, int64_t stride);

ReadVertexDataFn* g_readFuncs[] =
{
	ReadVertexDataT<int8_t>,
	ReadVertexDataT<uint8_t>,
	ReadVertexDataT<int16_t>,
	ReadVertexDataT<uint16_t>,
	ReadVertexDataT<int32_t>,
	ReadVertexDataT<uint32_t>,
	ReadVertexDataT<int64_t>,
	ReadVertexDataT<uint64_t>,
	ReadVertexDataT<float>,
	ReadVertexDataT<double>,
};

static void ReadVertexData(std::vector<float>& ret, IDataSource* src, MSVDType type, int destcomp, int readcomp, int64_t off, int64_t count, int64_t stride)
{
	ret.resize(destcomp * count, 0.0f);

	if (destcomp == 4 && readcomp < 4)
	{
		// fill in the ones
		for (int64_t i = 0; i < count; i++)
			ret[i * 4 + 3] = 1.0f;
	}

	g_readFuncs[int(type)](ret, src, destcomp, readcomp, off, count, stride);
}

template <class T> static void NormalizeDataT(std::vector<float>& data)
{
	float minval = std::numeric_limits<T>::min();
	float maxval = std::numeric_limits<T>::max();
	float tgtmin = minval ? -1 : 0;
	float tgtmax = 1;
	for (auto& elem : data)
		elem = lerp(tgtmin, tgtmax, invlerp(minval, maxval, elem));
}
static void NormalizeData_NONE(std::vector<float>&) {}
typedef void NormalizeDataFn(std::vector<float>& data);

NormalizeDataFn* g_normalizeFuncs[] =
{
	NormalizeDataT<int8_t>,
	NormalizeDataT<uint8_t>,
	NormalizeDataT<int16_t>,
	NormalizeDataT<uint16_t>,
	NormalizeDataT<int32_t>,
	NormalizeDataT<uint32_t>,
	NormalizeDataT<int64_t>,
	NormalizeDataT<uint64_t>,
	NormalizeData_NONE,
	NormalizeData_NONE,
};

static void NormalizeData(std::vector<float>& data, MSVDType type)
{
	g_normalizeFuncs[int(type)](data);
}

template <class T> static void CopyDataT(std::vector<T>& dest, const std::vector<float>& src)
{
	size_t oldCount = dest.size();
	dest.resize(oldCount + src.size() / (sizeof(T) / sizeof(float)));
	memcpy(&dest[oldCount], src.data(), src.size() * 4);
}

void MSN_VertexData::Do(MSContext& C)
{
	if (C.data->primitives.empty())
		return C.Error("missing primitive");
	if (ncomp < 1 || ncomp > 4)
		return C.Error("invalid component count");

	int64_t v_count = count.Evaluate(*C.vs);
	int64_t v_stride = stride.Evaluate(*C.vs);
	int64_t v_attrOff = attrOff.Evaluate(*C.vs);

	int destcomp = g_destComps[int(dest)];
	int realcomp = min(ncomp, destcomp);

	std::vector<float> data;
	ReadVertexData(data, C.src, type, destcomp, realcomp, v_attrOff, v_count, v_stride);
	auto& P = C.data->primitives.back();
	switch (dest)
	{
	case MSVDDest::Position: CopyDataT(P.positions, data); break;
	case MSVDDest::Normal:
		NormalizeData(data, type);
		CopyDataT(P.normals, data);
		break;
	case MSVDDest::Texcoord: CopyDataT(P.texcoords, data); break;
	case MSVDDest::Color:
		NormalizeData(data, type);
		CopyDataT(P.colors, data);
		break;
	}
}

static const char* g_dests[] = { "position", "normal", "texcoord", "color" };
static const char* g_types[] = { "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64" };

void MSN_VertexData::InlineEditUI(UIContainer* ctx)
{
	char bfr[64];
	snprintf(bfr, 64, "vtx.data(%s) %sx%d", g_dests[int(dest)], g_types[int(type)], ncomp);
	ctx->Text(bfr) + ui::Padding(5);
}

void MSN_VertexData::FullEditUI(UIContainer* ctx)
{
	if (ui::imm::PropButton(ctx, "Destination", g_dests[int(dest)]))
	{
		ui::MenuItem items[4];
		for (int i = 0; i < 4; i++)
			items[i].text = g_dests[i];
		int ret = ui::Menu(items).Show(ctx->GetCurrentNode());
		if (ret >= 0)
			dest = MSVDDest(ret);
	}

	if (ui::imm::PropButton(ctx, "Type", g_types[int(type)]))
	{
		ui::MenuItem items[10];
		for (int i = 0; i < 10; i++)
			items[i].text = g_types[i];
		int ret = ui::Menu(items).Show(ctx->GetCurrentNode());
		if (ret >= 0)
			type = MSVDType(ret);
	}

	ui::imm::PropEditInt(ctx, "# components", ncomp, {}, 1.0f, 1, 4);
	ui::imm::PropEditString(ctx, "Count", count.expr.c_str(), [this](const char* v) { count.SetExpr(v); });
	ui::imm::PropEditString(ctx, "Stride", stride.expr.c_str(), [this](const char* v) { stride.SetExpr(v); });
	ui::imm::PropEditString(ctx, "Attr.off.", attrOff.expr.c_str(), [this](const char* v) { attrOff.SetExpr(v); });
}


template <class T> static void ReadIndexDataT(std::vector<uint32_t>& ret, IDataSource* src, int64_t off, int64_t count)
{
	std::vector<T> tmp;
	tmp.resize(count);
	src->Read(off, count * sizeof(T), tmp.data());
	for (int64_t i = 0; i < count; i++)
		ret.push_back(tmp[i]);
}
typedef void ReadIndexDataFn(std::vector<uint32_t>& ret, IDataSource* src, int64_t off, int64_t count);

static ReadIndexDataFn* g_readIndexFuncs[] =
{
	ReadIndexDataT<uint8_t>,
	ReadIndexDataT<uint16_t>,
	ReadIndexDataT<uint32_t>,
};

static void ReadIndexData(std::vector<uint32_t>& ret, IDataSource* src, MSIDType type, int64_t off, int64_t count)
{
	ret.reserve(ret.size() + count);
	g_readIndexFuncs[int(type)](ret, src, off, count);
}

void MSN_IndexData::Do(MSContext& C)
{
	if (C.data->primitives.empty())
		return C.Error("missing primitive");

	int64_t v_count = count.Evaluate(*C.vs);
	int64_t v_attrOff = attrOff.Evaluate(*C.vs);

	auto& data = C.data->primitives.back().indices;
	ReadIndexData(data, C.src, type, v_attrOff, v_count);
}

static const char* g_idxtypes[] = { "u8", "u16", "u32" };

void MSN_IndexData::InlineEditUI(UIContainer* ctx)
{
	char bfr[64];
	snprintf(bfr, 64, "idx.data(%s)", g_idxtypes[int(type)]);
	ctx->Text(bfr) + ui::Padding(5);
}

void MSN_IndexData::FullEditUI(UIContainer* ctx)
{
	if (ui::imm::PropButton(ctx, "Type", g_idxtypes[int(type)]))
	{
		ui::MenuItem items[3];
		for (int i = 0; i < 3; i++)
			items[i].text = g_idxtypes[i];
		int ret = ui::Menu(items).Show(ctx->GetCurrentNode());
		if (ret >= 0)
			type = MSIDType(ret);
	}

	ui::imm::PropEditString(ctx, "Count", count.expr.c_str(), [this](const char* v) { count.SetExpr(v); });
	ui::imm::PropEditString(ctx, "Attr.off.", attrOff.expr.c_str(), [this](const char* v) { attrOff.SetExpr(v); });
}
