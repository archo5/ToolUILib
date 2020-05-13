
#include "pch.h"
#include "DataDesc.h"
#include "FileReaders.h"


ui::DataCategoryTag DCT_Marker[1];
ui::DataCategoryTag DCT_MarkedItems[1];
ui::DataCategoryTag DCT_Struct[1];

Color4f colorChar{ 0.3f, 0.9f, 0.1f, 0.3f };
Color4f colorFloat32{ 0.9f, 0.1f, 0.0f, 0.3f };
Color4f colorFloat64{ 0.9f, 0.2f, 0.0f, 0.3f };
Color4f colorInt8{ 0.3f, 0.1f, 0.9f, 0.3f };
Color4f colorInt16{ 0.2f, 0.2f, 0.9f, 0.3f };
Color4f colorInt32{ 0.1f, 0.3f, 0.9f, 0.3f };
Color4f colorInt64{ 0.0f, 0.4f, 0.9f, 0.3f };
Color4f colorASCII{ 0.1f, 0.9f, 0.0f, 0.3f };
Color4f colorInst{ 0.9f, 0.9f, 0.9f, 0.6f };
Color4f colorCurInst{ 0.9f, 0.2f, 0.0f, 0.8f };


static uint8_t typeSizes[] = { 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8 };
static const char* typeNames[] =
{
	"char",
	"i8",
	"u8",
	"i16",
	"u16",
	"i32",
	"u32",
	"i64",
	"u64",
	"f32",
	"f64",
};
const char* GetDataTypeName(DataType t)
{
	return typeNames[t];
}

bool Marker::Contains(uint64_t pos) const
{
	if (pos < at)
		return false;
	pos -= at;
	if (stride)
	{
		if (pos >= stride * repeats)
			return false;
		pos %= stride;
	}
	return pos < typeSizes[type] * count;
}

unsigned Marker::ContainInfo(uint64_t pos) const
{
	if (pos < at)
		return 0;
	pos -= at;
	if (stride)
	{
		if (pos >= stride * repeats)
			return 0;
		pos %= stride;
	}

	if (pos >= typeSizes[type] * count)
		return 0;

	unsigned ret = 1;
	pos %= typeSizes[type];
	if (pos == 0)
		ret |= 2;
	if (pos == typeSizes[type] - 1)
		ret |= 4;
	return ret;

}

uint64_t Marker::GetEnd() const
{
	return at + count * typeSizes[type] + stride * repeats;
}

Color4f Marker::GetColor() const
{
	switch (type)
	{
	case DT_CHAR: return colorChar;
	case DT_I8: return colorInt8;
	case DT_U8: return colorInt8;
	case DT_I16: return colorInt16;
	case DT_U16: return colorInt16;
	case DT_I32: return colorInt32;
	case DT_U32: return colorInt32;
	case DT_I64: return colorInt64;
	case DT_U64: return colorInt64;
	case DT_F32: return colorFloat32;
	case DT_F64: return colorFloat64;
	default: return { 0 };
	}
}


void MarkerData::AddMarker(DataType dt, uint64_t from, uint64_t to)
{
	markers.push_back({ dt, from, (to - from) / typeSizes[dt], 1, 0 });
	ui::Notify(DCT_MarkedItems, this);
}

void MarkerData::Load(const char* key, NamedTextSerializeReader& r)
{
	markers.clear();

	r.BeginDict(key);

	r.BeginArray("markers");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		Marker M;
		M.type = DT_CHAR;
		auto type = r.ReadString("type");
		for (int i = 0; i < DT__COUNT; i++)
			if (typeNames[i] == type)
				M.type = (DataType)i;
		M.at = r.ReadUInt64("at");
		M.count = r.ReadUInt64("count");
		M.repeats = r.ReadUInt64("repeats");
		M.stride = r.ReadUInt64("stride");
		markers.push_back(M);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.EndDict();
}

void MarkerData::Save(const char* key, NamedTextSerializeWriter& w)
{
	w.BeginDict(key);

	w.BeginArray("markers");
	for (const Marker& M : markers)
	{
		w.BeginDict("");

		w.WriteString("type", typeNames[M.type]);
		w.WriteInt("at", M.at);
		w.WriteInt("count", M.count);
		w.WriteInt("repeats", M.repeats);
		w.WriteInt("stride", M.stride);

		w.EndDict();
	}
	w.EndArray();

	w.EndDict();
}

enum COLS_MD
{
	MD_COL_At,
	MD_COL_Type,
	MD_COL_Count,
	MD_COL_Repeats,
	MD_COL_Stride,
};

std::string MarkerData::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string MarkerData::GetColName(size_t col)
{
	switch (col)
	{
	case MD_COL_At: return "Offset";
	case MD_COL_Type: return "Type";
	case MD_COL_Count: return "Count";
	case MD_COL_Repeats: return "Repeats";
	case MD_COL_Stride: return "Stride";
	default: return "";
	}
}

std::string MarkerData::GetText(size_t row, size_t col)
{
	switch (col)
	{
	case MD_COL_At: return std::to_string(markers[row].at);
	case MD_COL_Type: return typeNames[markers[row].type];
	case MD_COL_Count: return std::to_string(markers[row].count);
	case MD_COL_Repeats: return std::to_string(markers[row].repeats);
	case MD_COL_Stride: return std::to_string(markers[row].stride);
	default: return "";
	}
}


void MarkedItemEditor::Render(UIContainer* ctx)
{
	Subscribe(DCT_Marker, marker);
	ctx->Text("Marker");

	ctx->Push<ui::Panel>();
	if (ui::imm::PropButton(ctx, "Type", typeNames[marker->type]))
	{
		std::vector<ui::MenuItem> items;
		int at = 0;
		for (auto* type : typeNames)
			items.push_back(ui::MenuItem(type, {}, false, at++ == marker->type));
		ui::Menu menu(items);
		int pos = menu.Show(this);
		if (pos >= 0)
			marker->type = (DataType)pos;
	}
	ui::imm::PropEditInt(ctx, "Offset", marker->at);
	ui::imm::PropEditInt(ctx, "Count", marker->count);
	ui::imm::PropEditInt(ctx, "Repeats", marker->repeats);
	ui::imm::PropEditInt(ctx, "Stride", marker->stride);
	ctx->Pop();
}


