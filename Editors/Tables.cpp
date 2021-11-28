
#include <unordered_set>
#include "Tables.h"
#include "../Model/Theme.h"


namespace ui {

float MessageLogDataSource::GetMessageHeight(UIObject* context)
{
	StyleBlock* textStyle = context->styleProps;
	return textStyle->font_size * GetNumLines();
}

float MessageLogDataSource::GetMessageWidth(UIObject* context, size_t msg)
{
	StyleBlock* textStyle = context->styleProps;
	auto* font = textStyle->GetFont();

	float maxW = 0;
	size_t numLines = GetNumLines();
	for (size_t i = 0; i < numLines; i++)
	{
		auto msgLine = GetMessage(msg, i);
		float w = GetTextWidth(font, textStyle->font_size, msgLine);
		maxW = max(maxW, w);
	}
	return maxW;
}

void MessageLogDataSource::OnDrawMessage(UIObject* context, size_t msg, UIRect area)
{
	StyleBlock* textStyle = context->styleProps;
	auto* font = textStyle->GetFont();

	size_t numLines = GetNumLines();
	for (size_t i = 0; i < numLines; i++)
	{
		auto msgLine = GetMessage(msg, i);
		draw::TextLine(
			font,
			textStyle->font_size,
			area.x0, area.y0 + (i + 1) * textStyle->font_size,
			msgLine,
			textStyle->text_color);
	}
}


void MessageLogView::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	size_t numMsgs = _dataSource->GetNumMessages();
	float htMsg = _dataSource->GetMessageHeight(this);

	auto RC = GetContentRect();

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	RC.x1 -= sbw;

	size_t minMsg = floor(yOff / htMsg);
	size_t maxMsg = size_t(ceil((yOff + max(0.0f, sbrect.GetHeight())) / htMsg));
	if (minMsg > numMsgs)
		minMsg = numMsgs;
	if (maxMsg > numMsgs)
		maxMsg = numMsgs;

	draw::PushScissorRect(RC);
	for (size_t msgIdx = minMsg; msgIdx < maxMsg; msgIdx++)
	{
		UIRect rect =
		{
			RC.x0,
			RC.y0 - yOff + htMsg * msgIdx,
			RC.x1,
			RC.y0 - yOff + htMsg * (msgIdx + 1),
		};
		_dataSource->OnDrawMessage(this, msgIdx, rect);
	}
	draw::PopScissorRect();

	scrollbarV.OnPaint({ this, sbrect, RC.GetHeight(), numMsgs * htMsg, yOff });

	ph.PaintChildren(this, ctx);
}

void MessageLogView::OnEvent(Event& e)
{
	size_t numMsgs = _dataSource->GetNumMessages();
	float htMsg = _dataSource->GetMessageHeight(this);

	auto RC = GetContentRect();

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	RC.x1 -= sbw;

	scrollbarV.OnEvent({ this, sbrect, RC.GetHeight(), numMsgs * htMsg, yOff }, e);

	if (e.IsPropagationStopped())
		return;
}

void MessageLogView::OnReset()
{
	Buildable::OnReset();

	flags |= UIObject_SetsChildTextStyle;

	_dataSource = nullptr;
	scrollbarV.OnReset();
}

void MessageLogView::Build()
{
}

MessageLogDataSource* MessageLogView::GetDataSource() const
{
	return _dataSource;
}

void MessageLogView::SetDataSource(MessageLogDataSource* src)
{
	_dataSource = src;
}

bool MessageLogView::IsAtEnd()
{
	size_t numMsgs = _dataSource->GetNumMessages();
	float htMsg = _dataSource->GetMessageHeight(this);
	auto RC = GetContentRect();
	return yOff >= numMsgs * htMsg - RC.GetHeight();
}

void MessageLogView::ScrollToStart()
{
	yOff = 0;
}

void MessageLogView::ScrollToEnd()
{
	size_t numMsgs = _dataSource->GetNumMessages();
	float htMsg = _dataSource->GetMessageHeight(this);
	auto RC = GetContentRect();
	yOff = numMsgs * htMsg - RC.GetHeight();
}


struct TableViewImpl
{
	TableDataSource* dataSource = nullptr;
	ISelectionStorage* selStorage = nullptr;
	IListContextMenuSource* ctxMenuSrc = nullptr;
	bool firstColWidthCalc = true;
	std::vector<float> colEnds = { 1.0f };
	size_t hoverRow = SIZE_MAX;
	SelectionImplementation sel;
};

TableView::TableView()
{
	_impl = new TableViewImpl;
}

