
#include "Serialization.h"

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#define NONLS
#include <Windows.h>

#include "../../ThirdParty/json.h"


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
	Array<size_t> entryStack;
	entryStack.Reserve(32);

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

		int prevIndent = entries.Size() ? entries.Last().indent : -1;
		if (entries.Size() && indent > prevIndent + 1)
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
			entryStack.Append(entries.Size());
		else
		{
			// same level or moving back
			for (int i = indent; i < prevIndent; i++)
				entries[entryStack[i]].childSkip = entries.Size() - entryStack[i];
			entryStack.Resize(indent + 1);
			entryStack[indent] = entries.Size();
		}

		entries.Append({ key, value, indent, 1 });
	}

	int prevIndent = entries.Size() ? entries.Last().indent : -1;
	for (int i = 0; i < prevIndent; i++)
		entries[entryStack[i]].childSkip = entries.Size() - entryStack[i];

	//for (auto& E : entries)
	//	printf("indent=%d childSkip=%d key=\"%.*s\" value=\"%.*s\"\n", E.indent, int(E.childSkip), int(E.key.size()), E.key.data(), int(E.value.size()), E.value.data());
	return true;
}

NamedTextSerializeReader::EntryRange NamedTextSerializeReader::GetCurrentRange()
{
	if (stack.IsEmpty())
		return { entries.data(), entries.data() + entries.Size() };
	return stack.Last();
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
		stack.Append({ E + 1, E + E->childSkip });
	else
		stack.Append({ nullptr, nullptr });
}

void NamedTextSerializeReader::EndArray()
{
	stack.RemoveLast();
}

bool NamedTextSerializeReader::BeginDict(const char* key)
{
	auto E = FindEntry(key);
	if (E && E->ValueEquals("{}"))
	{
		stack.Append({ E + 1, E + E->childSkip });
		return true;
	}
	else
	{
		stack.Append({ nullptr, nullptr });
		return false;
	}
}

void NamedTextSerializeReader::EndDict()
{
	stack.RemoveLast();
}

void NamedTextSerializeReader::BeginEntry(Entry* E)
{
	stack.Append({ E, E + E->childSkip });
}

void NamedTextSerializeReader::EndEntry()
{
	stack.RemoveLast();
}


JSONLinearWriter::JSONLinearWriter()
{
	_data = "{";
	_starts = { { 0U, 2U } };
	_inArray = { false };
}

void JSONLinearWriter::WriteString(const char* key, StringView value)
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
	_starts.Last().weight += 9999;
}

void JSONLinearWriter::WriteNull(const char* key)
{
	_WritePrefix(key);
	_data += "null";
	_starts.Last().weight += 6;
}

void JSONLinearWriter::WriteBool(const char* key, bool value)
{
	_WritePrefix(key);
	_data += value ? "true" : "false";
	_starts.Last().weight += 6;
}

void JSONLinearWriter::WriteInt(const char* key, int64_t value)
{
	_WritePrefix(key);
	char bfr[32];
	snprintf(bfr, 32, "%" PRId64, value);
	_data += bfr;
	_starts.Last().weight += 6;
}

void JSONLinearWriter::WriteInt(const char* key, uint64_t value)
{
	_WritePrefix(key);
	char bfr[32];
	snprintf(bfr, 32, "%" PRIu64, value);
	_data += bfr;
	_starts.Last().weight += 6;
}

void JSONLinearWriter::WriteFloat(const char* key, double value)
{
	_WritePrefix(key);
	char bfr[32];
	snprintf(bfr, 32, "%g", value);
	for (auto& c : bfr)
		if (c == ',')
			c = '.';
	_data += bfr;
	_starts.Last().weight += 6;
}

void JSONLinearWriter::WriteRawNumber(const char* key, StringView value)
{
	_WritePrefix(key);
	_data.append(value.data(), value.size());
	_starts.Last().weight += 6;
}

void JSONLinearWriter::BeginArray(const char* key)
{
	_WritePrefix(key);
	_starts.Append({ _data.size(), 2U });
	_data += "[";
	_inArray.Append(true);
}

void JSONLinearWriter::EndArray()
{
	_inArray.RemoveLast();
	_WriteIndent(true);
	_data += "]";
	_OnEndObject();
}