void MarkedItemsList::Render(UIContainer* ctx)
{
	Subscribe(DCT_MarkedItems, markerData);
	ctx->Text("Edit marked items");
	for (auto& m : markerData->markers)
	{
		ctx->Push<ui::Panel>();
		if (ui::imm::PropButton(ctx, "Type", typeNames[m.type]))
		{
			std::vector<ui::MenuItem> items;
			int at = 0;
			for (auto* type : typeNames)
				items.push_back(ui::MenuItem(type, {}, false, at++ == m.type));
			ui::Menu menu(items);
			int pos = menu.Show(this);
			if (pos >= 0)
				m.type = (DataType)pos;
		}
		ui::imm::PropEditInt(ctx, "Offset", m.at);
		ui::imm::PropEditInt(ctx, "Count", m.count);
		ui::imm::PropEditInt(ctx, "Repeats", m.repeats);
		ui::imm::PropEditInt(ctx, "Stride", m.stride);
		ctx->Pop();
	}
}


struct BuiltinTypeInfo
{
	uint8_t size;
	bool separated;
	int64_t(*get_int64)(const void* src);
	void(*append_to_str)(const void* src, std::string& out);
};

static void AddChar(std::string& out, char c)
{
	if (c >= 0x20 && c < 0x7f)
		out.push_back(c);
	else
	{
		out.push_back('\\');
		out.push_back('x');
		out.push_back("0123456789abcdef"[c >> 4]);
		out.push_back("0123456789abcdef"[c & 15]);
	}
}

