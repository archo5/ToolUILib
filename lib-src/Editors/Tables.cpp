
#include <unordered_set>
#include "Tables.h"
#include "../Model/Theme.h"


namespace ui {

float MessageLogDataSource::GetMessageHeight(UIObject* context)
{
	return messageFont.size * GetNumLines();
}

float MessageLogDataSource::GetMessageWidth(UIObject* context, size_t msg)
{
	auto* font = messageFont.GetFont();

	float maxW = 0;
	size_t numLines = GetNumLines();
	for (size_t i = 0; i < numLines; i++)
	{
		auto msgLine = GetMessage(msg, i);
		float w = GetTextWidth(font, messageFont.size, msgLine);
		maxW = max(maxW, w);
	}
	return maxW;
}

void MessageLogDataSource::OnDrawMessage(UIObject* context, size_t msg, UIRect area)
{
	auto* font = messageFont.GetFont();

	size_t numLines = GetNumLines();
	for (size_t i = 0; i < numLines; i++)
	{
		auto msgLine = GetMessage(msg, i);
		draw::TextLine(
			font,
			messageFont.size,
			area.x0, area.y0 + i * messageFont.size,
			msgLine,
			messageColor,
			TextBaseline::Top);
	}
}


void MessageLogView::OnPaint(const UIPaintContext& ctx)
{
	size_t numMsgs = _dataSource->GetNumMessages();
	float htMsg = _dataSource->GetMessageHeight(this);

	auto RC = GetFinalRect();

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
}

void MessageLogView::OnEvent(Event& e)
{
	size_t numMsgs = _dataSource->GetNumMessages();
	float htMsg = _dataSource->GetMessageHeight(this);

	auto RC = GetFinalRect();

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
	FillerElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;

	_dataSource = nullptr;
	scrollbarV.OnReset();
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
	auto RC = GetFinalRect();
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
	auto RC = GetFinalRect();
	yOff = numMsgs * htMsg - RC.GetHeight();
}


void TableStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnFieldBorderBox(oi, "cellPadding", cellPadding);
	OnFieldBorderBox(oi, "rowHeaderPadding", rowHeaderPadding);
	OnFieldBorderBox(oi, "colHeaderPadding", colHeaderPadding);
	OnFieldPainter(oi, td, "cellBackgroundPainter", cellBackgroundPainter);
	OnFieldPainter(oi, td, "rowHeaderBackgroundPainter", rowHeaderBackgroundPainter);
	OnFieldPainter(oi, td, "colHeaderBackgroundPainter", colHeaderBackgroundPainter);
	OnFieldFontSettings(oi, td, "cellFont", cellFont);
	OnFieldFontSettings(oi, td, "rowHeaderFont", rowHeaderFont);
	OnFieldFontSettings(oi, td, "colHeaderFont", colHeaderFont);
}


std::string GenericGridDataSource::GetRowName(size_t row, uintptr_t id)
{
	return std::to_string(row + 1);
}


size_t TableDataSource::GetElements(Range<size_t> orderRange, Array<TreeElementRef>&)
{
	return GetNumRows();
}


static void TDS_GetElements(
	TreeDataSource* me,
	GenericGridDataSource::TreeElementRef elem,
	size_t& n,
	Range<size_t> orderRange,
	Array<GenericGridDataSource::TreeElementRef>& outElemList)
{
	size_t nch = me->GetChildCount(elem.id);
	for (size_t ch = 0; ch < nch; ch++)
	{
		uintptr_t chid = me->GetChild(elem.id, ch);
		size_t num = n++;
		GenericGridDataSource::TreeElementRef chelem = { chid, elem.depth + 1 };

		if (orderRange.Contains(num))
			outElemList.Append(chelem);

		if (me->IsOpen(chid))
			TDS_GetElements(me, chelem, n, orderRange, outElemList);
	}
}

size_t TreeDataSource::GetElements(Range<size_t> orderRange, Array<TreeElementRef>& outElemList)
{
	size_t n = 0;
	TDS_GetElements(this, { ROOT, SIZE_MAX }, n, orderRange, outElemList);
	return n;
}

Optional<bool> TreeDataSource::GetOpenState(uintptr_t id)
{
	if (GetChildCount(id) == 0)
		return {};
	return IsOpen(id);
}