void JSONLinearWriter::BeginDict(const char* key)
{
	_WriteIndent();
	if (!_inArray.Last())
	{
		_data += "\"";
		_data += key;
		_data += "\": ";
	}
	_starts.Append({ _data.size(), 2U });
	_data += "{";
	_inArray.Append(false);
}

void JSONLinearWriter::EndDict()
{
	_inArray.RemoveLast();
	_WriteIndent(true);
	_data += "}";
	_OnEndObject();
}

std::string& JSONLinearWriter::GetData()
{
	while (_inArray.size())
	{
		_data += _inArray.Last() ? "\n]" : "\n}";
		_inArray.RemoveLast();
	}
	return _data;
}

void JSONLinearWriter::_WriteIndent(bool skipComma)
{
	if (!skipComma && _data.back() != '{' && _data.back() != '[')
		_data += ",";
	_data += "\n";
	for (size_t i = 0; i < _inArray.size(); i++)
		_data += "\t";
}

void JSONLinearWriter::_WritePrefix(const char* key)
{
	_WriteIndent();
	if (!_inArray.Last())
	{
		_data += "\"";
		_data += key;
		_data += "\": ";
		_starts.Last().weight += strlen(key) + 4;
	}
}

void JSONLinearWriter::_OnEndObject()
{
	_starts.Last().weight += 2;
	if (_starts.Last().weight < 100U)
	{
		size_t dst = _starts.Last().start;
		for (size_t src = _starts.Last().start; src < _data.size(); src++)
		{
			if (_data[src] == '\n')
				_data[dst++] = ' ';
			else if (_data[src] != '\t')
				_data[dst++] = _data[src];
		}
		_data.resize(dst);
	}
	_starts[_starts.size() - 2].weight += _starts.Last().weight;
	_starts.RemoveLast();
}


JSONLinearReader::~JSONLinearReader()
{
	_Free();
}

void JSONLinearReader::_Free()
{
	_stack.Clear();
	if (_root)
	{
		free(_root);
		_root = nullptr;
	}
}

bool JSONLinearReader::Parse(StringView all, unsigned flags)
{
	_Free();
	_root = json_parse_ex(all.data(), all.size(), flags, nullptr, nullptr, nullptr);
	_stack.Append({ _root });
	return !!_root;
}

bool JSONLinearReader::HasMoreArrayElements()
{
	return !!_stack.Last().curElem;
}

size_t JSONLinearReader::GetCurrentArraySize()
{
	if (auto* e = _stack.Last().entry)
		if (auto* a = json_value_as_array(e))
			return a->length;
	return 0;
}

JSONLinearReader::Entry* JSONLinearReader::FindEntry(const char* key)
{
	if (auto*& e = _stack.Last().curElem)
	{
		auto* r = e->value;
		e = e->next;
		return r;
	}
	if (key)
	{
		if (auto* e = _stack.Last().entry)
		{
			if (auto* o = json_value_as_object(e))
			{
				for (auto* c = o->start; c; c = c->next)
				{
					if (strcmp(c->name->string, key) == 0)
						return c->value;
				}
			}
		}
	}
	return nullptr;
}

Optional<std::string> JSONLinearReader::ReadString(const char* key)
{
	if (auto* e = FindEntry(key))
	{
		if (auto* s = json_value_as_string(e))
			return std::string(s->string, s->string_size);
		if (auto* n = json_value_as_number(e))
			return std::string(n->number, n->number_size);
		if (e->type == json_type_true)
			return "true";
		if (e->type == json_type_false)
			return "false";
	}
	return {};
}

Optional<bool> JSONLinearReader::ReadBool(const char* key)
{
	if (auto* e = FindEntry(key))
	{
		if (e->type == json_type_true)
			return true;
		if (e->type == json_type_false)
			return false;
	}
	return {};
}

Optional<int> JSONLinearReader::ReadInt(const char* key)
{
	return ReadInt64(key).StaticCast<int>();
}

Optional<unsigned> JSONLinearReader::ReadUInt(const char* key)
{
	return ReadUInt64(key).StaticCast<unsigned>();
}

Optional<int64_t> JSONLinearReader::ReadInt64(const char* key)
{
	if (auto* e = FindEntry(key))
	{
		if (auto* n = json_value_as_number(e))
		{
			return strtoll(n->number, nullptr, 10);
		}
	}
	return {};
}

