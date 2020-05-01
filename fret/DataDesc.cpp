
#include "pch.h"
#include "DataDesc.h"
#include "FileReaders.h"


ui::DataCategoryTag DCT_MarkedItems[1];
ui::DataCategoryTag DCT_Struct[1];

Color4f colorFloat32{ 0.9f, 0.1f, 0.0f, 0.3f };
Color4f colorInt16{ 0.3f, 0.1f, 0.9f, 0.3f };
Color4f colorInt32{ 0.1f, 0.3f, 0.9f, 0.3f };
Color4f colorASCII{ 0.1f, 0.9f, 0.0f, 0.3f };


static uint8_t typeSizes[] = { 2, 2, 4, 4, 4 };

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
	case DT_I16: return colorInt16;
	case DT_U16: return colorInt16;
	case DT_I32: return colorInt32;
	case DT_U32: return colorInt32;
	case DT_F32: return colorFloat32;
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


void MarkedItemsList::Render(UIContainer* ctx)
{
	Subscribe(DCT_MarkedItems, markerData);
	ctx->Text("Edit marked items");
	for (auto& m : markerData->markers)
	{
		ctx->Push<ui::Panel>();
		if (ui::imm::PropButton(ctx, "Type", "EDIT"))
		{
			std::vector<ui::MenuItem> items;
			items.push_back(ui::MenuItem("int32", {}, false, true));
			ui::Menu m(items);
			m.Show(this);
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
	void(*append_to_str)(const void* src, std::string& out);
};

static std::unordered_map<std::string, BuiltinTypeInfo> g_builtinTypes
{
	{ "pad", { 1, false, [](const void* src, std::string& out) {} } },
	{ "char", { 1, false, [](const void* src, std::string& out) { out.push_back(*(const char*)src); } } },
	{ "i8", { 1, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId8, *(const int8_t*)src); out += bfr; } } },
	{ "u8", { 1, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu8, *(const uint8_t*)src); out += bfr; } } },
	{ "i16", { 2, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId16, *(const int16_t*)src); out += bfr; } } },
	{ "u16", { 2, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu16, *(const uint16_t*)src); out += bfr; } } },
	{ "i32", { 4, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId32, *(const int32_t*)src); out += bfr; } } },
	{ "u32", { 4, true, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu32, *(const uint32_t*)src); out += bfr; } } },
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

void DataDesc::ReadBuiltinFieldPreview(IDataSource* ds, uint64_t off, uint64_t count, const BuiltinTypeInfo& BTI, std::string& outPreview)
{
	char bfr[8];
	for (int i = 0; i < std::min(count, 16ULL); i++)
	{
		if (BTI.separated && i > 0)
			outPreview += ", ";
		ds->Read(off, BTI.size, bfr);
		BTI.append_to_str(bfr, outPreview);
		off += BTI.size;
	}
	if (count > 16)
	{
		outPreview += "...";
	}
}

