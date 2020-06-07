
#pragma once
#include "pch.h"
#include "FileReaders.h"
#include "MathExpr.h"


extern Color4f colorFloat32;
extern Color4f colorInt16;
extern Color4f colorInt32;
extern Color4f colorNearFileSize32;
extern Color4f colorNearFileSize64;
extern Color4f colorASCII;
extern Color4f colorInst;
extern Color4f colorCurInst;


struct IDataSource;
struct BuiltinTypeInfo;


enum DataType
{
	DT_CHAR,
	DT_I8,
	DT_U8,
	DT_I16,
	DT_U16,
	DT_I32,
	DT_U32,
	DT_I64,
	DT_U64,
	DT_F32,
	DT_F64,

	DT__COUNT,
};

const char* GetDataTypeName(DataType t);

struct Marker
{
	DataType type;
	uint64_t at;
	uint64_t count;
	uint64_t repeats;
	uint64_t stride;
	std::string notes;

	bool Contains(uint64_t pos) const;
	unsigned ContainInfo(uint64_t pos) const; // 1 - overlap, 2 - left edge, 4 - right edge
	uint64_t GetEnd() const;
	Color4f GetColor() const;
};

extern ui::DataCategoryTag DCT_Marker[1];
extern ui::DataCategoryTag DCT_MarkedItems[1];
struct MarkerData
{
	void AddMarker(DataType dt, uint64_t from, uint64_t to);

	void Load(const char* key, NamedTextSerializeReader& r);
	void Save(const char* key, NamedTextSerializeWriter& w);

	std::vector<Marker> markers;
};

struct MarkerDataSource : ui::TableDataSource
{
	size_t GetNumRows() override { return data->markers.size(); }
	size_t GetNumCols() override;
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;

	IDataSource* dataSource;
	MarkerData* data;
};

struct MarkedItemEditor : ui::Node
{
	void Render(UIContainer* ctx) override;

	IDataSource* dataSource;
	Marker* marker;
	std::vector<std::string> analysis;
};

struct MarkedItemsList : ui::Node
{
	void Render(UIContainer* ctx) override;

	MarkerData* markerData;
};


struct MathExprObj
{
	std::string expr;
	MathExpr* inst = nullptr;

	void Recompile()
	{
		delete inst;
		inst = nullptr;
		if (expr.size())
		{
			inst = new MathExpr;
			inst->Compile(expr.c_str());
		}
	}
	int64_t Evaluate(IVariableSource& vs)
	{
		return inst ? inst->Evaluate(&vs) : 0;
	}
	void SetExpr(const std::string& ne)
	{
		if (expr == ne)
			return;
		expr = ne;
		Recompile();
	}
};

struct DDFile
{
	uint64_t id = UINT64_MAX;
	std::string name;
	IDataSource* dataSource = nullptr;
	MarkerData markerData;
	MarkerDataSource mdSrc;
};
struct DDParam
{
	std::string name;
	int64_t intVal;
};
struct DDCondition
{
	std::string field;
	std::string value;
};
struct DDCompArg
{
	std::string name;
	std::string src;
	int64_t intVal;
};
struct DDField
{
	std::string type;
	std::string name;

	int64_t off = 0;
	MathExprObj offExpr;

	int64_t count = 1;
	std::string countSrc;
	bool countIsMaxSize = false;
	bool readUntil0 = false;
	std::vector<DDCompArg> structArgs;
	std::vector<DDCondition> conditions;

	bool IsComputed() const
	{
		return !offExpr.expr.empty();
	}
};
enum class DDStructResourceType
{
	None,
	Image,
};
bool EditImageFormat(UIContainer* ctx, const char* label, std::string& format);
struct DDRsrcImage
{
	std::string format;
	MathExprObj imgOff;
	MathExprObj palOff;
	MathExprObj width;
	MathExprObj height;
};
struct DDStructResource
{
	DDStructResourceType type = DDStructResourceType::None;
	DDRsrcImage* image = nullptr;

	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);
};
struct DDStruct
{
	std::string name;
	bool serialized = false;
	std::vector<DDParam> params;
	std::vector<DDField> fields;
	int64_t size = 0;
	std::string sizeSrc;
	DDStructResource resource;