TableView::~TableView()
{
	delete _impl;
}

static StaticID_Style sid_table_base("table_base");
static StaticID_Style sid_table_cell("table_cell");
static StaticID_Style sid_table_row_header("table_row_header");
static StaticID_Style sid_table_col_header("table_col_header");
void TableView::OnReset()
{
	Buildable::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = Theme::current->GetStyle(sid_table_base);
	cellStyle = Theme::current->GetStyle(sid_table_cell);
	rowHeaderStyle = Theme::current->GetStyle(sid_table_row_header);
	colHeaderStyle = Theme::current->GetStyle(sid_table_col_header);

	enableRowHeader = true;
	_impl->dataSource = nullptr;
	_impl->selStorage = nullptr;
	_impl->ctxMenuSrc = nullptr;
	_impl->sel.OnReset();
	scrollbarV.OnReset();
}

void TableView::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	PaintInfo info(this);
	ph.PaintBackground(info);

	size_t nc = _impl->dataSource->GetNumCols();
	size_t nr = _impl->dataSource->GetNumRows();

	auto RC = GetContentRect();

	auto padCH = colHeaderStyle->GetPaddingRect();
	auto padC = cellStyle->GetPaddingRect();

	UIRect padRH = {};
	float rhw = 0, rhh = 0;
	if (enableRowHeader)
	{
		padRH = rowHeaderStyle->GetPaddingRect();
		rhw = 80 + padRH.x0 + padRH.x1;
		rhh = 20 + padRH.y0 + padRH.y1;
	}
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = max(rhh, cellh);

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	sbrect.y0 += chh;
	RC.x1 -= sbw;

	size_t minR = floor(yOff / h);
	size_t maxR = size_t(ceil((yOff + max(0.0f, sbrect.GetHeight())) / h));
	if (minR > nr)
		minR = nr;
	if (maxR > nr)
		maxR = nr;

	_impl->dataSource->OnBeginReadRows(minR, maxR);

	if (enableRowHeader)
	{
		// - row header
		draw::PushScissorRect(UIRect{ RC.x0, RC.y0 + chh, RC.x0 + rhw, RC.y1 });
		// background:
		for (size_t r = minR; r < maxR; r++)
		{
			info.rect =
			{
				RC.x0,
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw,
				RC.y0 + chh - yOff + h * (r + 1),
			};
			rowHeaderStyle->background_painter->Paint(info);
		}
		// text:
		auto* rowHeaderFont = rowHeaderStyle->GetFont();
		for (size_t r = minR; r < maxR; r++)
		{
			UIRect rect =
			{
				RC.x0,
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw,
				RC.y0 + chh - yOff + h * (r + 1),
			};
			rect = rect.ShrinkBy(padRH);
			draw::TextLine(
				rowHeaderFont,
				rowHeaderStyle->font_size,
				rect.x0, (rect.y0 + rect.y1 + rowHeaderStyle->font_size) / 2,
				_impl->dataSource->GetRowName(r),
				rowHeaderStyle->text_color);
		}
		draw::PopScissorRect();
	}

	// - column header
	draw::PushScissorRect(UIRect{ RC.x0 + rhw, RC.y0, RC.x1, RC.y0 + chh });
	// background:
	for (size_t c = 0; c < nc; c++)
	{
		info.rect =
		{
			RC.x0 + rhw + _impl->colEnds[c],
			RC.y0,
			RC.x0 + rhw + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		colHeaderStyle->background_painter->Paint(info);
	}
	// text:
	auto* colHeaderFont = colHeaderStyle->GetFont();
	for (size_t c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + rhw + _impl->colEnds[c],
			RC.y0,
			RC.x0 + rhw + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		rect = rect.ShrinkBy(padCH);
		draw::TextLine(
			colHeaderFont,
			colHeaderStyle->font_size,
			rect.x0, (rect.y0 + rect.y1 + colHeaderStyle->font_size) / 2,
			_impl->dataSource->GetColName(c),
			colHeaderStyle->text_color);
	}
	draw::PopScissorRect();

	// - cells
	draw::PushScissorRect(UIRect{ RC.x0 + rhw, RC.y0 + chh, RC.x1, RC.y1 });
	// background:
	for (size_t r = minR; r < maxR; r++)
	{
		for (size_t c = 0; c < nc; c++)
		{
			info.rect =
			{
				RC.x0 + rhw + _impl->colEnds[c],
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[c + 1],
				RC.y0 + chh - yOff + h * (r + 1),
			};
			info.SetChecked(_impl->selStorage ? _impl->selStorage->GetSelectionState(r) : false);
			if (_impl->hoverRow == r)
				info.state |= PS_Hover;
			else
				info.state &= ~PS_Hover;
			cellStyle->background_painter->Paint(info);
		}
	}
	// text:
	auto* cellFont = cellStyle->GetFont();
	for (size_t r = minR; r < maxR; r++)
	{
		for (size_t c = 0; c < nc; c++)
		{
			UIRect rect =
			{
				RC.x0 + rhw + _impl->colEnds[c],
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[c + 1],
				RC.y0 + chh - yOff + h * (r + 1),
			};
			rect = rect.ShrinkBy(padC);
			draw::TextLine(
				cellFont,
				cellStyle->font_size,
				rect.x0, (rect.y0 + rect.y1 + cellStyle->font_size) / 2,
				_impl->dataSource->GetText(r, c),
				cellStyle->text_color);
		}
	}
	draw::PopScissorRect();

	_impl->dataSource->OnEndReadRows(minR, maxR);

	scrollbarV.OnPaint({ this, sbrect, RC.GetHeight(), chh + nr * h, yOff });

	ph.PaintChildren(this, ctx);
}

