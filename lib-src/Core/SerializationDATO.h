
#pragma once

#include "ObjectIteration.h"
#include "Optional.h"
#include "Array.h"

#include "../../ThirdParty/dato_reader.hpp"
#include "../../ThirdParty/dato_writer.hpp"


namespace ui {

struct DATOLinearWriter
{
	struct Entry
	{
		dato::KeyRef key;
		dato::ValueRef vref;
	};

	struct StagingObject
	{
		// TODO can be optimized and worth it?
		Array<dato::ValueRef> entriesArr;
		Array<dato::StringMapEntry> entriesStrMap;
		bool hasKeys = true;
	};

	dato::Writer _writer;
	Array<StagingObject> _stack = { {} };

	DATOLinearWriter(StringView prefix = "DATO", bool aligned = true, bool sortKeys = true, bool skipDuplicateKeys = true);

	void _AppendElem(const char* key, dato::ValueRef vref);

	void WriteNull(const char* key) { _AppendElem(key, _writer.WriteNull()); }
	void WriteBool(const char* key, bool v) { _AppendElem(key, _writer.WriteBool(v)); }
	void WriteInt32(const char* key, i32 v) { _AppendElem(key, _writer.WriteS32(v)); }
	void WriteUInt32(const char* key, u32 v) { _AppendElem(key, _writer.WriteU32(v)); }
	void WriteFloat32(const char* key, float v) { _AppendElem(key, _writer.WriteF32(v)); }
	void WriteInt64(const char* key, i64 v) { _AppendElem(key, _writer.WriteS64(v)); }
	void WriteUInt64(const char* key, u64 v) { _AppendElem(key, _writer.WriteU64(v)); }
	void WriteFloat64(const char* key, double v) { _AppendElem(key, _writer.WriteF64(v)); }

	void WriteString(const char* key, StringView s) { _AppendElem(key, _writer.WriteString8(s.Data(), s.Size())); }
	void WriteBytes(const char* key, StringView ba) { _AppendElem(key, _writer.WriteByteArray(ba.Data(), ba.Size())); }

