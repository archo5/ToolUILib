
#pragma once
#include "pch.h"


struct IDataSource
{
	virtual size_t Read(uint64_t at, size_t size, void* out) = 0;
	virtual uint64_t GetSize() = 0;
};

struct FileDataSource : IDataSource
{
	size_t Read(uint64_t at, size_t size, void* out) override
	{
		size_t rd = !fp || _fseeki64(fp, at, SEEK_SET) ? 0 : fread(out, 1, size, fp);
		if (rd < size)
			memset((char*)out + rd, 0, size - rd);
		return rd;
	}
	uint64_t GetSize() override
	{
		if (!fp || fseek(fp, 0, SEEK_END))
			return 0;
		return _ftelli64(fp);
	}

	FILE* fp;
};
