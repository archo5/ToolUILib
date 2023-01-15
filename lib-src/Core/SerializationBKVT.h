
#pragma once

#include "ObjectIteration.h"


// BKVT = binary key-value tree
// - a format that is readable without copying the tree elsewhere
// - uninteresting parts can be skipped, the format can be extended with additional types
/*

Root = u32-offset-to(Object)

Object =
{
	size: u16
	keys: Key[size]
	values: Value[size]
	types: u8[size]
}

Array =
{
	size: u32
	values: Value[size]
	types: u8[size]
}

Key =
{
	length: u8
	text: char[length]
	nullterm: char(0)
}

ByteArray =
{
	size: u32
	data: u8[size]
}
note: also used for the string type

Value = s32 | u32 | f32 | u32-offset-to(s64 | u64 | f64 | ByteArray | Array | Object)

*/

namespace ui {

enum class BKVT_Type : u8
{
	Null,
	// embedded value types
	S32,
	U32,
	F32,
	// references value types
	S64,
	U64,
	F64,
	String, // contains an extra byte for inline reading (not included in the size)
	ByteArray,
	Array,
	Object,

	NotFound = 255,
};

struct BKVTLinearWriter
{
	struct Entry
	{
		u8 type;
		u32 key;
		u32 value;
	};

	struct StagingObject
	{
		Array<Entry> entries;
		bool hasKeys = true;
	};

	u32 _AppendKey(const char* key);
	u32 _AppendMem(const void* mem, size_t size);
	void _WriteElem(const char* key, BKVT_Type type, u32 val);

	void WriteNull(const char* key);
	void WriteInt32(const char* key, i32 v);
	void WriteUInt32(const char* key, u32 v);
	void WriteFloat32(const char* key, float v);
	void WriteInt64(const char* key, i64 v);
	void WriteUInt64(const char* key, u64 v);
	void WriteFloat64(const char* key, double v);
	void WriteString(const char* key, StringView s);
	void WriteBytes(const char* key, StringView ba);

	u32 _WriteAndRemoveTopObject();
	void BeginObject(const char* key);
	void EndObject();
	void BeginArray(const char* key);
	void EndArray();

	StringView GetData(bool withPrefix);

	Array<StagingObject> _stack = { {} };
	Array<char> _data = { 'B', 'K', 'V', 'T', 0, 0, 0, 0 };
};

struct BKVTLinearReader
{
	struct EntryRef
	{
		u32 pos;
		BKVT_Type type;
	};
	struct StackElement
	{
		EntryRef entry;
		u32 curElemIndex;
	};

	// hasPrefix = check if data starts with "BKVT"
	bool Init(StringView data, bool hasPrefix);

	bool HasMoreArrayElements();
	size_t GetCurrentArraySize();
	EntryRef FindEntry(const char* key);
	Optional<StringView> ReadString(const char* key);
	Optional<StringView> ReadBytes(const char* key);
	Optional<i32> ReadInt32(const char* key);
	Optional<u32> ReadUInt32(const char* key);
	Optional<i64> ReadInt64(const char* key);
	Optional<u64> ReadUInt64(const char* key);
	Optional<float> ReadFloat32(const char* key);
	Optional<double> ReadFloat64(const char* key);
	bool BeginArray(const char* key);
	void EndArray();
	bool BeginObject(const char* key);
	void EndObject();
	void BeginEntry(EntryRef E);
	void EndEntry();

	u8 _ReadU8(u32 pos);
	u16 _ReadU16(u32 pos);
	u32 _ReadU32(u32 pos);
	u64 _ReadU64(u32 pos);
	StringView _GetKeyString(u32 pos);
	StringView _GetString(u32 pos);

	StringView _data;
	Array<StackElement> _stack;
};

struct BKVTSerializer : BKVTLinearWriter, IObjectIterator
{
	unsigned GetFlags() const override { OI_TYPE_Serializer | OIF_KeyMapped | OIF_Binary; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	void OnFieldBool(const FieldInfo& FI, bool& val) override;
	void OnFieldS8(const FieldInfo& FI, i8& val) override;
	void OnFieldU8(const FieldInfo& FI, u8& val) override;
	void OnFieldS16(const FieldInfo& FI, i16& val) override;
	void OnFieldU16(const FieldInfo& FI, u16& val) override;
	void OnFieldS32(const FieldInfo& FI, i32& val) override;
	void OnFieldU32(const FieldInfo& FI, u32& val) override;
	void OnFieldS64(const FieldInfo& FI, i64& val) override;
	void OnFieldU64(const FieldInfo& FI, u64& val) override;
	void OnFieldF32(const FieldInfo& FI, float& val) override;
	void OnFieldF64(const FieldInfo& FI, double& val) override;
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

struct BKVTUnserializer : BKVTLinearReader, IObjectIterator
{
	unsigned GetFlags() const override { OI_TYPE_Unserializer | OIF_KeyMapped | OIF_Binary; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	void OnFieldBool(const FieldInfo& FI, bool& val) override;
	void OnFieldS8(const FieldInfo& FI, i8& val) override;
	void OnFieldU8(const FieldInfo& FI, u8& val) override;
	void OnFieldS16(const FieldInfo& FI, i16& val) override;
	void OnFieldU16(const FieldInfo& FI, u16& val) override;
	void OnFieldS32(const FieldInfo& FI, i32& val) override;
	void OnFieldU32(const FieldInfo& FI, u32& val) override;
	void OnFieldS64(const FieldInfo& FI, i64& val) override;
	void OnFieldU64(const FieldInfo& FI, u64& val) override;
	void OnFieldF32(const FieldInfo& FI, float& val) override;
	void OnFieldF64(const FieldInfo& FI, double& val) override;
	void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;
};

} // ui