void DataDesc::ReadStruct(IDataSource* ds, const Struct& S, uint64_t off, std::vector<ReadField>& out)
{
	if (S.serialized)
	{
		for (const auto& F : S.fields)
		{
			ReadField rf = { "", off };
			auto it = g_builtinTypes.find(F.type);
			if (it != g_builtinTypes.end())
			{
				ReadBuiltinFieldPreview(ds, off, F.count, it->second, rf.preview);
				off += it->second.size * F.count;
			}
			out.push_back(rf);
		}
	}
	else
	{
		for (const auto& F : S.fields)
		{
			ReadField rf = { "", off };
			auto it = g_builtinTypes.find(F.type);
			if (it != g_builtinTypes.end())
			{
				ReadBuiltinFieldPreview(ds, off + F.off, F.count, it->second, rf.preview);
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

void DataDesc::Edit(UIContainer* ctx, const char* path)
{
	auto* all = ctx->Push<ui::Panel>();

	ui::imm::PropEditInt(ctx, "Current instance ID", curInst, {}, 1, 0, instances.empty() ? 0 : instances.size() - 1);
	ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
	ctx->Text("Edit:") + ui::Padding(5);
	BRB(ctx, "instance", editMode, 0);
	BRB(ctx, "struct", editMode, 1);
	BRB(ctx, "field", editMode, 2);
	ctx->Pop();

	if (editMode == 0)
	{
		if (curInst < instances.size())
		{
			auto& I = instances[curInst];

			ui::imm::PropEditString(ctx, "Notes", I.notes.c_str(), [&I](const char* s) { I.notes = s; });
			ui::imm::PropEditInt(ctx, "Offset", I.off);
			if (ui::imm::PropButton(ctx, "Edit struct:", I.type.c_str()))
			{
				editMode = 1;
				all->RerenderNode();
			}

			auto it = structs.find(I.type);
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
				FileDataSource fds;
				Data data;
				data.S = &S;
				fds.fp = fopen(("FRET_Plugins/" + std::string(path)).c_str(), "rb");
				ReadStruct(&fds, S, I.off, data.rfs);
				fclose(fds.fp);
				for (size_t i = 0; i < S.fields.size(); i++)
				{
					auto& F = S.fields[i];
					char bfr[256];
					snprintf(bfr, 256, "%s: %s[%" PRIu64 "]=%s", F.name.c_str(), F.type.c_str(), F.count, data.rfs[i].preview.c_str());
					*ctx->MakeWithText<ui::BoxElement>(bfr) + ui::Padding(5);
				}
				/*auto* tv = ctx->Make<ui::TableView>();
				tv->SetDataSource(&data); TODO */
			}
		}
	}
	if (editMode == 1)
	{
		if (curInst < instances.size())
		{
			auto& I = instances[curInst];

			ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
			ctx->Text("Struct:") + ui::Padding(5);
			ctx->Text(I.type.c_str()) + ui::Padding(5);
			ctx->Pop();

			auto it = structs.find(I.type);
			if (it != structs.end())
			{
				auto& S = *it->second;
				ui::imm::PropEditBool(ctx, "Is serialized?", S.serialized);
				ctx->Push<ui::Panel>();
				for (size_t i = 0; i < S.fields.size(); i++)
				{
					auto& F = S.fields[i];
					ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
					//ctx->Text(F.name.c_str());
					//ctx->Text(F.type.c_str());
					//ui::imm::EditInt(ctx, "\bCount", F.count);
					char info[128];
					snprintf(info, 128, "%s: %s[%" PRIu64 "]", F.name.c_str(), F.type.c_str(), F.count);
					*ctx->MakeWithText<ui::BoxElement>(info) + ui::Padding(5);
					//ctx->MakeWithText<ui::Button>("Edit");
					if (i > 0 && ui::imm::Button(ctx, "<", { &ui::Width(20) }))
					{
						std::swap(S.fields[i - 1], S.fields[i]);
						all->RerenderNode();
					}
					if (i + 1 < S.fields.size() && ui::imm::Button(ctx, ">", { &ui::Width(20) }))
					{
						std::swap(S.fields[i + 1], S.fields[i]);
						all->RerenderNode();
					}
					if (ui::imm::Button(ctx, "Edit", { &ui::Width(50) }))
					{
						editMode = 2;
						curField = i;
						all->RerenderNode();
					}
					if (ui::imm::Button(ctx, "X", { &ui::Width(20) }))
					{
						S.fields.erase(S.fields.begin() + i);
						all->RerenderNode();
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					S.fields.push_back({ "i32", "unnamed", 1 });
					editMode = 2;
					curField = S.fields.size() - 1;
					all->RerenderNode();
				}
				ctx->Pop();
			}
			else
			{
				ctx->Text("-- NOT FOUND --") + ui::Padding(10);
			}
		}
	}
	if (editMode == 2)
	{
		if (curInst < instances.size())
		{
			auto& I = instances[curInst];

			auto it = structs.find(I.type);
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
					ui::imm::PropEditInt(ctx, "Count", F.count, {}, 1, 1);
				}
			}
		}
	}

	ctx->Pop();
}
