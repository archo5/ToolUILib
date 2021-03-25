
#pragma once
#include "pch.h"


struct ConnectOffset
{
	uint64_t off;
	ui::Point2f tablePos;
	bool sel;
};

struct TableWithOffsets
{
	int GetOffsets(int max, ConnectOffset* buf)
	{
		if (!curTable)
			return 0;


		auto* ds = curTable->GetDataSource();
		size_t col = 0, nc = ds->GetNumCols();
		for (col = 0; col < nc; col++)
		{
			if (ds->GetColName(col) == "Offset")
				break;
		}
		if (col == nc)
			return 0;

		int ret = 0;

		auto hoverRow = curTable->GetHoverRow();
		TryAddRow(hoverRow, col, ret, max, buf);

		// TODO get visible range API
		auto numrows = ds->GetNumRows();
		auto rmin = curTable->GetRowAt(curTable->GetContentRect().y0);
		if (rmin >= numrows)
			rmin = 0;
		auto rmax = curTable->GetRowAt(curTable->GetContentRect().y1) + 1;
		if (rmax > numrows)
			rmax = numrows;

		for (auto row = rmin; row < rmax; row++)
		{
			if (curTable->GetSelectionStorage()->GetSelectionState(row))
				TryAddRow(row, col, ret, max, buf);
		}

		return ret;
	}

	void TryAddRow(size_t row, size_t col, int& ret, int max, ConnectOffset* buf)
	{
		if (!curTable->IsValidRow(row))
			return;

		auto* ds = curTable->GetDataSource();
		auto tcr = curTable->GetContentRect();

		auto offtext = ds->GetText(row, col);
		auto off = std::stoull(offtext);
		auto cr = curTable->GetCellRect(SIZE_MAX, row);
		float y = (cr.y0 + cr.y1) * 0.5f;
		if (tcr.Contains(cr.x0, y))
			buf[ret++] = { off, { cr.x0, y }, curTable->GetSelectionStorage()->GetSelectionState(row) };
	}

	ui::TableView* curTable = nullptr;
};
