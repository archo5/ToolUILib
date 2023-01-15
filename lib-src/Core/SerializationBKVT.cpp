
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
	u32 ret = _data.Size();
	if (!key)
		key = "";
	size_t len = strlen(key);
	if (len > 255)
		len = 255;
	_data.Append(u8(len));
	_data.AppendMany(key, len);
	_data.Append(0);
	return ret;
}

u32 BKVTLinearWriter::_AppendMem(const void* mem, size_t size)
{
	u32 ret = _data.Size();
	_data.AppendMany((const char*)mem, size);
	return ret;
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
	return withPrefix ? _data : StringView(_data).substr(4);
}


bool BKVTLinearReader::Init(StringView data, bool hasPrefix)
{
	if (!hasPrefix)
	{
		if (_data.Size() < 6)
			return false; // min. valid size = 6 (pointer to root object + empty object)
	}
	else
	{
		if (!data.starts_with("BKVT"))
			return false;
		if (_data.Size() < 10)
			return false;
	}
	_data = data.substr(4);
	return true;
}

bool BKVTLinearReader::HasMoreArrayElements()
{
	return _stack.NotEmpty()
		&& _stack.Last().entry.type == BKVT_Type::Array
		&& _stack.Last().curElemIndex < _ReadU32(_stack.Last().entry.pos);
}

size_t BKVTLinearReader::GetCurrentArraySize()
{
	if (_stack.NotEmpty() && _stack.Last().entry.type == BKVT_Type::Array)
		return _ReadU32(_stack.Last().entry.pos);
	return 0;
}

BKVTLinearReader::EntryRef BKVTLinearReader::FindEntry(const char* key)
{
	if (_stack.NotEmpty() && _stack.Last().entry.type == BKVT_Type::Object)
	{
		u32 pos = _stack.Last().entry.pos;
		u32 num = _ReadU16(pos);
		pos += 2;
		for (u32 i = 0; i < num; i++)
		{
			if (_GetKeyString(_ReadU32(pos + i * 4)) == key)
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
	return 0;
}

void BKVTUnserializer::EndArray()
{
	BKVTLinearReader::EndArray();
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

} // ui