Optional<uint64_t> JSONLinearReader::ReadUInt64(const char* key)
{
	if (auto* e = FindEntry(key))
	{
		if (auto* n = json_value_as_number(e))
		{
			return strtoull(n->number, nullptr, 10);
		}
	}
	return {};
}

Optional<double> JSONLinearReader::ReadFloat(const char* key)
{
	if (auto* e = FindEntry(key))
	{
		if (auto* n = json_value_as_number(e))
		{
			return strtod(n->number, nullptr);
		}
	}
	return {};
}

bool JSONLinearReader::BeginArray(const char* key)
{
	auto* e = FindEntry(key);
	_stack.Append({});
	if (e)
	{
		if (auto* a = json_value_as_array(e))
			_stack.Last() = { e, a->start };
	}
	return !!e;
}

void JSONLinearReader::EndArray()
{
	_stack.RemoveLast();
}

bool JSONLinearReader::BeginDict(const char* key)
{
	_stack.Append({ FindEntry(key) });
	if (auto* e = _stack.Last().entry)
		if (e->type != json_type_object)
			_stack.Last().entry = nullptr;
	return !!_stack.Last().entry;
}

void JSONLinearReader::EndDict()
{
	_stack.RemoveLast();
}

void JSONLinearReader::BeginEntry(Entry* E)
{
	_stack.Append({ E });
}

void JSONLinearReader::EndEntry()
{
	_stack.RemoveLast();
}


std::string Base16Encode(const void* src, size_t size)
{
	std::string tmp;
	auto* bsrc = static_cast<const uint8_t*>(src);
	tmp.resize(size * 2);
	for (size_t i = 0; i < size; i++)
	{
		uint8_t b = bsrc[i];
		tmp[i * 2 + 0] = "0123456789abcdef"[b >> 4];
		tmp[i * 2 + 1] = "0123456789abcdef"[b & 0xf];
	}
	return tmp;
}

std::string Base16Decode(StringView src, bool* valid)
{
	std::string tmp;
	size_t size = src.size() / 2;
	tmp.resize(size);
	for (size_t i = 0; i < size; i++)
	{
		char chi = src[i * 2 + 0];
		char clo = src[i * 2 + 1];
		uint8_t ihi = dechex(chi);
		uint8_t ilo = dechex(clo);
		if ((ihi == 0 && chi != '0') || (ilo == 0 && clo != '0'))
		{
			if (valid)
				*valid = false;
			return {};
		}
	}
	if (valid)
		*valid = true;
	return tmp;
}

static void B64GenAlphabet(char* alphabet, const char* endChars)
{
	memcpy(alphabet, "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789", 62);
	alphabet[62] = endChars && *endChars ? *endChars++ : '+';
	alphabet[63] = endChars && *endChars ? *endChars : '/';
}

static uint8_t B64Decode(char c, char c62, char c63)
{
	if (c == c62)
		return 62;
	if (c == c63)
		return 63;
	if (c == '=')
		return 0;
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	return 0xff;
}

std::string Base64Encode(const void* src, size_t size, const char* endChars, bool pad)
{
	char alphabet[64];
	B64GenAlphabet(alphabet, endChars);

	std::string tmp;
	tmp.reserve((size + 2) / 3 * 4);

	size_t rem = size % 3;
	auto* srcu = (const uint8_t*)src;
	auto* srcend = srcu + (size / 3 * 3);

	uint32_t mem;
	for (auto* s = srcu; s != srcend; s += 3)
	{
		mem = (uint32_t(s[0]) << 16) | (uint32_t(s[1]) << 8) | s[2];
		tmp.push_back(alphabet[(mem >> 18) & 0x3f]);
		tmp.push_back(alphabet[(mem >> 12) & 0x3f]);
		tmp.push_back(alphabet[(mem >> 6) & 0x3f]);
		tmp.push_back(alphabet[mem & 0x3f]);
	}
	switch (rem)
	{
	case 0:
		break;
	case 1:
		mem = (uint32_t(srcend[0]) << 16);
		tmp.push_back(alphabet[(mem >> 18) & 0x3f]);
		tmp.push_back(alphabet[(mem >> 12) & 0x3f]);
		if (pad)
		{
			tmp.push_back('=');
			tmp.push_back('=');
		}
		break;
	case 2:
		mem = (uint32_t(srcend[0]) << 16) | (uint32_t(srcend[1]) << 8);
		tmp.push_back(alphabet[(mem >> 18) & 0x3f]);
		tmp.push_back(alphabet[(mem >> 12) & 0x3f]);
		tmp.push_back(alphabet[(mem >> 6) & 0x3f]);
		if (pad)
		{
			tmp.push_back('=');
		}
		break;
	}
	return tmp;
}

