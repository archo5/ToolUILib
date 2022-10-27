
#pragma once
#include "EditCommon.h"
#include "../Model/Objects.h"
#include "../Model/Controls.h"


namespace ui {

struct MessageLogDataSource
{
	FontSettings messageFont;
	Color4b messageColor = Color4b::White();

	virtual size_t GetNumMessages() = 0;
	virtual std::string GetMessage(size_t msg, size_t line) = 0;

	// customizable parts with default implementations
	virtual size_t GetNumLines() { return 1; }
	virtual float GetMessageHeight(UIObject* context);
	virtual float GetMessageWidth(UIObject* context, size_t msg);
	virtual void OnDrawMessage(UIObject* context, size_t msg, UIRect area);
};

struct MessageLogView : FillerElement
{
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
	void OnReset() override;

	MessageLogDataSource* GetDataSource() const;
	void SetDataSource(MessageLogDataSource* src);

	bool IsAtEnd();
	void ScrollToStart();
	void ScrollToEnd();

	ScrollbarV scrollbarV;
	float yOff = 0;

	MessageLogDataSource* _dataSource = nullptr;
};


struct TableStyle
{
	static constexpr const char* NAME = "TableStyle";

	AABB2f cellPadding;
	AABB2f rowHeaderPadding;
	AABB2f colHeaderPadding;
	PainterHandle cellBackgroundPainter;
	PainterHandle rowHeaderBackgroundPainter;
	PainterHandle colHeaderBackgroundPainter;
	FontSettings cellFont;
	FontSettings rowHeaderFont;
	FontSettings colHeaderFont;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct GenericGridDataSource
{
	struct TreeElementRef
	{
		uintptr_t id;
		size_t depth;
	};

	virtual bool IsTree() = 0;

	// columns
	virtual size_t GetNumCols() = 0;
	virtual size_t GetTreeCol() { return SIZE_MAX; }
	virtual std::string GetColName(size_t col) = 0;

	// getting viewed elements
	virtual size_t GetElements(Range<size_t> orderRange, Array<TreeElementRef>& outElemList) = 0;

	// cell contents
	virtual std::string GetText(uintptr_t id, size_t col) = 0;

	// row extras
	virtual std::string GetRowName(size_t row, uintptr_t id);
	virtual Optional<bool> GetOpenState(uintptr_t id) = 0;
	virtual void ToggleOpenState(uintptr_t id) = 0;
};

struct TableDataSource : GenericGridDataSource
{
	virtual size_t GetNumRows() = 0;
	virtual std::string GetRowName(size_t row) = 0;

	bool IsTree() override { return false; }
	size_t GetElements(Range<size_t> orderRange, Array<TreeElementRef>&) override;
	std::string GetRowName(size_t row, uintptr_t id) override { return GetRowName(row); }
	Optional<bool> GetOpenState(uintptr_t id) override { return {}; }
	void ToggleOpenState(uintptr_t id) override {}
};

struct TreeDataSource : GenericGridDataSource
{
	static constexpr uintptr_t ROOT = uintptr_t(-1);

	virtual size_t GetChildCount(uintptr_t id) = 0;
	virtual uintptr_t GetChild(uintptr_t id, size_t which) = 0;
	virtual bool IsOpen(uintptr_t id) { return true; }
	void ToggleOpenState(uintptr_t id) override {}

	bool IsTree() override { return true; }
	size_t GetTreeCol() override { return 0; }
	size_t GetElements(Range<size_t> orderRange, Array<TreeElementRef>& outElemList) override;
	Optional<bool> GetOpenState(uintptr_t id) override;
};

struct TableView : FrameElement
{
	TableStyle style;
	IconStyle expandButtonStyle;

	TableView();
	~TableView();
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	GenericGridDataSource* GetDataSource() const;
	void SetDataSource(GenericGridDataSource* src);
	ISelectionStorage* GetSelectionStorage() const;
	void SetSelectionStorage(ISelectionStorage* src);
	void SetSelectionMode(SelectionMode mode);
	IListContextMenuSource* GetContextMenuSource() const;
	void SetContextMenuSource(IListContextMenuSource* src);

	void CalculateColumnWidths(bool includeHeader = true, bool firstTimeOnly = true);

	bool IsValidRow(uintptr_t pos);
	size_t GetRowAt(float y);
	UIRect GetCellRect(size_t col, size_t row);
	Range<size_t> GetVisibleRange() const;
	size_t GetHoverRow() const;

	bool enableRowHeader = true;
	ScrollbarV scrollbarV;
	float yOff = 0;

private:
	struct TableViewImpl* _impl;
};

} // ui