void TableView::OnEvent(Event& e)
{
	size_t nr = _impl->dataSource->GetNumRows();

	auto RC = GetContentRect();

	auto padCH = colHeaderStyle->GetPaddingRect();
	auto padC = cellStyle->GetPaddingRect();

	float rhw = 0, rhh = 0;
	if (enableRowHeader)
	{
		auto padRH = rowHeaderStyle->GetPaddingRect();
		rhw = 80 + padRH.x0 + padRH.x1;
		rhh = 20 + padRH.y0 + padRH.y1;
	}
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = max(rhh, cellh);

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	sbrect.y0 += chh;
	RC.x1 -= sbw;
	ScrollbarData sbd = { this, sbrect, RC.GetHeight(), chh + nr * h, yOff };
	scrollbarV.OnEvent(sbd, e);

	_PerformDefaultBehaviors(e, UIObject_DB_CaptureMouseOnLeftClick | UIObject_DB_FocusOnLeftClick);

	if (e.type == EventType::ContextMenu)
	{
		auto& CM = ContextMenu::Get();
		if (_impl->hoverRow != SIZE_MAX)
		{
			if (_impl->ctxMenuSrc)
				_impl->ctxMenuSrc->FillItemContextMenu(CM, _impl->hoverRow, 0); // todo col
		}
		if (_impl->ctxMenuSrc)
			_impl->ctxMenuSrc->FillListContextMenu(CM);
	}

	if (e.type == EventType::MouseMove)
	{
		if (e.position.x < RC.x1 && e.position.y > RC.y0 + chh)
			_impl->hoverRow = GetRowAt(e.position.y);
		else
			_impl->hoverRow = SIZE_MAX;
	}
	if (e.type == EventType::MouseLeave)
	{
		_impl->hoverRow = SIZE_MAX;
	}
	if (_impl->sel.OnEvent(e, _impl->selStorage, _impl->hoverRow, e.position.x < RC.x1 && e.position.y > RC.y0 + chh))
	{
		Event selev(e.context, this, EventType::SelectionChange);
		e.context->BubblingEvent(selev);
	}
}

void TableView::Build()
{
}

TableDataSource* TableView::GetDataSource() const
{
	return _impl->dataSource;
}

void TableView::SetDataSource(TableDataSource* src)
{
	if (src != _impl->dataSource)
		_impl->firstColWidthCalc = true;
	_impl->dataSource = src;

	size_t nc = src->GetNumCols();
	while (_impl->colEnds.size() < nc + 1)
		_impl->colEnds.push_back(_impl->colEnds.back() + 100);
	while (_impl->colEnds.size() > nc + 1)
		_impl->colEnds.pop_back();

	Rebuild();
}

ISelectionStorage* TableView::GetSelectionStorage() const
{
	return _impl->selStorage;
}

void TableView::SetSelectionStorage(ISelectionStorage* src)
{
	_impl->selStorage = src;
}

void TableView::SetSelectionMode(SelectionMode mode)
{
	_impl->sel.selectionMode = mode;
}

IListContextMenuSource* TableView::GetContextMenuSource() const
{
	return _impl->ctxMenuSrc;
}

void TableView::SetContextMenuSource(IListContextMenuSource* src)
{
	_impl->ctxMenuSrc = src;
}

