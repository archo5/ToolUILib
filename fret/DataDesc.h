
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
	DT_I16,
	DT_U16,
	DT_I32,
	DT_U32,
	DT_F32,
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

struct MarkedItemsList : ui::Node
{
	void Render(UIContainer* ctx) override;

	MarkerData* markerData;
};


extern ui::DataCategoryTag DCT_Struct[1];
struct DataDesc
{
	struct Field
	{
		std::string type;
		std::string name;
		uint64_t count = 1;
		uint64_t off = 0;
	};
	struct Struct
	{
		bool serialized = false;
		std::vector<Field> fields;
		uint64_t size = 0;
	};
	struct StructInst
	{
		std::string type;
		uint64_t off;
		std::string notes;
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
	};
	static void ReadBuiltinFieldPreview(IDataSource* ds, uint64_t off, uint64_t count, const BuiltinTypeInfo& BTI, std::string& outPreview);
	void ReadStruct(IDataSource* ds, const Struct& S, uint64_t off, std::vector<ReadField>& out);
	void Edit(UIContainer* ctx, IDataSource* ds);
};
