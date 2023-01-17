
#include "SerializationBKVT.h"


namespace ui {

template <class To, class From>
static To BitCast(From v)
{
	To ret;
	memcpy(&ret, &v, sizeof(ret));
	return ret;
}

u32 BKVTLinearWriter::_AppendKey(const char* key)
{
	if (!key)
		key = "";
	u8 len = u8(min(strlen(key), size_t(255)));

	if (skipDuplicateKeys)
	{
		KeyStringRef ref = { &key, 0, len };
		if (auto* poff = _writtenKeys.GetValuePtr(ref))
			return *poff;

		u32 ret = _data.Size() - 4;
		_data.Append(len);
		_data.AppendMany(key, len);
		_data.Append(0);

		KeyStringRef wkey = { &_data._data, ret + 5, len };
		_writtenKeys.Insert(wkey, ret);

		return ret;
	}
	else
	{
		u32 ret = _data.Size() - 4;
		_data.Append(len);
		_data.AppendMany(key, len);
		_data.Append(0);
		return ret;
	}
}

u32 BKVTLinearWriter::_AppendMem(const void* mem, size_t size)
{
	u32 ret = _data.Size() - 4;
	_data.AppendMany((const char*)mem, size);
	return ret;
}

u32 BKVTLinearWriter::_AppendArray(size_t num, size_t esize, const void* data)
{
	u32 slen = u32(num);
	u32 spos = _AppendMem(&slen, sizeof(slen));
	_data.AppendMany((const char*)data, num * esize);
	return spos;
}

void BKVTLinearWriter::_WriteElem(const char* key, BKVT_Type type, u32 val)
{
	auto& S = _stack.Last();
	S.entries.Append({ u8(type), S.hasKeys ? _AppendKey(key) : 0, val });
}

void BKVTLinearWriter::WriteNull(const char* key)
{
	_WriteElem(key, BKVT_Type::Null, 0);
}

void BKVTLinearWriter::WriteInt32(const char* key, i32 v)
{
	_WriteElem(key, BKVT_Type::S32, v);
}

void BKVTLinearWriter::WriteUInt32(const char* key, u32 v)
{
	_WriteElem(key, BKVT_Type::U32, v);
}

void BKVTLinearWriter::WriteFloat32(const char* key, float v)
{
	_WriteElem(key, BKVT_Type::F32, BitCast<u32>(v));
}

void BKVTLinearWriter::WriteInt64(const char* key, i64 v)
{
	_WriteElem(key, BKVT_Type::S64, _AppendMem(&v, sizeof(v)));
}

void BKVTLinearWriter::WriteUInt64(const char* key, u64 v)
{
	_WriteElem(key, BKVT_Type::U64, _AppendMem(&v, sizeof(v)));
}

void BKVTLinearWriter::WriteFloat64(const char* key, double v)
{
	_WriteElem(key, BKVT_Type::F64, _AppendMem(&v, sizeof(v)));
}

void BKVTLinearWriter::WriteString(const char* key, StringView s)
{
	u32 slen = s.Size();
	u32 spos = _AppendMem(&slen, sizeof(slen));
	_data.AppendRange(s);
	_data.Append(0);
	_WriteElem(key, BKVT_Type::String, spos);
}

void BKVTLinearWriter::WriteBytes(const char* key, StringView ba)
{
	u32 slen = ba.Size();
	u32 spos = _AppendMem(&slen, sizeof(slen));
	_data.AppendRange(ba);
	_WriteElem(key, BKVT_Type::ByteArray, spos);
}

void BKVTLinearWriter::WriteTypedArrayInt8(const char* key, ArrayView<i8> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayS8, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayUInt8(const char* key, ArrayView<u8> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayU8, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayInt16(const char* key, ArrayView<i16> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayS16, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayUInt16(const char* key, ArrayView<u16> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayU16, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayInt32(const char* key, ArrayView<i32> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayS32, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayUInt32(const char* key, ArrayView<u32> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayU32, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayInt64(const char* key, ArrayView<i64> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayS64, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayUInt64(const char* key, ArrayView<u64> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayU64, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayFloat32(const char* key, ArrayView<float> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayF32, _AppendTypedArray(v));
}

void BKVTLinearWriter::WriteTypedArrayFloat64(const char* key, ArrayView<double> v)
{
	_WriteElem(key, BKVT_Type::TypedArrayF64, _AppendTypedArray(v));
}

u32 BKVTLinearWriter::_WriteAndRemoveTopObject()
{
	auto& S = _stack.Last();
	u16 num = u16(S.entries.Size());

	u32 pos = _AppendMem(&num, sizeof(num));
	for (auto& e : S.entries)
		_AppendMem(&e.key, sizeof(e.key));
	for (auto& e : S.entries)
		_AppendMem(&e.value, sizeof(e.value));
	for (auto& e : S.entries)
		_data.Append(e.type);

	_stack.RemoveLast();
	return pos;
}

void BKVTLinearWriter::BeginObject(const char* key)
{
	_WriteElem(key, BKVT_Type::Object, 0); // value to be filled in later
	_stack.Append({});
	_stack.Last().hasKeys = true;
}

void BKVTLinearWriter::EndObject()
{
	u32 pos = _WriteAndRemoveTopObject();
	_stack.Last().entries.Last().value = pos;
}

void BKVTLinearWriter::BeginArray(const char* key)
{
	_WriteElem(key, BKVT_Type::Array, 0); // value to be filled in later
	_stack.Append({});
	_stack.Last().hasKeys = false;
}

void BKVTLinearWriter::EndArray()
{
	auto& S = _stack.Last();
	u32 num = S.entries.Size();

	u32 pos = _AppendMem(&num, sizeof(num));
	for (auto& e : S.entries)
		_AppendMem(&e.value, sizeof(e.value));
	for (auto& e : S.entries)
		_data.Append(e.type);

	_stack.RemoveLast();

	_stack.Last().entries.Last().value = pos;
}

StringView BKVTLinearWriter::GetData(bool withPrefix)
{
	while (_stack.Size() > 1)
	{
		if (_stack.Last().hasKeys)
			EndObject();
		else
			EndArray();
	}
	if (_stack.Size() == 1)
	{
		// finalize
		u32 rootpos = _WriteAndRemoveTopObject();
		memcpy(&_data[4], &rootpos, sizeof(rootpos));
	}
	StringView ret = _data;
	return withPrefix ? ret : ret.substr(4);
}


bool BKVTLinearReader::Init(StringView data, bool hasPrefix)
{
	if (!hasPrefix)
	{
		if (data.Size() < 6)
			return false; // min. valid size = 6 (pointer to root object + empty object)
	}
	else
	{
		if (!data.starts_with("BKVT"))
			return false;
		if (data.Size() < 10)
			return false;
	}
	_data = data.substr(4);
	_stack.Append({ { _ReadU32(0), BKVT_Type::Object }, 0 });
	return true;
}

bool BKVTLinearReader::HasMoreArrayElements()
{
	if (_stack.IsEmpty())
		return false;

	auto& S = _stack.Last();
	return S.entry.type == BKVT_Type::Array
		&& S.curElemIndex < _ReadU32(S.entry.pos);
}

size_t BKVTLinearReader::GetCurrentArraySize()
{
	if (_stack.IsEmpty())
		return 0;

	auto& S = _stack.Last();
	if (S.entry.type != BKVT_Type::Array)
		return 0;

	return _ReadU32(_stack.Last().entry.pos);
}

BKVTLinearReader::EntryRef BKVTLinearReader::FindEntry(const char* key)
{
	if (_stack.IsEmpty())
		return { 0, BKVT_Type::NotFound };

	auto& S = _stack.Last();
	if (S.entry.type == BKVT_Type::Array)
	{
		u32 idx = S.curElemIndex++;
		u32 len = _ReadU32(S.entry.pos);
		if (idx < len)
		{
			EntryRef e;
			{
				e.pos = _ReadU32(S.entry.pos + 4 + idx * 4);
				e.type = BKVT_Type(_ReadU8(S.entry.pos + 4 + len * 4 + idx));
			}
			return e;
		}
	}
	else if (S.entry.type == BKVT_Type::Object)
	{
		//StringView svkey = key;
		u32 pos = S.entry.pos;
		u32 num = _ReadU16(pos);
		pos += 2;
		for (u32 i = 0; i < num; i++)
		{
			//if (_GetKeyString(_ReadU32(pos + i * 4)) == svkey)
			if (strcmp(_GetKeyString(_ReadU32(pos + i * 4)).Data(), key) == 0) // slightly faster than the other option
			{
				EntryRef e;
				{
					e.pos = _ReadU32(pos + (num + i) * 4);
					e.type = BKVT_Type(_ReadU8(pos + num * 8 + i));
				}
				return e;
			}
		}
	}
	return { 0, BKVT_Type::NotFound };
}

Optional<i32> BKVTLinearReader::ReadInt32(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::S32 || e.type == BKVT_Type::U32)
		return e.pos;
	if (e.type == BKVT_Type::F32)
		return i32(BitCast<float>(e.pos));
	if (e.type == BKVT_Type::S64)
		return i32(i64(_ReadU64(e.pos)));
	if (e.type == BKVT_Type::U64)
		return i32(_ReadU64(e.pos));
	if (e.type == BKVT_Type::F64)
		return i32(BitCast<double>(_ReadU64(e.pos)));
	return {};
}

Optional<u32> BKVTLinearReader::ReadUInt32(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::S32 || e.type == BKVT_Type::U32)
		return e.pos;
	if (e.type == BKVT_Type::F32)
		return u32(BitCast<float>(e.pos));
	if (e.type == BKVT_Type::S64 || e.type == BKVT_Type::U64)
		return u32(_ReadU64(e.pos));
	if (e.type == BKVT_Type::F64)
		return u32(BitCast<double>(_ReadU64(e.pos)));
	return {};
}

Optional<i64> BKVTLinearReader::ReadInt64(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::S32)
		return i32(e.pos);
	if (e.type == BKVT_Type::U32)
		return e.pos;
	if (e.type == BKVT_Type::F32)
		return i64(BitCast<float>(e.pos));
	if (e.type == BKVT_Type::S64 || e.type == BKVT_Type::U64)
		return i64(_ReadU64(e.pos));
	if (e.type == BKVT_Type::F64)
		return i64(BitCast<double>(_ReadU64(e.pos)));
	return {};
}

Optional<u64> BKVTLinearReader::ReadUInt64(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::S32)
		return i32(e.pos);
	if (e.type == BKVT_Type::U32)
		return e.pos;
	if (e.type == BKVT_Type::F32)
		return u64(BitCast<float>(e.pos));
	if (e.type == BKVT_Type::S64 || e.type == BKVT_Type::U64)
		return u64(_ReadU64(e.pos));
	if (e.type == BKVT_Type::F64)
		return u64(BitCast<double>(_ReadU64(e.pos)));
	return {};
}

Optional<float> BKVTLinearReader::ReadFloat32(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::S32)
		return float(i32(e.pos));
	if (e.type == BKVT_Type::U32)
		return float(e.pos);
	if (e.type == BKVT_Type::F32)
		return BitCast<float>(e.pos);
	if (e.type == BKVT_Type::S64)
		return float(i64(_ReadU64(e.pos)));
	if (e.type == BKVT_Type::U64)
		return float(_ReadU64(e.pos));
	if (e.type == BKVT_Type::F64)
		return float(BitCast<double>(_ReadU64(e.pos)));
	return {};
}

Optional<double> BKVTLinearReader::ReadFloat64(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::S32)
		return i32(e.pos);
	if (e.type == BKVT_Type::U32)
		return e.pos;
	if (e.type == BKVT_Type::F32)
		return BitCast<float>(e.pos);
	if (e.type == BKVT_Type::S64)
		return double(i64(_ReadU64(e.pos)));
	if (e.type == BKVT_Type::U64)
		return double(_ReadU64(e.pos));
	if (e.type == BKVT_Type::F64)
		return BitCast<double>(_ReadU64(e.pos));
	return {};
}

Optional<StringView> BKVTLinearReader::ReadString(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::String)
		return _GetString(e.pos);
	return {};
}

Optional<StringView> BKVTLinearReader::ReadBytes(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::ByteArray)
		return _GetString(e.pos);
	return {};
}

