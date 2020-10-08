
#pragma once
#include "pch.h"
#include "FileReaders.h"


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
	uint8_t bitstart = 0;
	uint8_t bitend = 64;
	bool excludeZeroes = false;
	uint64_t at;
	uint64_t count;
	uint64_t repeats;
	uint64_t stride;
	std::string notes;

	bool Contains(uint64_t pos) const;
	unsigned ContainInfo(uint64_t pos) const; // 1 - overlap, 2 - left edge, 4 - right edge
	uint64_t GetEnd() const;
	ui::Color4f GetColor() const;
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

struct MarkerDataSource : ui::TableDataSource, ui::ISelectionStorage
{
	size_t GetNumRows() override { return data->markers.size(); }
	size_t GetNumCols() override;
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;

	void ClearSelection() override;
	bool GetSelectionState(size_t row) override;
	void SetSelectionState(size_t row, bool sel) override;

	IDataSource* dataSource;
	MarkerData* data;
	size_t selected = SIZE_MAX;
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
