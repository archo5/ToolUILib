
#pragma once

#include "String.h"
#include "ObjectIteration.h"

#include <inttypes.h>
#include <type_traits>
#include <string>
#include <vector>


struct json_value_s;
struct json_array_element_s;


namespace ui {

struct SettingsSerializer
{
	virtual bool IsWriter() = 0;
	virtual void SetSubdirectory(StringView) = 0;
	virtual void _SerializeInt(StringView name, int64_t&) = 0;
	virtual void _SerializeFloat(StringView name, double&) = 0;
	virtual void SerializeString(StringView name, std::string&) = 0;

	template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
	void SerializeInt(StringView name, T& v)
	{
		int64_t i = v;
		_SerializeInt(name, i);
		v = i;
	}

	template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
	void SerializeFloat(StringView name, T& v)
	{
		double f = v;
		_SerializeFloat(name, f);
		v = f;
	}
};

struct NamedTextSerializeWriter
{
	void WriteString(const char* key, StringView value);
	void WriteBool(const char* key, bool value);
	void WriteInt(const char* key, int value) { WriteInt(key, int64_t(value)); }
	void WriteInt(const char* key, unsigned value) { WriteInt(key, uint64_t(value)); }
	void WriteInt(const char* key, int64_t value);
	void WriteInt(const char* key, uint64_t value);
	void WriteFloat(const char* key, double value);
	void BeginArray(const char* key);
	void EndArray();
	void BeginDict(const char* key);
	void EndDict();

	void _WriteString(StringView s);
	void _WriteIndent();

	std::string data;
	int _indent = 0;
};

struct NamedTextSerializeReader
{
	struct Entry
	{
		StringView key;
		StringView value;
		int indent = 0;
		size_t childSkip = 0;

		bool IsSimpleStringValue();
		std::string GetStringValue();
		StringView GetStringValue(std::string& tmp);
		bool ValueEquals(StringView cmpval);
		bool GetBoolValue();
		int GetIntValue();
		unsigned GetUIntValue();
		int64_t GetInt64Value();
		uint64_t GetUInt64Value();
		double GetFloatValue();
	};
	struct EntryIterator
	{
		Entry* E;

		bool operator != (const EntryIterator& o) const { return E != o.E; }
		void operator ++ () { E += E->childSkip; }
		Entry* operator * () { return E; }
	};
	struct EntryRange
	{
		Entry* _start;
		Entry* _end;

		EntryIterator begin() const { return { _start }; }
		EntryIterator end() const { return { _end }; }
	};

	bool Parse(StringView all);

	EntryRange GetCurrentRange();
	Entry* FindEntry(const char* key, Entry* def = nullptr);
	std::string ReadString(const char* key, const std::string& def = "");
	bool ReadBool(const char* key, bool def = false);
	int ReadInt(const char* key, int def = 0);
	unsigned ReadUInt(const char* key, unsigned def = 0);
	int64_t ReadInt64(const char* key, int64_t def = 0);
	uint64_t ReadUInt64(const char* key, uint64_t def = 0);
	double ReadFloat(const char* key, double def = 0);
	void BeginArray(const char* key);
	void EndArray();
	bool BeginDict(const char* key);
	void EndDict();
	void BeginEntry(Entry* E);
	void EndEntry();

	std::vector<Entry> entries;
	std::vector<EntryRange> stack;
};

struct JSONLinearWriter
{
	struct CompactScope
	{
		size_t start;
		size_t weight;
	};

	JSONLinearWriter();
	void WriteString(const char* key, StringView value);
	void WriteNull(const char* key);
	void WriteBool(const char* key, bool value);
	void WriteInt(const char* key, int value) { WriteInt(key, int64_t(value)); }
	void WriteInt(const char* key, unsigned value) { WriteInt(key, uint64_t(value)); }
	void WriteInt(const char* key, int64_t value);
	void WriteInt(const char* key, uint64_t value);
	void WriteFloat(const char* key, double value);
	void WriteRawNumber(const char* key, StringView value);
	void BeginArray(const char* key);
	void EndArray();
	void BeginDict(const char* key);
	void EndDict();

	std::string& GetData();

	void _WriteIndent(bool skipComma = false);
	void _WritePrefix(const char* key);
	void _OnEndObject();

	std::string _data;
	std::vector<CompactScope> _starts;
	std::vector<bool> _inArray;
};

enum JSONParseFlags
{
	JPF_Default = 0,
	JPF_AllowTrailingComma = 0x1,
	JPF_AllowUnquotedKeys = 0x2,
	JPF_AllowGlobalObject = 0x4,
	JPF_AllowEqualsInObject = 0x8,
	JPF_AllowNoCommas = 0x10,
	JPF_AllowCStyleComments = 0x20,
	JPF_AllowSingleQuotedStrings = 0x100,
	JPF_AllowHexadecimalNumbers = 0x200,
	JPF_AllowLeadingPlusSign = 0x400,
	JPF_AllowLeadingOrTrailingDecimalPoint = 0x800,
	JPF_AllowInfAndNan = 0x1000,
	JPF_AllowMultiLineStrings = 0x2000,