Optional<ArrayView<i8>> BKVTLinearReader::ReadTypedArrayInt8(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayS8)
		return _ReadTypedArray<i8>(e.pos);
	return {};
}

Optional<ArrayView<u8>> BKVTLinearReader::ReadTypedArrayUInt8(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayU8)
		return _ReadTypedArray<u8>(e.pos);
	return {};
}

Optional<ArrayView<i16>> BKVTLinearReader::ReadTypedArrayInt16(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayS16)
		return _ReadTypedArray<i16>(e.pos);
	return {};
}

Optional<ArrayView<u16>> BKVTLinearReader::ReadTypedArrayUInt16(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayU16)
		return _ReadTypedArray<u16>(e.pos);
	return {};
}

Optional<ArrayView<i32>> BKVTLinearReader::ReadTypedArrayInt32(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayS32)
		return _ReadTypedArray<i32>(e.pos);
	return {};
}

Optional<ArrayView<u32>> BKVTLinearReader::ReadTypedArrayUInt32(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayU32)
		return _ReadTypedArray<u32>(e.pos);
	return {};
}

Optional<ArrayView<i64>> BKVTLinearReader::ReadTypedArrayInt64(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayS64)
		return _ReadTypedArray<i64>(e.pos);
	return {};
}

