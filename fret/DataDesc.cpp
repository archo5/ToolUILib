
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

enum COLS
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
	{ "char", { 1, false, [](const void* src) -> int64_t { return 0; }, [](const void* src, std::string& out) { out.push_back(*(const char*)src); } } },
	{ "i8", { 1, true, [](const void* src) -> int64_t { return *(const int8_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId8, *(const int8_t*)src); out += bfr; } } },
	{ "u8", { 1, true, [](const void* src) -> int64_t { return *(const uint8_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu8, *(const uint8_t*)src); out += bfr; } } },
	{ "i16", { 2, true, [](const void* src) -> int64_t { return *(const int16_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId16, *(const int16_t*)src); out += bfr; } } },
	{ "u16", { 2, true, [](const void* src) -> int64_t { return *(const uint16_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu16, *(const uint16_t*)src); out += bfr; } } },
	{ "i32", { 4, true, [](const void* src) -> int64_t { return *(const int32_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId32, *(const int32_t*)src); out += bfr; } } },
	{ "u32", { 4, true, [](const void* src) -> int64_t { return *(const uint32_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu32, *(const uint32_t*)src); out += bfr; } } },
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

void DataDesc::ReadBuiltinFieldPrimary(IDataSource* ds, const BuiltinTypeInfo& BTI, ReadField& rf)
{
	char bfr[8];
	uint64_t off = rf.off;
	for (int64_t i = 0; i < std::min(rf.count, 24LL); i++)
	{
		if (BTI.separated && i > 0)
			rf.preview += ", ";
		ds->Read(off, BTI.size, bfr);
		BTI.append_to_str(bfr, rf.preview);
		off += BTI.size;
	}
	if (rf.count > 24)
	{
		rf.preview += "...";
	}
}

int64_t DataDesc::GetFieldCount(IDataSource* ds, const StructInst& SI, const Field& F, const std::vector<ReadField>& rfs)
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
			return ia.intVal;
	}

	// search parameter default values
	for (auto& P : SI.def->params)
	{
		if (F.countSrc == P.name)
			return P.intVal;
	}

	return 0;
}

void DataDesc::ReadStruct(IDataSource* ds, const StructInst& SI, uint64_t off, std::vector<ReadField>& out)
{
	if (SI.def->serialized)
	{
		for (const auto& F : SI.def->fields)
		{
			ReadField rf = { "", off, GetFieldCount(ds, SI, F, out), 0 };
			auto it = g_builtinTypes.find(F.type);
			if (it != g_builtinTypes.end())
			{
				ReadBuiltinFieldPrimary(ds, it->second, rf);
				off += it->second.size * F.count;
			}
			out.push_back(rf);
		}
	}
	else
	{
		for (const auto& F : SI.def->fields)
		{
			ReadField rf = { "", off + F.off, GetFieldCount(ds, SI, F, out), 0 };
			auto it = g_builtinTypes.find(F.type);
			if (it != g_builtinTypes.end())
			{
				ReadBuiltinFieldPrimary(ds, it->second, rf);
			}
			out.push_back(rf);
		}
	}
}


static void BRB(UIContainer* ctx, const char* text, int& at, int val)
{
	auto* rb = ctx->MakeWithText<ui::RadioButtonT<int>>(text);
	rb->Init(at, val);
	rb->onChange = [rb]() { rb->RerenderNode(); };
	rb->SetStyle(ui::Theme::current->button);
}

void DataDesc::Edit(UIContainer* ctx, IDataSource* ds)
{
	ctx->Push<ui::Panel>();

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
			ctx->MakeWithText<ui::BoxElement>("Data");
			Data data;
			data.S = &S;
			ReadStruct(ds, SI, SI.off, data.rfs);
			for (size_t i = 0; i < S.fields.size(); i++)
			{
				auto& F = S.fields[i];
				char bfr[256];
				snprintf(bfr, 256, "%s: %s[%" PRId64 "]=%s", F.name.c_str(), F.type.c_str(), data.rfs[i].count, data.rfs[i].preview.c_str());
				ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
				ctx->Text(bfr) + ui::Padding(5);
				if (g_builtinTypes.find(F.type) == g_builtinTypes.end())
				{
					if (ui::imm::Button(ctx, "View"))
					{
						curInst = AddInst(F.type, data.rfs[i].off, false);
					}
				}
				ctx->Pop();
			}
			/*auto* tv = ctx->Make<ui::TableView>();
			tv->SetDataSource(&data); TODO */
		}
	}
}

void DataDesc::EditStruct(UIContainer* ctx)
{
	if (curInst < instances.size())
	{
		auto& SI = instances[curInst];

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("Struct:") + ui::Padding(5);
		ctx->Text(SI.def->name) + ui::Padding(5);
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
					S.fields.erase(S.fields.begin() + i);
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
				snprintf(info, 128, "%s: %s[%s%s%s%" PRId64 "]",
					F.name.c_str(),
					F.type.c_str(),
					F.countIsMaxSize ? "up to " : "",
					F.countSrc.c_str(),
					F.count >= 0 && F.countSrc.size() ? "+" : "",
					F.count);
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
				S.fields.push_back({ "i32", "unnamed", 1 });
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
			}
		}
	}
}

size_t DataDesc::AddInst(const std::string& name, uint64_t off, bool userCreated)
{
	for (size_t i = 0; i < instances.size(); i++)
	{
		auto& I = instances[i];
		if (userCreated)
			I.userCreated = true;
		if (I.off == off && I.def->name == name)
			return i;
	}
	instances.push_back({ structs.find(name)->second, off, "", userCreated });
	return instances.size() - 1;
}
