
#include "pch.h"
#include "DataDesc.h"
#include "FileReaders.h"


ui::DataCategoryTag DCT_Marker[1];
ui::DataCategoryTag DCT_MarkedItems[1];
ui::DataCategoryTag DCT_Struct[1];

Color4f colorFloat32{ 0.9f, 0.1f, 0.0f, 0.3f };
Color4f colorFloat64{ 0.9f, 0.2f, 0.0f, 0.3f };
Color4f colorInt8{ 0.3f, 0.1f, 0.9f, 0.3f };
Color4f colorInt16{ 0.2f, 0.2f, 0.9f, 0.3f };
Color4f colorInt32{ 0.1f, 0.3f, 0.9f, 0.3f };
Color4f colorInt64{ 0.0f, 0.4f, 0.9f, 0.3f };
Color4f colorASCII{ 0.1f, 0.9f, 0.0f, 0.3f };


static uint8_t typeSizes[] = { 1, 1, 2, 2, 4, 4, 8, 8, 4, 8 };
static const char* typeNames[] =
{
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

Color4f Marker::GetColor() const
{
	switch (type)
	{
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


Color4f MarkerData::GetMarkedColor(uint64_t pos)
{
	for (const auto& m : markers)
	{
		if (m.Contains(pos))
			return m.GetColor();
	}
	return Color4f::Zero();
}

bool MarkerData::IsMarked(uint64_t pos, uint64_t len)
{
	// TODO optimize
	for (uint64_t i = 0; i < len; i++)
	{
		for (const auto& m : markers)
			if (m.Contains(pos + i))
				return true;
	}
	return false;
}

void MarkerData::AddMarker(DataType dt, uint64_t from, uint64_t to)
{
	markers.push_back({ dt, from, (to - from) / typeSizes[dt], 1, 0 });
	ui::Notify(DCT_MarkedItems, this);
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

static std::unordered_map<std::string, BuiltinTypeInfo> g_builtinTypes
{
	{ "pad", { 1, false, [](const void* src) -> int64_t { return 0; }, [](const void* src, std::string& out) {} } },
	{ "char", { 1, false, [](const void* src) -> int64_t { return *(const char*)src; }, [](const void* src, std::string& out) { out.push_back(*(const char*)src); } } },
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

static void ReadBuiltinFieldPrimary(IDataSource* ds, const BuiltinTypeInfo& BTI, DataDesc::ReadField& rf, bool readUntil0)
{
	__declspec(align(8)) char bfr[8];
	uint64_t off = rf.off;
	for (int64_t i = 0; i < std::min(rf.count, 24LL); i++)
	{
		if (BTI.separated && i > 0)
			rf.preview += ", ";
		ds->Read(off, BTI.size, bfr);
		BTI.append_to_str(bfr, rf.preview);
		rf.intVal = BTI.get_int64(bfr);
		off += BTI.size;
		if (readUntil0 && rf.intVal == 0)
			return;
	}
	if (rf.count > 24)
	{
		rf.preview += "...";
	}
}

static int64_t GetFieldCount(const DataDesc::StructInst& SI, const DataDesc::Field& F, const std::vector<DataDesc::ReadField>& rfs)
{
	if (F.countSrc.empty())
		return F.count;

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
					ReadBuiltinFieldPrimary(ds, it->second, rf, F.readUntil0);
					off += it->second.size * F.count;
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
			out.push_back(rf);
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
void DataDesc::Edit(UIContainer* ctx, IDataSource* ds)
{
	ctx->Push<ui::Panel>();

	ui::Property::Begin(ctx);
	if (ui::imm::Button(ctx, "Expand all instances"))
	{
		ExpandAllInstances(ds);
	}
	if (ui::imm::Button(ctx, "Delete auto-created"))
	{
		instances.erase(std::remove_if(instances.begin(), instances.end(), [](const StructInst& SI) { return !SI.userCreated; }), instances.end());
	}
	ui::Property::End(ctx);

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
		EditInstance(ctx, ds);
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

void DataDesc::EditInstance(UIContainer* ctx, IDataSource* ds)
{
	if (curInst < instances.size())
	{
		auto& SI = instances[curInst];

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
			auto size = ReadStruct(ds, SI, SI.off, data.rfs);
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
				snprintf(bfr, 256, "%s: %s[%s%" PRId64 "]=%s",
					F.name.c_str(),
					F.type.c_str(),
					F.countIsMaxSize ? "up to " : "",
					data.rfs[i].count,
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

size_t DataDesc::AddInst(const StructInst& src)
{
	printf("trying to create %s @ %" PRId64 "\n", src.def->name.c_str(), src.off);
	for (size_t i = 0; i < instances.size(); i++)
	{
		auto& I = instances[i];
		if (src.userCreated)
			I.userCreated = true;
		if (I.off == src.off && I.def == src.def)
			return i;
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
	StructInst newSI = { structs.find(F.type)->second, rfs[fieldID].off, "", false };
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

void DataDesc::ExpandAllInstances(IDataSource* ds)
{
	std::vector<ReadField> rfs;
	for (size_t i = 0; i < instances.size(); i++)
	{
		auto SI = instances[i];
		auto& S = *SI.def;
		rfs.clear();
		auto size = ReadStruct(ds, SI, SI.off, rfs);
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


enum COLS_DDI
{
	DDI_COL_ID,
	DDI_COL_User,
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
	return filterStruct && dataSource ? DDI_COL_HEADER_SIZE + filterStruct->fields.size() : DDI_COL_HEADER_SIZE;
}

std::string DataDescInstanceSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string DataDescInstanceSource::GetColName(size_t col)
{
	switch (col)
	{
	case DDI_COL_ID: return "ID";
	case DDI_COL_User: return "User";
	case DDI_COL_Offset: return "Offset";
	case DDI_COL_Struct: return "Struct";
	default: return filterStruct->fields[col - DDI_COL_FirstField].name;
	}
}

std::string DataDescInstanceSource::GetText(size_t row, size_t col)
{
	switch (col)
	{
	case DDI_COL_ID: return std::to_string(_indices[row]);
	case DDI_COL_User: return dataDesc->instances[_indices[row]].userCreated ? "+" : "";
	case DDI_COL_Offset: return std::to_string(dataDesc->instances[_indices[row]].off);
	case DDI_COL_Struct: return dataDesc->instances[_indices[row]].def->name;
	default:
		// TODO optimize to avoid reparsing for each col
	{
		auto& inst = dataDesc->instances[_indices[row]];
		std::vector<DataDesc::ReadField> rfs;
		dataDesc->ReadStruct(dataSource, inst, inst.off, rfs);
		auto& rf = rfs[col - DDI_COL_FirstField];
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
		if (filterUserCreated && !I.userCreated)
			continue;
		_indices.push_back(i);
	}

	refilter = false;
}
