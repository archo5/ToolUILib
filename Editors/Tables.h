
#pragma once
#include "EditCommon.h"
#include "../Model/Objects.h"
#include "../Model/Controls.h"


namespace ui {

struct MessageLogDataSource
{
	virtual size_t GetNumMessages() = 0;
	virtual std::string GetMessage(size_t msg, size_t line) = 0;

	// customizable parts with default implementations
	virtual size_t GetNumLines() { return 1; }
	virtual float GetMessageHeight(UIObject* context);
	virtual float GetMessageWidth(UIObject* context, size_t msg);
	virtual void OnDrawMessage(UIObject* context, size_t msg, UIRect area);
};

struct MessageLogView : Buildable
{
	void OnPaint() override;
	void OnEvent(Event& e) override;
	void OnSerialize(IDataSerializer& s) override;
	void Build(UIContainer* ctx) override;

	MessageLogDataSource* GetDataSource() const;
	void SetDataSource(MessageLogDataSource* src);

	bool IsAtEnd();
	void ScrollToStart();
	void ScrollToEnd();

	ScrollbarV scrollbarV;
	float yOff = 0;

	MessageLogDataSource* _dataSource;
};


struct TableDataSource
{
	virtual size_t GetNumRows() = 0;
	virtual size_t GetNumCols() = 0;
	virtual std::string GetRowName(size_t row) = 0;
	virtual std::string GetColName(size_t col) = 0;
	virtual std::string GetText(size_t row, size_t col) = 0;
	// cache interface
	virtual void OnBeginReadRows(size_t startRow, size_t endRow) {}
	virtual void OnEndReadRows(size_t startRow, size_t endRow) {}
};

struct TableView : Buildable
{
	TableView();
	~TableView();
	void OnPaint() override;
	void OnEvent(Event& e) override;
	void OnSerialize(IDataSerializer& s) override;
	void Build(UIContainer* ctx) override;

	TableDataSource* GetDataSource() const;
	void SetDataSource(TableDataSource* src);
	ISelectionStorage* GetSelectionStorage() const;
	void SetSelectionStorage(ISelectionStorage* src);
	void SetSelectionMode(SelectionMode mode);
	IListContextMenuSource* GetContextMenuSource() const;
	void SetContextMenuSource(IListContextMenuSource* src);

	void CalculateColumnWidths(bool includeHeader = true, bool firstTimeOnly = true);

	bool IsValidRow(uintptr_t pos);
	size_t GetRowAt(float y);
	UIRect GetCellRect(size_t col, size_t row);
	size_t GetHoverRow() const;

	bool enableRowHeader = true;
	style::BlockRef cellStyle;
	style::BlockRef rowHeaderStyle;
	style::BlockRef colHeaderStyle;
	ScrollbarV scrollbarV;
	float yOff = 0;

private:
	struct TableViewImpl* _impl;
};


struct TreeDataSource
{
	static constexpr uintptr_t ROOT = uintptr_t(-1);

	virtual size_t GetNumCols() = 0;
	virtual std::string GetColName(size_t col) = 0;
	virtual size_t GetChildCount(uintptr_t id) = 0;
	virtual uintptr_t GetChild(uintptr_t id, size_t which) = 0;
	virtual std::string GetText(uintptr_t id, size_t col) = 0;
};

struct TreeView : Buildable
{
	struct PaintState;

	TreeView();
	~TreeView();
	void OnPaint() override;
	void _PaintOne(uintptr_t id, int lvl, PaintState& ps);
	void OnEvent(Event& e) override;
	void Build(UIContainer* ctx) override;

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
