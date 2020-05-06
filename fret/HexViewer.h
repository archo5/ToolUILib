
#pragma once
#include "pch.h"
#include "FileReaders.h"
#include "DataDesc.h"


struct HighlightSettings
{
	bool excludeZeroes = true;
	bool enableFloat32 = true;
	float minFloat32 = 0.0001f;
	float maxFloat32 = 10000;
	bool enableInt16 = true;
	int32_t minInt16 = -2000;
	int32_t maxInt16 = 2000;
	bool enableInt32 = true;
	int32_t minInt32 = -10000;
	int32_t maxInt32 = 10000;
	int minASCIIChars = 3;

	void EditUI(UIContainer* ctx);
};

struct Highlighter : HighlightSettings
{
	Color4f GetByteTypeBin(uint64_t basePos, uint8_t* buf, int at, int sz);
	Color4f GetByteTypeASCII(uint64_t basePos, uint8_t* buf, int at, int sz);

	bool IsInt16InRange(uint64_t basePos, uint8_t* buf, int at, int sz);
	bool IsInt32InRange(uint64_t basePos, uint8_t* buf, int at, int sz);
	bool IsFloat32InRange(uint64_t basePos, uint8_t* buf, int at, int sz);
	bool IsASCIIInRange(uint64_t basePos, uint8_t* buf, int at, int sz);

	MarkerData* markerData = nullptr;
};

struct HexViewer : UIElement
{
	HexViewer()
	{
		GetStyle().SetWidth(style::Coord::Percent(100));
		GetStyle().SetHeight(style::Coord::Percent(100));
	}
	~HexViewer()
	{
	}
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;
	void OnSerialize(IDataSerializer& s) override
	{
		s << hoverByte;
		s << hoverSection;
		s << selectionStart;
		s << selectionEnd;
		s << mouseDown;
	}

	uint64_t GetBasePos()
	{
		return *basePos;
	}

	void Init(IDataSource* ds, uint64_t* bpos, uint32_t* bwid, Highlighter* hltr)
	{
		dataSource = ds;
		basePos = bpos;
		byteWidth = bwid;
		highlighter = hltr;
	}

	// input data
	IDataSource* dataSource = nullptr;
	uint64_t* basePos = nullptr;
	uint32_t* byteWidth = nullptr;
	Highlighter* highlighter = nullptr;

	Color4f colorHover{ 1, 1, 1, 0.3f };
	Color4f colorSelect{ 1, 0.7f, 0.65f, 0.5f };
	uint64_t hoverByte = UINT64_MAX;
	int hoverSection = -1;
	uint64_t selectionStart = UINT64_MAX;
	uint64_t selectionEnd = UINT64_MAX;
	bool mouseDown = false;
};
