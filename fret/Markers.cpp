
#include "pch.h"
#include "Markers.h"


ui::DataCategoryTag DCT_Marker[1];
ui::DataCategoryTag DCT_MarkedItems[1];


ui::Color4f colorChar{ 0.3f, 0.9f, 0.1f, 0.3f };
ui::Color4f colorFloat32{ 0.9f, 0.1f, 0.0f, 0.3f };
ui::Color4f colorFloat64{ 0.9f, 0.2f, 0.0f, 0.3f };
ui::Color4f colorInt8{ 0.3f, 0.1f, 0.9f, 0.3f };
ui::Color4f colorInt16{ 0.2f, 0.2f, 0.9f, 0.3f };
ui::Color4f colorInt32{ 0.1f, 0.3f, 0.9f, 0.3f };
ui::Color4f colorInt64{ 0.0f, 0.4f, 0.9f, 0.3f };
ui::Color4f colorNearFileSize32{ 0.1f, 0.3f, 0.7f, 0.3f };
ui::Color4f colorNearFileSize64{ 0.0f, 0.4f, 0.7f, 0.3f };
ui::Color4f colorASCII{ 0.1f, 0.9f, 0.0f, 0.3f };
ui::Color4f colorInst{ 0.9f, 0.9f, 0.9f, 0.6f };
ui::Color4f colorCurInst{ 0.9f, 0.2f, 0.0f, 0.8f };


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


enum AnalysisDataColumns
{
	ADC_Count,
	ADC_Unique,
	ADC_Features,
	ADC_VMin,
	ADC_VMax,
	ADC_VGCD,
	ADC_DMin,
	ADC_DMax,
	ADC_DGCD,

	ADC__COUNT,
};

const char* g_analysisDataColNames[] =
{
	"Count",
	"Unique",
	"Features",
	"Min",
	"Max",
	"GCD",
	"dMin",
	"dMax",
	"dGCD",
};

size_t AnalysisData::GetNumCols()
{
	return ADC__COUNT;
}

std::string AnalysisData::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string AnalysisData::GetColName(size_t col)
{
	return g_analysisDataColNames[col];
}

std::string AnalysisData::GetText(size_t row, size_t col)
{
	const auto& R = results[row];
	switch (col)
	{
	case ADC_Count: return std::to_string(R.count);
	case ADC_Unique: return std::to_string(R.unique);
	case ADC_Features: {
		std::string ret;
		if (R.flags & AnalysisResult::Equal)
			ret += "eq ";
		if (R.flags & AnalysisResult::Asc)
			ret += "asc ";
		if (R.flags & AnalysisResult::AscEq)
			ret += "asceq ";
		if (!ret.empty())
			ret.pop_back();
		return ret; }
	case ADC_VMin: return R.vmin;
	case ADC_VMax: return R.vmax;
	case ADC_VGCD: return R.vgcd;
	case ADC_DMin: return R.dmin;
	case ADC_DMax: return R.dmax;
	case ADC_DGCD: return R.dgcd;
	default: return "";
	}
}


template <class T> T modulus(T a, T b) { return a % b; }
float modulus(float a, float b) { return fmodf(a, b); }
double modulus(double a, double b) { return fmod(a, b); }
template <class T> T greatest_common_divisor(T a, T b)
{
	if (b != b || b == 0)
		return a;
	return greatest_common_divisor(b, modulus(a, b));
}

template<class T> struct get_signed : std::make_signed<T> {};
template<> struct get_signed<float> { using type = float; };
template<> struct get_signed<double> { using type = double; };
template<> struct get_signed<bool> { using type = bool; };

template<class T> void ApplyStartEndBits(T& r, int sb, int eb)
{
	uint64_t mask2 = (1ULL << sb) - 1ULL;
	r = r & ~mask2;
	if (eb < 64)
	{
		uint64_t mask = (1ULL << eb) - 1ULL;
		r = (r & mask) | (r < 0 ? ~mask : 0);
	}
}
void ApplyStartEndBits(float& r, int sb, int eb) {}
void ApplyStartEndBits(double& r, int sb, int eb) {}

