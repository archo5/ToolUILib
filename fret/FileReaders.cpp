
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


void NamedTextSerializeWriter::WriteString(const char* key, StringView value)
{
	_WriteIndent();
	data += key;
	data += '=';
	_WriteString(value);
	data += '\n';
}

void NamedTextSerializeWriter::WriteBool(const char* key, bool value)
{
	_WriteIndent();
	data += key;
	data += '=';
	data += value ? "true" : "false";
	data += '\n';
}

void NamedTextSerializeWriter::WriteInt(const char* key, int64_t value)
{
	_WriteIndent();
	data += key;
	data += '=';
	char bfr[32];
	snprintf(bfr, 32, "%" PRId64 "\n", value);
	data += bfr;
}

void NamedTextSerializeWriter::WriteInt(const char* key, uint64_t value)
{
	_WriteIndent();
	data += key;
	data += '=';
	char bfr[32];
	snprintf(bfr, 32, "%" PRIu64 "\n", value);
	data += bfr;
}

void NamedTextSerializeWriter::WriteFloat(const char* key, double value)
{
	_WriteIndent();
	data += key;
	data += '=';
	char bfr[64];
	snprintf(bfr, 64, "%g\n", value);
	data += bfr;
}

void NamedTextSerializeWriter::BeginArray(const char* key)
{
	WriteString(key, "[]");
	_indent++;
}

void NamedTextSerializeWriter::EndArray()
{
	_indent--;
}

void NamedTextSerializeWriter::BeginDict(const char* key)
{
	WriteString(key, "{}");
	_indent++;
}

void NamedTextSerializeWriter::EndDict()
{
	_indent--;
}

void NamedTextSerializeWriter::_WriteString(StringView s)
{
	if (data.size() + s.size() > data.capacity())
		data.reserve(data.capacity() * 2 + s.size());

	if (s.size() < 64)
	{
		bool foundBad = false;
		for (auto c : s)
		{
			if (c <= ' ' || c >= '\x7f' || c == '"')
			{
				foundBad = true;
				break;
			}
		}
		if (!foundBad)
		{
			data.insert(data.size(), s.data(), s.size());
			return;
		}
	}

	data += '"';
	for (auto c : s)
	{
		if (c == '\n') data += "\\n";
		else if (c == '\r') data += "\\r";
		else if (c == '\t') data += "\\t";
		else if (c == '"') data += "\\\"";
		else if (c == '\\') data += "\\\\";
		else if (c < ' ' || c >= '\x7f')
		{
			char bfr[5] = { '\\', 'x', "0123456789abcdef"[(c & 0xf0) >> 4], "0123456789abcdef"[c & 0xf] };
			data += bfr;
		}
		else data += c;
	}
	data += '"';
}

void NamedTextSerializeWriter::_WriteIndent()
{
	for (int i = 0; i < _indent; i++)
		data += "\t";
}


bool NamedTextSerializeReader::Entry::IsSimpleStringValue()
{
	return value.size() < 2 || value.first() != '"' || value.last() != '"';
}

std::string NamedTextSerializeReader::Entry::GetStringValue()
{
	if (IsSimpleStringValue())
		return std::string(value.data(), value.size());
	std::string tmp;
	GetStringValue(tmp);
	return tmp;
}

