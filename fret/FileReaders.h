
#pragma once
#include "pch.h"


struct IDataSource
{
	virtual ~IDataSource() {}
	virtual size_t Read(uint64_t at, size_t size, void* out) = 0;
	virtual uint64_t GetSize() = 0;

	void GetASCIIText(char* buf, size_t bufsz, uint64_t pos);
	void GetInt8Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetInt16Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetInt32Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetInt64Text(char* buf, size_t bufsz, uint64_t pos, bool sign);
	void GetFloat32Text(char* buf, size_t bufsz, uint64_t pos);
	void GetFloat64Text(char* buf, size_t bufsz, uint64_t pos);
};

struct MemoryDataSource : IDataSource
{
	MemoryDataSource(void* mem, size_t size, bool own);
	~MemoryDataSource();

	size_t Read(uint64_t at, size_t size, void* out) override;
	uint64_t GetSize() override;

	void* _mem;
	size_t _size;
	bool _own;
};

struct FileDataSource : IDataSource
{
	FileDataSource(const char* path);
	~FileDataSource();

	size_t Read(uint64_t at, size_t size, void* out) override;
	uint64_t GetSize() override;

	FILE* _fp;
	uint64_t _pos = 0;
	uint64_t _size = 0;
};

struct SliceDataSource : IDataSource
{
	SliceDataSource(IDataSource* src, uint64_t off, uint64_t size);
	~SliceDataSource();

	size_t Read(uint64_t at, size_t size, void* out) override;
	uint64_t GetSize() override;

	IDataSource* _src;
	uint64_t _off;
	uint64_t _size;
};


// TODO move to core
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