void TableView::CalculateColumnWidths(bool includeHeader, bool firstTimeOnly)
{
	if (firstTimeOnly && !_impl->firstColWidthCalc)
		return;

	_impl->firstColWidthCalc = false;

	auto nc = _impl->dataSource->GetNumCols();
	std::vector<float> colWidths;
	colWidths.resize(nc, 0.0f);

	auto RC = GetContentRect();
	auto padCH = colHeaderStyle->GetPaddingRect();
	auto padC = cellStyle->GetPaddingRect();

	if (includeHeader)
	{
		auto* rowHeaderFont = rowHeaderStyle->GetFont();
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetColName(c);
			float w = GetTextWidth(rowHeaderFont, rowHeaderStyle->font_size, text) + padCH.x0 + padCH.x1;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
	}

	auto* cellFont = cellStyle->GetFont();
	for (size_t i = 0, n = _impl->dataSource->GetNumRows(); i < n; i++)
	{
		_impl->dataSource->OnBeginReadRows(i, i + 1);
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetText(i, c);
			float w = GetTextWidth(cellFont, cellStyle->font_size, text) + padC.x0 + padC.x1;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
		_impl->dataSource->OnEndReadRows(i, i + 1);
	}

	for (size_t c = 0; c < nc; c++)
		_impl->colEnds[c + 1] = _impl->colEnds[c] + colWidths[c];
}

bool TableView::IsValidRow(uintptr_t pos)
{
	return pos < _impl->dataSource->GetNumRows();
}

size_t TableView::GetRowAt(float y)
{
	auto RC = GetContentRect();

	auto padCH = colHeaderStyle->GetPaddingRect();
	auto padC = cellStyle->GetPaddingRect();

	float rhh = 0;
	if (enableRowHeader)
	{
		auto padRH = rowHeaderStyle->GetPaddingRect();
		rhh = 20 + padRH.y0 + padRH.y1;
	}
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = max(rhh, cellh);

	y += yOff;
	y -= RC.y0;
	y -= chh;
	y = floor(y / h);
	size_t row = y;
	size_t numRows = _impl->dataSource->GetNumRows();
	return row < numRows ? row : SIZE_MAX;
}

UIRect TableView::GetCellRect(size_t col, size_t row)
{
	size_t numCols = _impl->dataSource->GetNumCols();
	if (col >= numCols && col != SIZE_MAX)
		col = numCols ? numCols - 1 : 0;
	size_t numRows = _impl->dataSource->GetNumRows();
	if (row >= numRows)
		row = numRows ? numRows - 1 : 0;

	auto RC = GetContentRect();

	auto padCH = colHeaderStyle->GetPaddingRect();
	auto padC = cellStyle->GetPaddingRect();

	float rhw = 0, rhh = 0;
	if (enableRowHeader)
	{
		auto padRH = rowHeaderStyle->GetPaddingRect();
		rhw = 80 + padRH.x0 + padRH.x1;
		rhh = 20 + padRH.y0 + padRH.y1;
	}
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = max(rhh, cellh);

	float x0, x1;
	if (col == SIZE_MAX)
	{
		x0 = RC.x0;
		x1 = x0 + rhw;
	}
	else
	{
		x0 = RC.x0 + rhw + _impl->colEnds[col];
		x1 = RC.x0 + rhw + _impl->colEnds[col + 1];
	}
	float y0 = RC.y0 + chh + h * row - yOff;
	float y1 = y0 + h;

	return { x0, y0, x1, y1 };
}

size_t TableView::GetHoverRow() const
{
	return _impl->hoverRow;
}


struct TreeView::PaintState
{
	PaintInfo info;
	int nc;
	float x;
	float y;
};

struct TreeViewImpl
{
	TreeDataSource* dataSource = nullptr;
	bool firstColWidthCalc = true;
	std::vector<float> colEnds = { 1.0f };
};

TreeView::TreeView()
{
	_impl = new TreeViewImpl;
}

TreeView::~TreeView()
{
	delete _impl;
}

static StaticID_Style sid_tree_expand("tree_expand");
void TreeView::OnReset()
{
	Buildable::OnReset();

	flags |= UIObject_SetsChildTextStyle;

	styleProps = Theme::current->GetStyle(sid_table_base); // TODO separate styles for TreeView
	cellStyle = Theme::current->GetStyle(sid_table_cell);
	expandButtonStyle = Theme::current->GetStyle(sid_tree_expand);
	colHeaderStyle = Theme::current->GetStyle(sid_table_col_header);

	_impl->dataSource = nullptr;
}

