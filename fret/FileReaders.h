
#pragma once
#include "pch.h"


struct IDataSource
{
	virtual void Read(uint64_t at, size_t size, void* out) = 0;
};

struct FileDataSource : IDataSource
{
	void Read(uint64_t at, size_t size, void* out) override
	{
		_fseeki64(fp, at, SEEK_SET);
		size_t rd = fread(out, 1, size, fp);
		if (rd < size)
			memset((char*)out + rd, 0, size - rd);
	}

	FILE* fp;
};
