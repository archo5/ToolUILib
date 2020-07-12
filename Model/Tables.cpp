
#include <unordered_set>
#include "Tables.h"
#include "Theme.h"


namespace ui {

struct Selection1DImpl
{
	std::unordered_set<uintptr_t> sel;
};

Selection1D::Selection1D()
{
	_impl = new Selection1DImpl;
}

Selection1D::~Selection1D()
{
	delete _impl;
}

void Selection1D::OnSerialize(IDataSerializer& s)
{
	auto size = _impl->sel.size();
	s << size;
	if (s.IsWriter())
	{
		for (auto v : _impl->sel)
			s << v;
	}
	else
	{
		for (size_t i = 0; i < size; i++)
		{
			uintptr_t v;
			s << v;
			_impl->sel.insert(v);
		}
	}
}

void Selection1D::Clear()
{
	_impl->sel.clear();
}

bool Selection1D::AnySelected()
{
	return _impl->sel.size() > 0;
}

uintptr_t Selection1D::GetFirstSelection()
{
	for (auto v : _impl->sel)
		return v;
	return UINTPTR_MAX;
}

bool Selection1D::IsSelected(uintptr_t id)
{
	return _impl->sel.find(id) != _impl->sel.end();
}

void Selection1D::SetSelected(uintptr_t id, bool sel)
{
	if (sel)
		_impl->sel.insert(id);
	else
		_impl->sel.erase(id);
}


struct TableViewImpl
{
	TableDataSource* dataSource = nullptr;
	bool firstColWidthCalc = true;
	std::vector<float> colEnds = { 1.0f };
	size_t hoverRow = SIZE_MAX;
};

TableView::TableView()
{
	_impl = new TableViewImpl;
	styleProps = Theme::current->tableBase;
	cellStyle = Theme::current->tableCell;
	rowHeaderStyle = Theme::current->tableRowHeader;
	colHeaderStyle = Theme::current->tableColHeader;
}

TableView::~TableView()
{
	delete _impl;
}

void TableView::OnPaint()
{
	styleProps->paint_func(this);

	size_t nc = _impl->dataSource->GetNumCols();
	size_t nr = _impl->dataSource->GetNumRows();

	style::PaintInfo info(this);

	auto RC = GetContentRect();

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhw = 80 + padRH.x0 + padRH.x1;
	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	sbrect.y0 += chh;
	RC.x1 -= sbw;

	size_t minR = floor(yOff / h);
	size_t maxR = size_t(ceil((yOff + sbrect.GetHeight()) / h));
	if (minR > nr)
		minR = nr;
	if (maxR > nr)
		maxR = nr;

	_impl->dataSource->OnBeginReadRows(minR, maxR);

	// - row header
	GL::PushScissorRect(RC.x0, RC.y0 + chh, RC.x0 + rhw, RC.y1);
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
		rowHeaderStyle->paint_func(info);
	}
	// text:
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
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetRowName(r).c_str(), 1, 1, 1);
	}
	GL::PopScissorRect();

	// - column header
	GL::PushScissorRect(RC.x0 + rhw, RC.y0, RC.x1, RC.y0 + chh);
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
		colHeaderStyle->paint_func(info);
	}
	// text:
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
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetColName(c).c_str(), 1, 1, 1);
	}
	GL::PopScissorRect();

	// - cells
	GL::PushScissorRect(RC.x0 + rhw, RC.y0 + chh, RC.x1, RC.y1);
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
			if (selection.IsSelected(r))
				info.state |= style::PS_Checked;
			else
				info.state &= ~style::PS_Checked;
			if (_impl->hoverRow == r)
				info.state |= style::PS_Hover;
			else
				info.state &= ~style::PS_Hover;
			cellStyle->paint_func(info);
		}
	}
	// text:
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
			DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetText(r, c).c_str(), 1, 1, 1);
		}
	}
	GL::PopScissorRect();

	_impl->dataSource->OnEndReadRows(minR, maxR);

	scrollbarV.OnPaint({ this, sbrect, RC.GetHeight(), chh + nr * h, yOff });

	PaintChildren();
}

