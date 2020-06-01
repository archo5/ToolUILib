
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
	unsigned minASCIIChars = 3;

	void Load(const char* key, NamedTextSerializeReader& r);
	void Save(const char* key, NamedTextSerializeWriter& w);

	void EditUI(UIContainer* ctx);
};

struct ByteColors
{
	Color4f hexColor = { 0, 0 };
	Color4f asciiColor = { 0, 0 };
	Color4f leftBracketColor = { 0, 0 };
	Color4f rightBracketColor = { 0, 0 };
};

struct HexViewerState
{
	uint64_t basePos = 0;
	uint32_t byteWidth = 8;

	Color4f colorHover{ 1, 1, 1, 0.3f };
	Color4f colorSelect{ 1, 0.7f, 0.65f, 0.5f };
	uint64_t hoverByte = UINT64_MAX;
	int hoverSection = -1;
	uint64_t selectionStart = UINT64_MAX;
	uint64_t selectionEnd = UINT64_MAX;
	bool mouseDown = false;
};
extern ui::DataCategoryTag DCT_HexViewerState[1];

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

	uint64_t GetBasePos()
	{
		return state->basePos;
	}

	void Init(DataDesc* dd, DDFile* f, HexViewerState* hvs, HighlightSettings* hs)
	{
		dataDesc = dd;
		file = f;
		state = hvs;
		highlightSettings = hs;
	}

	// input data
	DataDesc* dataDesc = nullptr;
	DDFile* file = nullptr;
	HexViewerState* state = nullptr;
	HighlightSettings* highlightSettings = nullptr;
};