	size_t FindFieldByName(StringView name);
	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);
};
struct DDArg
{
	std::string name;
	int64_t intVal;
};
struct DDStructInst
{
	DDStruct* def = nullptr;
	DDFile* file = nullptr;
	int64_t off = 0;
	std::string notes;
	bool userCreated = true;
	bool remainingCountIsSize = false;
	int64_t remainingCount = 1;
	std::vector<DDArg> args;
};


extern ui::DataCategoryTag DCT_Struct[1];
struct DataDesc
{
	struct Image
	{
		DDFile* file = nullptr;
		int64_t offImage = 0;
		int64_t offPalette = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		std::string format;
		std::string notes;
		bool userCreated = true;
	};

	// data
	std::vector<DDFile*> files;
	std::unordered_map<std::string, DDStruct*> structs;
	std::vector<DDStructInst> instances;
	std::vector<Image> images;

	// ID allocation
	uint64_t fileIDAlloc = 0;

	// ui state
	int editMode = 0;
	uint32_t curInst = 0;
	uint32_t curImage = 0;
	uint32_t curField = 0;

	uint64_t GetFixedFieldSize(const DDField& field);

	struct ReadField
	{
		std::string preview;
		int64_t off;
		int64_t count;
		int64_t intVal;
		bool present;
	};
	int64_t ReadStruct(const DDStructInst& SI, std::vector<ReadField>& out, bool computed = true);

	void EditStructuralItems(UIContainer* ctx);
	void EditInstance(UIContainer* ctx);
	void EditStruct(UIContainer* ctx);
	void EditField(UIContainer* ctx);

	void EditImageItems(UIContainer* ctx);

	size_t AddInst(const DDStructInst& src);
	size_t CreateNextInstance(const DDStructInst& SI, int64_t structSize);
	size_t CreateFieldInstance(const DDStructInst& SI, const std::vector<ReadField>& rfs, size_t fieldID);
	void ExpandAllInstances(DDFile* filterFile = nullptr);
	void DeleteAllInstances(DDFile* filterFile = nullptr, DDStruct* filterStruct = nullptr);
	DataDesc::Image GetInstanceImage(const DDStructInst& SI);

	~DataDesc();
	void Clear();
	DDFile* CreateNewFile();
	DDFile* FindFileByID(uint64_t id);
	DDStruct* CreateNewStruct(const std::string& name);
	DDStruct* FindStructByName(const std::string& name);

	void DeleteImage(size_t id);
	size_t DuplicateImage(size_t id);

	void Load(const char* key, NamedTextSerializeReader& r);
	void Save(const char* key, NamedTextSerializeWriter& w);
};

struct DataDescInstanceSource : ui::TableDataSource
{
	size_t GetNumRows() override;
	size_t GetNumCols() override;
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;
	void OnBeginReadRows(size_t startRow, size_t endRow) override;

	void Edit(UIContainer* ctx);

	void _Refilter();

	std::vector<std::vector<DataDesc::ReadField>> _rfsv;
	size_t _startRow = 0;

	std::vector<size_t> _indices;
	bool refilter = true;

	DataDesc* dataDesc = nullptr;

	bool filterStructEnable = true;
	bool filterStructFollow = true;
	DDStruct* filterStruct = nullptr;

	bool filterHideStructsEnable = true;
	std::unordered_set<DDStruct*> filterHideStructs;

	bool filterFileEnable = true;
	bool filterFileFollow = true;
	DDFile* filterFile = nullptr;

	bool filterUserCreated = false;
};

struct DataDescImageSource : ui::TableDataSource
{
	size_t GetNumRows() override;
	size_t GetNumCols() override;
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;

	void Edit(UIContainer* ctx);

	void _Refilter();

	std::vector<size_t> _indices;
	bool refilter = true;

	DataDesc* dataDesc = nullptr;
	DDFile* filterFile = nullptr;
	bool filterUserCreated = false;
};
