
#include "Serialization.h"

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NONLS
#include <Windows.h>


namespace ui {


class SettingsBackend
{
public:
	virtual int64_t ReadInt(StringView path) = 0;
	virtual void WriteInt(StringView path, int64_t) = 0;
	virtual double ReadFloat(StringView path) = 0;
	virtual void WriteFloat(StringView path, double) = 0;
	virtual std::string ReadString(StringView path) = 0;
	virtual void WriteString(StringView path, StringView) = 0;
};


class RegistryBackendWin32Registry : SettingsBackend
{
	int64_t ReadInt(StringView path) override
	{
		DWORD type = 0, size = 0;
		int64_t value = 0;
		auto ret = RegGetValueA(HKEY_LOCAL_MACHINE, "Software/UITEST", path.data(), RRF_RT_REG_DWORD | RRF_RT_REG_QWORD | RRF_ZEROONFAILURE, &type, &value, &size);
		if (ret == ERROR_SUCCESS)
			return value;
		printf("failed to read: %08X\n", ret);
		return 0;
	}
	void WriteInt(StringView path, int64_t) override
	{
	}
	double ReadFloat(StringView path) override
	{
	}
	void WriteFloat(StringView path, double) override
	{
	}
	std::string ReadString(StringView path) override
	{
	}
	void WriteString(StringView path, StringView) override
	{
	}
};


class SettingsHierarchySerializerBase : public SettingsSerializer
{
public:
	SettingsHierarchySerializerBase(SettingsBackend* be, StringView basePath) :
		_backend(be),
		_basePath(basePath.data(), basePath.size()),
		_curPath(_basePath)
	{
	}
	std::string GetFullPath(StringView name)
	{
		std::string out = _curPath;
		out.push_back('/');
		out.append(name.data(), name.size());
		return out;
	}
	void SetSubdirectory(StringView name) override
	{
		_curPath = _basePath;
		_curPath.push_back('/');
		_curPath.append(name.data(), name.size());
	}

protected:
	SettingsBackend* _backend;
	std::string _basePath;
	std::string _curPath;
};

class Reader : public SettingsHierarchySerializerBase
{
public:
	bool IsWriter() override { return false; }
	void _SerializeInt(StringView name, int64_t& v) override { v = _backend->ReadInt(GetFullPath(name)); }
	void _SerializeFloat(StringView name, double& v) override { v = _backend->ReadFloat(GetFullPath(name)); }
	void SerializeString(StringView name, std::string& v) override { v = _backend->ReadString(GetFullPath(name)); }
};

class Writer : public SettingsHierarchySerializerBase
{
public:
	bool IsWriter() override { return true; }
	void _SerializeInt(StringView name, int64_t& v) override { _backend->WriteInt(GetFullPath(name), v); }
	void _SerializeFloat(StringView name, double& v) override { _backend->WriteFloat(GetFullPath(name), v); }
	void SerializeString(StringView name, std::string& v) override { _backend->WriteString(GetFullPath(name), v); }
};


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

	//for (auto& E : entries)
	//	printf("indent=%d childSkip=%d key=\"%.*s\" value=\"%.*s\"\n", E.indent, int(E.childSkip), int(E.key.size()), E.key.data(), int(E.value.size()), E.value.data());
	return true;
}

NamedTextSerializeReader::EntryRange NamedTextSerializeReader::GetCurrentRange()
{
	if (stack.empty())
		return { entries.data(), entries.data() + entries.size() };
	return stack.back();
}

static NamedTextSerializeReader::Entry defaultEntry = {};
NamedTextSerializeReader::Entry* NamedTextSerializeReader::FindEntry(const char* key, Entry* def)
{
	for (auto E : GetCurrentRange())
		if (E->key == key)
			return E;
	return def;
}

std::string NamedTextSerializeReader::ReadString(const char* key, const std::string& def)
{
	if (auto* v = FindEntry(key))
		return v->GetStringValue();
	return def;
}

bool NamedTextSerializeReader::ReadBool(const char* key, bool def)
{
	if (auto* v = FindEntry(key))
		return v->GetBoolValue();
	return def;
}

int NamedTextSerializeReader::ReadInt(const char* key, int def)
{
	if (auto* v = FindEntry(key))
		return v->GetIntValue();
	return def;
}

unsigned NamedTextSerializeReader::ReadUInt(const char* key, unsigned def)
{
	if (auto* v = FindEntry(key))
		return v->GetUIntValue();
	return def;
}

int64_t NamedTextSerializeReader::ReadInt64(const char* key, int64_t def)
{
	if (auto* v = FindEntry(key))
		return v->GetInt64Value();
	return def;
}

uint64_t NamedTextSerializeReader::ReadUInt64(const char* key, uint64_t def)
{
	if (auto* v = FindEntry(key))
		return v->GetUInt64Value();
	return def;
}