typedef AnalysisResult AnalysisFunc(IDataSource* ds, uint64_t off, uint64_t stride, uint64_t count, uint8_t sb, uint8_t eb, bool excl0);
template <class T> AnalysisResult AnalysisFuncImpl(IDataSource* ds, uint64_t off, uint64_t stride, uint64_t count, uint8_t sb, uint8_t eb, bool excl0)
{
	using ST = typename get_signed<T>::type;
	T min = std::numeric_limits<T>::max();
	T max = std::numeric_limits<T>::min();
	T gcd = 0;
	ST dmin = std::numeric_limits<ST>::max();
	ST dmax = std::numeric_limits<ST>::min();
	ST dgcd = 0;
	T prev = 0;
	bool eq = true;
	bool asc = true;
	bool asceq = true;
	uint64_t numfound = 0;
	std::unordered_map<T, uint64_t> counts;
	for (uint64_t i = 0; i < count; i++)
	{
		T val;
		ds->Read(off + stride * i, sizeof(val), &val);
		ApplyStartEndBits(val, sb, eb);
		if (excl0 && val == 0)
			continue;
		*counts.insert({ val, 0 }).first++;
		min = std::min(min, val);
		max = std::max(max, val);
		gcd = greatest_common_divisor(gcd, val);
		if (numfound > 0)
		{
			ST d = ST(val) - ST(prev);
			dmin = std::min(dmin, d);
			dmax = std::max(dmax, d);
			dgcd = greatest_common_divisor<ST>(dgcd, d);
			if (val != prev)
				eq = false;
			if (val <= prev)
				asc = false;
			if (val < prev)
				asceq = false;
		}
		prev = val;
		numfound++;
	}
	AnalysisResult r;
	{
		r.count = numfound;
		r.unique = counts.size();
		if (r.count > 1)
		{
			if (eq)
				r.flags |= AnalysisResult::Equal;
			else if (asc)
				r.flags |= AnalysisResult::Asc;
			else if (asceq)
				r.flags |= AnalysisResult::AscEq;
		}
		r.vmin = std::to_string(min);
		r.vmax = std::to_string(max);
		r.vgcd = std::to_string(gcd);
		r.dmin = std::to_string(dmin);
		r.dmax = std::to_string(dmax);
		r.dgcd = std::to_string(dgcd);
	}
	return r;
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
typedef void MarkerReadFunc(std::string& outbuf, IDataSource* ds, uint64_t off, uint8_t sb, uint8_t eb);
template <class T, DataType ty> void MarkerReadFuncImpl(std::string& outbuf, IDataSource* ds, uint64_t off, uint8_t sb, uint8_t eb)
{
	T val;
	ds->Read(off, sizeof(T), &val);
	ApplyStartEndBits(val, sb, eb);
	char bfr[128];
	snprintf(bfr, 128, markerReadCodes[ty], val);
	outbuf += bfr;
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
			markerReadFuncs[marker.type](text, src, marker.at + i * marker.stride + j * typeSizes[marker.type], marker.bitstart, marker.bitend);
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
std::string GetMarkerSingleLine(const Marker& marker, IDataSource* src, size_t which)
{
	std::string text;
	for (uint64_t j = 0; j < marker.count; j++)
	{
		if (j > 0 && marker.type != DT_CHAR)
			text += ',';
		markerReadFuncs[marker.type](text, src, marker.at + which * marker.stride + j * typeSizes[marker.type], marker.bitstart, marker.bitend);
	}
	return text;
}

typedef int64_t Int64ReadFunc(IDataSource* ds, uint64_t off, uint8_t sb, uint8_t eb);
template <class T> int64_t Int64ReadFuncImpl(IDataSource* ds, uint64_t off, uint8_t sb, uint8_t eb)
{
	T v;
	ds->Read(off, sizeof(v), &v);
	ApplyStartEndBits(v, sb, eb);
	return int64_t(v);
}
static Int64ReadFunc* int64ReadFuncs[] =
{
	Int64ReadFuncImpl<char>,
	Int64ReadFuncImpl<int8_t>,
	Int64ReadFuncImpl<uint8_t>,
	Int64ReadFuncImpl<int16_t>,
	Int64ReadFuncImpl<uint16_t>,
	Int64ReadFuncImpl<int32_t>,
	Int64ReadFuncImpl<uint32_t>,
	Int64ReadFuncImpl<int64_t>,
	Int64ReadFuncImpl<uint64_t>,
	Int64ReadFuncImpl<float>,
	Int64ReadFuncImpl<double>,
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
	return at + count * typeSizes[type] + stride * (repeats ? repeats - 1 : 0);
}

ui::Color4f Marker::GetColor() const
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
	Marker m;
	{
		m.type = dt;
		m.at = from;
		m.count = (to - from) / typeSizes[dt];
		m.repeats = 1;
		m.stride = 0;
	}
	markers.push_back(m);
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
		M.bitstart = r.ReadUInt("bitstart", 0);
		M.bitend = r.ReadUInt("bitend", 64);
		M.excludeZeroes = r.ReadBool("excludeZeroes");
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
		w.WriteInt("bitstart", M.bitstart);
		w.WriteInt("bitend", M.bitend);
		w.WriteBool("excludeZeroes", M.excludeZeroes);
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
	MD_COL_BitRange,
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
	case MD_COL_BitRange: return "Bit range";
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
	case MD_COL_BitRange: return std::to_string(markers[row].bitstart) + "-" + std::to_string(markers[row].bitend);
	case MD_COL_Notes: return markers[row].notes;
	case MD_COL_Preview: return GetMarkerPreview(markers[row], dataSource, 32);
	default: return "";
	}
}

