
#pragma once
#include "Objects.h"
#include "Controls.h"


namespace ui {

struct Selection1D
{
	Selection1D();
	~Selection1D();
	void OnSerialize(IDataSerializer& s);
	void Clear();
	bool AnySelected();
	uintptr_t GetFirstSelection();
	bool IsSelected(uintptr_t id);
	void SetSelected(uintptr_t id, bool sel);

	struct Selection1DImpl* _impl;
};

struct TableDataSource
{
	virtual size_t GetNumRows() = 0;
	virtual size_t GetNumCols() = 0;
	virtual std::string GetRowName(size_t row) = 0;
	virtual std::string GetColName(size_t col) = 0;
	virtual std::string GetText(size_t row, size_t col) = 0;
};

class TableView : public ui::Node
{
public:
	TableView();
	~TableView();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnSerialize(IDataSerializer& s) override;
	void Render(UIContainer* ctx) override;

	TableDataSource* GetDataSource() const;
	void SetDataSource(TableDataSource* src);
	void CalculateColumnWidths(bool includeHeader = true, bool firstTimeOnly = true);

	bool IsValidRow(uintptr_t pos);
	size_t GetRowAt(float y);
	size_t GetHoverRow() const;

	style::BlockRef cellStyle;
	style::BlockRef rowHeaderStyle;
	style::BlockRef colHeaderStyle;
	Selection1D selection;
	ScrollbarV scrollbarV;
	float yOff = 0;

private:
	struct TableViewImpl* _impl;
};

class TreeDataSource
{
public:
	static constexpr uintptr_t ROOT = uintptr_t(-1);

	virtual size_t GetNumCols() = 0;
	virtual std::string GetColName(size_t col) = 0;
	virtual size_t GetChildCount(uintptr_t id) = 0;
	virtual uintptr_t GetChild(uintptr_t id, size_t which) = 0;
	virtual std::string GetText(uintptr_t id, size_t col) = 0;
};

class TreeView : public ui::Node
{
public:
	struct PaintState;

	TreeView();
	~TreeView();
	void OnPaint() override;
	void _PaintOne(uintptr_t id, int lvl, PaintState& ps);
	void OnEvent(UIEvent& e) override;
	void Render(UIContainer* ctx) override;

	TreeDataSource* GetDataSource() const;
	void SetDataSource(TreeDataSource* src);
	void CalculateColumnWidths(bool firstTimeOnly = true);

	style::BlockRef cellStyle;
	style::BlockRef expandButtonStyle;
	style::BlockRef colHeaderStyle;

private:
	struct TreeViewImpl* _impl;
};

} // ui
