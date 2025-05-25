
#pragma once

#include "ObjectIteration.h"
#include "Optional.h"
#include "HashMap.h"


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

TypedArray<T> =
{
	size: u32
	data: T[size]
}

ByteArray = TypedArray<u8>
note: also used for the string type

Value = s32 | u32 | f32 | u32-offset-to(s64 | u64 | f64 | Array | Object | TypedArray)

*/

namespace ui {

enum class BKVT_Type : u8
{
	Null,
	// embedded value types
	S32,
	U32,
	F32,
	// external value reference types
	// - one value
	S64,
	U64,
	F64,
	// - generic containers
	Array,
	Object,
	// - raw arrays (identified by purpose, 8-bit elements)
	String, // expected to be UTF-8, contains an extra byte for inline reading (not included in the size)
	ByteArray,
	// - typed arrays (identified by type)
	TypedArrayS8,
	TypedArrayU8,
	TypedArrayS16,
	TypedArrayU16,
	TypedArrayS32,
	TypedArrayU32,
	TypedArrayS64,
	TypedArrayU64,
	TypedArrayF32,
	TypedArrayF64,

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

	struct KeyStringRef
	{
		char const* const* src;
		u32 off;
		u8 len;

		struct Comparer
		{
			static bool AreEqual(const KeyStringRef& a, const KeyStringRef& b)
			{
				return a.len == b.len && memcmp(*a.src + a.off, *b.src + b.off, a.len) == 0;
			}
			static size_t GetHash(const KeyStringRef& v)
			{
				return HashValue(StringView(*v.src + v.off, v.len));
			}
		};
	};

	Array<StagingObject> _stack = { {} };
	Array<char> _data = { 'B', 'K', 'V', 'T', 0, 0, 0, 0 };
	HashMap<KeyStringRef, u32, KeyStringRef::Comparer> _writtenKeys;

	bool skipDuplicateKeys = true;

	u32 _AppendKey(const char* key);
	u32 _AppendMem(const void* mem, size_t size);
	u32 _AppendArray(size_t num, size_t nbytes, const void* data);
	template <class R> UI_FORCEINLINE u32 _AppendTypedArray(const R& r)
	{
		return _AppendArray(r.Size(), r.SizeInBytes(), r.Data());
	}
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

	void WriteTypedArrayInt8(const char* key, ArrayView<i8> v);
	void WriteTypedArrayUInt8(const char* key, ArrayView<u8> v);
	void WriteTypedArrayInt16(const char* key, ArrayView<i16> v);
	void WriteTypedArrayUInt16(const char* key, ArrayView<u16> v);
	void WriteTypedArrayInt32(const char* key, ArrayView<i32> v);
	void WriteTypedArrayUInt32(const char* key, ArrayView<u32> v);
	void WriteTypedArrayInt64(const char* key, ArrayView<i64> v);
	void WriteTypedArrayUInt64(const char* key, ArrayView<u64> v);
	void WriteTypedArrayFloat32(const char* key, ArrayView<float> v);
	void WriteTypedArrayFloat64(const char* key, ArrayView<double> v);

	u32 _WriteAndRemoveTopObject();
	void BeginObject(const char* key);
	void EndObject();
	void BeginArray(const char* key);
	void EndArray();

	StringView GetData(bool withPrefix);
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

	Optional<i32> ReadInt32(const char* key);
	Optional<u32> ReadUInt32(const char* key);
	Optional<i64> ReadInt64(const char* key);
	Optional<u64> ReadUInt64(const char* key);
	Optional<float> ReadFloat32(const char* key);
	Optional<double> ReadFloat64(const char* key);

	Optional<StringView> ReadString(const char* key);
	Optional<StringView> ReadBytes(const char* key);

	Optional<ArrayView<i8>> ReadTypedArrayInt8(const char* key);
	Optional<ArrayView<u8>> ReadTypedArrayUInt8(const char* key);
	Optional<ArrayView<i16>> ReadTypedArrayInt16(const char* key);
	Optional<ArrayView<u16>> ReadTypedArrayUInt16(const char* key);
	Optional<ArrayView<i32>> ReadTypedArrayInt32(const char* key);
	Optional<ArrayView<u32>> ReadTypedArrayUInt32(const char* key);
	Optional<ArrayView<i64>> ReadTypedArrayInt64(const char* key);
	Optional<ArrayView<u64>> ReadTypedArrayUInt64(const char* key);
	Optional<ArrayView<float>> ReadTypedArrayFloat32(const char* key);
	Optional<ArrayView<double>> ReadTypedArrayFloat64(const char* key);

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
	template <class T>
	ArrayView<T> _ReadTypedArray(u32 pos)
	{
		u32 len = _ReadU32(pos);
		pos += 4;
		return ArrayView<T>((const T*)(_data.Data() + pos), len);
	}

	StringView _data;
	Array<StackElement> _stack;
};

struct BKVTSerializer : BKVTLinearWriter, IObjectIterator
{
	unsigned GetFlags() const override { return OI_TYPE_Serializer | OITF_KeyMapped | OITF_Binary; }

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

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override;
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override;
};

struct BKVTUnserializer : BKVTLinearReader, IObjectIterator
{
	unsigned GetFlags() const override { return OI_TYPE_Unserializer | OITF_KeyMapped | OITF_Binary; }

	void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	size_t BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	bool HasMoreArrayElements() override;
	bool HasField(const char* name) override;

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

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override;
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override;
};

} // ui