	void WriteVectorInt8(const char* key, u8 num, const i8* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorUInt8(const char* key, u8 num, const u8* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorInt16(const char* key, u8 num, const i16* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorUInt16(const char* key, u8 num, const u16* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorInt32(const char* key, u8 num, const i32* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorUInt32(const char* key, u8 num, const u32* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorInt64(const char* key, u8 num, const i64* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorUInt64(const char* key, u8 num, const u64* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorFloat32(const char* key, u8 num, const float* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }
	void WriteVectorFloat64(const char* key, u8 num, const double* v) { _AppendElem(key, _writer.WriteVectorT(v, num)); }

	dato::ValueRef _WriteAndRemoveTopObject();
	void BeginObject(const char* key);
	void EndObject();
	void BeginArray(const char* key);
	void EndArray();

	StringView GetData();
};

struct DATOLinearReader
{
	struct StackElement
	{
		u8 type;
		u32 curElemIndex;
		dato::Reader::StringMapAccessor smap;
		dato::Reader::ArrayAccessor arr;
	};

	dato::Reader _reader;
	Array<StackElement> _stack;

	bool Init(StringView data, StringView prefix = "DATO");

	bool HasMoreArrayElements();
	size_t GetCurrentArraySize();
	dato::Reader::DynamicAccessor FindEntry(const char* key);

	Optional<i32> ReadInt32(const char* key);
	Optional<u32> ReadUInt32(const char* key);
	Optional<i64> ReadInt64(const char* key);
	Optional<u64> ReadUInt64(const char* key);
	Optional<float> ReadFloat32(const char* key);
	Optional<double> ReadFloat64(const char* key);

	Optional<StringView> ReadString(const char* key);
	Optional<StringView> ReadBytes(const char* key);

	Optional<ArrayView<i8>> ReadVectorInt8(const char* key, u8 expectedElemCount);
	Optional<ArrayView<u8>> ReadVectorUInt8(const char* key, u8 expectedElemCount);
	Optional<ArrayView<i16>> ReadVectorInt16(const char* key, u8 expectedElemCount);
	Optional<ArrayView<u16>> ReadVectorUInt16(const char* key, u8 expectedElemCount);
	Optional<ArrayView<i32>> ReadVectorInt32(const char* key, u8 expectedElemCount);
	Optional<ArrayView<u32>> ReadVectorUInt32(const char* key, u8 expectedElemCount);
	Optional<ArrayView<i64>> ReadVectorInt64(const char* key, u8 expectedElemCount);
	Optional<ArrayView<u64>> ReadVectorUInt64(const char* key, u8 expectedElemCount);
	Optional<ArrayView<float>> ReadVectorFloat32(const char* key, u8 expectedElemCount);
	Optional<ArrayView<double>> ReadVectorFloat64(const char* key, u8 expectedElemCount);

	bool BeginArray(const char* key);
	void EndArray();
	bool BeginObject(const char* key);
	void EndObject();
	void BeginEntry(const dato::Reader::DynamicAccessor& E);
	void EndEntry();
};


struct DATOSerializer : DATOLinearWriter, IObjectIterator
{
	using DATOLinearWriter::DATOLinearWriter;

	unsigned GetFlags() const override { return OI_TYPE_Serializer | OITF_KeyMapped | OITF_Binary; }

	bool BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	ArrayFieldState BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	bool OnFieldNull(const FieldInfo& FI) override;
	bool OnFieldBool(const FieldInfo& FI, bool& val) override;
	bool OnFieldS8(const FieldInfo& FI, i8& val) override;
	bool OnFieldU8(const FieldInfo& FI, u8& val) override;
	bool OnFieldS16(const FieldInfo& FI, i16& val) override;
	bool OnFieldU16(const FieldInfo& FI, u16& val) override;
	bool OnFieldS32(const FieldInfo& FI, i32& val) override;
	bool OnFieldU32(const FieldInfo& FI, u32& val) override;
	bool OnFieldS64(const FieldInfo& FI, i64& val) override;
	bool OnFieldU64(const FieldInfo& FI, u64& val) override;
	bool OnFieldF32(const FieldInfo& FI, float& val) override;
	bool OnFieldF64(const FieldInfo& FI, double& val) override;

	bool OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	bool OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override;
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override;
};

struct DATOUnserializer : DATOLinearReader, IObjectIterator
{
	unsigned GetFlags() const override { return OI_TYPE_Unserializer | OITF_KeyMapped | OITF_Binary; }

	bool BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) override;
	void EndObject() override;
	ArrayFieldState BeginArray(size_t size, const FieldInfo& FI) override;
	void EndArray() override;

	bool HasMoreArrayElements() override;
	bool HasField(const char* name) override;

	bool OnFieldNull(const FieldInfo& FI) override;
	bool OnFieldBool(const FieldInfo& FI, bool& val) override;
	bool OnFieldS8(const FieldInfo& FI, i8& val) override;
	bool OnFieldU8(const FieldInfo& FI, u8& val) override;
	bool OnFieldS16(const FieldInfo& FI, i16& val) override;
	bool OnFieldU16(const FieldInfo& FI, u16& val) override;
	bool OnFieldS32(const FieldInfo& FI, i32& val) override;
	bool OnFieldU32(const FieldInfo& FI, u32& val) override;
	bool OnFieldS64(const FieldInfo& FI, i64& val) override;
	bool OnFieldU64(const FieldInfo& FI, u64& val) override;
	bool OnFieldF32(const FieldInfo& FI, float& val) override;
	bool OnFieldF64(const FieldInfo& FI, double& val) override;

	bool OnFieldString(const FieldInfo& FI, const IBufferRW& brw) override;
	bool OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) override;

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override;
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override;
};

} // ui
