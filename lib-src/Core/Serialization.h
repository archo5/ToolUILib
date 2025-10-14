
#pragma once

#include "String.h"
#include "ObjectIteration.h"

#include <inttypes.h>
#include <type_traits>
#include <string>


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
	void WriteFloatSingle(const char* key, float value);
	void WriteFloatDouble(const char* key, double value);
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

	Array<Entry> entries;
	Array<EntryRange> stack;
};

std::string Base16Encode(const void* src, size_t size);
std::string Base16Decode(StringView src, bool* valid = nullptr);
std::string Base64Encode(const void* src, size_t size, const char* endChars = "+/", bool pad = true);
std::string Base64Decode(StringView src, const char* endChars = "+/", bool needPadding = false, bool* valid = nullptr);

} // ui
