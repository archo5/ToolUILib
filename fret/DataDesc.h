
#pragma once
#include "pch.h"
#include "FileReaders.h"


extern Color4f colorFloat32;
extern Color4f colorInt16;
extern Color4f colorInt32;
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

	bool Contains(uint64_t pos) const;
	unsigned ContainInfo(uint64_t pos) const; // 1 - overlap, 2 - left edge, 4 - right edge
	uint64_t GetEnd() const;
	Color4f GetColor() const;
};

extern ui::DataCategoryTag DCT_Marker[1];
extern ui::DataCategoryTag DCT_MarkedItems[1];
struct MarkerData : ui::TableDataSource
{
	void AddMarker(DataType dt, uint64_t from, uint64_t to);

	void Load(const char* key, NamedTextSerializeReader& r);
	void Save(const char* key, NamedTextSerializeWriter& w);

	size_t GetNumRows() override { return markers.size(); }
	size_t GetNumCols() override { return 5; }
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;

	std::vector<Marker> markers;
};

struct MarkedItemEditor : ui::Node
{
	void Render(UIContainer* ctx) override;

	Marker* marker;
};

struct MarkedItemsList : ui::Node
{
	void Render(UIContainer* ctx) override;

	MarkerData* markerData;
};


extern ui::DataCategoryTag DCT_Struct[1];
struct DataDesc
{
	struct File
	{
		uint64_t id = UINT64_MAX;
		std::string name;
		IDataSource* dataSource = nullptr;
		MarkerData markerData;
	};
	struct Param
	{
		std::string name;
		int64_t intVal;
	};
	struct Arg
	{
		std::string name;
		int64_t intVal;
	};
	struct CompArg
	{
		std::string name;
		std::string src;
		int64_t intVal;
	};
	struct Condition
	{
		std::string field;
		std::string value;
	};
	struct Field
	{
		std::string type;
		std::string name;
		int64_t off = 0;
		int64_t count = 1;
		std::string countSrc;
		bool countIsMaxSize = false;
		bool readUntil0 = false;
		std::vector<CompArg> structArgs;
		std::vector<Condition> conditions;
	};
	struct Struct
	{
		std::string name;
		bool serialized = false;
		std::vector<Param> params;
		std::vector<Field> fields;
		int64_t size = 0;
		std::string sizeSrc;
	};
	struct StructInst
	{
		Struct* def = nullptr;
		File* file = nullptr;
		int64_t off = 0;
		std::string notes;
		bool userCreated = true;
		bool remainingCountIsSize = false;
		int64_t remainingCount = 1;
		std::vector<Arg> args;
	};
	struct Image
	{
		File* file = nullptr;
		int64_t offImage = 0;
		int64_t offPalette = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		std::string format;
		std::string notes;
		bool userCreated = true;
	};

	// data
	std::vector<File*> files;
	std::unordered_map<std::string, Struct*> structs;
	std::vector<StructInst> instances;
	std::vector<Image> images;

	// ID allocation
	uint64_t fileIDAlloc = 0;

	// ui state
	int editMode = 0;
	uint32_t curInst = 0;
	uint32_t curImage = 0;
	uint32_t curField = 0;

	uint64_t GetFixedFieldSize(const Field& field);

	struct ReadField
	{
		std::string preview;
		int64_t off;
		int64_t count;
		int64_t intVal;
		bool present;
	};
	int64_t ReadStruct(IDataSource* ds, const StructInst& SI, int64_t off, std::vector<ReadField>& out);

	void EditStructuralItems(UIContainer* ctx);
	void EditInstance(UIContainer* ctx);
	void EditStruct(UIContainer* ctx);
	void EditField(UIContainer* ctx);

	void EditImageItems(UIContainer* ctx);

	size_t AddInst(const StructInst& src);
	size_t CreateNextInstance(const StructInst& SI, int64_t structSize);
	size_t CreateFieldInstance(const StructInst& SI, const std::vector<ReadField>& rfs, size_t fieldID);
	void ExpandAllInstances(File* filterFile = nullptr);
	void DeleteAllInstances(File* filterFile = nullptr, Struct* filterStruct = nullptr);

	~DataDesc();
	void Clear();
	File* CreateNewFile();
	File* FindFileByID(uint64_t id);
	Struct* CreateNewStruct(const std::string& name);
	Struct* FindStructByName(const std::string& name);

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

	std::vector<DataDesc::ReadField> _rfs;
	std::vector<size_t> _indices;
	bool refilter = true;

	DataDesc* dataDesc = nullptr;
	DataDesc::Struct* filterStruct = nullptr;
	DataDesc::File* filterFile = nullptr;
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
	DataDesc::File* filterFile = nullptr;
	bool filterUserCreated = false;
};