void MarkerDataSource::ClearSelection()
{
	selected = SIZE_MAX;
}

bool MarkerDataSource::GetSelectionState(uintptr_t item)
{
	return selected == item;
}

void MarkerDataSource::SetSelectionState(uintptr_t item, bool sel)
{
	if (sel)
		selected = item;
	else if (GetSelectionState(item))
		selected = SIZE_MAX;
}


void MarkedItemEditor::Build()
{
	ui::Push<ui::EdgeSliceLayoutElement>();

	Subscribe(DCT_Marker, marker);
	ui::MakeWithText<ui::LabelFrame>("Marker");

	ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
	ui::Push<ui::StackTopDownLayoutElement>();
	ui::imm::PropDropdownMenuList("Type", marker->type, ui::BuildAlloc<ui::CStrArrayOptionList>(typeNames));
	ui::imm::PropEditInt("Offset", marker->at);
	ui::imm::PropEditInt("Count", marker->count, { ui::AddLabelTooltip("The number of elements within a single packed array") });
	ui::imm::PropEditInt("Repeats", marker->repeats, { ui::AddLabelTooltip(">1 turns on analysis across repeats instead of packed array") });
	ui::imm::PropEditInt("Stride", marker->stride, { ui::AddLabelTooltip("Distance in bytes between arrays of elements") });
	unsigned bs = marker->bitstart;
	if (ui::imm::PropEditInt("Start bit", bs, {}, 1U, { 0U, 64U }))
		marker->bitstart = bs;
	unsigned be = marker->bitend;
	if (ui::imm::PropEditInt("End bit", be, {}, 1U, { 0U, 64U }))
		marker->bitend = be;
	ui::imm::PropEditBool("Exclude zeroes", marker->excludeZeroes, { ui::AddLabelTooltip("Whether to ignore zeroes during analysis") });
	ui::imm::PropEditString("Notes", marker->notes.c_str(), [this](const char* v) { marker->notes = v; });
	ui::Pop();
	ui::Pop();

	ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
	ui::Push<ui::EdgeSliceLayoutElement>();
	if (ui::imm::Button("Analyze"))
	{
		analysisData.results.clear();
		if (marker->repeats <= 1)
		{
			// analyze a single array
			analysisData.results.push_back(analysisFuncs[marker->type](dataSource, marker->at, typeSizes[marker->type],
				marker->count, marker->bitstart, marker->bitend, marker->excludeZeroes));
		}
		else
		{
			// analyze each array element separately
			for (uint64_t i = 0; i < marker->count; i++)
			{
				analysisData.results.push_back(analysisFuncs[marker->type](dataSource, marker->at + i * typeSizes[marker->type],
					marker->stride, marker->repeats, marker->bitstart, marker->bitend, marker->excludeZeroes));
			}
		}
	}
	if (marker->type == DT_F32 && marker->count == 3 && marker->repeats > 1 && ui::imm::Button("Export points to .obj"))
	{
		if (FILE* fp = fopen("positions.obj", "w"))
		{
			for (uint64_t i = 0; i < marker->repeats; i++)
			{
				float vec[3];
				dataSource->Read(marker->at + marker->stride * i, sizeof(vec), vec);
				fprintf(fp, "v %g %g %g\n", vec[0], vec[1], vec[2]);
			}
			fclose(fp);
		}
	}
	if (marker->type >= DT_I8 && marker->type <= DT_U64 && marker->count == 2 && marker->repeats > 1 && ui::imm::Button("Export links to .dot"))
	{
		if (FILE* fp = fopen("graph.dot", "w"))
		{
			fprintf(fp, "graph exported {\n");
			for (uint64_t i = 0; i < marker->repeats; i++)
			{
				int64_t A = int64ReadFuncs[marker->type](
					dataSource,
					marker->at + marker->stride * i,
					marker->bitstart,
					marker->bitend);
				int64_t B = int64ReadFuncs[marker->type](
					dataSource,
					marker->at + marker->stride * i + typeSizes[marker->type],
					marker->bitstart,
					marker->bitend);
				fprintf(fp, "\t%" PRId64 " -- %" PRId64 "\n", A, B);
			}
			fprintf(fp, "}\n");
			fclose(fp);
		}
	}
	if (ui::imm::Button("Export data to CSV"))
	{
		if (FILE* fp = fopen("marker.csv", "w"))
		{
			for (size_t i = 0; i < marker->repeats; i++)
			{
				auto line = GetMarkerSingleLine(*marker, dataSource, i);
				fwrite(line.data(), 1, line.size(), fp);
				fputc('\n', fp);
			}
			fclose(fp);
		}
	}
	if (analysisData.results.size())
	{
		auto& tbl = ui::Make<ui::TableView>();
		tbl.SetDataSource(&analysisData);
		tbl.CalculateColumnWidths();
	}
	else
	{
		// something to take the space so that the last button isn't extended
		ui::Make<ui::WrapperElement>();
	}
	ui::Pop();
	ui::Pop();

	ui::Pop();
}


void MarkedItemsList::Build()
{
	Subscribe(DCT_MarkedItems, markerData);
	ui::Text("Edit marked items");
	for (auto& m : markerData->markers)
	{
		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::Push<ui::StackTopDownLayoutElement>();
		ui::imm::PropDropdownMenuList("Type", m.type, ui::BuildAlloc<ui::CStrArrayOptionList>(typeNames));
		ui::imm::PropEditInt("Offset", m.at);
		ui::imm::PropEditInt("Count", m.count);
		ui::imm::PropEditInt("Repeats", m.repeats);
		ui::imm::PropEditInt("Stride", m.stride);
		ui::Pop();
		ui::Pop();
	}
}
