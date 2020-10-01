
#include "pch.h"
#include "FileReaders.h"


void IDataSource::GetASCIIText(char* buf, size_t bufsz, uint64_t pos)
{
	size_t rd = Read(pos, bufsz - 1, buf);
	buf[rd] = 0;
	for (size_t i = 0; i < rd; i++)
		if (buf[i] < 32 || buf[i] >= 127)
			buf[i] = '?';
}

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


MemoryDataSource::MemoryDataSource(void* mem, size_t size, bool own) : _mem(mem), _size(size), _own(own)
{
}

MemoryDataSource::~MemoryDataSource()
{
	if (_mem && _own)
		delete _mem;
}

size_t MemoryDataSource::Read(uint64_t at, size_t size, void* out)
{
	size_t from = std::min(at, uint64_t(_size));
	size_t end = std::min(at + size, uint64_t(_size));
	size_t nw = end - from;
	memcpy(out, (char*)_mem + from, nw);
	if (nw < size)
		memset((char*)out + nw, 0, size - nw);
	return nw;
}

uint64_t MemoryDataSource::GetSize()
{
	return _size;
}


FileDataSource::FileDataSource(const char* path)
{
	_fp = fopen(path, "rb");
	if (_fp)
	{
		_fseeki64(_fp, 0, SEEK_END);
		_size = _ftelli64(_fp);
		_fseeki64(_fp, 0, SEEK_SET);
	}
}

FileDataSource::~FileDataSource()
{
	if (_fp)
		fclose(_fp);
}

size_t FileDataSource::Read(uint64_t at, size_t size, void* out)
{
	size_t rd = 0;
	if (_fp)
	{
		if (_pos != at)
		{
			if (at > _size)
				at = _size;
			if (!_fseeki64(_fp, at, SEEK_SET))
				_pos = at;
		}
		rd = fread(out, 1, size, _fp);
		_pos += rd;
	}
	if (rd < size)
		memset((char*)out + rd, 0, size - rd);
	return rd;
}

uint64_t FileDataSource::GetSize()
{
	return _size;
}


SliceDataSource::SliceDataSource(IDataSource* src, uint64_t off, uint64_t size) : _src(src), _off(off), _size(size)
{
}

SliceDataSource::~SliceDataSource()
{
}

size_t SliceDataSource::Read(uint64_t at, size_t size, void* out)
{
	auto from = std::min(at, _size);
	auto end = std::min(at + size, _size);
	size_t nw = _src->Read(at + _off, end - from, out);
	if (nw < size)
		memset((char*)out + nw, 0, size - nw);
	return nw;
}

uint64_t SliceDataSource::GetSize()
{
	return _size;
}

