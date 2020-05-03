
#include "pch.h"
#include "FileReaders.h"


void IDataSource::GetInt8Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
{
	int8_t v;
	if (Read(pos, sizeof(v), &v) < sizeof(v))
	{
		strncpy(buf, "-", bufsz);
		return;
	}
	snprintf(buf, bufsz, sign ? "%" PRId8 : "%" PRIu8, v);
}

void IDataSource::GetInt16Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
{
	int16_t v;
	if (Read(pos, sizeof(v), &v) < sizeof(v))
	{
		strncpy(buf, "-", bufsz);
		return;
	}
	snprintf(buf, bufsz, sign ? "%" PRId16 : "%" PRIu16, v);
}

void IDataSource::GetInt32Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
{
	int32_t v;
	if (Read(pos, sizeof(v), &v) < sizeof(v))
	{
		strncpy(buf, "-", bufsz);
		return;
	}
	snprintf(buf, bufsz, sign ? "%" PRId32 : "%" PRIu32, v);
}

void IDataSource::GetInt64Text(char* buf, size_t bufsz, uint64_t pos, bool sign)
{
	int64_t v;
	if (Read(pos, sizeof(v), &v) < sizeof(v))
	{
		strncpy(buf, "-", bufsz);
		return;
	}
	snprintf(buf, bufsz, sign ? "%" PRId64 : "%" PRIu64, v);
}

void IDataSource::GetFloat32Text(char* buf, size_t bufsz, uint64_t pos)
{
	float v;
	if (Read(pos, sizeof(v), &v) < sizeof(v))
	{
		strncpy(buf, "-", bufsz);
		return;
	}
	snprintf(buf, bufsz, "%g", v);
}

void IDataSource::GetFloat64Text(char* buf, size_t bufsz, uint64_t pos)
{
	double v;
	if (Read(pos, sizeof(v), &v) < sizeof(v))
	{
		strncpy(buf, "-", bufsz);
		return;
	}
	snprintf(buf, bufsz, "%g", v);
}


size_t FileDataSource::Read(uint64_t at, size_t size, void* out)
{
	size_t rd = !fp || _fseeki64(fp, at, SEEK_SET) ? 0 : fread(out, 1, size, fp);
	if (rd < size)
		memset((char*)out + rd, 0, size - rd);
	return rd;
}

uint64_t FileDataSource::GetSize()
{
	if (!fp || fseek(fp, 0, SEEK_END))
		return 0;
	return _ftelli64(fp);
}
