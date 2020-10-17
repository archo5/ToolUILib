
#include "pch.h"
#include "MeshScript.h"


ui::DataCategoryTag DCT_MeshScriptChanged[1];


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
	roots.clear();
}

MSData MeshScript::RunScript(IDataSource* src, IVariableSource* instCtx)
{
	MSData ret;
	MSContext ctx = { &ret, src, instCtx };
	for (auto& r : roots)
		r->Do(ctx);
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
		return { &roots, path.last() };
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
		for (auto& node : roots)
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
		return roots.size();
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
	mic.Add("New primitive") = [this, path]() { Insert(path, new MSN_NewPrimitive); };
	mic.Add("Vertex data") = [this, path]() { Insert(path, new MSN_VertexData); };
}

void MeshScript::Insert(ui::TreePathRef path, MSNode* node)
{
	if (path.empty())
		roots.push_back(MSNode::Ptr(node));
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
	C.data->primitives.push_back(P);
}

static const char* g_primTypeNames[] =
{
	"Points",
	"Lines",
	"Triangles",
	"Quads",
};

void MSN_NewPrimitive::InlineEditUI(UIContainer* ctx)
{
	char bfr[64];
	snprintf(bfr, 64, "New primitive(%s)", g_primTypeNames[int(type)]);
	ctx->Text(bfr) + ui::Padding(5);
}

void MSN_NewPrimitive::FullEditUI(UIContainer* ctx)
{
	if (ui::imm::PropButton(ctx, "Type", g_primTypeNames[int(type)]))
	{
		ui::MenuItem items[4];
		for (int i = 0; i < 4; i++)
			items[i].text = g_primTypeNames[i];
		int ret = ui::Menu(items).Show(ctx->GetCurrentNode());
		if (ret >= 0)
			type = MSPrimType(ret);
	}
}

static int g_destComps[4] = { 3, 3, 2, 4 };

template <class T> static void ReadDataT(std::vector<float>& ret, IDataSource* src, int destcomp, int readcomp, int64_t off, int64_t count, int64_t stride)
{
	T buf[4];

	for (int64_t i = 0; i < count; i++, off += stride)
	{
		src->Read(off, sizeof(T) * readcomp, &buf);
		for (int j = 0; j < readcomp; j++)
			ret[i * destcomp + j] = float(buf[j]);
	}
}
typedef void ReadDataFn(std::vector<float>& ret, IDataSource* src, int destcomp, int readcomp, int64_t off, int64_t count, int64_t stride);

ReadDataFn* g_readFuncs[] =
{
	ReadDataT<int8_t>,
	ReadDataT<uint8_t>,
	ReadDataT<int16_t>,
	ReadDataT<uint16_t>,
	ReadDataT<int32_t>,
	ReadDataT<uint32_t>,
	ReadDataT<int64_t>,
	ReadDataT<uint64_t>,
	ReadDataT<float>,
	ReadDataT<double>,
};

static void ReadData(std::vector<float>& ret, IDataSource* src, MSVDType type, int destcomp, int readcomp, int64_t off, int64_t count, int64_t stride)
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
	ReadData(data, C.src, type, destcomp, realcomp, v_attrOff, v_count, v_stride);
	auto& P = C.data->primitives.back();
	switch (dest)
	{
	case MSVDDest::Position: CopyDataT(P.positions, data); break;
	case MSVDDest::Normal: CopyDataT(P.normals, data); break;
	case MSVDDest::Texcoord: CopyDataT(P.texcoords, data); break;
	case MSVDDest::Color: CopyDataT(P.colors, data); break;
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
