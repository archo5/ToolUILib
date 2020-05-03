
#pragma once
#include "pch.h"


extern Color4f colorFloat32;
extern Color4f colorInt16;
extern Color4f colorInt32;
extern Color4f colorASCII;


struct IDataSource;
struct BuiltinTypeInfo;


enum DataType
{
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
};

struct Marker
{
	DataType type;
	uint64_t at;
	uint64_t count;
	uint64_t repeats;
	uint64_t stride;

	bool Contains(uint64_t pos) const;
	Color4f GetColor() const;
};

extern ui::DataCategoryTag DCT_Marker[1];
extern ui::DataCategoryTag DCT_MarkedItems[1];
struct MarkerData : ui::TableDataSource
{
	Color4f GetMarkedColor(uint64_t pos);
	bool IsMarked(uint64_t pos, uint64_t len);
	void AddMarker(DataType dt, uint64_t from, uint64_t to);

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
	struct Field
	{
		std::string type;
		std::string name;
		uint64_t off = 0;
		int64_t count = 1;
		std::string countSrc;
		bool countIsMaxSize = false;
	};
	struct Struct
	{
		std::string name;
		bool serialized = false;
		std::vector<Param> params;
		std::vector<Field> fields;
		uint64_t size = 0;
	};
	struct StructInst
	{
		Struct* def = nullptr;
		uint64_t off = 0;
		std::string notes;
		bool userCreated = true;
		std::vector<Arg> args;
	};

	// data
	std::unordered_map<std::string, Struct*> structs;
	std::vector<StructInst> instances;

	// ui state
	int editMode = 0;
	uint32_t curInst = 0;
	uint32_t curField = 0;

	uint64_t GetFixedFieldSize(const Field& field);

	struct ReadField
	{
		std::string preview;
		uint64_t off;
		int64_t count;
		int64_t intVal;
	};
	static void ReadBuiltinFieldPrimary(IDataSource* ds, const BuiltinTypeInfo& BTI, ReadField& rf);
	static int64_t GetFieldCount(IDataSource* ds, const StructInst& SI, const Field& F, const std::vector<ReadField>& rfs);
	static void ReadStruct(IDataSource* ds, const StructInst& SI, uint64_t off, std::vector<ReadField>& out);

	void Edit(UIContainer* ctx, IDataSource* ds);
	void EditInstance(UIContainer* ctx, IDataSource* ds);
	void EditStruct(UIContainer* ctx);
	void EditField(UIContainer* ctx);

	size_t AddInst(const std::string& name, uint64_t off, bool userCreated);
};