void TreeView::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	int chh = 20;
	int h = 20;

	int nc = _impl->dataSource->GetNumCols();
	int nr = 0;// _dataSource->GetNumRows();

	PaintInfo info(this);

	auto RC = GetContentRect();

	// backgrounds
	for (int c = 0; c < nc; c++)
	{
		info.rect =
		{
			RC.x0 + _impl->colEnds[c],
			RC.y0,
			RC.x0 + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		colHeaderStyle->background_painter->Paint(info);
	}

	// text
	auto* colHeaderFont = colHeaderStyle->GetFont();
	for (int c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + _impl->colEnds[c],
			RC.y0,
			RC.x0 + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		draw::TextLine(
			colHeaderFont,
			colHeaderStyle->font_size,
			rect.x0, (rect.y0 + rect.y1 + colHeaderStyle->font_size) / 2,
			_impl->dataSource->GetColName(c),
			colHeaderStyle->text_color);
	}

	PaintState ps = { info, nc, RC.x0, RC.y0 + chh };
	for (size_t i = 0, n = _impl->dataSource->GetChildCount(TreeDataSource::ROOT); i < n; i++)
		_PaintOne(_impl->dataSource->GetChild(TreeDataSource::ROOT, i), 0, ps);

	ph.PaintChildren(this, ctx);
}

void TreeView::_PaintOne(uintptr_t id, int lvl, PaintState& ps)
{
	int h = 20;
	int tab = 20;

	// backgrounds
	for (int c = 0; c < ps.nc; c++)
	{
		ps.info.rect =
		{
			ps.x + _impl->colEnds[c],
			ps.y,
			ps.x + _impl->colEnds[c + 1],
			ps.y + h,
		};
		cellStyle->background_painter->Paint(ps.info);
	}

	// text
	auto* cellFont = cellStyle->GetFont();
	for (int c = 0; c < ps.nc; c++)
	{
		UIRect rect =
		{
			ps.x + _impl->colEnds[c],
			ps.y,
			ps.x + _impl->colEnds[c + 1],
			ps.y + h,
		};
		if (c == 0)
			rect.x0 += tab * lvl;
		draw::TextLine(
			cellFont,
			cellStyle->font_size,
			rect.x0, (rect.y0 + rect.y1 + cellStyle->font_size) / 2,
			_impl->dataSource->GetText(id, c),
			cellStyle->text_color);
	}

	ps.y += h;

	if (true) // open
	{
		for (size_t i = 0, n = _impl->dataSource->GetChildCount(id); i < n; i++)
			_PaintOne(_impl->dataSource->GetChild(id, i), lvl + 1, ps);
	}
}

void TreeView::OnEvent(Event& e)
{
	if (e.type == EventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
}

void TreeView::Build()
{
}

TreeDataSource* TreeView::GetDataSource() const
{
	return _impl->dataSource;
}

void TreeView::SetDataSource(TreeDataSource* src)
{
	if (src != _impl->dataSource)
		_impl->firstColWidthCalc = true;
	_impl->dataSource = src;

	size_t nc = src->GetNumCols();
	while (_impl->colEnds.size() < nc + 1)
		_impl->colEnds.push_back(_impl->colEnds.back() + 100);
	while (_impl->colEnds.size() > nc + 1)
		_impl->colEnds.pop_back();

	Rebuild();
}

void TreeView::CalculateColumnWidths(bool firstTimeOnly)
{
	if (firstTimeOnly && !_impl->firstColWidthCalc)
		return;

	_impl->firstColWidthCalc = false;

	int tab = 20;

	auto nc = _impl->dataSource->GetNumCols();
	std::vector<float> colWidths;
	colWidths.resize(nc, 0.0f);

	auto* cellFont = cellStyle->GetFont();
	auto cellFontSize = cellStyle->font_size;

	struct Entry
	{
		uintptr_t id;
		int lev;
	};
	std::vector<Entry> stack = { Entry{ TreeDataSource::ROOT, 0 } };
	while (!stack.empty())
	{
		Entry E = stack.back();
		stack.pop_back();

		for (size_t i = 0, n = _impl->dataSource->GetChildCount(E.id); i < n; i++)
		{
			uintptr_t chid = _impl->dataSource->GetChild(E.id, i);
			stack.push_back({ chid, E.lev + 1 });

			for (size_t c = 0; c < nc; c++)
			{
				std::string text = _impl->dataSource->GetText(chid, c);
				float w = GetTextWidth(cellFont, cellFontSize, text);
				if (c == 0)
					w += tab * E.lev;
				if (colWidths[c] < w)
					colWidths[c] = w;
			}
		}
	}

	for (size_t c = 0; c < nc; c++)
		_impl->colEnds[c + 1] = _impl->colEnds[c] + colWidths[c];
}

} // ui