Optional<ArrayView<u64>> BKVTLinearReader::ReadTypedArrayUInt64(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayU64)
		return _ReadTypedArray<u64>(e.pos);
	return {};
}

Optional<ArrayView<float>> BKVTLinearReader::ReadTypedArrayFloat32(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayF32)
		return _ReadTypedArray<float>(e.pos);
	return {};
}

Optional<ArrayView<double>> BKVTLinearReader::ReadTypedArrayFloat64(const char* key)
{
	auto e = FindEntry(key);
	if (e.type == BKVT_Type::TypedArrayF64)
		return _ReadTypedArray<double>(e.pos);
	return {};
}

bool BKVTLinearReader::BeginArray(const char* key)
{
	auto e = FindEntry(key);
	_stack.Append({ e, 0 });
	return e.type == BKVT_Type::Array;
}

void BKVTLinearReader::EndArray()
{
	_stack.RemoveLast();
}

bool BKVTLinearReader::BeginObject(const char* key)
{
	auto e = FindEntry(key);
	_stack.Append({ e, 0 });
	return e.type == BKVT_Type::Object;
}

void BKVTLinearReader::EndObject()
{
	_stack.RemoveLast();
}

void BKVTLinearReader::BeginEntry(EntryRef E)
{
	_stack.Append({ E, 0 });
}

