
#include "pch.h"
#include "DataDesc.h"
#include "FileReaders.h"
#include "ImageParsers.h"


ui::DataCategoryTag DCT_Marker[1];
ui::DataCategoryTag DCT_MarkedItems[1];
ui::DataCategoryTag DCT_Struct[1];
ui::DataCategoryTag DCT_CurStructInst[1];

Color4f colorChar{ 0.3f, 0.9f, 0.1f, 0.3f };
Color4f colorFloat32{ 0.9f, 0.1f, 0.0f, 0.3f };
Color4f colorFloat64{ 0.9f, 0.2f, 0.0f, 0.3f };
Color4f colorInt8{ 0.3f, 0.1f, 0.9f, 0.3f };
Color4f colorInt16{ 0.2f, 0.2f, 0.9f, 0.3f };
Color4f colorInt32{ 0.1f, 0.3f, 0.9f, 0.3f };
Color4f colorInt64{ 0.0f, 0.4f, 0.9f, 0.3f };
Color4f colorNearFileSize32{ 0.1f, 0.3f, 0.7f, 0.3f };
Color4f colorNearFileSize64{ 0.0f, 0.4f, 0.7f, 0.3f };
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

template <class T> T modulus(T a, T b) { return a % b; }
float modulus(float a, float b) { return fmodf(a, b); }
double modulus(double a, double b) { return fmod(a, b); }
template <class T> T greatest_common_divisor(T a, T b)
{
	if (b == 0)
		return a;
	return greatest_common_divisor(b, modulus(a, b));
}

typedef std::string AnalysisFunc(IDataSource* ds, uint64_t off, uint64_t stride, uint64_t count);
template <class T> std::string AnalysisFuncImpl(IDataSource* ds, uint64_t off, uint64_t stride, uint64_t count)
{
	T min = std::numeric_limits<T>::max();
	T max = std::numeric_limits<T>::min();
	T gcd = 0;
	T prev = 0;
	bool eq = count > 1;
	bool asc = count > 1;
	bool asceq = count > 1;
	for (uint64_t i = 0; i < count; i++)
	{
		T val;
		ds->Read(off + stride * i, sizeof(val), &val);
		min = std::min(min, val);
		max = std::max(max, val);
		gcd = greatest_common_divisor(gcd, val);
		if (i > 0)
		{
			if (val != prev)
				eq = false;
			if (val <= prev)
				asc = false;
			if (val < prev)
				asceq = false;
		}
		prev = val;
	}
	auto ret = "min=" + std::to_string(min) + " max=" + std::to_string(max) + " gcd=" + std::to_string(gcd);
	if (eq)
		ret += " eq";
	else if (asc)
		ret += " asc";
	else if (asceq)
		ret += " asceq";
	return ret;
}
static AnalysisFunc* analysisFuncs[] =
{
	AnalysisFuncImpl<char>,
	AnalysisFuncImpl<int8_t>,
	AnalysisFuncImpl<uint8_t>,
	AnalysisFuncImpl<int16_t>,
	AnalysisFuncImpl<uint16_t>,
	AnalysisFuncImpl<int32_t>,
	AnalysisFuncImpl<uint32_t>,
	AnalysisFuncImpl<int64_t>,
	AnalysisFuncImpl<uint64_t>,
	AnalysisFuncImpl<float>,
	AnalysisFuncImpl<double>,
};

static const char* markerReadCodes[] =
{
	"%c",
	"%" PRId8,
	"%" PRIu8,
	"%" PRId16,
	"%" PRIu16,
	"%" PRId32,
	"%" PRIu32,
	"%" PRId64,
	"%" PRIu64,
	"%g",
	"%g",
};
typedef void MarkerReadFunc(std::string& sb, IDataSource* ds, uint64_t off);
template <class T, DataType ty> void MarkerReadFuncImpl(std::string& sb, IDataSource* ds, uint64_t off)
{
	T val;
	ds->Read(off, sizeof(T), &val);
	char bfr[128];
	snprintf(bfr, 128, markerReadCodes[ty], val);
	sb += bfr;
}

static MarkerReadFunc* markerReadFuncs[] =
{
	MarkerReadFuncImpl<char, DT_CHAR>,
	MarkerReadFuncImpl<int8_t, DT_I8>,
	MarkerReadFuncImpl<uint8_t, DT_U8>,
	MarkerReadFuncImpl<int16_t, DT_I16>,
	MarkerReadFuncImpl<uint16_t, DT_U16>,
	MarkerReadFuncImpl<int32_t, DT_I32>,
	MarkerReadFuncImpl<uint32_t, DT_U32>,
	MarkerReadFuncImpl<int64_t, DT_I64>,
	MarkerReadFuncImpl<uint64_t, DT_U64>,
	MarkerReadFuncImpl<float, DT_F32>,
	MarkerReadFuncImpl<double, DT_F64>,
};
std::string GetMarkerPreview(const Marker& marker, IDataSource* src, size_t maxLen)
{
	std::string text;
	for (uint64_t i = 0; i < marker.repeats; i++)
	{
		if (i) text += '/';
		//if (marker.repeats > 1) text += '[';
		for (uint64_t j = 0; j < marker.count; j++)
		{
			if (j > 0 && marker.type != DT_CHAR)
				text += ';';
			markerReadFuncs[marker.type](text, src, marker.at + i * marker.stride + j * typeSizes[marker.type]);
			if (text.size() > maxLen)
			{
				text.erase(text.begin() + 32, text.end());
				text += "...";
				return text;
			}
		}
		//if (marker.repeats > 1) text += ']';
	}
	return text;
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
		M.notes = r.ReadString("notes");
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
		w.WriteString("notes", M.notes);

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
	MD_COL_Notes,
	MD_COL_Preview,

	MD_COL__COUNT,
};

size_t MarkerDataSource::GetNumCols()
{
	return MD_COL__COUNT;
}

std::string MarkerDataSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string MarkerDataSource::GetColName(size_t col)
{
	switch (col)
	{
	case MD_COL_At: return "Offset";
	case MD_COL_Type: return "Type";
	case MD_COL_Count: return "Count";
	case MD_COL_Repeats: return "Repeats";
	case MD_COL_Stride: return "Stride";
	case MD_COL_Notes: return "Notes";
	case MD_COL_Preview: return "Preview";
	default: return "";
	}
}