struct TableViewImpl
{
	GenericGridDataSource* dataSource = nullptr;
	ISelectionStorage* selStorage = nullptr;
	IListContextMenuSource* ctxMenuSrc = nullptr;
	bool firstColWidthCalc = true;
	Array<float> colEnds = { 1.0f };
	size_t lastRowCount = 0;
	Range<size_t> lastVisibleRowRange = { 0, 0 };
	bool hoverIsIcon = false;
	size_t hoverRow = SIZE_MAX;
	uintptr_t hoverItem = UINTPTR_MAX;
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

static StaticID<FrameStyle> sid_framestyle_table_frame("table_frame");
static StaticID<TableStyle> sid_table_style("table");
static StaticID<IconStyle> sid_iconstyle_tree_expand("tree_expand");
void TableView::OnReset()
{
	FrameElement::OnReset();

	SetFrameStyle(sid_framestyle_table_frame);
	style = *GetCurrentTheme()->GetStruct(sid_table_style);
	expandButtonStyle = *GetCurrentTheme()->GetStruct(sid_iconstyle_tree_expand);

	enableRowHeader = true;
	_impl->dataSource = nullptr;
	_impl->selStorage = nullptr;
	_impl->ctxMenuSrc = nullptr;
	_impl->sel.OnReset();
	scrollbarV.OnReset();
}

static GenericGridDataSource::TreeElementRef ExtractID(const Array<GenericGridDataSource::TreeElementRef>& ids, size_t r, size_t minR)
{
	if (r - minR < ids.size())
		return ids[r - minR];
	return { r, 0 };
}

void TableView::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();
	PaintInfo info(this);

	size_t nc = _impl->dataSource->GetNumCols();
	size_t treeCol = _impl->dataSource->GetTreeCol();

	auto RC = GetContentRect();

	auto padCH = style.colHeaderPadding;
	auto padC = style.cellPadding;

	UIRect padRH = {};
	float rhw = 0, rhh = 0;
	if (enableRowHeader)
	{
		padRH = style.rowHeaderPadding;
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
	Range<size_t> rowRange = { minR, maxR };
	Array<GenericGridDataSource::TreeElementRef> ids;
	size_t nr = _impl->dataSource->GetElements(rowRange, ids);
	minR = min(minR, nr);
	maxR = min(maxR, nr);
	_impl->lastRowCount = nr;
	_impl->lastVisibleRowRange = { minR, maxR };

	if (enableRowHeader)
	{
		ContentPaintAdvice rowcpa = cpa;
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
			rowcpa = style.rowHeaderBackgroundPainter->Paint(info);
		}
		// text:
		auto* rowHeaderFont = style.rowHeaderFont.GetFont();
		for (size_t r = minR; r < maxR; r++)
		{
			auto rowRef = ExtractID(ids, r, minR);
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
				style.rowHeaderFont.size,
				rect.x0, (rect.y0 + rect.y1) / 2,
				_impl->dataSource->GetRowName(r, rowRef.id),
				rowcpa.HasTextColor() ? rowcpa.GetTextColor() : ctx.textColor,
				TextBaseline::Middle);
		}
		draw::PopScissorRect();
	}

	// - column header
	ContentPaintAdvice colcpa = cpa;
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
		colcpa = style.colHeaderBackgroundPainter->Paint(info);
	}
	// text:
	auto* colHeaderFont = style.colHeaderFont.GetFont();
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
			style.colHeaderFont.size,
			rect.x0, (rect.y0 + rect.y1) / 2,
			_impl->dataSource->GetColName(c),
			colcpa.HasTextColor() ? colcpa.GetTextColor() : ctx.textColor,
			TextBaseline::Middle);
	}
	draw::PopScissorRect();

	// - cells
	ContentPaintAdvice cellcpa = cpa;
	draw::PushScissorRect(UIRect{ RC.x0 + rhw, RC.y0 + chh, RC.x1, RC.y1 });
	// background:
	for (size_t r = minR; r < maxR; r++)
	{
		auto rowRef = ExtractID(ids, r, minR);

		info.SetChecked(_impl->selStorage ? _impl->selStorage->GetSelectionState(rowRef.id) : false);
		if (_impl->hoverRow == r)
			info.state |= PS_Hover;
		else
			info.state &= ~PS_Hover;

		for (size_t c = 0; c < nc; c++)
		{
			info.rect =
			{
				RC.x0 + rhw + _impl->colEnds[c],
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[c + 1],
				RC.y0 + chh - yOff + h * (r + 1),
			};
			cellcpa = style.cellBackgroundPainter->Paint(info);
		}
	}
	// text:
	auto* cellFont = style.cellFont.GetFont();
	for (size_t r = minR; r < maxR; r++)
	{
		auto rowRef = ExtractID(ids, r, minR);
		for (size_t c = 0; c < nc; c++)
		{
			UIRect rect =
			{
				RC.x0 + rhw + _impl->colEnds[c],
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[c + 1],
				RC.y0 + chh - yOff + h * (r + 1),
			};
			if (c == treeCol)
				rect.x0 += rowRef.depth * 20 + h;
			rect = rect.ShrinkBy(padC);
			draw::TextLine(
				cellFont,
				style.cellFont.size,
				rect.x0, (rect.y0 + rect.y1) / 2,
				_impl->dataSource->GetText(rowRef.id, c),
				cellcpa.HasTextColor() ? cellcpa.GetTextColor() : ctx.textColor,
				TextBaseline::Middle);
		}
	}
	// tree icons
	if (_impl->dataSource->IsTree() && treeCol < nc)
	{
		for (size_t r = minR; r < maxR; r++)
		{
			auto elem = ExtractID(ids, r, minR);
			float xo = elem.depth * 20;
			auto cs = _impl->dataSource->GetOpenState(elem.id);
			if (!cs.HasValue())
				continue;
			PaintInfo pi;
			pi.obj = this;
			pi.rect =
			{
				RC.x0 + rhw + _impl->colEnds[treeCol] + xo,
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[treeCol] + xo + h,
				RC.y0 + chh - yOff + h * (r + 1),
			};
			if (_impl->hoverRow == r && _impl->hoverIsIcon)
				pi.state |= PS_Hover;
			pi.SetChecked(cs.GetValue());
			expandButtonStyle.painter->Paint(pi);
		}
	}
	draw::PopScissorRect();

	scrollbarV.OnPaint({ this, sbrect, RC.GetHeight(), chh + nr * h, yOff });

	PaintChildren(ctx, cpa);
}