void BKVTLinearReader::EndEntry()
{
	_stack.RemoveLast();
}

u8 BKVTLinearReader::_ReadU8(u32 pos)
{
	return u8(_data[pos]);
}

u16 BKVTLinearReader::_ReadU16(u32 pos)
{
	u16 ret;
	memcpy(&ret, _data.Data() + pos, sizeof(ret));
	return ret;
}

u32 BKVTLinearReader::_ReadU32(u32 pos)
{
	u32 ret;
	memcpy(&ret, _data.Data() + pos, sizeof(ret));
	return ret;
}

u64 BKVTLinearReader::_ReadU64(u32 pos)
{
	u64 ret;
	memcpy(&ret, _data.Data() + pos, sizeof(ret));
	return ret;
}

StringView BKVTLinearReader::_GetKeyString(u32 pos)
{
	u8 len = _ReadU8(pos++);
	return StringView(_data.Data() + pos, len);
}

StringView BKVTLinearReader::_GetString(u32 pos)
{
	u32 len = _ReadU32(pos);
	pos += 4;
	return StringView(_data.Data() + pos, len);
}


void BKVTSerializer::BeginObject(const FieldInfo& FI, const char* objname, std::string* outName)
{
	BKVTLinearWriter::BeginObject(FI.GetNameOrEmptyStr());
	if (outName)
		WriteString("__", *outName);
}

void BKVTSerializer::EndObject()
{
	BKVTLinearWriter::EndObject();
}

size_t BKVTSerializer::BeginArray(size_t size, const FieldInfo& FI)
{
	BKVTLinearWriter::BeginArray(FI.GetNameOrEmptyStr());
	return 0;
}

void BKVTSerializer::EndArray()
{
	BKVTLinearWriter::EndArray();
}

