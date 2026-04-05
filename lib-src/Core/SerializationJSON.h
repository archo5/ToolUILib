
#pragma once

#include "ObjectIteration.h"
#include "Optional.h"


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

	std::string _data;
	Array<CompactScope> _starts;
	Array<bool> _inArray;

	// null = no indent or spacing
	// other values = the specified string will be used for indentation ..
	// .. but the output will also contain extra spacing/newlines
	const char* indent = "\t";

	JSONLinearWriter();

	void WriteString(StringView key, StringView value);
	void WriteNull(StringView key);
	void WriteBool(StringView key, bool value);
	void WriteInt(StringView key, int value) { WriteInt(key, int64_t(value)); }
	void WriteInt(StringView key, unsigned value) { WriteInt(key, uint64_t(value)); }
	void WriteInt(StringView key, int64_t value);
	void WriteInt(StringView key, uint64_t value);
	void WriteFloatSingle(StringView key, float value);
	void WriteFloatDouble(StringView key, double value);
	void WriteRawNumber(StringView key, StringView value);

	void BeginArray(StringView key);
	void EndArray();
	void BeginDict(StringView key);
	void EndDict();

	std::string& GetData();

	void _WriteIndent(bool skipComma = false);
	void _WritePrefix(StringView key);
	void _OnEndObject();
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

	json_value_s* _root = nullptr;
	Array<StackElement> _stack;

	~JSONLinearReader();
	void _Free();

	bool Parse(StringView all, unsigned flags = JPF_Default);

	bool HasMoreArrayElements();
	size_t GetCurrentArraySize();
	Entry* FindEntry(StringView key);
	Optional<std::string> ReadString(StringView key);
	Optional<bool> ReadBool(StringView key);
	Optional<int> ReadInt(StringView key);
	Optional<unsigned> ReadUInt(StringView key);
	Optional<int64_t> ReadInt64(StringView key);
	Optional<uint64_t> ReadUInt64(StringView key);
	Optional<double> ReadFloat(StringView key);
	bool BeginArray(StringView key);
	void EndArray();
	bool BeginDict(StringView key);
	void EndDict();
	void BeginEntry(Entry* E);
	void EndEntry();
};

struct JSONSerializerObjectIterator : JSONLinearWriter, IObjectIteratorMinTypeSerializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Serializer | OITF_KeyMapped; }

	bool BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	ArrayFieldState BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	bool OnFieldNull(const FieldInfo& FI) override;
	bool OnFieldBool(const FieldInfo& FI, bool& val) override;
	bool OnFieldS64(const FieldInfo& FI, int64_t& val) override;
	bool OnFieldU64(const FieldInfo& FI, uint64_t& val) override;
	bool OnFieldF32(const FieldInfo& FI, float& val) override;
	bool OnFieldF64(const FieldInfo& FI, double& val) override;
	bool OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	bool OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

struct JSONUnserializerObjectIterator : JSONLinearReader, IObjectIteratorMinTypeUnserializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Unserializer | OITF_KeyMapped; }

	bool BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	ArrayFieldState BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;
	bool HasMoreArrayElements() override;
	bool HasField(StringView name) override;

	bool OnFieldNull(const FieldInfo& FI) override;
	bool OnFieldBool(const FieldInfo& FI, bool& val) override;
	bool OnFieldS64(const FieldInfo& FI, int64_t& val) override;
	bool OnFieldU64(const FieldInfo& FI, uint64_t& val) override;
	bool OnFieldF64(const FieldInfo& FI, double& val) override;
	bool OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	bool OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

} // ui