double NamedTextSerializeReader::ReadFloat(const char* key, double def)
{
	if (auto* v = FindEntry(key))
		return v->GetFloatValue();
	return def;
}

void NamedTextSerializeReader::BeginArray(const char* key)
{
	auto E = FindEntry(key);
	if (E && E->ValueEquals("[]"))
		stack.push_back({ E + 1, E + E->childSkip });
	else
		stack.push_back({ nullptr, nullptr });
}

void NamedTextSerializeReader::EndArray()
{
	stack.pop_back();
}

bool NamedTextSerializeReader::BeginDict(const char* key)
{
	auto E = FindEntry(key);
	if (E && E->ValueEquals("{}"))
	{
		stack.push_back({ E + 1, E + E->childSkip });
		return true;
	}
	else
	{
		stack.push_back({ nullptr, nullptr });
		return false;
	}
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


JSONSerializeWriter::JSONSerializeWriter()
{
	_data = "{";
	_starts = { { 0U, 2U } };
	_inArray = { false };
}

void JSONSerializeWriter::WriteString(const char* key, StringView value)
{
	_WritePrefix(key);
	_data += "\"";
	for (char c : value)
	{
		if (c == '\n') _data += "\\n";
		else if (c == '\r') _data += "\\r";
		else if (c == '\t') _data += "\\t";
		else if (c == '\b') _data += "\\b";
		else if (c == '\\') _data += "\\\\";
		else if (c < 0x20)
		{
			_data += "\\u00";
			_data += "0123456789abcdef"[c >> 4];
			_data += "0123456789abcdef"[c & 0xf];
		}
		else _data += c;
	}
	_data += "\"";
	_starts.back().weight += 9999;
}

void JSONSerializeWriter::WriteBool(const char* key, bool value)
{
	_WritePrefix(key);
	_data += value ? "true" : "false";
	_starts.back().weight += 6;
}

void JSONSerializeWriter::WriteInt(const char* key, int64_t value)
{
	_WritePrefix(key);
	char bfr[32];
	snprintf(bfr, 32, "%" PRId64, value);
	_data += bfr;
	_starts.back().weight += 6;
}

void JSONSerializeWriter::WriteInt(const char* key, uint64_t value)
{
	_WritePrefix(key);
	char bfr[32];
	snprintf(bfr, 32, "%" PRIu64, value);
	_data += bfr;
	_starts.back().weight += 6;
}

void JSONSerializeWriter::WriteFloat(const char* key, double value)
{
	_WritePrefix(key);
	char bfr[32];
	snprintf(bfr, 32, "%g", value);
	for (auto& c : bfr)
		if (c == ',')
			c = '.';
	_data += bfr;
	_starts.back().weight += 6;
}

void JSONSerializeWriter::BeginArray(const char* key)
{
	_WritePrefix(key);
	_starts.push_back({ _data.size(), 2U });
	_data += "[";
	_inArray.push_back(true);
}

void JSONSerializeWriter::EndArray()
{
	_inArray.pop_back();
	_WriteIndent(true);
	_data += "]";
	_OnEndObject();
}

void JSONSerializeWriter::BeginDict(const char* key)
{
	_WriteIndent();
	if (!_inArray.back())
	{
		_data += "\"";
		_data += key;
		_data += "\": ";
	}
	_starts.push_back({ _data.size(), 2U });
	_data += "{";
	_inArray.push_back(false);
}

void JSONSerializeWriter::EndDict()
{
	_inArray.pop_back();
	_WriteIndent(true);
	_data += "}";
	_OnEndObject();
}

std::string& JSONSerializeWriter::GetData()
{
	while (_inArray.size())
	{
		_data += _inArray.back() ? "\n]" : "\n}";
		_inArray.pop_back();
	}
	return _data;
}

void JSONSerializeWriter::_WriteIndent(bool skipComma)
{
	if (!skipComma && _data.back() != '{' && _data.back() != '[')
		_data += ",";
	_data += "\n";
	for (size_t i = 0; i < _inArray.size(); i++)
		_data += "\t";
}

void JSONSerializeWriter::_WritePrefix(const char* key)
{
	_WriteIndent();
	if (!_inArray.back())
	{
		_data += "\"";
		_data += key;
		_data += "\": ";
		_starts.back().weight += strlen(key) + 4;
	}
}

void JSONSerializeWriter::_OnEndObject()
{
	_starts.back().weight += 2;
	if (_starts.back().weight < 100U)
	{
		size_t dst = _starts.back().start;
		for (size_t src = _starts.back().start; src < _data.size(); src++)
		{
			if (_data[src] == '\n')
				_data[dst++] = ' ';
			else if (_data[src] != '\t')
				_data[dst++] = _data[src];
		}
		_data.resize(dst);
	}
	_starts[_starts.size() - 2].weight += _starts.back().weight;
	_starts.pop_back();
}


} // ui
