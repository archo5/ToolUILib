
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

	bool Parse(StringView all);

	bool HasMoreArrayElements();
	size_t GetCurrentArraySize();
	Entry* FindEntry(const char* key);
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

	json_value_s* _root = nullptr;
	std::vector<StackElement> _stack;
};

std::string Base16Encode(const void* src, size_t size);
std::string Base16Decode(StringView src, bool* valid = nullptr);

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
			*outName = ReadString("__");
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
		val = ReadBool(FI.name);
	}
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		val = ReadInt64(FI.name);
	}
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		val = ReadUInt64(FI.name);
	}
	void OnFieldF64(const FieldInfo& FI, double& val) override
	{
		val = ReadFloat(FI.name);
	}
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override
	{
		brw.Assign(ReadString(FI.name));
	}
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override
	{
		brw.Assign(Base16Decode(ReadString(FI.name)));
	}
};

} // ui