std::string Base64Decode(StringView src, const char* endChars, bool needPadding, bool* valid)
{
	if (needPadding && (src.size() % 4 != 0))
	{
		if (valid)
			*valid = false;
		return {};
	}

	while (!src.empty() && src.last() == '=')
		src = src.substr(0, src.size() - 1);
	if (src.size() == 1)
	{
		if (valid)
			*valid = false;
		return {};
	}

	std::string tmp;
	tmp.reserve((src.size() + 3) / 4 * 3);

	char c62 = endChars && *endChars ? *endChars++ : '+';
	char c63 = endChars && *endChars ? *endChars : '/';

	while (src.size() >= 4)
	{
		uint8_t v0 = B64Decode(src[0], c62, c63);
		uint8_t v1 = B64Decode(src[1], c62, c63);
		uint8_t v2 = B64Decode(src[2], c62, c63);
		uint8_t v3 = B64Decode(src[3], c62, c63);
		if (v0 == 0xff || v1 == 0xff || v2 == 0xff || v3 == 0xff)
		{
			if (valid)
				*valid = false;
			return {};
		}
		// 33333322 22221111 11000000
		uint8_t b0 = (v0 << 2) | (v1 >> 4);
		uint8_t b1 = (v1 << 4) | (v2 >> 2);
		uint8_t b2 = (v2 << 6) | v3;
		tmp.push_back(b0);
		tmp.push_back(b1);
		tmp.push_back(b2);
		src = src.substr(4);
	}

	if (src.size() == 2)
	{
		uint8_t v0 = B64Decode(src[0], c62, c63);
		uint8_t v1 = B64Decode(src[1], c62, c63);
		if (v0 == 0xff || v1 == 0xff)
		{
			if (valid)
				*valid = false;
			return {};
		}
		uint8_t b0 = (v0 << 2) | (v1 >> 4);
		tmp.push_back(b0);
	}
	else if (src.size() == 3)
	{
		uint8_t v0 = B64Decode(src[0], c62, c63);
		uint8_t v1 = B64Decode(src[1], c62, c63);
		uint8_t v2 = B64Decode(src[2], c62, c63);
		if (v0 == 0xff || v1 == 0xff || v2 == 0xff)
		{
			if (valid)
				*valid = false;
			return {};
		}
		uint8_t b0 = (v0 << 2) | (v1 >> 4);
		uint8_t b1 = (v1 << 4) | (v2 >> 2);
		tmp.push_back(b0);
		tmp.push_back(b1);
	}

	if (valid)
		*valid = true;
	return tmp;
}


#if 0
#include <stdio.h>
#include <stdlib.h>
#define ASSERT_EQUAL(xref, x) if ((xref) != (x)) \
	printf("%d: ERROR (%s exp. %s): [%zu]\"%s\" is not [%zu]\"%s\"\n", \
		__LINE__, #x, #xref, (x).size(), (x).c_str(), (xref).size(), (xref).c_str());
struct TestSerialization
{
	TestSerialization()
	{
		TestBase64();
		exit(0);
	}

	void TestBase64()
	{
		// one-way
		ASSERT_EQUAL(std::string("AA=="), Base64Encode("\0", 1));
		ASSERT_EQUAL(std::string("Y2F0"), Base64Encode("cat", 3));
		ASSERT_EQUAL(std::string("Y2F0cw=="), Base64Encode("cats", 4));

		// roundtrip
		ASSERT_EQUAL(std::string("\0", 1), Base64Decode(Base64Encode("\0", 1)));
		ASSERT_EQUAL(std::string("cat"), Base64Decode(Base64Encode("cat", 3)));
		ASSERT_EQUAL(std::string("cats"), Base64Decode(Base64Encode("cats", 4)));
	}
}
g_tests;
#endif

} // ui
