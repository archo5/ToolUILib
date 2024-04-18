
#include "SerializationJSON.h"

#include "Serialization.h"
#include "Logging.h"

#include "../../ThirdParty/json.h"


namespace ui {

LogCategory LOG_SERIALIZATION_JSON("SerializationJSON");

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
		else if (c == '\"') _data += "\\\"";
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
	if (!isfinite(value))
	{
		LogWarn(LOG_SERIALIZATION_JSON, "cannot serialize a NaN/inf number in JSON, replacing with 0");
		value = 0;
	}
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
		const char* end = _inArray.Last() ? "\n]" : "\n}";
		_data += indent ? end : end + 1;
		_inArray.RemoveLast();
	}
	return _data;
}

void JSONLinearWriter::_WriteIndent(bool skipComma)
{
	if (!skipComma && _data.back() != '{' && _data.back() != '[')
		_data += ",";
	if (indent)
	{
		_data += "\n";
		for (size_t i = 0; i < _inArray.size(); i++)
			_data += indent;
	}
}

void JSONLinearWriter::_WritePrefix(const char* key)
{
	_WriteIndent();
	if (!_inArray.Last())
	{
		_data += "\"";
		_data += key;
		_data += indent ? "\": " : "\":";
		_starts.Last().weight += strlen(key) + 4;
	}
}

void JSONLinearWriter::_OnEndObject()
{
	_starts.Last().weight += 2;
	if (_starts.Last().weight < 100U && indent)
	{
		size_t dst = _starts.Last().start;
		char* data = &_data[0];
		size_t dataSize = _data.size();
		for (size_t src = _starts.Last().start; src < dataSize; src++)
		{
			if (data[src] == '\n')
				data[dst++] = ' ';
			else if (data[src] != '\t')
				data[dst++] = data[src];
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
	json_parse_result_s parseResult = {};
	_root = json_parse_ex(all.data(), all.size(), flags, nullptr, nullptr, &parseResult);
	if (!_root || parseResult.error != json_parse_error_none)
	{
		LogError(LOG_SERIALIZATION_JSON, "json_parse_ex failed with error=%zu (line=%zu offset=%zu row=%zu)",
			parseResult.error, parseResult.error_line_no, parseResult.error_offset, parseResult.error_row_no);
	}
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


void JSONSerializerObjectIterator::BeginObject(const FieldInfo& FI, const char* objname, std::string* outName)
{
	BeginDict(FI.GetNameOrEmptyStr());
	if (outName)
		WriteString("__", *outName);
}

void JSONSerializerObjectIterator::EndObject()
{
	EndDict();
}

size_t JSONSerializerObjectIterator::BeginArray(size_t size, const FieldInfo& FI)
{
	JSONLinearWriter::BeginArray(FI.GetNameOrEmptyStr());
	return 0;
}

void JSONSerializerObjectIterator::EndArray()
{
	JSONLinearWriter::EndArray();
}

void JSONSerializerObjectIterator::OnFieldBool(const FieldInfo& FI, bool& val)
{
	WriteBool(FI.GetNameOrEmptyStr(), val);
}

void JSONSerializerObjectIterator::OnFieldS64(const FieldInfo& FI, int64_t& val)
{
	WriteInt(FI.GetNameOrEmptyStr(), val);
}

void JSONSerializerObjectIterator::OnFieldU64(const FieldInfo& FI, uint64_t& val)
{
	WriteInt(FI.GetNameOrEmptyStr(), val);
}

void JSONSerializerObjectIterator::OnFieldF64(const FieldInfo& FI, double& val)
{
	WriteFloat(FI.GetNameOrEmptyStr(), val);
}

void JSONSerializerObjectIterator::OnFieldString(const FieldInfo& FI, const IBufferRW& brw)
{
	WriteString(FI.GetNameOrEmptyStr(), brw.Read());
}

void JSONSerializerObjectIterator::OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw)
{
	auto src = brw.Read();
	WriteString(FI.GetNameOrEmptyStr(), Base16Encode(src.data(), src.size()));
}


void JSONUnserializerObjectIterator::BeginObject(const FieldInfo& FI, const char* objname, std::string* outName)
{
	BeginDict(FI.name);
	if (outName)
		*outName = ReadString("__").GetValueOrDefault({});
}

void JSONUnserializerObjectIterator::EndObject()
{
	EndDict();
}

size_t JSONUnserializerObjectIterator::BeginArray(size_t size, const FieldInfo& FI)
{
	JSONLinearReader::BeginArray(FI.name);
	return GetCurrentArraySize();
}

void JSONUnserializerObjectIterator::EndArray()
{
	JSONLinearReader::EndArray();
}

bool JSONUnserializerObjectIterator::HasMoreArrayElements()
{
	return JSONLinearReader::HasMoreArrayElements();
}

bool JSONUnserializerObjectIterator::HasField(const char* name)
{
	return !!FindEntry(name);
}

void JSONUnserializerObjectIterator::OnFieldBool(const FieldInfo& FI, bool& val)
{
	if (auto v = ReadBool(FI.name))
		val = v.GetValue();
}

void JSONUnserializerObjectIterator::OnFieldS64(const FieldInfo& FI, int64_t& val)
{
	if (auto v = ReadInt64(FI.name))
		val = v.GetValue();
}

void JSONUnserializerObjectIterator::OnFieldU64(const FieldInfo& FI, uint64_t& val)
{
	if (auto v = ReadUInt64(FI.name))
		val = v.GetValue();
}

void JSONUnserializerObjectIterator::OnFieldF64(const FieldInfo& FI, double& val)
{
	if (auto v = ReadFloat(FI.name))
		val = v.GetValue();
}

void JSONUnserializerObjectIterator::OnFieldString(const FieldInfo& FI, const IBufferRW& brw)
{
	if (auto v = ReadString(FI.name))
		brw.Assign(v.GetValue());
}

void JSONUnserializerObjectIterator::OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw)
{
	if (auto v = ReadString(FI.name))
		brw.Assign(Base16Decode(v.GetValue()));
}

} // ui