void TableView::OnEvent(Event& e)
{
	auto RC = GetContentRect();

	auto padCH = style.colHeaderPadding;
	auto padC = style.cellPadding;

	float rhw = 0, rhh = 0;
	if (enableRowHeader)
	{
		auto padRH = style.rowHeaderPadding;
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
	Range<size_t> rowRange = { minR, maxR };
	Array<GenericGridDataSource::TreeElementRef> ids;
	size_t nr = _impl->dataSource->GetElements(rowRange, ids);
	minR = min(minR, nr);
	maxR = min(maxR, nr);
	_impl->lastRowCount = nr;
	_impl->lastVisibleRowRange = { minR, maxR };

	ScrollbarData sbd = { this, sbrect, RC.GetHeight(), chh + nr * h, yOff };
	scrollbarV.OnEvent(sbd, e);

	_PerformDefaultBehaviors(e, UIObject_DB_CaptureMouseOnLeftClick | UIObject_DB_FocusOnLeftClick);

	if (e.type == EventType::ContextMenu)
	{
		auto& CM = ContextMenu::Get();
		if (_impl->hoverItem != UINTPTR_MAX)
		{
			if (_impl->ctxMenuSrc)
				_impl->ctxMenuSrc->FillItemContextMenu(CM, _impl->hoverItem, 0); // todo col
		}
		if (_impl->ctxMenuSrc)
			_impl->ctxMenuSrc->FillListContextMenu(CM);
	}

	if (e.type == EventType::MouseMove)
	{
		if (e.position.x < RC.x1 && e.position.y > RC.y0 + chh)
		{
			_impl->hoverRow = GetRowAt(e.position.y);
			auto rowRef = ExtractID(ids, _impl->hoverRow, minR);
			_impl->hoverItem = rowRef.id;
			_impl->hoverIsIcon =
				_impl->dataSource->GetOpenState(_impl->hoverItem).HasValue() &&
				e.position.x >= RC.x0 + rowRef.depth * 20 &&
				e.position.x < RC.x0 + rowRef.depth * 20 + cellh;
		}
		else
		{
			_impl->hoverRow = SIZE_MAX;
			_impl->hoverItem = UINTPTR_MAX;
			_impl->hoverIsIcon = false;
		}
	}
	if (e.type == EventType::MouseLeave)
	{
		_impl->hoverRow = SIZE_MAX;
		_impl->hoverItem = UINTPTR_MAX;
		_impl->hoverIsIcon = false;
	}
	if (e.type == EventType::ButtonDown && _impl->hoverIsIcon && _impl->hoverItem != UINTPTR_MAX)
	{
		_impl->dataSource->ToggleOpenState(_impl->hoverItem);
	}
	bool hovering = !_impl->hoverIsIcon && e.position.x < RC.x1 && e.position.y > RC.y0 + chh;
	if (_impl->sel.OnEvent(e, _impl->selStorage, _impl->hoverItem, hovering))
	{
		Event selev(e.context, this, EventType::SelectionChange);
		e.context->BubblingEvent(selev);
	}
}

GenericGridDataSource* TableView::GetDataSource() const
{
	return _impl->dataSource;
}

void TableView::SetDataSource(GenericGridDataSource* src)
{
	if (src != _impl->dataSource)
		_impl->firstColWidthCalc = true;
	_impl->dataSource = src;

	size_t nc = src->GetNumCols();
	while (_impl->colEnds.size() < nc + 1)
		_impl->colEnds.Append(_impl->colEnds.Last() + 100);
	while (_impl->colEnds.size() > nc + 1)
		_impl->colEnds.RemoveLast();
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
	size_t treeCol = _impl->dataSource->GetTreeCol();
	Array<float> colWidths;
	colWidths.ResizeWith(nc, 0.0f);

	auto RC = GetContentRect();
	auto padCH = style.colHeaderPadding;
	auto padC = style.cellPadding;
	float cellh = 20 + padC.y0 + padC.y1;

	if (includeHeader)
	{
		auto* rowHeaderFont = style.rowHeaderFont.GetFont();
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetColName(c);
			float w = GetTextWidth(rowHeaderFont, style.rowHeaderFont.size, text) + padCH.x0 + padCH.x1;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
	}

	Array<GenericGridDataSource::TreeElementRef> ids;
	size_t nr = _impl->dataSource->GetElements(All{}, ids);
	_impl->lastRowCount = nr;

	auto* cellFont = style.cellFont.GetFont();
	for (size_t i = 0; i < nr; i++)
	{
		auto rowRef = ExtractID(ids, i, 0);
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetText(rowRef.id, c);
			float w = GetTextWidth(cellFont, style.cellFont.size, text) + padC.x0 + padC.x1;
			if (c == treeCol)
				w += rowRef.depth * 20 + cellh;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
	}

	for (size_t c = 0; c < nc; c++)
		_impl->colEnds[c + 1] = _impl->colEnds[c] + colWidths[c];
}

bool TableView::IsValidRow(uintptr_t pos)
{
	return pos < _impl->lastRowCount;
}

size_t TableView::GetRowAt(float y)
{
	auto RC = GetContentRect();

	auto padCH = style.colHeaderPadding;
	auto padC = style.cellPadding;

	float rhh = 0;
	if (enableRowHeader)
	{
		auto padRH = style.rowHeaderPadding;
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
	size_t numRows = _impl->lastRowCount;
	return row < numRows ? row : SIZE_MAX;
}

UIRect TableView::GetCellRect(size_t col, size_t row)
{
	size_t numCols = _impl->dataSource->GetNumCols();
	if (col >= numCols && col != SIZE_MAX)
		col = numCols ? numCols - 1 : 0;
	size_t numRows = _impl->lastRowCount;
	if (row >= numRows)
		row = numRows ? numRows - 1 : 0;

	auto RC = GetContentRect();

	auto padCH = style.colHeaderPadding;
	auto padC = style.cellPadding;

	float rhw = 0, rhh = 0;
	if (enableRowHeader)
	{
		auto padRH = style.rowHeaderPadding;
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

Range<size_t> TableView::GetVisibleRange() const
{
	return _impl->lastVisibleRowRange;
}

size_t TableView::GetHoverRow() const
{
	return _impl->hoverRow;
}

} // ui
