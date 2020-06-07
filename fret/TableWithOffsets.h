
#pragma once
#include "pch.h"


struct ConnectOffset
{
	uint64_t off;
	Point<float> tablePos;
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
#if 0
		size_t nr = ds->GetNumRows();
		for (size_t i = 0; i < nr && ret < max; i++)
		{
			TryAddRow(i, col, ret, max, buf);
		}
#else
		auto hoverRow = curTable->GetHoverRow();
		if (curTable->IsValidRow(hoverRow))
			TryAddRow(hoverRow, col, ret, max, buf);
		if (curTable->selection.AnySelected())
			TryAddRow(curTable->selection.GetFirstSelection(), col, ret, max, buf);
#endif
		return ret;
	}

	void TryAddRow(size_t row, size_t col, int& ret, int max, ConnectOffset* buf)
	{
		auto* ds = curTable->GetDataSource();
		auto tcr = curTable->GetContentRect();

		auto offtext = ds->GetText(row, col);
		auto off = std::stoull(offtext);
		auto cr = curTable->GetCellRect(SIZE_MAX, row);
		float y = (cr.y0 + cr.y1) * 0.5f;
		if (tcr.Contains(cr.x0, y))
			buf[ret++] = { off, { cr.x0, y }, curTable->selection.IsSelected(row) };
	}

	ui::TableView* curTable = nullptr;
};