std::string MarkerDataSource::GetText(size_t row, size_t col)
{
	const auto& markers = data->markers;
	switch (col)
	{
	case MD_COL_At: return std::to_string(markers[row].at);
	case MD_COL_Type: return typeNames[markers[row].type];
	case MD_COL_Count: return std::to_string(markers[row].count);
	case MD_COL_Repeats: return std::to_string(markers[row].repeats);
	case MD_COL_Stride: return std::to_string(markers[row].stride);
	case MD_COL_Notes: return markers[row].notes;
	case MD_COL_Preview: return GetMarkerPreview(markers[row], dataSource, 32);
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
	ui::imm::PropEditString(ctx, "Notes", marker->notes.c_str(), [this](const char* v) { marker->notes = v; });
	ctx->Pop();

	ctx->Push<ui::Panel>();
	if (ui::imm::Button(ctx, "Analyze"))
	{
		analysis.clear();
		if (marker->repeats <= 1)
		{
			// analyze a single array
			analysis.push_back("array: " + analysisFuncs[marker->type](dataSource, marker->at, typeSizes[marker->type], marker->count));
		}
		else
		{
			// analyze each array element separately
			for (uint64_t i = 0; i < marker->count; i++)
			{
				analysis.push_back(std::to_string(i) + ": " + analysisFuncs[marker->type](dataSource, marker->at + i * typeSizes[marker->type], marker->stride, marker->repeats));
			}
		}
	}
	if (marker->type == DT_F32 && marker->count == 3 && marker->repeats > 1 && ui::imm::Button(ctx, "Export points to .obj"))
	{
		FILE* fp = fopen("positions.obj", "w");
		for (uint64_t i = 0; i < marker->repeats; i++)
		{
			float vec[3];
			dataSource->Read(marker->at + marker->stride * i, sizeof(vec), vec);
			fprintf(fp, "v %g %g %g\n", vec[0], vec[1], vec[2]);
		}
		fclose(fp);
	}
	for (const auto& S : analysis)
		ctx->Text(S.c_str());
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

bool EditImageFormat(UIContainer* ctx, const char* label, std::string& format)
{
	if (ui::imm::PropButton(ctx, label, format.c_str()))
	{
		std::vector<ui::MenuItem> imgFormats;
		StringView prevCat;
		for (size_t i = 0, count = GetImageFormatCount(); i < count; i++)
		{
			StringView cat = GetImageFormatCategory(i);
			if (cat != prevCat)
			{
				prevCat = cat;
				imgFormats.push_back(ui::MenuItem(cat, {}, true));
			}
			StringView name = GetImageFormatName(i);
			imgFormats.push_back(ui::MenuItem(name).Func([&format, name]() { format.assign(name.data(), name.size()); }));
		}
		ui::Menu(imgFormats).Show(ctx->GetCurrentNode());
		return true;
	}
	return false;
}

void DDStructResource::Load(NamedTextSerializeReader& r)
{
	auto rsrcType = r.ReadString("type");
	if (rsrcType == "image")
		type = DDStructResourceType::Image;

	if (r.BeginDict("image"))
	{
		image = new DDRsrcImage;
		image->format = r.ReadString("format");

		r.BeginArray("formatOverrides");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			DDRsrcImage::FormatOverride FO;
			FO.format = r.ReadString("format");

			r.BeginArray("conditions");
			for (auto E3 : r.GetCurrentRange())
			{
				r.BeginEntry(E3);
				r.BeginDict("");

				DDConditionExt CE;
				CE.field = r.ReadString("field");
				CE.value = r.ReadString("value");
				CE.useExpr = r.ReadBool("useExpr");
				CE.expr.SetExpr(r.ReadString("expr"));
				FO.conditions.push_back(CE);

				r.EndDict();
				r.EndEntry();
			}
			r.EndArray();

			image->formatOverrides.push_back(FO);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();

		image->imgOff.SetExpr(r.ReadString("imgOff"));
		image->palOff.SetExpr(r.ReadString("palOff"));
		image->width.SetExpr(r.ReadString("width"));
		image->height.SetExpr(r.ReadString("height"));
	}
	r.EndDict();
}

void DDStructResource::Save(NamedTextSerializeWriter& w)
{
	switch (type)
	{
	case DDStructResourceType::Image:
		w.WriteString("type", "image");
		break;
	}

	if (image)
	{
		w.BeginDict("image");

		w.WriteString("format", image->format);

		w.BeginArray("formatOverrides");
		for (auto& FO : image->formatOverrides)
		{
			w.BeginDict("");
			w.WriteString("format", FO.format);

			w.BeginArray("conditions");
			for (auto& CE : FO.conditions)
			{
				w.BeginDict("");
				w.WriteString("field", CE.field);
				w.WriteString("value", CE.value);
				w.WriteBool("useExpr", CE.useExpr);
				w.WriteString("expr", CE.expr.expr);
				w.EndDict();
			}
			w.EndArray();

			w.EndDict();
		}
		w.EndArray();

		w.WriteString("imgOff", image->imgOff.expr);
		w.WriteString("palOff", image->palOff.expr);
		w.WriteString("width", image->width.expr);
		w.WriteString("height", image->height.expr);

		w.EndDict();
	}
}

size_t DDStruct::FindFieldByName(StringView name)
{
	for (size_t i = 0; i < fields.size(); i++)
		if (StringView(fields[i].name) == name)
			return i;
	return SIZE_MAX;
}

void DDStruct::Load(NamedTextSerializeReader& r)
{
	serialized = r.ReadBool("serialized");

	r.BeginArray("params");
	for (auto E2 : r.GetCurrentRange())
	{
		r.BeginEntry(E2);
		r.BeginDict("");

		DDParam P;
		P.name = r.ReadString("name");
		P.intVal = r.ReadInt64("intVal");
		params.push_back(P);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("fields");
	for (auto E2 : r.GetCurrentRange())
	{
		r.BeginEntry(E2);
		r.BeginDict("");

		DDField F;
		F.type = r.ReadString("type");
		F.name = r.ReadString("name");
		F.off = r.ReadInt64("off");
		F.offExpr.SetExpr(r.ReadString("offExpr"));
		F.count = r.ReadInt64("count");
		F.countSrc = r.ReadString("countSrc");
		F.countIsMaxSize = r.ReadBool("countIsMaxSize");
		F.readUntil0 = r.ReadBool("readUntil0");

		r.BeginArray("structArgs");
		for (auto E3 : r.GetCurrentRange())
		{
			r.BeginEntry(E3);
			r.BeginDict("");

			DDCompArg A;
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

			DDCondition C;
			C.field = r.ReadString("field");
			C.value = r.ReadString("value");
			F.conditions.push_back(C);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();
		fields.push_back(F);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginDict("resource");
	resource.Load(r);
	r.EndDict();

	size = r.ReadInt64("size");
	sizeSrc = r.ReadString("sizeSrc");
}

void DDStruct::Save(NamedTextSerializeWriter& w)
{
	w.WriteString("name", name);
	w.WriteBool("serialized", serialized);

	w.BeginArray("params");
	for (DDParam& P : params)
	{
		w.BeginDict("");
		w.WriteString("name", P.name);
		w.WriteInt("intVal", P.intVal);
		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("fields");
	for (const DDField& F : fields)
	{
		w.BeginDict("");
		w.WriteString("type", F.type);
		w.WriteString("name", F.name);
		w.WriteInt("off", F.off);
		w.WriteString("offExpr", F.offExpr.expr);
		w.WriteInt("count", F.count);
		w.WriteString("countSrc", F.countSrc);
		w.WriteBool("countIsMaxSize", F.countIsMaxSize);
		w.WriteBool("readUntil0", F.readUntil0);

		w.BeginArray("structArgs");
		for (const DDCompArg& A : F.structArgs)
		{
			w.BeginDict("");
			w.WriteString("name", A.name);
			w.WriteString("src", A.src);
			w.WriteInt("intVal", A.intVal);
			w.EndDict();
		}
		w.EndArray();

		w.BeginArray("conditions");
		for (const DDCondition& C : F.conditions)
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

	if (resource.type != DDStructResourceType::None)
	{
		w.BeginDict("resource");
		resource.Save(w);
		w.EndDict();
	}

	w.WriteInt("size", size);
	w.WriteString("sizeSrc", sizeSrc);
}


std::string DDStructInst::GetFieldDescLazy(size_t i, bool* incomplete) const
{
	_EnumerateFields(i + 1, true);

	if (i >= cachedFields.size() && incomplete)
		*incomplete = true;
	
	auto& F = def->fields[i];

	std::string ret = F.name;
	ret += ": ";
	ret += F.type;

	if (F.countIsMaxSize || F.countSrc.size() || F.count != 1)
	{
		ret += "[";
		if (F.countIsMaxSize)
			ret += "up to ";
		int64_t elcount = GetFieldElementCount(i, true);
		if (elcount != F_NO_VALUE)
			ret += std::to_string(elcount);
		else
		{
			ret += "?";
			if (incomplete)
				*incomplete = true;
		}
		ret += "]";
	}

	ret += "@";
	int64_t off = GetFieldOffset(i, true);
	if (off != F_NO_VALUE)
		ret += std::to_string(off);
	else
	{
		ret += "?";
		if (incomplete)
			*incomplete = true;
	}

	ret += "=";
	ret += GetFieldPreview(i, true);

	return ret;
}

int64_t DDStructInst::GetSize() const
{
	if (cachedSize == -1 ||
		cacheSizeVersionSI != editVersionSI ||
		cacheSizeVersionS != def->editVersionS)
	{
		cachedSize = _CalcSize();
		cacheSizeVersionSI = editVersionSI;
		cacheSizeVersionS = def->editVersionS;
	}
	return cachedSize;
}

bool DDStructInst::IsFieldPresent(size_t i) const
{
	_EnumerateFields(i + 1);
	return cachedFields[i].present;
}

OptionalBool DDStructInst::IsFieldPresent(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (i < cachedFields.size())
		return cachedFields[i].present ? OptionalBool::True : OptionalBool::False;
	return OptionalBool::Unknown;
}

std::string g_emptyString;
std::string g_notLoadedStr = "<?>";
std::string g_notPresentStr = "<not present>";
#define MAX_PREVIEW_LENGTH 24
const std::string& DDStructInst::GetFieldPreview(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (lazy && i >= cachedFields.size())
		return g_notLoadedStr;
	auto& CF = cachedFields[i];
	if (!CF.present)
		return g_notPresentStr;
	if (CF.preview.empty())
	{
		// complex offset
		if (GetFieldOffset(i, lazy) == F_NO_VALUE)
			return g_notLoadedStr;

		bool separated = true;
		{
			auto it = g_builtinTypes.find(def->fields[i].type);
			if (it != g_builtinTypes.end())
				separated = it->second.separated;
		}
		for (size_t n = 0; CF.preview.size() < MAX_PREVIEW_LENGTH; n++)
		{
			const auto& fvp = GetFieldValuePreview(i, n);
			if (fvp.empty())
				break;
			if (n && separated)
				CF.preview += ", ";
			CF.preview += fvp;
		}
		if (CF.preview.size() > MAX_PREVIEW_LENGTH || CF.preview.empty())
		{
			CF.preview += "...";
		}
	}
	return CF.preview;
}

const std::string& DDStructInst::GetFieldValuePreview(size_t i, size_t n) const
{
	if (_ReadFieldValues(i, n + 1))
	{
		return cachedFields[i].values[n].preview;
	}
	return g_emptyString;
}

int64_t DDStructInst::GetFieldIntValue(size_t i, size_t n) const
{
	if (_ReadFieldValues(i, n + 1))
	{
		return cachedFields[i].values[n].intVal;
	}
	return 0;
}

int64_t DDStructInst::GetFieldOffset(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (lazy && i >= cachedFields.size())
		return F_NO_VALUE;
	auto& CF = cachedFields[i];
	if (CF.off == F_NO_VALUE && !lazy)
	{
		auto& F = def->fields[i];
		if (F.IsComputed())
		{
			if (F.offExpr.inst)
			{
				VariableSource vs;
				{
					vs.desc = desc;
					vs.root = this;
				}
				CF.off = F.offExpr.inst->Evaluate(&vs);
			}
		}
		else
		{
			CF.off = 0;
			puts("invalid state, expected enumeration to provide offset");
		}
	}
	return CF.off;
}

int64_t DDStructInst::GetFieldElementCount(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (lazy && i >= cachedFields.size())
		return F_NO_VALUE;
	auto& CF = cachedFields[i];
	if (CF.count == F_NO_VALUE)
		CF.count = CF.present ? _CalcFieldElementCount(i) : 0;
	return CF.count;
}

int64_t DDStructInst::GetFieldTotalSize(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1);
	if (lazy && i >= cachedFields.size())
		return F_NO_VALUE;
	auto& CF = cachedFields[i];
	if (CF.totalSize == F_NO_VALUE)
	{
		auto& F = def->fields[i];
		auto fs = desc->GetFixedTypeSize(F.type);
		if (fs != UINT64_MAX)
		{
			int64_t numElements = GetFieldElementCount(i);
			if (F.readUntil0)
			{
				_ReadFieldValues(i, numElements);
				if (numElements > CF.values.size())
					numElements = CF.values.size();
			}
			if (F.countIsMaxSize)
				numElements /= fs;
			CF.totalSize = fs * numElements;
		}
		else if (!lazy) // serialized struct, need to create instances
		{
			int64_t numElements = GetFieldElementCount(i);
			int64_t totalSize = 0;
			if (0 < numElements)
			{
				DDStructInst* II = CreateFieldInstance(i, CreationReason::Query);
				int64_t size = II->GetSize();
				totalSize += size;
				int64_t remSizeSub = II->remainingCountIsSize ? (II->sizeOverrideEnable ? II->sizeOverrideValue : size) : 1;
				while (II->remainingCount - remSizeSub > 0)
				{
					II = desc->CreateNextInstance(*II, size, CreationReason::Query);
					size = II->GetSize();
					totalSize += size;
				}
			}
			CF.totalSize = totalSize;
		}
	}
	return CF.totalSize;
}

bool DDStructInst::EvaluateCondition(const DDCondition& cond, size_t until) const
{
	_EnumerateFields(until);

	if (cond.field.empty())
		return true;

	size_t fid = def->FindFieldByName(cond.field);
	if (fid != SIZE_MAX && GetFieldPreview(fid) == cond.value)
		return true;

	return false;
}

bool DDStructInst::EvaluateConditions(const std::vector<DDCondition>& conds, size_t until) const
{
	if (conds.empty())
		return true;
	for (auto& cond : conds)
		if (EvaluateCondition(cond, until))
			return true;
	return false;
}

bool DDStructInst::EvaluateCondition(const DDConditionExt& cond, size_t until) const
{
	_EnumerateFields(until);

	if (cond.useExpr)
	{
		VariableSource vs;
		{
			vs.desc = desc;
			vs.root = this;
		}
		return cond.expr.Evaluate(vs);
	}
	else
	{
		if (cond.field.empty())
			return true;

		size_t fid = def->FindFieldByName(cond.field);
		if (fid != SIZE_MAX && GetFieldPreview(fid) == cond.value)
			return true;

		return false;
	}
}

bool DDStructInst::EvaluateConditions(const std::vector<DDConditionExt>& conds, size_t until) const
{
	if (conds.empty())
		return true;
	for (auto& cond : conds)
		if (EvaluateCondition(cond, until))
			return true;
	return false;
}

int64_t DDStructInst::GetCompArgValue(const DDCompArg& arg) const
{
	if (arg.src.empty())
		return arg.intVal;

	// search fields
	// TODO: previous only?
	size_t at = def->FindFieldByName(arg.src);
	if (at != SIZE_MAX)
		return GetFieldIntValue(at) + arg.intVal;

	// search arguments
	for (auto& ia : args)
	{
		if (arg.src == ia.name)
			return ia.intVal + arg.intVal;
	}

	// search parameter default values
	for (auto& P : def->params)
	{
		if (arg.src == P.name)
			return P.intVal + arg.intVal;
	}

	return 0;
}

DDStructInst* DDStructInst::CreateFieldInstance(size_t i, CreationReason cr) const
{
	auto& F = def->fields[i];
	DDStructInst newSI = { -1, desc, desc->structs.find(F.type)->second, file, GetFieldOffset(i), "", cr };
	newSI.remainingCountIsSize = F.countIsMaxSize;
	newSI.remainingCount = GetFieldElementCount(i);
	size_t prevSize = desc->instances.size();
	DDStructInst* newInst = desc->AddInstance(newSI);
	if (prevSize != desc->instances.size())
	{
		for (auto& SA : F.structArgs)
		{
			newInst->args.push_back({ SA.name, GetCompArgValue(SA) });
		}
	}
	return newInst;
}

void DDStructInst::_CheckFieldCache() const
{
	if (cacheFieldsVersionSI != editVersionSI ||
		cacheFieldsVersionS != def->editVersionS)
	{
		cachedFields.clear();
		cachedReadOff = off;
		cacheFieldsVersionSI = editVersionSI;
		cacheFieldsVersionS = def->editVersionS;
	}
}

void DDStructInst::_EnumerateFields(size_t untilNum, bool lazy) const
{
	if (untilNum > def->fields.size())
		untilNum = def->fields.size();

	_CheckFieldCache();

	while (cachedFields.size() < untilNum)
	{
		size_t i = cachedFields.size();
		const auto& F = def->fields[i];

		cachedFields.emplace_back();
		DDReadField& rf = cachedFields.back();
		rf.present = EvaluateConditions(F.conditions, i);

		if (rf.present && !F.IsComputed())
		{
			rf.off = cachedReadOff;
			if (def->serialized)
			{
				int64_t fts = GetFieldTotalSize(i, lazy);
				if (lazy && fts == F_NO_VALUE)
					break; // too complicated to do
				cachedReadOff += fts;
			}
			else
			{
				rf.off += F.off;
			}
		}
	}
}

int64_t DDStructInst::_CalcSize() const
{
	if (def->sizeSrc.size())
	{
#if 0
		for (auto& ca : F.structArgs)
			if (ca.name == def->sizeSrc)
				return GetCompArgValue(SI, F, rfs, ca) + def->size;
#endif

		for (size_t i = 0; i < def->fields.size(); i++)
			if (def->fields[i].name == def->sizeSrc)
				return GetFieldIntValue(i) + def->size;

#if 1
		for (auto& arg : args)
			if (arg.name == def->sizeSrc)
				return arg.intVal + def->size;
#endif

		return def->size;
	}
	else if (def->serialized)
	{
		_EnumerateFields(def->fields.size());
		return cachedReadOff - off;
	}
	else
		return def->size;
}

int64_t DDStructInst::_CalcFieldElementCount(size_t i) const
{
	auto& F = def->fields[i];

	if (F.countSrc.empty())
		return F.count;

	if (F.countSrc == ":file-size")
		return file->dataSource->GetSize() + F.count;

	// search previous fields
	for (size_t at = 0; at < i; at++)
	{
		auto& field = def->fields[at];
		if (F.countSrc == field.name)
			return GetFieldIntValue(at) + F.count;
	}

	// search arguments
	for (auto& ia : args)
	{
		if (F.countSrc == ia.name)
			return ia.intVal + F.count;
	}

	// search parameter default values
	for (auto& P : def->params)
	{
		if (F.countSrc == P.name)
			return P.intVal + F.count;
	}

	return 0;
}

bool DDStructInst::_CanReadMoreFields(size_t i) const
{
	auto& F = def->fields[i];
	auto& CF = cachedFields[i];
	if (F.countIsMaxSize)
		return CF.count > CF.readOff - CF.off;
	else
		return CF.count > CF.values.size();
}

bool DDStructInst::_ReadFieldValues(size_t i, size_t n) const
{
	_EnumerateFields(i + 1);
	auto& CF = cachedFields[i];
	if (CF.values.size() >= n)
		return true;

	auto& F = def->fields[i];
	auto it = g_builtinTypes.find(F.type);
	if (it == g_builtinTypes.end())
		return false;

	if (CF.readOff == F_NO_VALUE)
		CF.readOff = GetFieldOffset(i);

	GetFieldElementCount(i);

	auto& BTI = it->second;
	while (CF.values.size() < n && _CanReadMoreFields(i))
	{
		if (F.readUntil0 && CF.values.size() && CF.values.back().intVal == 0)
			break;

		CF.values.emplace_back();
		auto& CFV = CF.values.back();

		CFV.off = CF.readOff;
		__declspec(align(8)) char bfr[8];
		file->dataSource->Read(CF.readOff, BTI.size, bfr);
		CFV.intVal = BTI.get_int64(bfr);
		BTI.append_to_str(bfr, CFV.preview);

		CF.readOff += BTI.size;
	}
	return CF.values.size() >= n;
}


uint64_t DataDesc::GetFixedTypeSize(const std::string& type)
{
	auto it = g_builtinTypes.find(type);
	if (it != g_builtinTypes.end())
		return it->second.size;
	auto sit = structs.find(type);
	if (sit != structs.end() && !sit->second->serialized)
		return sit->second->size;
	return UINT64_MAX;
}


static void BRB(UIContainer* ctx, const char* text, int& at, int val)
{
	ui::imm::RadioButton(ctx, at, val, text, { ui::Style(ui::Theme::current->button) });
}

static bool advancedAccess = false;
static MathExprObj testQuery;
void DataDesc::EditStructuralItems(UIContainer* ctx)
{
	ctx->Push<ui::Panel>();

	ui::imm::PropEditBool(ctx, "Advanced access", advancedAccess);
	if (advancedAccess)
	{
		ui::imm::PropEditString(ctx, "\bTQ:", testQuery.expr.c_str(), [](const char* v) { testQuery.SetExpr(v); });
		if (testQuery.inst)
		{
			char bfr[128];
			VariableSource vs;
			{
				vs.desc = this;
				vs.root = curInst;
			}
			snprintf(bfr, 128, "Value: %" PRId64, testQuery.inst->Evaluate(&vs));
			ctx->Text(bfr);
		}

		auto id = curInst ? curInst->id : -1LL;
		ui::imm::PropEditInt(ctx, "Current instance ID", id, {}, 1);
		if (!curInst || curInst->id != id)
			SetCurrentInstance(FindInstanceByID(id));
	}

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

static const char* CreationReasonToString(CreationReason cr)
{
	switch (cr)
	{
	case CreationReason::UserDefined: return "user-defined";
	case CreationReason::ManualExpand: return "manual expand";
	case CreationReason::AutoExpand: return "auto expand";
	case CreationReason::Query: return "query";
	default: return "unknown";
	}
}

static const char* CreationReasonToStringShort(CreationReason cr)
{
	switch (cr)
	{
	case CreationReason::UserDefined: return "U";
	case CreationReason::ManualExpand: return "ME";
	case CreationReason::AutoExpand: return "AE";
	case CreationReason::Query: return "Q";
	default: return "?";
	}
}

static bool EditCreationReason(UIContainer* ctx, const char* label, CreationReason& cr)
{
	if (ui::imm::PropButton(ctx, label, CreationReasonToString(cr)))
	{
		ui::MenuItem items[] =
		{
			ui::MenuItem(CreationReasonToString(CreationReason::UserDefined)).Func([&cr]() { cr = CreationReason::UserDefined; }),
			ui::MenuItem(CreationReasonToString(CreationReason::ManualExpand)).Func([&cr]() { cr = CreationReason::ManualExpand; }),
			ui::MenuItem(CreationReasonToString(CreationReason::AutoExpand)).Func([&cr]() { cr = CreationReason::AutoExpand; }),
			ui::MenuItem(CreationReasonToString(CreationReason::Query)).Func([&cr]() { cr = CreationReason::Query; }),
		};
		ui::Menu(items).Show(ctx->GetCurrentNode());
		return true;
	}
	return false;
}

void DataDesc::EditInstance(UIContainer* ctx)
{
	if (auto* SI = curInst)
	{
		bool del = false;

		if (ui::imm::Button(ctx, "Delete"))
		{
			del = true;
		}
		ui::imm::PropEditString(ctx, "Notes", SI->notes.c_str(), [&SI](const char* s) { SI->notes = s; });
		ui::imm::PropEditInt(ctx, "Offset", SI->off);
		if (ui::imm::PropButton(ctx, "Edit struct:", SI->def->name.c_str()))
		{
			editMode = 1;
			ctx->GetCurrentNode()->Rerender();
		}

		if (advancedAccess)
		{
			EditCreationReason(ctx, "Creation reason", SI->creationReason);
			ui::imm::PropEditBool(ctx, "Use remaining size", SI->remainingCountIsSize);
			ui::imm::PropEditInt(ctx, SI->remainingCountIsSize ? "Remaining size" : "Remaining count", SI->remainingCount);

			ui::Property::Begin(ctx);
			auto& lbl = ui::Property::Label(ctx, "Size override");
			ui::imm::EditBool(ctx, SI->sizeOverrideEnable);
			ui::imm::EditInt(ctx, &lbl, SI->sizeOverrideValue);
			ui::Property::End(ctx);
		}

		ctx->Text("Arguments") + ui::Padding(5);
		ctx->Push<ui::Panel>();
		for (size_t i = 0; i < SI->args.size(); i++)
		{
			auto& A = SI->args[i];
			ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
			ui::imm::PropEditString(ctx, "\bName", A.name.c_str(), [&A](const char* v) { A.name = v; });
			ui::imm::PropEditInt(ctx, "\bValue", A.intVal);
			if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
			{
				SI->args.erase(SI->args.begin() + i);
				ctx->GetCurrentNode()->Rerender();
			}
			ctx->Pop();
		}
		if (ui::imm::Button(ctx, "Add"))
		{
			SI->args.push_back({ "unnamed", 0 });
			ctx->GetCurrentNode()->Rerender();
		}
		ctx->Pop();

		if (SI->def)
		{
			auto& S = *SI->def;
			struct Data : ui::TableDataSource
			{
				size_t GetNumRows() override { return SI->def->GetFieldCount(); }
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
					case 0: return SI->def->fields[row].name;
					case 1: return SI->def->fields[row].type + "[" + std::to_string(SI->def->fields[row].count) + "]";
					case 2: return SI->GetFieldPreview(row);
					default: return "";
					}
				}

				DDStructInst* SI;
			};
			Data data;
			data.SI = SI;
			auto size = SI->GetSize();
			int64_t remSizeSub = SI->remainingCountIsSize ? (SI->sizeOverrideEnable ? SI->sizeOverrideValue : size) : 1;

			char bfr[256];
			snprintf(bfr, 256, "Next instance (@%" PRId64 ", after %" PRId64 ", rem. %s: %" PRId64 ")",
				SI->off + size,
				SI->sizeOverrideEnable ? SI->sizeOverrideValue : size,
				SI->remainingCountIsSize ? "size" : "count",
				SI->remainingCount - remSizeSub);
			if (SI->remainingCount - remSizeSub && ui::imm::Button(ctx, bfr, { ui::Enable(SI->remainingCount - remSizeSub > 0) }))
			{
				SetCurrentInstance(CreateNextInstance(*SI, size, CreationReason::ManualExpand));
			}

			snprintf(bfr, 256, "Data (size=%" PRId64 ")", size);
			ctx->Text(bfr) + ui::Padding(5);

			bool incomplete = false;
			for (size_t i = 0; i < S.fields.size(); i++)
			{
				auto desc = SI->GetFieldDescLazy(i, &incomplete);
				ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
				ctx->Text(desc) + ui::Padding(5);
				
				if (FindStructByName(S.fields[i].type) && SI->IsFieldPresent(i, true) == OptionalBool::True)
				{
					if (ui::imm::Button(ctx, "View"))
					{
						SetCurrentInstance(SI->CreateFieldInstance(i, CreationReason::ManualExpand));
					}
				}
				ctx->Pop();
			}
			/*auto* tv = ctx->Make<ui::TableView>();
			tv->SetDataSource(&data); TODO */

			if (incomplete && ui::imm::Button(ctx, "Load completely"))
			{
				for (size_t i = 0; i < S.fields.size(); i++)
				{
					SI->GetFieldElementCount(i);
					SI->GetFieldOffset(i);
					SI->GetFieldPreview(i);
				}
				SI->GetSize();
			}

			if (S.resource.type == DDStructResourceType::Image)
			{
				if (ui::imm::Button(ctx, "Open image in tab", { ui::Enable(!!S.resource.image) }))
				{
					images.push_back(GetInstanceImage(*SI));

					curImage = images.size() - 1;
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenImage, images.size() - 1);
				}
				if (ui::imm::Button(ctx, "Open image in editor", { ui::Enable(!!S.resource.image) }))
				{
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenImageRsrcEditor, uintptr_t(SI->def));
				}
			}
		}

		if (del)
		{
			DeleteInstance(curInst);
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
		if (ui::imm::Button(ctx, "OK", { ui::Height(30) }))
		{
			rename = true;
			SetVisible(false);
		}
		*ctx->Make<ui::BoxElement>() + ui::Width(16);
		if (ui::imm::Button(ctx, "Cancel", { ui::Height(30) }))
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
	if (auto* SI = curInst)
	{
		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("Struct:") + ui::Padding(5);
		ctx->Text(SI->def->name) + ui::Padding(5);
		if (ui::imm::Button(ctx, "Rename"))
		{
			RenameDialog rd(SI->def->name);
			for (;;)
			{
				rd.Show();
				if (rd.rename && rd.newName != SI->def->name)
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
					structs[rd.newName] = SI->def;
					structs.erase(SI->def->name);
					SI->def->name = rd.newName;
				}
				break;
			}
		}
		ctx->Pop();

		auto it = structs.find(SI->def->name);
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
				if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
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
				if (ui::imm::Button(ctx, "<", { ui::Width(20), ui::Enable(i > 0) }))
				{
					std::swap(S.fields[i - 1], S.fields[i]);
					ctx->GetCurrentNode()->Rerender();
				}
				if (ui::imm::Button(ctx, ">", { ui::Width(20), ui::Enable(i + 1 < S.fields.size()) }))
				{
					std::swap(S.fields[i + 1], S.fields[i]);
					ctx->GetCurrentNode()->Rerender();
				}
				if (ui::imm::Button(ctx, "Edit", { ui::Width(50) }))
				{
					editMode = 2;
					curField = i;
					ctx->GetCurrentNode()->Rerender();
				}
				if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
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

			ctx->Text("Resource") + ui::Padding(5);
			ui::Property::Begin(ctx);
			ctx->Text("Type:") + ui::Padding(5);
			ui::imm::RadioButton(ctx, S.resource.type, DDStructResourceType::None, "None", { ui::Style(ui::Theme::current->button) });
			ui::imm::RadioButton(ctx, S.resource.type, DDStructResourceType::Image, "Image", { ui::Style(ui::Theme::current->button) });
			ui::Property::End(ctx);

			if (S.resource.type == DDStructResourceType::Image)
			{
				if (ui::imm::Button(ctx, "Edit image"))
				{
					if (!S.resource.image)
						S.resource.image = new DDRsrcImage;
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenImageRsrcEditor, uintptr_t(SI->def));
				}
			}
		}
		else
		{
			ctx->Text("-- NOT FOUND --") + ui::Padding(10);
		}
	}
}

void DataDesc::EditField(UIContainer* ctx)
{
	if (curInst)
	{
		auto it = structs.find(curInst->def->name);
		if (it != structs.end())
		{
			auto& S = *it->second;
			if (curField < S.fields.size())
			{
				auto& F = S.fields[curField];
				if (!S.serialized)
					ui::imm::PropEditInt(ctx, "Offset", F.off);
				ui::imm::PropEditString(ctx, "Off.expr.", F.offExpr.expr.c_str(), [&F](const char* v) { F.offExpr.SetExpr(v); });
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
					if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
					{
						F.structArgs.erase(F.structArgs.begin() + i);
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					F.structArgs.push_back({ "unnamed", "", 0 });
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
					if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
					{
						F.conditions.erase(F.conditions.begin() + i);
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					F.conditions.push_back({ "unnamed", "" });
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

		bool chg = false;
		chg |= ui::imm::PropEditString(ctx, "Notes", I.notes.c_str(), [&I](const char* s) { I.notes = s; });
		chg |= EditImageFormat(ctx, "Format", I.format);
		chg |= ui::imm::PropEditInt(ctx, "Image offset", I.offImage);
		chg |= ui::imm::PropEditInt(ctx, "Palette offset", I.offPalette);
		chg |= ui::imm::PropEditInt(ctx, "Width", I.width);
		chg |= ui::imm::PropEditInt(ctx, "Height", I.height);
		if (chg)
			ctx->GetCurrentNode()->Rerender();

		ctx->Pop();
	}
}

DDStructInst* DataDesc::AddInstance(const DDStructInst& src)
{
	printf("trying to create %s @ %" PRId64 "\n", src.def->name.c_str(), src.off);
	for (size_t i = 0, num = instances.size(); i < num; i++)
	{
		auto* I = instances[i];
		if (I->off == src.off && I->def == src.def && I->file == src.file)
		{
			I->creationReason = std::min(I->creationReason, src.creationReason);
			I->remainingCount = src.remainingCount;
			I->remainingCountIsSize = src.remainingCountIsSize;
			return I;
		}
	}
	auto* copy = new DDStructInst(src);
	copy->id = instIDAlloc++;
	copy->OnEdit();
	instances.push_back(copy);
	return copy;
}

void DataDesc::DeleteInstance(DDStructInst* inst)
{
	delete inst;
	_OnDeleteInstance(inst);
	instances.erase(std::remove_if(instances.begin(), instances.end(), [inst](DDStructInst* SI) { return inst == SI; }), instances.end());
}

void DataDesc::SetCurrentInstance(DDStructInst* inst)
{
	curInst = inst;
	ui::Notify(DCT_CurStructInst, nullptr);
}

void DataDesc::_OnDeleteInstance(DDStructInst* inst)
{
	if (curInst == inst)
		SetCurrentInstance(nullptr);
}

DDStructInst* DataDesc::CreateNextInstance(const DDStructInst& SI, int64_t structSize, CreationReason cr)
{
	int64_t remSizeSub = SI.remainingCountIsSize ? (SI.sizeOverrideEnable ? SI.sizeOverrideValue : structSize) : 1;
	assert(SI.remainingCount - remSizeSub);
	{
		DDStructInst SIcopy = SI;
		SIcopy.off += SI.sizeOverrideEnable ? SI.sizeOverrideValue : structSize;
		SIcopy.remainingCount -= remSizeSub;
		SIcopy.sizeOverrideEnable = false;
		SIcopy.sizeOverrideValue = 0;
		SIcopy.creationReason = cr;
		return AddInstance(SIcopy);
	}
}

void DataDesc::ExpandAllInstances(DDFile* filterFile)
{
	for (size_t i = 0; i < instances.size(); i++)
	{
		if (filterFile && instances[i]->file != filterFile)
			continue;
		auto* SI = instances[i];
		auto& S = *SI->def;
		auto size = SI->GetSize();

		int64_t remSizeSub = SI->remainingCountIsSize ? (SI->sizeOverrideEnable ? SI->sizeOverrideValue : size) : 1;
		if (SI->remainingCount - remSizeSub > 0)
			CreateNextInstance(*instances[i], size, CreationReason::AutoExpand);

		for (size_t j = 0, jn = S.fields.size(); j < jn; j++)
			if (SI->IsFieldPresent(j) && structs.count(S.fields[j].type))
				SI->CreateFieldInstance(j, CreationReason::AutoExpand);
	}
}

void DataDesc::DeleteAllInstances(DDFile* filterFile, DDStruct* filterStruct)
{
	instances.erase(std::remove_if(instances.begin(), instances.end(), [this, filterFile, filterStruct](DDStructInst* SI)
	{
		if (SI->creationReason <= CreationReason::ManualExpand)
			return false;
		if (filterFile && SI->file != filterFile)
			return false;
		if (filterStruct && SI->def != filterStruct)
			return false;
		_OnDeleteInstance(SI);
		delete SI;
		return true;
	}), instances.end());
}

DataDesc::Image DataDesc::GetInstanceImage(const DDStructInst& SI)
{
	VariableSource vs;
	{
		vs.desc = this;
		vs.root = &SI;
	}
	auto& II = *SI.def->resource.image;
	Image img;
	img.file = SI.file;
	img.offImage = II.imgOff.Evaluate(vs);
	img.offPalette = II.palOff.Evaluate(vs);
	img.width = II.width.Evaluate(vs);
	img.height = II.height.Evaluate(vs);
	img.format = II.format;

	for (auto& FO : II.formatOverrides)
	{
		if (SI.EvaluateConditions(FO.conditions))
		{
			img.format = FO.format;
			break;
		}
	}
	img.userCreated = false;
	return img;
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

DDFile* DataDesc::CreateNewFile()
{
	auto* F = new DDFile;
	F->id = fileIDAlloc++;
	F->mdSrc.data = &F->markerData;
	F->mdSrc.dataSource = F->dataSource;
	files.push_back(F);
	return F;
}

DDFile* DataDesc::FindFileByID(uint64_t id)
{
	for (auto* F : files)
		if (F->id == id)
			return F;
	return nullptr;
}

DDStruct* DataDesc::CreateNewStruct(const std::string& name)
{
	assert(structs.count(name) == 0);
	auto* S = new DDStruct;
	S->name = name;
	structs[name] = S;
	return S;
}

DDStruct* DataDesc::FindStructByName(const std::string& name)
{
	auto it = structs.find(name);
	if (it != structs.end())
		return it->second;
	return nullptr;
}

DDStructInst* DataDesc::FindInstanceByID(int64_t id)
{
	for (auto* SI : instances)
		if (SI->id == id)
			return SI;
	return nullptr;
}

void DataDesc::DeleteImage(size_t id)
{
	images.erase(images.begin() + id);
	if (curImage == id)
		curImage = 0;
}

size_t DataDesc::DuplicateImage(size_t id)
{
	images.push_back(images[id]);
	return images.size() - 1;
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
	instIDAlloc = r.ReadUInt64("instIDAlloc");

	r.BeginArray("structs");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto name = r.ReadString("name");
		auto* S = CreateNewStruct(name);
		S->Load(r);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("instances");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto* SI = new DDStructInst;
		auto id = r.ReadUInt64("id", UINT64_MAX);
		SI->id = id == UINT64_MAX ? instIDAlloc++ : id;
		SI->desc = this;
		SI->def = FindStructByName(r.ReadString("struct"));
		SI->file = FindFileByID(r.ReadUInt64("file"));
		SI->off = r.ReadInt64("off");
		SI->notes = r.ReadString("notes");
		SI->creationReason = (CreationReason)r.ReadInt("creationReason",
			int(r.ReadBool("userCreated") ? CreationReason::UserDefined : CreationReason::AutoExpand));
		SI->remainingCountIsSize = r.ReadBool("remainingCountIsSize");
		SI->remainingCount = r.ReadInt64("remainingCount");
		SI->sizeOverrideEnable = r.ReadBool("sizeOverrideEnable");
		SI->sizeOverrideValue = r.ReadInt64("sizeOverrideValue");

		r.BeginArray("args");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			DDArg A;
			A.name = r.ReadString("name");
			A.intVal = r.ReadInt64("intVal");
			SI->args.push_back(A);

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
	auto curInstID = r.ReadInt64("curInst", -1);
	SetCurrentInstance(curInstID == -1 ? nullptr : FindInstanceByID(curInstID));
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
	w.WriteInt("instIDAlloc", instIDAlloc);

	w.BeginArray("structs");
	std::vector<std::string> structNames;
	for (const auto& sp : structs)
		structNames.push_back(sp.first);
	std::sort(structNames.begin(), structNames.end());
	for (const auto& sname : structNames)
	{
		DDStruct* S = structs.find(sname)->second;

		w.BeginDict("");

		S->Save(w);

		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("instances");
	for (const DDStructInst* SI : instances)
	{
		if (SI->creationReason >= CreationReason::AutoExpand)
			continue;

		w.BeginDict("");

		w.WriteInt("id", SI->id);
		w.WriteString("struct", SI->def->name);
		w.WriteInt("file", SI->file->id);
		w.WriteInt("off", SI->off);
		w.WriteString("notes", SI->notes);
		w.WriteInt("creationReason", int(SI->creationReason));
		w.WriteBool("remainingCountIsSize", SI->remainingCountIsSize);
		w.WriteInt("remainingCount", SI->remainingCount);
		w.WriteBool("sizeOverrideEnable", SI->sizeOverrideEnable);
		w.WriteInt("sizeOverrideValue", SI->sizeOverrideValue);

		w.BeginArray("args");
		for (const DDArg& A : SI->args)
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
	w.WriteInt("curInst", curInst ? curInst->id : -1LL);
	w.WriteInt("curImage", curImage);
	w.WriteInt("curField", curField);

	w.EndDict();
}


enum COLS_DDI
{
	DDI_COL_ID,
	DDI_COL_CR,
	DDI_COL_File,
	DDI_COL_Offset,
	DDI_COL_Struct,
	DDI_COL_Bytes,

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
	size_t ncols = filterStructEnable && filterStruct ? DDI_COL_HEADER_SIZE + filterStruct->fields.size() : DDI_COL_HEADER_SIZE;
	if (filterFileEnable && filterFile)
		ncols--;
	if (filterStructEnable && filterStruct)
		ncols--;
	if (showBytes == 0)
		ncols--;
	return ncols;
}

std::string DataDescInstanceSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string DataDescInstanceSource::GetColName(size_t col)
{
	if (filterFileEnable && filterFile && col >= DDI_COL_File)
		col++;
	if (filterStructEnable && filterStruct && col >= DDI_COL_Struct)
		col++;
	if (showBytes == 0 && col >= DDI_COL_Bytes)
		col++;
	switch (col)
	{
	case DDI_COL_ID: return "ID";
	case DDI_COL_CR: return "CR";
	case DDI_COL_File: return "File";
	case DDI_COL_Offset: return "Offset";
	case DDI_COL_Struct: return "Struct";
	case DDI_COL_Bytes: return "Bytes";
	default: return filterStruct->fields[col - DDI_COL_FirstField].name;
	}
}

std::string DataDescInstanceSource::GetText(size_t row, size_t col)
{
	if (filterFileEnable && filterFile && col >= DDI_COL_File)
		col++;
	if (filterStructEnable && filterStruct && col >= DDI_COL_Struct)
		col++;
	if (showBytes == 0 && col >= DDI_COL_Bytes)
		col++;
	switch (col)
	{
	case DDI_COL_ID: return std::to_string(_indices[row]);
	case DDI_COL_CR: return CreationReasonToStringShort(dataDesc->instances[_indices[row]]->creationReason);
	case DDI_COL_File: return dataDesc->instances[_indices[row]]->file->name;
	case DDI_COL_Offset: return std::to_string(dataDesc->instances[_indices[row]]->off);
	case DDI_COL_Struct: return dataDesc->instances[_indices[row]]->def->name;
	case DDI_COL_Bytes: {
		uint32_t nbytes = std::min(showBytes, 128U);
		uint8_t buf[128];
		auto* inst = dataDesc->instances[_indices[row]];
		inst->file->dataSource->Read(inst->off, nbytes, buf);
		std::string text;
		for (uint32_t i = 0; i < nbytes; i++)
		{
			if (i)
				text += " ";
			char tbf[32];
			snprintf(tbf, 32, "%02X", buf[i]);
			text += tbf;
		}
		return text;
	} break;
	default: return dataDesc->instances[_indices[row]]->GetFieldPreview(col - DDI_COL_FirstField);
	}
}

void DataDescInstanceSource::Edit(UIContainer* ctx)
{
	ui::Property::Begin(ctx, "Filter by struct");
	if (ui::imm::EditBool(ctx, filterStructEnable))
		refilter = true;
	if (ui::imm::Button(ctx, filterStruct ? filterStruct->name.c_str() : "<none>"))
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
	ui::imm::PropEditInt(ctx, "\bBytes", showBytes, {}, 1, 0, 128);
	ui::Property::End(ctx);

	if (!filterStructEnable || !filterStruct)
	{
		ui::Property::Begin(ctx, "Hide structs");
		if (ui::imm::EditBool(ctx, filterHideStructsEnable))
			refilter = true;

		std::vector<DDStruct*> structs(filterHideStructs.begin(), filterHideStructs.end());
		std::sort(structs.begin(), structs.end(), [](const DDStruct* A, const DDStruct* B) { return A->name < B->name; });

		std::string buttonName;
		for (auto* S : structs)
		{
			if (buttonName.size())
				buttonName += ", ";
			buttonName += S->name;
		}
		if (ui::imm::Button(ctx, structs.empty() ? "<none>" : buttonName.c_str()))
		{
			structs.clear();
			for (auto& kvp : dataDesc->structs)
				structs.push_back(kvp.second);
			std::sort(structs.begin(), structs.end(), [](const DDStruct* A, const DDStruct* B) { return A->name < B->name; });

			std::vector<ui::MenuItem> items;
			items.push_back(ui::MenuItem("<none>").Func([this]() { filterHideStructs.clear(); refilter = true; }));
			items.push_back(ui::MenuItem("<all>").Func([this, &structs]() { filterHideStructs.insert(structs.begin(), structs.end()); refilter = true; }));
			items.push_back(ui::MenuItem::Separator());
			for (auto* S : structs)
			{
				items.push_back(ui::MenuItem(S->name, {}, false, filterHideStructs.count(S)).Func([this, S]()
				{
					if (filterHideStructs.count(S))
						filterHideStructs.erase(S);
					else
						filterHideStructs.insert(S);
					refilter = true;
				}));
			}
			ui::Menu(items).Show(ctx->GetCurrentNode());
		}
		ui::Property::End(ctx);
	}

	ui::Property::Begin(ctx, "Filter by file");
	if (ui::imm::EditBool(ctx, filterFileEnable))
		refilter = true;
	if (ui::imm::Button(ctx, filterFile ? filterFile->name.c_str() : "<none>"))
	{
		std::vector<ui::MenuItem> items;
		items.push_back(ui::MenuItem("<none>").Func([this]() { filterFile = nullptr; refilter = true; }));
		for (auto* F : dataDesc->files)
		{
			items.push_back(ui::MenuItem(F->name).Func([this, F]() { filterFile = F; refilter = true; }));
		}
		ui::Menu(items).Show(ctx->GetCurrentNode());
	}
	ui::imm::PropEditBool(ctx, "\bFollow", filterFileFollow);
	ui::Property::End(ctx);

	if (EditCreationReason(ctx, "Filter by creation reason", filterCreationReason))
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
		auto* I = dataDesc->instances[i];
		if (filterStructEnable && filterStruct && filterStruct != I->def)
			continue;
		else if (filterHideStructsEnable && filterHideStructs.count(I->def))
			continue;
		if (filterFileEnable && filterFile && filterFile != I->file)
			continue;
		if (I->creationReason > filterCreationReason)
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
