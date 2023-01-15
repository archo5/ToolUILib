
#pragma once

#include "ObjectIteration.h"


struct json_value_s;
struct json_array_element_s;


namespace ui {

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
	Array<CompactScope> _starts;
	Array<bool> _inArray;
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
	void BeginEntry(Entry* E);
	void EndEntry();

	json_value_s* _root = nullptr;
	Array<StackElement> _stack;
};

struct JSONSerializerObjectIterator : JSONLinearWriter, IObjectIteratorMinTypeSerializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Serializer | OIF_KeyMapped; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	void OnFieldBool(const FieldInfo& FI, bool& val) override;
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override;
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override;
	void OnFieldF64(const FieldInfo& FI, double& val) override;
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

struct JSONUnserializerObjectIterator : JSONLinearReader, IObjectIteratorMinTypeUnserializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Unserializer | OIF_KeyMapped; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;
	bool HasMoreArrayElements() override;
	bool HasField(const char* name) override;

	void OnFieldBool(const FieldInfo& FI, bool& val) override;
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override;
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override;
	void OnFieldF64(const FieldInfo& FI, double& val) override;
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

} // ui