static unsigned dechex(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

StringView NamedTextSerializeReader::Entry::GetStringValue(std::string& tmp)
{
	if (IsSimpleStringValue())
		return value;

	auto val = value.substr(1, value.size() - 2);
	tmp.clear();
	tmp.reserve(val.size());

	for (auto it = val; !it.empty(); )
	{
		auto ch = it.take_char();
		if (ch == '\\' && it.size() >= 1)
		{
			ch = it.take_char();
			if (ch == '\\' || ch == '"') tmp.push_back(ch);
			else if (ch == 'n') tmp.push_back('\n');
			else if (ch == 'r') tmp.push_back('\r');
			else if (ch == 't') tmp.push_back('\t');
			else if (ch == 'x' && it.size() >= 2)
			{
				auto ch1 = it.take_char();
				auto ch2 = it.take_char();
				tmp.push_back((dechex(ch1) << 4) | dechex(ch2));
			}
			else
			{
				tmp.push_back('\\');
				tmp.push_back(ch);
			}
		}
		else
			tmp.push_back(ch);
	}

	return tmp;
}

bool NamedTextSerializeReader::Entry::ValueEquals(StringView cmpval)
{
	if (IsSimpleStringValue())
		return value == cmpval;

	return StringView(GetStringValue()) == cmpval;
}

bool NamedTextSerializeReader::Entry::GetBoolValue()
{
	std::string tmp;
	return GetStringValue(tmp) == "true";
}

int NamedTextSerializeReader::Entry::GetIntValue()
{
	std::string tmp = GetStringValue();
	int out = 0;
	sscanf(tmp.c_str(), " %d", &out);
	return out;
}

unsigned NamedTextSerializeReader::Entry::GetUIntValue()
{
	std::string tmp = GetStringValue();
	unsigned out = 0;
	sscanf(tmp.c_str(), " %u", &out);
	return out;
}

int64_t NamedTextSerializeReader::Entry::GetInt64Value()
{
	std::string tmp = GetStringValue();
	int64_t out = 0;
	sscanf(tmp.c_str(), " %" PRId64, &out);
	return out;
}

uint64_t NamedTextSerializeReader::Entry::GetUInt64Value()
{
	std::string tmp = GetStringValue();
	uint64_t out = 0;
	sscanf(tmp.c_str(), " %" PRIu64, &out);
	return out;
}

double NamedTextSerializeReader::Entry::GetFloatValue()
{
	std::string tmp = GetStringValue();
	double out = 0;
	sscanf(tmp.c_str(), " %lg", &out);
	return out;
}

bool NamedTextSerializeReader::Parse(StringView all)
{
	std::vector<size_t> entryStack;
	entryStack.reserve(32);

	StringView rem = all;
	while (!rem.empty())
	{
		auto at = rem.find_first_at("\n");
		auto line = rem.substr(0, at);
		rem = rem.substr(at + 1);

		// comment
		if (line.first_char_is([](char c) { return c == '#'; }))
			continue;

		// depth
		int indent = 0;
		while (line.first_char_is([](char c) { return c == ' ' || c == '\t'; }))
		{
			line.take_char();
			indent++;
		}

		// empty line (possibly with whitespace)
		if (line.empty())
			continue;

		int prevIndent = entries.size() ? entries.back().indent : -1;
		if (entries.size() && indent > prevIndent + 1)
		{
			// indent jumps too high
			return false;
		}

		// data
		size_t eqpos = line.find_first_at("=");
		if (eqpos == SIZE_MAX)
		{
			// no equal sign
			return false;
		}
		StringView key = line.substr(0, eqpos).trim();
		StringView value = line.substr(eqpos + 1).trim();

		// linkage
		if (indent > prevIndent) // moving deeper
			entryStack.push_back(entries.size());
		else
		{
			// same level or moving back
			for (int i = indent; i < prevIndent; i++)
				entries[entryStack[i]].childSkip = entries.size() - entryStack[i];
			entryStack.resize(indent + 1);
			entryStack[indent] = entries.size();
		}

		entries.push_back({ key, value, indent, 1 });
	}

	int prevIndent = entries.size() ? entries.back().indent : -1;
	for (int i = 0; i < prevIndent; i++)
		entries[entryStack[i]].childSkip = entries.size() - entryStack[i];

	for (auto& E : entries)
		printf("indent=%d childSkip=%d key=\"%.*s\" value=\"%.*s\"\n", E.indent, int(E.childSkip), int(E.key.size()), E.key.data(), int(E.value.size()), E.value.data());
	return true;
}

NamedTextSerializeReader::EntryRange NamedTextSerializeReader::GetCurrentRange()
{
	if (stack.empty())
		return { entries.data(), entries.data() + entries.size() };
	return stack.back();
}

static NamedTextSerializeReader::Entry defaultEntry = {};
NamedTextSerializeReader::Entry* NamedTextSerializeReader::FindEntry(const char* key)
{
	for (auto E : GetCurrentRange())
		if (E->key == key)
			return E;
	return &defaultEntry;
}

std::string NamedTextSerializeReader::ReadString(const char* key)
{
	return FindEntry(key)->GetStringValue();
}

bool NamedTextSerializeReader::ReadBool(const char* key)
{
	return FindEntry(key)->GetBoolValue();
}

int NamedTextSerializeReader::ReadInt(const char* key)
{
	return FindEntry(key)->GetIntValue();
}

unsigned NamedTextSerializeReader::ReadUInt(const char* key)
{
	return FindEntry(key)->GetUIntValue();
}

int64_t NamedTextSerializeReader::ReadInt64(const char* key)
{
	return FindEntry(key)->GetInt64Value();
}

uint64_t NamedTextSerializeReader::ReadUInt64(const char* key)
{
	return FindEntry(key)->GetUInt64Value();
}

double NamedTextSerializeReader::ReadFloat(const char* key)
{
	return FindEntry(key)->GetFloatValue();
}

void NamedTextSerializeReader::BeginArray(const char* key)
{
	auto E = FindEntry(key);
	if (E->ValueEquals("[]"))
		stack.push_back({ E + 1, E + E->childSkip });
	else
		stack.push_back({ nullptr, nullptr });
}

void NamedTextSerializeReader::EndArray()
{
	stack.pop_back();
}

void NamedTextSerializeReader::BeginDict(const char* key)
{
	auto E = FindEntry(key);
	if (E->ValueEquals("{}"))
		stack.push_back({ E + 1, E + E->childSkip });
	else
		stack.push_back({ nullptr, nullptr });
}

void NamedTextSerializeReader::EndDict()
{
	stack.pop_back();
}

void NamedTextSerializeReader::BeginEntry(Entry* E)
{
	stack.push_back({ E, E + E->childSkip });
}

void NamedTextSerializeReader::EndEntry()
{
	stack.pop_back();
}
