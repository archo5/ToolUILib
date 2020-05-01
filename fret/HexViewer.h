
#pragma once
#include "pch.h"
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
		if (fp)
			fclose(fp);
	}
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;
	void OnSerialize(IDataSerializer& s) override
	{
		s << hoverByte;
		s << hoverSection;
	}

	uint64_t GetBasePos()
	{
		return *basePos;
	}

	void Init(const char* file, uint64_t* bpos, uint32_t* bwid, Highlighter* hltr)
	{
		if (fp)
			fclose(fp);
		fp = fopen(file, "rb");
		basePos = bpos;
		byteWidth = bwid;
		highlighter = hltr;
	}

	uint64_t GetFileLength()
	{
		if (!fp)
			return 0;
		fseek(fp, 0, SEEK_END);
		return _ftelli64(fp);
	}

	void GetInt16Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
	{
		int16_t v;
		size_t sz = 0;
		if (!fp || fseek(fp, pos, SEEK_SET) || !fread(&v, 2, 1, fp))
		{
			strncpy(buf, "-", bufsz);
			return;
		}
		snprintf(buf, bufsz, sign ? "%" PRId16 : "%" PRIu16, v);
	}
	void GetInt32Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
	{
		int32_t v;
		size_t sz = 0;
		if (!fp || fseek(fp, pos, SEEK_SET) || !fread(&v, 4, 1, fp))
		{
			strncpy(buf, "-", bufsz);
			return;
		}
		snprintf(buf, bufsz, sign ? "%" PRId32 : "%" PRIu32, v);
	}
	void GetFloat32Text(char* buf, size_t bufsz, uint64_t pos)
	{
		float v;
		size_t sz = 0;
		if (!fp || fseek(fp, pos, SEEK_SET) || !fread(&v, 4, 1, fp))
		{
			strncpy(buf, "-", bufsz);
			return;
		}
		snprintf(buf, bufsz, "%g", v);
	}

	// input data
	uint64_t* basePos;
	uint32_t* byteWidth;
	Highlighter* highlighter = nullptr;

	FILE* fp = nullptr;

	Color4f colorHover{ 1, 1, 1, 0.3f };
	uint64_t hoverByte = UINT64_MAX;
	int hoverSection = -1;
};