void BKVTSerializer::OnFieldBool(const FieldInfo& FI, bool& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldS8(const FieldInfo& FI, i8& val)
{
	WriteInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldU8(const FieldInfo& FI, u8& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldS16(const FieldInfo& FI, i16& val)
{
	WriteInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldU16(const FieldInfo& FI, u16& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldS32(const FieldInfo& FI, i32& val)
{
	WriteInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldU32(const FieldInfo& FI, u32& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldS64(const FieldInfo& FI, i64& val)
{
	WriteInt64(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldU64(const FieldInfo& FI, u64& val)
{
	WriteUInt64(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldF32(const FieldInfo& FI, float& val)
{
	WriteFloat32(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldF64(const FieldInfo& FI, double& val)
{
	WriteFloat64(FI.GetNameOrEmptyStr(), val);
}

void BKVTSerializer::OnFieldString(const FieldInfo& FI, const IBufferRW& brw)
{
	WriteString(FI.GetNameOrEmptyStr(), brw.Read());
}

void BKVTSerializer::OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw)
{
	WriteBytes(FI.GetNameOrEmptyStr(), brw.Read());
}

bool BKVTSerializer::OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr)
{
	WriteTypedArrayInt32(FI.GetNameOrEmptyStr(), { arr, count });
	return true;
}

bool BKVTSerializer::OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr)
{
	WriteTypedArrayFloat32(FI.GetNameOrEmptyStr(), { arr, count });
	return true;
}


void BKVTUnserializer::BeginObject(const FieldInfo& FI, const char* objname, std::string* outName)
{
	BKVTLinearReader::BeginObject(FI.GetNameOrEmptyStr());
	if (outName)
	{
		if (auto s = ReadString("__"))
			*outName = to_string(s.GetValue());
	}
}

void BKVTUnserializer::EndObject()
{
	BKVTLinearReader::EndObject();
}

size_t BKVTUnserializer::BeginArray(size_t size, const FieldInfo& FI)
{
	BKVTLinearReader::BeginArray(FI.GetNameOrEmptyStr());
	return GetCurrentArraySize();
}

void BKVTUnserializer::EndArray()
{
	BKVTLinearReader::EndArray();
}

bool BKVTUnserializer::HasMoreArrayElements()
{
	return BKVTLinearReader::HasMoreArrayElements();
}

bool BKVTUnserializer::HasField(const char* name)
{
	return FindEntry(name).type != BKVT_Type::NotFound;
}

void BKVTUnserializer::OnFieldBool(const FieldInfo& FI, bool& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = v.GetValue() != 0;
}

void BKVTUnserializer::OnFieldS8(const FieldInfo& FI, i8& val)
{
	if (auto v = ReadInt32(FI.GetNameOrEmptyStr()))
		val = i8(v.GetValue());
}

void BKVTUnserializer::OnFieldU8(const FieldInfo& FI, u8& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = u8(v.GetValue());
}

void BKVTUnserializer::OnFieldS16(const FieldInfo& FI, i16& val)
{
	if (auto v = ReadInt32(FI.GetNameOrEmptyStr()))
		val = i16(v.GetValue());
}

void BKVTUnserializer::OnFieldU16(const FieldInfo& FI, u16& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = u16(v.GetValue());
}

void BKVTUnserializer::OnFieldS32(const FieldInfo& FI, i32& val)
{
	if (auto v = ReadInt32(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void BKVTUnserializer::OnFieldU32(const FieldInfo& FI, u32& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void BKVTUnserializer::OnFieldS64(const FieldInfo& FI, i64& val)
{
	if (auto v = ReadInt64(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void BKVTUnserializer::OnFieldU64(const FieldInfo& FI, u64& val)
{
	if (auto v = ReadUInt64(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void BKVTUnserializer::OnFieldF32(const FieldInfo& FI, float& val)
{
	if (auto v = ReadFloat32(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void BKVTUnserializer::OnFieldF64(const FieldInfo& FI, double& val)
{
	if (auto v = ReadFloat64(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void BKVTUnserializer::OnFieldString(const FieldInfo& FI, const IBufferRW& brw)
{
	if (auto v = ReadString(FI.GetNameOrEmptyStr()))
		brw.Assign(v.GetValue());
}

void BKVTUnserializer::OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw)
{
	if (auto v = ReadBytes(FI.GetNameOrEmptyStr()))
		brw.Assign(v.GetValue());
}

bool BKVTUnserializer::OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr)
{
	if (auto v = ReadTypedArrayInt32(FI.GetNameOrEmptyStr()))
	{
		ImplManyCopy(count, arr, v.GetValue());
		return true;
	}
	return false;
}

bool BKVTUnserializer::OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr)
{
	if (auto v = ReadTypedArrayFloat32(FI.GetNameOrEmptyStr()))
	{
		ImplManyCopy(count, arr, v.GetValue());
		return true;
	}
	return false;
}

} // ui
