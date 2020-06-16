
#pragma once
#include "pch.h"
#include "FileReaders.h"
#include "MathExpr.h"
#include "ImageParsers.h"


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

using CacheVersion = uint32_t;
constexpr int64_t F_NO_VALUE = -1;


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
	int64_t Evaluate(IVariableSource& vs) const
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
struct DDConditionExt
{
	std::string field;
	std::string value;
	MathExprObj expr;
	bool useExpr = false;
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
	struct FormatOverride
	{
		std::string format;
		std::vector<DDConditionExt> conditions;
	};
	std::string format;
	std::vector<FormatOverride> formatOverrides;
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

	CacheVersion editVersionS = 1;

	size_t GetFieldCount() const { return fields.size(); }
	size_t FindFieldByName(StringView name);
	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);
};
struct DDArg
{
	std::string name;
	int64_t intVal;
};
enum class CreationReason : uint8_t
{
	UserDefined,
	ManualExpand,
	AutoExpand,
	Query,
};
enum class OptionalBool : int8_t
{
	Unknown = -1,
	False = 0,
	True = 1,
};
struct DDReadFieldValue
{
	std::string preview;
	int64_t off;
	int64_t intVal;
};
struct DDReadField
{
	std::vector<DDReadFieldValue> values;
	std::string preview;
	int64_t off = F_NO_VALUE;
	int64_t count = F_NO_VALUE;
	int64_t totalSize = F_NO_VALUE;
	int64_t readOff = F_NO_VALUE;
	bool present;
};
struct DDStructInst
{
	int64_t id = -1;
	DataDesc* desc = nullptr;
	DDStruct* def = nullptr;
	DDFile* file = nullptr;
	int64_t off = 0;
	std::string notes;
	CreationReason creationReason = CreationReason::UserDefined;
	bool remainingCountIsSize = false;
	int64_t remainingCount = 1;
	bool sizeOverrideEnable = false;
	int64_t sizeOverrideValue = 0;
	std::vector<DDArg> args;

	CacheVersion editVersionSI = 1;
	mutable CacheVersion cacheSizeVersionSI = 0;
	mutable CacheVersion cacheSizeVersionS = 0;
	mutable int64_t cachedSize = F_NO_VALUE;
	mutable CacheVersion cacheFieldsVersionSI = 0;
	mutable CacheVersion cacheFieldsVersionS = 0;
	mutable int64_t cachedReadOff = F_NO_VALUE;
	mutable std::vector<DDReadField> cachedFields;

	std::string GetFieldDescLazy(size_t i, bool* incomplete = nullptr) const;
	int64_t GetSize() const;
	bool IsFieldPresent(size_t i) const;
	OptionalBool IsFieldPresent(size_t i, bool lazy) const;
	const std::string& GetFieldPreview(size_t i, bool lazy = false) const;
	const std::string& GetFieldValuePreview(size_t i, size_t n = 0) const;
	int64_t GetFieldIntValue(size_t i, size_t n = 0) const;
	int64_t GetFieldOffset(size_t i, bool lazy = false) const;
	int64_t GetFieldElementCount(size_t i, bool lazy = false) const;
	int64_t GetFieldTotalSize(size_t i, bool lazy = false) const;
	bool EvaluateCondition(const DDCondition& cond, size_t until = SIZE_MAX) const;
	bool EvaluateConditions(const std::vector<DDCondition>& conds, size_t until = SIZE_MAX) const;
	bool EvaluateCondition(const DDConditionExt& cond, size_t until = SIZE_MAX) const;
	bool EvaluateConditions(const std::vector<DDConditionExt>& conds, size_t until = SIZE_MAX) const;
	int64_t GetCompArgValue(const DDCompArg& arg) const;
	DDStructInst* CreateFieldInstance(size_t i, CreationReason cr) const;

	void _CheckFieldCache() const;
	void _EnumerateFields(size_t untilNum, bool lazy = false) const;
	int64_t _CalcSize() const;
	int64_t _CalcFieldElementCount(size_t i) const;
	bool _CanReadMoreFields(size_t i) const;
	bool _ReadFieldValues(size_t i, size_t n) const;

	void OnEdit()
	{
		editVersionSI++;
	}
};


extern ui::DataCategoryTag DCT_Struct[1];
extern ui::DataCategoryTag DCT_CurStructInst[1];
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
	std::vector<DDStructInst*> instances;
	std::vector<Image> images;

	// ID allocation
	uint64_t fileIDAlloc = 0;
	int64_t instIDAlloc = 0;

	// ui state
	int editMode = 0;
	DDStructInst* curInst = nullptr;
	uint32_t curImage = 0;
	uint32_t curField = 0;

	uint64_t GetFixedTypeSize(const std::string& type);

	void EditStructuralItems(UIContainer* ctx);
	void EditInstance(UIContainer* ctx);
	void EditStruct(UIContainer* ctx);
	void EditField(UIContainer* ctx);

	void EditImageItems(UIContainer* ctx);

	DDStructInst* AddInstance(const DDStructInst& src);
	void DeleteInstance(DDStructInst* inst);
	void SetCurrentInstance(DDStructInst* inst);
	void _OnDeleteInstance(DDStructInst* inst);
	DDStructInst* CreateNextInstance(const DDStructInst& SI, int64_t structSize, CreationReason cr);
	void ExpandAllInstances(DDFile* filterFile = nullptr);
	void DeleteAllInstances(DDFile* filterFile = nullptr, DDStruct* filterStruct = nullptr);
	DataDesc::Image GetInstanceImage(const DDStructInst& SI);

	~DataDesc();
	void Clear();
	DDFile* CreateNewFile();
	DDFile* FindFileByID(uint64_t id);
	DDStruct* CreateNewStruct(const std::string& name);
	DDStruct* FindStructByName(const std::string& name);
	DDStructInst* FindInstanceByID(int64_t id);

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

	void Edit(UIContainer* ctx);

	void _Refilter();

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

	uint32_t showBytes = 0;

	CreationReason filterCreationReason = CreationReason::Query;
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

struct CachedImage
{
	~CachedImage()
	{
		delete curImg;
	}
	ui::Image* GetImage(const DataDesc::Image& imgDesc)
	{
		if (curImg)
		{
			if (imgDesc.file == curImgDesc.file &&
				imgDesc.offImage == curImgDesc.offImage &&
				imgDesc.offPalette == curImgDesc.offPalette &&
				imgDesc.format == curImgDesc.format &&
				imgDesc.width == curImgDesc.width &&
				imgDesc.height == curImgDesc.height)
			{
				return curImg;
			}
		}

		delete curImg;
		curImg = CreateImageFrom(imgDesc.file->dataSource, imgDesc.format.c_str(), { imgDesc.offImage, imgDesc.offPalette, imgDesc.width, imgDesc.height });
		curImgDesc = imgDesc;
		return curImg;
	}

	ui::Image* curImg = nullptr;
	DataDesc::Image curImgDesc;
};