void TableView::OnEvent(UIEvent& e)
{
	size_t nr = _impl->dataSource->GetNumRows();

	auto RC = GetContentRect();

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhw = 80 + padRH.x0 + padRH.x1;
	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	sbrect.y0 += chh;
	RC.x1 -= sbw;
	ScrollbarData sbd = { this, sbrect, RC.GetHeight(), chh + nr * h, yOff };
	scrollbarV.OnEvent(sbd, e);

	if (e.IsPropagationStopped())
		return;

	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::MouseMove)
	{
		if (e.x < RC.x1 && e.y > RC.y0 + chh)
			_impl->hoverRow = GetRowAt(e.y);
		else
			_impl->hoverRow = SIZE_MAX;
	}
	if (e.type == UIEventType::MouseLeave)
	{
		_impl->hoverRow = SIZE_MAX;
	}
	if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Left)
	{
		if (e.x < RC.x1 && e.y > RC.y0 + chh)
		{
			selection.Clear();
			size_t at = GetRowAt(e.y);
			if (at < SIZE_MAX)
				selection.SetSelected(at, true);
			e.StopPropagation();

			UIEvent selev(e.context, this, UIEventType::SelectionChange);
			e.context->BubblingEvent(selev);
		}
	}
}

void TableView::OnSerialize(IDataSerializer& s)
{
	s << _impl->firstColWidthCalc;
	auto size = _impl->colEnds.size();
	s << size;
	_impl->colEnds.resize(size);
	for (auto& v : _impl->colEnds)
		s << v;
	s << yOff;
	selection.OnSerialize(s);
}

void TableView::Render(UIContainer* ctx)
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

	Rerender();
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
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	if (includeHeader)
	{
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetColName(c);
			float w = GetTextWidth(text.c_str()) + padCH.x0 + padCH.x1;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
	}

	for (size_t i = 0, n = _impl->dataSource->GetNumRows(); i < n; i++)
	{
		_impl->dataSource->OnBeginReadRows(i, i + 1);
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetText(i, c);
			float w = GetTextWidth(text.c_str()) + padC.x0 + padC.x1;
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

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

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

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhw = 80 + padRH.x0 + padRH.x1;
	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

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
	style::PaintInfo info;
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
	styleProps = Theme::current->tableBase;
	cellStyle = Theme::current->tableCell;
	expandButtonStyle = Theme::current->collapsibleTreeNode;
	colHeaderStyle = Theme::current->tableColHeader;
}

TreeView::~TreeView()
{
	delete _impl;
}

void TreeView::OnPaint()
{
	styleProps->paint_func(this);

	int chh = 20;
	int h = 20;

	int nc = _impl->dataSource->GetNumCols();
	int nr = 0;// _dataSource->GetNumRows();

	style::PaintInfo info(this);

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
		colHeaderStyle->paint_func(info);
	}

	// text
	for (int c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + _impl->colEnds[c],
			RC.y0,
			RC.x0 + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetColName(c).c_str(), 1, 1, 1);
	}

	PaintState ps = { info, nc, RC.x0, RC.y0 + chh };
	for (size_t i = 0, n = _impl->dataSource->GetChildCount(TreeDataSource::ROOT); i < n; i++)
		_PaintOne(_impl->dataSource->GetChild(TreeDataSource::ROOT, i), 0, ps);

	PaintChildren();
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
		cellStyle->paint_func(ps.info);
	}

	// text
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
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetText(id, c).c_str(), 1, 1, 1);
	}

	ps.y += h;

	if (true) // open
	{
		for (size_t i = 0, n = _impl->dataSource->GetChildCount(id); i < n; i++)
			_PaintOne(_impl->dataSource->GetChild(id, i), lvl + 1, ps);
	}
}

void TreeView::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
}

void TreeView::Render(UIContainer* ctx)
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

	Rerender();
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
				float w = GetTextWidth(text.c_str());
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
