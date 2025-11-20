
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

	void WriteString(const char* key, StringView value);
	void WriteNull(const char* key);
	void WriteBool(const char* key, bool value);
	void WriteInt(const char* key, int value) { WriteInt(key, int64_t(value)); }
	void WriteInt(const char* key, unsigned value) { WriteInt(key, uint64_t(value)); }
	void WriteInt(const char* key, int64_t value);
	void WriteInt(const char* key, uint64_t value);
	void _WriteFloatPrec(const char* key, double value, int prec);
	void WriteFloatSingle(const char* key, float value) { _WriteFloatPrec(key, value, 9); }
	void WriteFloatDouble(const char* key, double value) { _WriteFloatPrec(key, value, 17); }
	void WriteRawNumber(const char* key, StringView value);

	void BeginArray(const char* key);
	void EndArray();
	void BeginDict(const char* key);
	void EndDict();

	std::string& GetData();

	void _WriteIndent(bool skipComma = false);
	void _WritePrefix(const char* key);
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
};

struct JSONSerializerObjectIterator : JSONLinearWriter, IObjectIteratorMinTypeSerializeBase
{
	unsigned GetFlags() const override { return OI_TYPE_Serializer | OITF_KeyMapped; }

	bool BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
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
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;
	bool HasMoreArrayElements() override;
	bool HasField(const char* name) override;

	bool OnFieldNull(const FieldInfo& FI) override;
	bool OnFieldBool(const FieldInfo& FI, bool& val) override;
	bool OnFieldS64(const FieldInfo& FI, int64_t& val) override;
	bool OnFieldU64(const FieldInfo& FI, uint64_t& val) override;
	bool OnFieldF64(const FieldInfo& FI, double& val) override;
	bool OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	bool OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

} // ui
