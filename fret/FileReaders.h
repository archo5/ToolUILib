
#pragma once
#include "pch.h"


struct IDataSource
{
	virtual size_t Read(uint64_t at, size_t size, void* out) = 0;
	virtual uint64_t GetSize() = 0;

	void GetInt8Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetInt16Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetInt32Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetInt64Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetFloat32Text(char* buf, size_t bufsz, uint64_t pos);
	void GetFloat64Text(char* buf, size_t bufsz, uint64_t pos);
};

struct FileDataSource : IDataSource
{
	size_t Read(uint64_t at, size_t size, void* out) override;
	uint64_t GetSize() override;

	FILE* fp;
};