static std::unordered_map<std::string, BuiltinTypeInfo> g_builtinTypes
{
	{ "pad", { 1, false, [](const void* src) -> int64_t { return 0; }, [](const void* src, std::string& out) {} } },
	{ "char", { 1, false, [](const void* src) -> int64_t { return *(const char*)src; }, [](const void* src, std::string& out) { AddChar(out, *(const char*)src); } } },
	{ "i8", { 1, true, [](const void* src) -> int64_t { return *(const int8_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId8, *(const int8_t*)src); out += bfr; } } },
	{ "u8", { 1, true, [](const void* src) -> int64_t { return *(const uint8_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu8, *(const uint8_t*)src); out += bfr; } } },
	{ "i16", { 2, true, [](const void* src) -> int64_t { return *(const int16_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId16, *(const int16_t*)src); out += bfr; } } },
	{ "u16", { 2, true, [](const void* src) -> int64_t { return *(const uint16_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu16, *(const uint16_t*)src); out += bfr; } } },
	{ "i32", { 4, true, [](const void* src) -> int64_t { return *(const int32_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId32, *(const int32_t*)src); out += bfr; } } },
	{ "u32", { 4, true, [](const void* src) -> int64_t { return *(const uint32_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu32, *(const uint32_t*)src); out += bfr; } } },
	{ "f32", { 4, true, [](const void* src) -> int64_t { return *(const float*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%g", *(const float*)src); out += bfr; } } },
};

uint64_t DataDesc::GetFixedFieldSize(const Field& field)
{
	auto it = g_builtinTypes.find(field.type);
	if (it != g_builtinTypes.end())
		return it->second.size;
	auto sit = structs.find(field.type);
	if (sit != structs.end() && !sit->second->serialized)
		return sit->second->size;
	return UINT64_MAX;
}

static int64_t ReadBuiltinFieldPrimary(IDataSource* ds, const BuiltinTypeInfo& BTI, DataDesc::ReadField& rf, bool readUntil0)
{
	__declspec(align(8)) char bfr[8];
	int64_t off = rf.off;

	int64_t retSize = rf.count * BTI.size;
	int64_t readCount = rf.count;
	if (!readUntil0 && readCount > 24LL)
		readCount = 24LL;

	bool previewAppend = true;
	for (int64_t i = 0; i < readCount; i++)
	{
		ds->Read(off, BTI.size, bfr);

		if (rf.preview.size() <= 24)
		{
			if (BTI.separated && i > 0)
				rf.preview += ", ";
			BTI.append_to_str(bfr, rf.preview);
		}

		rf.intVal = BTI.get_int64(bfr);

		off += BTI.size;

		if (readUntil0 && rf.intVal == 0)
		{
			retSize = off - rf.off;
			break;
		}
	}
	if (rf.preview.size() > 24)
	{
		rf.preview += "...";
	}

	return retSize;
}

static int64_t GetFieldCount(const DataDesc::StructInst& SI, const DataDesc::Field& F, const std::vector<DataDesc::ReadField>& rfs)
{
	if (F.countSrc.empty())
		return F.count;

	if (F.countSrc == ":file-size")
		return SI.file->dataSource->GetSize() + F.count;

	// search previous fields
	int at = 0;
	for (auto& field : SI.def->fields)
	{
		if (&field == &F)
			break;
		if (F.countSrc == field.name)
			return rfs[at].intVal + F.count;
		at++;
	}

	// search arguments
	for (auto& ia : SI.args)
	{
		if (F.countSrc == ia.name)
			return ia.intVal + F.count;
	}

	// search parameter default values
	for (auto& P : SI.def->params)
	{
		if (F.countSrc == P.name)
			return P.intVal + F.count;
	}

	return 0;
}

static int64_t GetCompArgValue(const DataDesc::StructInst& SI, const DataDesc::Field& F, const std::vector<DataDesc::ReadField>& rfs, const DataDesc::CompArg& arg)
{
	if (arg.src.empty())
		return arg.intVal;

	// search previous fields
	int at = 0;
	for (auto& field : SI.def->fields)
	{
		if (&field == &F)
			break;
		if (arg.src == field.name)
			return rfs[at].intVal + arg.intVal;
		at++;
	}

	// search arguments
	for (auto& ia : SI.args)
	{
		if (arg.src == ia.name)
			return ia.intVal + arg.intVal;
	}

	// search parameter default values
	for (auto& P : SI.def->params)
	{
		if (arg.src == P.name)
			return P.intVal + arg.intVal;
	}

	return 0;
}

static bool EvaluateCondition(const DataDesc::Condition& cnd, const DataDesc::StructInst& SI, const std::vector<DataDesc::ReadField>& rfs)
{
	if (cnd.field.empty())
		return true;

	for (size_t i = 0; i < rfs.size(); i++)
	{
		if (SI.def->fields[i].name == cnd.field && rfs[i].preview == cnd.value)
			return true;
	}

	return false;
}

static bool EvaluateConditions(const DataDesc::Field& F, const DataDesc::StructInst& SI, const std::vector<DataDesc::ReadField>& rfs)
{
	if (F.conditions.empty())
		return true;
	for (auto& cnd : F.conditions)
		if (EvaluateCondition(cnd, SI, rfs))
			return true;
	return false;
}

static int64_t DUMP(const std::string& txt, int64_t val)
{
	printf("%s: %" PRId64 "\n", txt.c_str(), val);
	return val;
}

static int64_t GetStructSize(const DataDesc::Struct* S, const DataDesc::StructInst& SI/*, const DataDesc::Field& F*/, const std::vector<DataDesc::ReadField>& rfs)
{
	if (S->sizeSrc.size())
	{
#if 0
		for (auto& ca : F.structArgs)
			if (ca.name == S->sizeSrc)
				return DUMP(S->name + "|structArg", GetCompArgValue(SI, F, rfs, ca) + S->size);
#endif

		for (size_t i = 0; i < rfs.size(); i++)
			if (S->fields[i].name == S->sizeSrc)
				return rfs[i].intVal + S->size;

#if 1
		for (auto& arg : SI.args)
			if (arg.name == S->sizeSrc)
				return DUMP(S->name + "|instArg", arg.intVal + S->size);
#endif
	}
	return DUMP(S->name + "|noSrc", S->size);
}

int64_t DataDesc::ReadStruct(IDataSource* ds, const StructInst& SI, int64_t off, std::vector<ReadField>& out)
{
	if (SI.def->serialized)
	{
		auto origOff = off;
		for (const auto& F : SI.def->fields)
		{
			DataDesc::ReadField rf = { "", off, 0, 0, false };
			if (EvaluateConditions(F, SI, out))
			{
				rf.present = true;
				rf.count = GetFieldCount(SI, F, out);

				auto it = g_builtinTypes.find(F.type);
				if (it != g_builtinTypes.end())
				{
					if (F.countIsMaxSize)
						rf.count /= it->second.size;
					off += ReadBuiltinFieldPrimary(ds, it->second, rf, F.readUntil0);
				}
				else
				{
					auto sit = structs.find(F.type);
					if (sit != structs.end() && sit->second->sizeSrc.empty())
					{
						// only add size if it's manually specified
						off += sit->second->size;
					}
				}
			}
			out.push_back(std::move(rf));
		}
		return off - origOff;
	}
	else
	{
		for (const auto& F : SI.def->fields)
		{
			DataDesc::ReadField rf = { "", off + F.off, 0, 0 };
			if (EvaluateConditions(F, SI, out))
			{
				rf.present = true;
				rf.count = GetFieldCount(SI, F, out);

				auto it = g_builtinTypes.find(F.type);
				if (it != g_builtinTypes.end())
				{
					if (F.countIsMaxSize)
						rf.count /= it->second.size;
					ReadBuiltinFieldPrimary(ds, it->second, rf, F.readUntil0);
				}
			}
			out.push_back(rf);
		}
		return SI.def->size;
	}
}


static void BRB(UIContainer* ctx, const char* text, int& at, int val)
{
	auto* rb = ctx->MakeWithText<ui::RadioButtonT<int>>(text);
	rb->Init(at, val);
	rb->onChange = [rb]() { rb->RerenderNode(); };
	rb->SetStyle(ui::Theme::current->button);
}

static bool advancedAccess = false;
void DataDesc::EditStructuralItems(UIContainer* ctx)
{
	ctx->Push<ui::Panel>();

	ui::imm::PropEditBool(ctx, "Advanced access", advancedAccess);
	ui::imm::PropEditInt(ctx, "Current instance ID", curInst, {}, 1, 0, instances.empty() ? 0 : instances.size() - 1);
	ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
	ctx->Text("Edit:") + ui::Padding(5);
	BRB(ctx, "instance", editMode, 0);
	BRB(ctx, "struct", editMode, 1);
	BRB(ctx, "field", editMode, 2);
	ctx->Pop();

	if (editMode == 0)
	{
		EditInstance(ctx);
	}
	if (editMode == 1)
	{
		EditStruct(ctx);
	}
	if (editMode == 2)
	{
		EditField(ctx);
	}

	ctx->Pop();
}

void DataDesc::EditInstance(UIContainer* ctx)
{
	if (curInst < instances.size())
	{
		auto& SI = instances[curInst];
		bool del = false;

		if (ui::imm::Button(ctx, "Delete"))
		{
			del = true;
		}
		ui::imm::PropEditString(ctx, "Notes", SI.notes.c_str(), [&SI](const char* s) { SI.notes = s; });
		ui::imm::PropEditInt(ctx, "Offset", SI.off);
		if (ui::imm::PropButton(ctx, "Edit struct:", SI.def->name.c_str()))
		{
			editMode = 1;
			ctx->GetCurrentNode()->Rerender();
		}

		if (advancedAccess)
		{
			ui::imm::PropEditBool(ctx, "User created", SI.userCreated);
			ui::imm::PropEditBool(ctx, "Use remaining size", SI.remainingCountIsSize);
			ui::imm::PropEditInt(ctx, SI.remainingCountIsSize ? "Remaining size" : "Remaining count", SI.remainingCount);
		}

		ctx->Text("Arguments") + ui::Padding(5);
		ctx->Push<ui::Panel>();
		for (size_t i = 0; i < SI.args.size(); i++)
		{
			auto& A = SI.args[i];
			ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
			ui::imm::PropEditString(ctx, "\bName", A.name.c_str(), [&A](const char* v) { A.name = v; });
			ui::imm::PropEditInt(ctx, "\bValue", A.intVal);
			if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
			{
				SI.args.erase(SI.args.begin() + i);
				ctx->GetCurrentNode()->Rerender();
			}
			ctx->Pop();
		}
		if (ui::imm::Button(ctx, "Add"))
		{
			SI.args.push_back({ "unnamed", 0 });
			ctx->GetCurrentNode()->Rerender();
		}
		ctx->Pop();

		auto it = structs.find(SI.def->name);
		if (it != structs.end())
		{
			auto& S = *it->second;
			struct Data : ui::TableDataSource
			{
				size_t GetNumRows() override { return rfs.size(); }
				size_t GetNumCols() override { return 3; }
				std::string GetRowName(size_t row) override { return std::to_string(row + 1); }
				std::string GetColName(size_t col) override
				{
					if (col == 0) return "Name";
					if (col == 1) return "Type";
					if (col == 2) return "Preview";
					return "";
				}
				std::string GetText(size_t row, size_t col) override
				{
					switch (col)
					{
					case 0: return S->fields[row].name;
					case 1: return S->fields[row].type + "[" + std::to_string(S->fields[row].count) + "]";
					case 2: return rfs[row].preview;
					default: return "";
					}
				}

				Struct* S;
				std::vector<ReadField> rfs;
			};
			Data data;
			data.S = &S;
			auto size = ReadStruct(SI.file->dataSource, SI, SI.off, data.rfs);
			if (S.sizeSrc.size())
			{
				size = GetStructSize(&S, SI, data.rfs);
			}
			auto remSizeSub = SI.remainingCountIsSize ? size : 1;

			char bfr[256];
			snprintf(bfr, 256, "Next instance (@%" PRId64 ", after %" PRId64 ", rem. %s: %" PRId64 ")",
				SI.off + size,
				size,
				SI.remainingCountIsSize ? "size" : "count",
				SI.remainingCount - remSizeSub);
			if (SI.remainingCount - remSizeSub && ui::imm::Button(ctx, bfr))
			{
				curInst = CreateNextInstance(SI, size);
			}

			snprintf(bfr, 256, "Data (size=%" PRId64 ")", size);
			ctx->Text(bfr) + ui::Padding(5);

			for (size_t i = 0; i < S.fields.size(); i++)
			{
				auto& F = S.fields[i];
				char bfr[256];
				snprintf(bfr, 256, "%s: %s[%s%" PRId64 "]@%" PRId64 "=%s",
					F.name.c_str(),
					F.type.c_str(),
					F.countIsMaxSize ? "up to " : "",
					data.rfs[i].count,
					data.rfs[i].off,
					data.rfs[i].present ? data.rfs[i].preview.c_str() : "<not present>");
				ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
				ctx->Text(bfr) + ui::Padding(5);

				auto strit = structs.find(F.type);
				if (strit != structs.end() && data.rfs[i].present)
				{
					if (ui::imm::Button(ctx, "View"))
					{
						curInst = CreateFieldInstance(SI, data.rfs, i);
					}
				}
				ctx->Pop();
			}
			/*auto* tv = ctx->Make<ui::TableView>();
			tv->SetDataSource(&data); TODO */
		}

		if (del)
		{
			instances.erase(instances.begin() + curInst);
			curInst = 0;
		}
	}
}

struct RenameDialog : ui::NativeDialogWindow
{
	RenameDialog(StringView name)
	{
		newName.assign(name.data(), name.size());
		SetTitle(("Rename struct: " + newName).c_str());
		SetSize(400, 16 * 3 + 24 * 2);
		SetStyle(ui::WindowStyle::WS_TitleBar);
	}
	void OnClose() override
	{
		rename = false;
		SetVisible(false);
	}
	void OnRender(UIContainer* ctx) override
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice()) + ui::Padding(16);
		ui::imm::PropEditString(ctx, "New name:", newName.c_str(), [this](const char* s) { newName = s; });

		*ctx->Make<ui::BoxElement>() + ui::Height(16);

		ui::Property::Begin(ctx);
		if (ui::imm::Button(ctx, "OK", { &ui::Height(30) }))
		{
			rename = true;
			SetVisible(false);
		}
		*ctx->Make<ui::BoxElement>() + ui::Width(16);
		if (ui::imm::Button(ctx, "Cancel", { &ui::Height(30) }))
		{
			rename = false;
			SetVisible(false);
		}
		ui::Property::End(ctx);
		ctx->Pop();
	}

	bool rename = false;
	std::string newName;
};

void DataDesc::EditStruct(UIContainer* ctx)
{
	if (curInst < instances.size())
	{
		auto& SI = instances[curInst];

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("Struct:") + ui::Padding(5);
		ctx->Text(SI.def->name) + ui::Padding(5);
		if (ui::imm::Button(ctx, "Rename"))
		{
			RenameDialog rd(SI.def->name);
			for (;;)
			{
				rd.Show();
				if (rd.rename && rd.newName != SI.def->name)
				{
					if (rd.newName.empty())
					{
						// TODO msgbox/rt
						puts("Name empty!");
						continue;
					}
					if (structs.find(rd.newName) != structs.end())
					{
						// TODO msgbox/rt
						puts("Name already used!");
						continue;
					}
					structs[rd.newName] = SI.def;
					structs.erase(SI.def->name);
					SI.def->name = rd.newName;
				}
				break;
			}
		}
		ctx->Pop();

		auto it = structs.find(SI.def->name);
		if (it != structs.end())
		{
			auto& S = *it->second;
			ui::imm::PropEditBool(ctx, "Is serialized?", S.serialized);

			ui::imm::PropEditInt(ctx, "Size", S.size);
			ui::imm::PropEditString(ctx, "Size source", S.sizeSrc.c_str(), [&S](const char* v) { S.sizeSrc = v; });

			ctx->Text("Parameters") + ui::Padding(5);
			ctx->Push<ui::Panel>();
			for (size_t i = 0; i < S.params.size(); i++)
			{
				auto& P = S.params[i];
				ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
				ui::imm::PropEditString(ctx, "\bName", P.name.c_str(), [&P](const char* v) { P.name = v; });
				ui::imm::PropEditInt(ctx, "\bValue", P.intVal);
				if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
				{
					S.params.erase(S.params.begin() + i);
					ctx->GetCurrentNode()->Rerender();
				}
				ctx->Pop();
			}
			if (ui::imm::Button(ctx, "Add"))
			{
				S.params.push_back({ "unnamed", 0 });
				ctx->GetCurrentNode()->Rerender();
			}
			ctx->Pop();

			ctx->Text("Fields") + ui::Padding(5);
			ctx->Push<ui::Panel>();
			for (size_t i = 0; i < S.fields.size(); i++)
			{
				auto& F = S.fields[i];
				ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
				//ctx->Text(F.name.c_str());
				//ctx->Text(F.type.c_str());
				//ui::imm::EditInt(ctx, "\bCount", F.count);
				char info[128];
				int cc = snprintf(info, 128, "%s: %s[%s%s%s%" PRId64 "]",
					F.name.c_str(),
					F.type.c_str(),
					F.countIsMaxSize ? "up to " : "",
					F.countSrc.c_str(),
					F.count >= 0 && F.countSrc.size() ? "+" : "",
					F.count);
				if (!S.serialized)
					snprintf(info + cc, 128 - cc, " @%" PRId64, F.off);
				*ctx->MakeWithText<ui::BoxElement>(info) + ui::Padding(5);
				//ctx->MakeWithText<ui::Button>("Edit");
				if (ui::imm::Button(ctx, "<", { &ui::Width(20), &ui::Enable(i > 0) }))
				{
					std::swap(S.fields[i - 1], S.fields[i]);
					ctx->GetCurrentNode()->Rerender();
				}
				if (ui::imm::Button(ctx, ">", { &ui::Width(20), &ui::Enable(i + 1 < S.fields.size()) }))
				{
					std::swap(S.fields[i + 1], S.fields[i]);
					ctx->GetCurrentNode()->Rerender();
				}
				if (ui::imm::Button(ctx, "Edit", { &ui::Width(50) }))
				{
					editMode = 2;
					curField = i;
					ctx->GetCurrentNode()->Rerender();
				}
				if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
				{
					S.fields.erase(S.fields.begin() + i);
					ctx->GetCurrentNode()->Rerender();
				}
				ctx->Pop();
			}
			if (ui::imm::Button(ctx, "Add"))
			{
				S.fields.push_back({ "i32", "unnamed" });
				editMode = 2;
				curField = S.fields.size() - 1;
				ctx->GetCurrentNode()->Rerender();
			}
			ctx->Pop();
		}
		else
		{
			ctx->Text("-- NOT FOUND --") + ui::Padding(10);
		}
	}
}

void DataDesc::EditField(UIContainer* ctx)
{
	if (curInst < instances.size())
	{
		auto& SI = instances[curInst];

		auto it = structs.find(SI.def->name);
		if (it != structs.end())
		{
			auto& S = *it->second;
			if (curField < S.fields.size())
			{
				auto& F = S.fields[curField];
				if (!S.serialized)
					ui::imm::PropEditInt(ctx, "Offset", F.off);
				ui::imm::PropEditString(ctx, "Name", F.name.c_str(), [&F](const char* s) { F.name = s; });
				ui::imm::PropEditString(ctx, "Type", F.type.c_str(), [&F](const char* s) { F.type = s; });
				ui::imm::PropEditInt(ctx, "Count", F.count, {}, 1);
				ui::imm::PropEditString(ctx, "Count source", F.countSrc.c_str(), [&F](const char* s) { F.countSrc = s; });
				ui::imm::PropEditBool(ctx, "Count is max. size", F.countIsMaxSize);
				ui::imm::PropEditBool(ctx, "Read until 0", F.readUntil0);

				ctx->Text("Struct arguments") + ui::Padding(5);
				ctx->Push<ui::Panel>();
				for (size_t i = 0; i < F.structArgs.size(); i++)
				{
					auto& SA = F.structArgs[i];
					ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
					ui::imm::PropEditString(ctx, "\bName", SA.name.c_str(), [&SA](const char* v) { SA.name = v; });
					ui::imm::PropEditString(ctx, "\bSource", SA.src.c_str(), [&SA](const char* v) { SA.src = v; });
					ui::imm::PropEditInt(ctx, "\bOffset", SA.intVal);
					if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
					{
						F.structArgs.erase(F.structArgs.begin() + i);
						ctx->GetCurrentNode()->Rerender();
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					F.structArgs.push_back({ "unnamed", "", 0 });
					ctx->GetCurrentNode()->Rerender();
				}
				ctx->Pop();

				ctx->Text("Conditions") + ui::Padding(5);
				ctx->Push<ui::Panel>();
				for (size_t i = 0; i < F.conditions.size(); i++)
				{
					auto& C = F.conditions[i];
					ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
					ui::imm::PropEditString(ctx, "\bField", C.field.c_str(), [&C](const char* v) { C.field = v; });
					ui::imm::PropEditString(ctx, "\bValue", C.value.c_str(), [&C](const char* v) { C.value = v; });
					if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
					{
						F.conditions.erase(F.conditions.begin() + i);
						ctx->GetCurrentNode()->Rerender();
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					F.conditions.push_back({ "unnamed", "" });
					ctx->GetCurrentNode()->Rerender();
				}
				ctx->Pop();
			}
		}
	}
}

void DataDesc::EditImageItems(UIContainer* ctx)
{
	if (curImage < images.size())
	{
		ctx->Push<ui::Panel>();

		auto& I = images[curImage];
		bool del = false;

		if (ui::imm::Button(ctx, "Delete"))
		{
			del = true;
		}
		ui::imm::PropEditString(ctx, "Notes", I.notes.c_str(), [&I](const char* s) { I.notes = s; });
		ui::imm::PropEditInt(ctx, "Image offset", I.offImage);
		ui::imm::PropEditInt(ctx, "Palette offset", I.offPalette);
		ui::imm::PropEditInt(ctx, "Width", I.width);
		ui::imm::PropEditInt(ctx, "Height", I.height);

		if (del)
		{
			images.erase(images.begin() + curImage);
			curImage = 0;
		}

		ctx->Pop();
	}
}

size_t DataDesc::AddInst(const StructInst& src)
{
	printf("trying to create %s @ %" PRId64 "\n", src.def->name.c_str(), src.off);
	for (size_t i = 0; i < instances.size(); i++)
	{
		auto& I = instances[i];
		if (I.off == src.off && I.def == src.def)
		{
			if (src.userCreated)
				I.userCreated = true;
			I.remainingCount = src.remainingCount;
			I.remainingCountIsSize = src.remainingCountIsSize;
			return i;
		}
	}
	auto copy = src;
	instances.push_back(copy);
	return instances.size() - 1;
}

size_t DataDesc::CreateNextInstance(const StructInst& SI, int64_t structSize)
{
	int64_t remSizeSub = SI.remainingCountIsSize ? structSize : 1;
	assert(SI.remainingCount - remSizeSub);
	{
		auto SIcopy = SI;
		SIcopy.off += structSize;
		SIcopy.remainingCount -= remSizeSub;
		SIcopy.userCreated = false;
		return AddInst(SIcopy);
	}
}

size_t DataDesc::CreateFieldInstance(const StructInst& SI, const std::vector<ReadField>& rfs, size_t fieldID)
{
	auto& F = SI.def->fields[fieldID];
	auto SIcopy = SI;
	StructInst newSI = { structs.find(F.type)->second, SIcopy.file, rfs[fieldID].off, "", false };
	newSI.remainingCountIsSize = F.countIsMaxSize;
	newSI.remainingCount = rfs[fieldID].count;
	size_t prevSize = instances.size();
	auto newInst = AddInst(newSI);
	auto& CI = instances[newInst];
	if (prevSize != instances.size())
	{
		for (auto& SA : F.structArgs)
		{
			CI.args.push_back({ SA.name, GetCompArgValue(SIcopy, F, rfs, SA) });
		}
	}
	return newInst;
}

void DataDesc::ExpandAllInstances(File* filterFile)
{
	std::vector<ReadField> rfs;
	for (size_t i = 0; i < instances.size(); i++)
	{
		if (filterFile && instances[i].file != filterFile)
			continue;
		auto SI = instances[i];
		auto& S = *SI.def;
		rfs.clear();
		auto size = ReadStruct(SI.file->dataSource, SI, SI.off, rfs);
		if (S.sizeSrc.size())
		{
			size = GetStructSize(&S, SI, rfs);
		}

		auto remSizeSub = SI.remainingCountIsSize ? size : 1;
		if (SI.remainingCount - remSizeSub)
			CreateNextInstance(instances[i], size);

		for (size_t j = 0; j < S.fields.size(); j++)
			if (rfs[j].present && structs.count(S.fields[j].type))
				CreateFieldInstance(SI, rfs, j);
	}
}

void DataDesc::DeleteAllInstances(File* filterFile, Struct* filterStruct)
{
	instances.erase(std::remove_if(instances.begin(), instances.end(), [filterFile, filterStruct](const DataDesc::StructInst& SI)
	{
		if (SI.userCreated)
			return false;
		if (filterFile && SI.file != filterFile)
			return false;
		if (filterStruct && SI.def != filterStruct)
			return false;
		return true;
	}), instances.end());
}

DataDesc::~DataDesc()
{
	Clear();
}

void DataDesc::Clear()
{
	for (auto* F : files)
		delete F;
	files.clear();
	fileIDAlloc = 0;

	for (auto& sp : structs)
		delete sp.second;
	structs.clear();

	instances.clear();

	images.clear();
}

DataDesc::File* DataDesc::CreateNewFile()
{
	auto* F = new File;
	F->id = fileIDAlloc++;
	files.push_back(F);
	return F;
}

DataDesc::File* DataDesc::FindFileByID(uint64_t id)
{
	for (auto* F : files)
		if (F->id == id)
			return F;
	return nullptr;
}

DataDesc::Struct* DataDesc::CreateNewStruct(const std::string& name)
{
	assert(structs.count(name) == 0);
	auto* S = new Struct;
	S->name = name;
	structs[name] = S;
	return S;
}

DataDesc::Struct* DataDesc::FindStructByName(const std::string& name)
{
	auto it = structs.find(name);
	if (it != structs.end())
		return it->second;
	return nullptr;
}

void DataDesc::Load(const char* key, NamedTextSerializeReader& r)
{
	r.BeginDict(key);

	r.BeginArray("files");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto* F = CreateNewFile();
		F->id = r.ReadUInt64("id");
		F->name = r.ReadString("name");
		F->markerData.Load("markerData", r);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	fileIDAlloc = r.ReadUInt64("fileIDAlloc");

	r.BeginArray("structs");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto name = r.ReadString("name");
		auto* S = CreateNewStruct(name);
		S->serialized = r.ReadBool("serialized");

		r.BeginArray("params");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			Param P;
			P.name = r.ReadString("name");
			P.intVal = r.ReadInt64("intVal");
			S->params.push_back(P);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();

		r.BeginArray("fields");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			Field F;
			F.type = r.ReadString("type");
			F.name = r.ReadString("name");
			F.off = r.ReadInt64("off");
			F.count = r.ReadInt64("count");
			F.countSrc = r.ReadString("countSrc");
			F.countIsMaxSize = r.ReadBool("countIsMaxSize");
			F.readUntil0 = r.ReadBool("readUntil0");

			r.BeginArray("structArgs");
			for (auto E3 : r.GetCurrentRange())
			{
				r.BeginEntry(E3);
				r.BeginDict("");

				CompArg A;
				A.name = r.ReadString("name");
				A.src = r.ReadString("src");
				A.intVal = r.ReadInt64("intVal");
				F.structArgs.push_back(A);

				r.EndDict();
				r.EndEntry();
			}
			r.EndArray();

			r.BeginArray("conditions");
			for (auto E3 : r.GetCurrentRange())
			{
				r.BeginEntry(E3);
				r.BeginDict("");

				Condition C;
				C.field = r.ReadString("field");
				C.value = r.ReadString("value");
				F.conditions.push_back(C);

				r.EndDict();
				r.EndEntry();
			}
			r.EndArray();
			S->fields.push_back(F);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();

		S->size = r.ReadInt64("size");
		S->sizeSrc = r.ReadString("sizeSrc");

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("instances");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		StructInst SI;
		SI.def = FindStructByName(r.ReadString("struct"));
		SI.file = FindFileByID(r.ReadUInt64("file"));
		SI.off = r.ReadInt64("off");
		SI.notes = r.ReadString("notes");
		SI.userCreated = r.ReadBool("userCreated");
		SI.remainingCountIsSize = r.ReadBool("remainingCountIsSize");
		SI.remainingCount = r.ReadInt64("remainingCount");

		r.BeginArray("args");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			Arg A;
			A.name = r.ReadString("name");
			A.intVal = r.ReadInt64("intVal");
			SI.args.push_back(A);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();
		instances.push_back(SI);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("images");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		Image I;
		I.file = FindFileByID(r.ReadUInt64("file"));
		I.offImage = r.ReadInt64("offImage");
		I.offPalette = r.ReadInt64("offPalette");
		I.width = r.ReadUInt("width");
		I.height = r.ReadUInt("height");
		I.format = r.ReadString("format");
		I.notes = r.ReadString("notes");
		I.userCreated = r.ReadBool("userCreated");
		images.push_back(I);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	editMode = r.ReadInt("editMode");
	curInst = r.ReadUInt("curInst");
	curImage = r.ReadUInt("curImage");
	curField = r.ReadUInt("curField");

	r.EndDict();
}

void DataDesc::Save(const char* key, NamedTextSerializeWriter& w)
{
	w.BeginDict(key);

	w.BeginArray("files");
	for (auto* F : files)
	{
		w.BeginDict("");

		w.WriteInt("id", F->id);
		w.WriteString("name", F->name);
		F->markerData.Save("markerData", w);

		w.EndDict();
	}
	w.EndArray();

	w.WriteInt("fileIDAlloc", fileIDAlloc);

	w.BeginArray("structs");
	for (const auto& sp : structs)
	{
		Struct* S = sp.second;

		w.BeginDict("");

		w.WriteString("name", S->name);
		w.WriteBool("serialized", S->serialized);

		w.BeginArray("params");
		for (Param& P : S->params)
		{
			w.BeginDict("");
			w.WriteString("name", P.name);
			w.WriteInt("intVal", P.intVal);
			w.EndDict();
		}
		w.EndArray();

		w.BeginArray("fields");
		for (const Field& F : S->fields)
		{
			w.BeginDict("");
			w.WriteString("type", F.type);
			w.WriteString("name", F.name);
			w.WriteInt("off", F.off);
			w.WriteInt("count", F.count);
			w.WriteString("countSrc", F.countSrc);
			w.WriteBool("countIsMaxSize", F.countIsMaxSize);
			w.WriteBool("readUntil0", F.readUntil0);

			w.BeginArray("structArgs");
			for (const CompArg& A : F.structArgs)
			{
				w.BeginDict("");
				w.WriteString("name", A.name);
				w.WriteString("src", A.src);
				w.WriteInt("intVal", A.intVal);
				w.EndDict();
			}
			w.EndArray();

			w.BeginArray("conditions");
			for (const Condition& C : F.conditions)
			{
				w.BeginDict("");
				w.WriteString("field", C.field);
				w.WriteString("value", C.value);
				w.EndDict();
			}
			w.EndArray();

			w.EndDict();
		}
		w.EndArray();

		w.WriteInt("size", S->size);
		w.WriteString("sizeSrc", S->sizeSrc);

		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("instances");
	for (const StructInst& SI : instances)
	{
		w.BeginDict("");

		w.WriteString("struct", SI.def->name);
		w.WriteInt("file", SI.file->id);
		w.WriteInt("off", SI.off);
		w.WriteString("notes", SI.notes);
		w.WriteBool("userCreated", SI.userCreated);
		w.WriteBool("remainingCountIsSize", SI.remainingCountIsSize);
		w.WriteInt("remainingCount", SI.remainingCount);

		w.BeginArray("args");
		for (const Arg& A : SI.args)
		{
			w.BeginDict("");
			w.WriteString("name", A.name);
			w.WriteInt("intVal", A.intVal);
			w.EndDict();
		}
		w.EndArray();

		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("images");
	for (const Image& I : images)
	{
		w.BeginDict("");

		w.WriteInt("file", I.file->id);
		w.WriteInt("offImage", I.offImage);
		w.WriteInt("offPalette", I.offPalette);
		w.WriteInt("width", I.width);
		w.WriteInt("height", I.height);
		w.WriteString("format", I.format);
		w.WriteString("notes", I.notes);
		w.WriteBool("userCreated", I.userCreated);

		w.EndDict();
	}
	w.EndArray();

	w.WriteInt("editMode", editMode);
	w.WriteInt("curInst", curInst);
	w.WriteInt("curImage", curImage);
	w.WriteInt("curField", curField);

	w.EndDict();
}


enum COLS_DDI
{
	DDI_COL_ID,
	DDI_COL_User,
	DDI_COL_File,
	DDI_COL_Offset,
	DDI_COL_Struct,

	DDI_COL_FirstField,
	DDI_COL_HEADER_SIZE = DDI_COL_FirstField,
};

size_t DataDescInstanceSource::GetNumRows()
{
	_Refilter();
	return _indices.size();
}

size_t DataDescInstanceSource::GetNumCols()
{
	size_t ncols = filterStruct ? DDI_COL_HEADER_SIZE + filterStruct->fields.size() : DDI_COL_HEADER_SIZE;
	if (filterFile)
		ncols--;
	if (filterStruct)
		ncols--;
	return ncols;
}

std::string DataDescInstanceSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string DataDescInstanceSource::GetColName(size_t col)
{
	if (filterFile && col >= DDI_COL_File)
		col++;
	if (filterStruct && col >= DDI_COL_Struct)
		col++;
	switch (col)
	{
	case DDI_COL_ID: return "ID";
	case DDI_COL_User: return "User";
	case DDI_COL_File: return "File";
	case DDI_COL_Offset: return "Offset";
	case DDI_COL_Struct: return "Struct";
	default: return filterStruct->fields[col - DDI_COL_FirstField].name;
	}
}

std::string DataDescInstanceSource::GetText(size_t row, size_t col)
{
	if (filterFile && col >= DDI_COL_File)
		col++;
	if (filterStruct && col >= DDI_COL_Struct)
		col++;
	switch (col)
	{
	case DDI_COL_ID: return std::to_string(_indices[row]);
	case DDI_COL_User: return dataDesc->instances[_indices[row]].userCreated ? "+" : "";
	case DDI_COL_File: return dataDesc->instances[_indices[row]].file->name;
	case DDI_COL_Offset: return std::to_string(dataDesc->instances[_indices[row]].off);
	case DDI_COL_Struct: return dataDesc->instances[_indices[row]].def->name;
	default:
		// TODO optimize to avoid reparsing for each col
	{
		auto& inst = dataDesc->instances[_indices[row]];
		_rfs.clear();
		dataDesc->ReadStruct(inst.file->dataSource, inst, inst.off, _rfs);
		auto& rf = _rfs[col - DDI_COL_FirstField];
		return rf.preview;
	} break;
	}
}

void DataDescInstanceSource::Edit(UIContainer* ctx)
{
	if (ui::imm::PropButton(ctx, "Filter by struct", filterStruct ? filterStruct->name.c_str() : "<none>"))
	{
		std::vector<ui::MenuItem> items;
		items.push_back(ui::MenuItem("<none>").Func([this]() { filterStruct = nullptr; refilter = true; }));
		for (auto& sp : dataDesc->structs)
		{
			auto* S = sp.second;
			items.push_back(ui::MenuItem(sp.first).Func([this, S]() { filterStruct = S; refilter = true; }));
		}
		ui::Menu(items).Show(ctx->GetCurrentNode());
	}
	if (ui::imm::PropButton(ctx, "Filter by file", filterFile ? filterFile->name.c_str() : "<none>"))
	{
		std::vector<ui::MenuItem> items;
		items.push_back(ui::MenuItem("<none>").Func([this]() { filterFile = nullptr; refilter = true; }));
		for (auto* F : dataDesc->files)
		{
			items.push_back(ui::MenuItem(F->name).Func([this, F]() { filterFile = F; refilter = true; }));
		}
		ui::Menu(items).Show(ctx->GetCurrentNode());
	}
	if (ui::imm::PropEditBool(ctx, "Show user created only", filterUserCreated))
		refilter = true;
}

void DataDescInstanceSource::_Refilter()
{
	if (!refilter)
		return;

	_indices.clear();
	_indices.reserve(dataDesc->instances.size());
	for (size_t i = 0; i < dataDesc->instances.size(); i++)
	{
		auto& I = dataDesc->instances[i];
		if (filterStruct && filterStruct != I.def)
			continue;
		if (filterFile && filterFile != I.file)
			continue;
		if (filterUserCreated && !I.userCreated)
			continue;
		_indices.push_back(i);
	}

	refilter = false;
}


enum COLS_DDIMG
{
	DDIMG_COL_ID,
	DDIMG_COL_User,
	DDIMG_COL_File,
	DDIMG_COL_ImgOff,
	DDIMG_COL_PalOff,
	DDIMG_COL_Format,
	DDIMG_COL_Width,
	DDIMG_COL_Height,

	DDIMG_COL_HEADER_SIZE,
};

size_t DataDescImageSource::GetNumRows()
{
	_Refilter();
	return _indices.size();
}

size_t DataDescImageSource::GetNumCols()
{
	size_t ncols = DDIMG_COL_HEADER_SIZE;
	if (filterFile)
		ncols--;
	return ncols;
}

std::string DataDescImageSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string DataDescImageSource::GetColName(size_t col)
{
	if (filterFile && col >= DDIMG_COL_File)
		col++;
	switch (col)
	{
	case DDIMG_COL_ID: return "ID";
	case DDIMG_COL_User: return "User";
	case DDIMG_COL_File: return "File";
	case DDIMG_COL_ImgOff: return "Image Offset";
	case DDIMG_COL_PalOff: return "Palette Offset";
	case DDIMG_COL_Format: return "Format";
	case DDIMG_COL_Width: return "Width";
	case DDIMG_COL_Height: return "Height";
	default: return "???";
	}
}

std::string DataDescImageSource::GetText(size_t row, size_t col)
{
	if (filterFile && col >= DDIMG_COL_File)
		col++;
	switch (col)
	{
	case DDIMG_COL_ID: return std::to_string(_indices[row]);
	case DDIMG_COL_User: return dataDesc->images[_indices[row]].userCreated ? "+" : "";
	case DDIMG_COL_File: return dataDesc->images[_indices[row]].file->name;
	case DDIMG_COL_ImgOff: return std::to_string(dataDesc->images[_indices[row]].offImage);
	case DDIMG_COL_PalOff: return std::to_string(dataDesc->images[_indices[row]].offPalette);
	case DDIMG_COL_Format: return dataDesc->images[_indices[row]].format;
	case DDIMG_COL_Width: return std::to_string(dataDesc->images[_indices[row]].width);
	case DDIMG_COL_Height: return std::to_string(dataDesc->images[_indices[row]].height);
	default: return "???";
	}
}

void DataDescImageSource::Edit(UIContainer* ctx)
{
	if (ui::imm::PropButton(ctx, "Filter by file", filterFile ? filterFile->name.c_str() : "<none>"))
	{
		std::vector<ui::MenuItem> items;
		items.push_back(ui::MenuItem("<none>").Func([this]() { filterFile = nullptr; refilter = true; }));
		for (auto* F : dataDesc->files)
		{
			items.push_back(ui::MenuItem(F->name).Func([this, F]() { filterFile = F; refilter = true; }));
		}
		ui::Menu(items).Show(ctx->GetCurrentNode());
	}
	if (ui::imm::PropEditBool(ctx, "Show user created only", filterUserCreated))
		refilter = true;
}

void DataDescImageSource::_Refilter()
{
	if (!refilter)
		return;

	_indices.clear();
	_indices.reserve(dataDesc->images.size());
	for (size_t i = 0; i < dataDesc->images.size(); i++)
	{
		auto& I = dataDesc->images[i];
		if (filterFile && filterFile != I.file)
			continue;
		if (filterUserCreated && !I.userCreated)
			continue;
		_indices.push_back(i);
	}

	refilter = false;
}