	JPF_AllowSimplifiedJson = (
		JPF_AllowTrailingComma |
		JPF_AllowUnquotedKeys |
		JPF_AllowGlobalObject |
		JPF_AllowEqualsInObject |
		JPF_AllowNoCommas),
	JPF_AllowJson5 = (
		JPF_AllowTrailingComma |
		JPF_AllowUnquotedKeys |
		JPF_AllowCStyleComments |
		JPF_AllowSingleQuotedStrings |
		JPF_AllowHexadecimalNumbers |
		JPF_AllowLeadingPlusSign |
		JPF_AllowLeadingOrTrailingDecimalPoint |
		JPF_AllowInfAndNan |
		JPF_AllowMultiLineStrings),
	JPF_AllowAll = JPF_AllowSimplifiedJson | JPF_AllowJson5,
};

struct JSONLinearReader
{
	using Entry = json_value_s;
	struct StackElement
	{
		Entry* entry;
		json_array_element_s* curElem;
	};

	~JSONLinearReader();
	void _Free();

	bool Parse(StringView all, unsigned flags = JPF_Default);

	bool HasMoreArrayElements();
	size_t GetCurrentArraySize();
	Entry* FindEntry(const char* key);
	Optional<std::string> ReadString(const char* key);
	Optional<bool> ReadBool(const char* key);
	Optional<int> ReadInt(const char* key);
	Optional<unsigned> ReadUInt(const char* key);
	Optional<int64_t> ReadInt64(const char* key);
	Optional<uint64_t> ReadUInt64(const char* key);
	Optional<double> ReadFloat(const char* key);
	bool BeginArray(const char* key);
	void EndArray();
	bool BeginDict(const char* key);
	void EndDict();

	json_value_s* _root = nullptr;
	std::vector<StackElement> _stack;
};

std::string Base16Encode(const void* src, size_t size);
std::string Base16Decode(StringView src, bool* valid = nullptr);
std::string Base64Encode(const void* src, size_t size, const char* endChars = "+/", bool pad = true);
std::string Base64Decode(StringView src, const char* endChars = "+/", bool needPadding = false, bool* valid = nullptr);

struct JSONSerializerObjectIterator : JSONLinearWriter, IObjectIteratorMinTypeSerializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Serializer | OIF_KeyMapped; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override
	{
		BeginDict(FI.GetNameOrEmptyStr());
		if (outName)
			WriteString("__", *outName);
	}
	void EndObject() override
	{
		EndDict();
	}
	size_t BeginArray(size_t size, const FieldInfo& FI) override
	{
		JSONLinearWriter::BeginArray(FI.GetNameOrEmptyStr());
		return 0;
	}
	void EndArray() override
	{
		JSONLinearWriter::EndArray();
	}

	void OnFieldBool(const FieldInfo& FI, bool& val) override
	{
		WriteBool(FI.GetNameOrEmptyStr(), val);
	}
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		WriteInt(FI.GetNameOrEmptyStr(), val);
	}
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		WriteInt(FI.GetNameOrEmptyStr(), val);
	}
	void OnFieldF64(const FieldInfo& FI, double& val) override
	{
		WriteFloat(FI.GetNameOrEmptyStr(), val);
	}
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override
	{
		WriteString(FI.GetNameOrEmptyStr(), brw.Read());
	}
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override
	{
		auto src = brw.Read();
		WriteString(FI.GetNameOrEmptyStr(), Base16Encode(src.data(), src.size()));
	}
};

struct JSONUnserializerObjectIterator : JSONLinearReader, IObjectIteratorMinTypeUnserializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Unserializer | OIF_KeyMapped; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override
	{
		BeginDict(FI.name);
		if (outName)
			*outName = ReadString("__").GetValueOrDefault({});
	}
	void EndObject() override
	{
		EndDict();
	}
	size_t BeginArray(size_t size, const FieldInfo& FI) override
	{
		JSONLinearReader::BeginArray(FI.name);
		return GetCurrentArraySize();
	}
	void EndArray() override
	{
		JSONLinearReader::EndArray();
	}
	bool HasMoreArrayElements() override { return JSONLinearReader::HasMoreArrayElements(); }
	bool HasField(const char* name) override { return !!FindEntry(name); }

	void OnFieldBool(const FieldInfo& FI, bool& val) override
	{
		if (auto v = ReadBool(FI.name))
			val = v.GetValue();
	}
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		if (auto v = ReadInt64(FI.name))
			val = v.GetValue();
	}
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		if (auto v = ReadUInt64(FI.name))
			val = v.GetValue();
	}
	void OnFieldF64(const FieldInfo& FI, double& val) override
	{
		if (auto v = ReadFloat(FI.name))
			val = v.GetValue();
	}
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override
	{
		if (auto v = ReadString(FI.name))
			brw.Assign(v.GetValue());
	}
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override
	{
		if (auto v = ReadString(FI.name))
			brw.Assign(Base16Decode(v.GetValue()));
	}
};

} // ui
