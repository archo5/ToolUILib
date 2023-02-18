
#include "SerializationDATO.h"


namespace ui {

DATOLinearWriter::DATOLinearWriter(StringView prefix, bool aligned, bool sortKeys, bool skipDuplicateKeys)
	: _writer(
		prefix.Data(),
		prefix.Size(),
		(aligned ? dato::FLAG_Aligned : 0) | (sortKeys ? dato::FLAG_SortedKeys : 0),
		skipDuplicateKeys
	)
{
}

void DATOLinearWriter::_AppendElem(const char* key, dato::ValueRef vref)
{
	auto& S = _stack.Last();
	if (S.hasKeys)
		S.entriesStrMap.Append({ _writer.WriteStringKey(key), vref });
	else
		S.entriesArr.Append(vref);
}

dato::ValueRef DATOLinearWriter::_WriteAndRemoveTopObject()
{
	auto& S = _stack.Last();
	auto vref = _writer.WriteStringMap(S.entriesStrMap.Data(), S.entriesStrMap.Size());
	_stack.RemoveLast();
	return vref;
}

void DATOLinearWriter::BeginObject(const char* key)
{
	_AppendElem(key, {}); // value to be filled in later
	_stack.Append({});
	_stack.Last().hasKeys = true;
}

void DATOLinearWriter::EndObject()
{
	auto vref = _WriteAndRemoveTopObject();
	auto& S = _stack.Last();
	if (S.hasKeys)
		S.entriesStrMap.Last().value = vref;
	else
		S.entriesArr.Last() = vref;
}

void DATOLinearWriter::BeginArray(const char* key)
{
	_AppendElem(key, {}); // value to be filled in later
	_stack.Append({});
	_stack.Last().hasKeys = false;
}

void DATOLinearWriter::EndArray()
{
	auto& S0 = _stack.Last();
	auto vref = _writer.WriteArray(S0.entriesArr.Data(), S0.entriesArr.Size());
	_stack.RemoveLast();

	auto& S = _stack.Last();
	if (S.hasKeys)
		S.entriesStrMap.Last().value = vref;
	else
		S.entriesArr.Last() = vref;
}

StringView DATOLinearWriter::GetData()
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
		auto root = _WriteAndRemoveTopObject();
		_writer.SetRoot(root);
	}
	return { (const char*)_writer.GetData(), _writer.GetSize() };
}


bool DATOLinearReader::Init(StringView data, StringView prefix)
{
	if (!_reader.Init(data.Data(), data.Size(), prefix.Data(), prefix.Size()))
		return false;
	_stack.Append({ dato::TYPE_StringMap, 0, _reader.GetRoot().TryGetStringMap() });
	return true;
}

bool DATOLinearReader::HasMoreArrayElements()
{
	if (_stack.IsEmpty())
		return false;

	auto& S = _stack.Last();
	return S.type == dato::TYPE_Array
		&& S.curElemIndex < S.arr.GetSize();
}

size_t DATOLinearReader::GetCurrentArraySize()
{
	if (_stack.IsEmpty())
		return 0;

	auto& S = _stack.Last();
	if (S.type != dato::TYPE_Array)
		return 0;

	return S.arr.GetSize();
}

dato::Reader::DynamicAccessor DATOLinearReader::FindEntry(const char* key)
{
	if (_stack.IsEmpty())
		return {};

	auto& S = _stack.Last();
	if (S.type == dato::TYPE_Array)
	{
		u32 idx = S.curElemIndex++;
		return S.arr.TryGetValueByIndex(idx);
	}
	else if (S.type == dato::TYPE_StringMap)
	{
		return S.smap.FindValueByKey(key);
	}
	return {};
}

Optional<i32> DATOLinearReader::ReadInt32(const char* key)
{
	auto e = FindEntry(key);
	if (e.IsNumber())
		return e.CastToNumber<i32>();
	return {};
}

Optional<u32> DATOLinearReader::ReadUInt32(const char* key)
{
	auto e = FindEntry(key);
	if (e.IsNumber())
		return e.CastToNumber<u32>();
	return {};
}

Optional<i64> DATOLinearReader::ReadInt64(const char* key)
{
	auto e = FindEntry(key);
	if (e.IsNumber())
		return e.CastToNumber<i64>();
	return {};
}

Optional<u64> DATOLinearReader::ReadUInt64(const char* key)
{
	auto e = FindEntry(key);
	if (e.IsNumber())
		return e.CastToNumber<u64>();
	return {};
}

Optional<float> DATOLinearReader::ReadFloat32(const char* key)
{
	auto e = FindEntry(key);
	if (e.IsNumber())
		return e.CastToNumber<float>();
	return {};
}

Optional<double> DATOLinearReader::ReadFloat64(const char* key)
{
	auto e = FindEntry(key);
	if (e.IsNumber())
		return e.CastToNumber<double>();
	return {};
}

Optional<StringView> DATOLinearReader::ReadString(const char* key)
{
	auto e = FindEntry(key);
	if (auto s = e.TryGetString8())
		return StringView(s.GetData(), s.GetSize());
	return {};
}

Optional<StringView> DATOLinearReader::ReadBytes(const char* key)
{
	auto e = FindEntry(key);
	if (auto b = e.TryGetByteArray())
		return StringView((const char*)b.GetData(), b.GetSize());
	return {};
}

Optional<ArrayView<i8>> DATOLinearReader::ReadVectorInt8(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<i8>(expectedElemCount))
		return ArrayView<i8>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<u8>> DATOLinearReader::ReadVectorUInt8(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<u8>(expectedElemCount))
		return ArrayView<u8>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<i16>> DATOLinearReader::ReadVectorInt16(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<i16>(expectedElemCount))
		return ArrayView<i16>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<u16>> DATOLinearReader::ReadVectorUInt16(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<u16>(expectedElemCount))
		return ArrayView<u16>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<i32>> DATOLinearReader::ReadVectorInt32(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<i32>(expectedElemCount))
		return ArrayView<i32>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<u32>> DATOLinearReader::ReadVectorUInt32(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<u32>(expectedElemCount))
		return ArrayView<u32>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<i64>> DATOLinearReader::ReadVectorInt64(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<i64>(expectedElemCount))
		return ArrayView<i64>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<u64>> DATOLinearReader::ReadVectorUInt64(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<u64>(expectedElemCount))
		return ArrayView<u64>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<float>> DATOLinearReader::ReadVectorFloat32(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<float>(expectedElemCount))
		return ArrayView<float>(v._data, v.GetElementCount());
	return {};
}

Optional<ArrayView<double>> DATOLinearReader::ReadVectorFloat64(const char* key, u8 expectedElemCount)
{
	auto e = FindEntry(key);
	if (auto v = e.TryGetVector<double>(expectedElemCount))
		return ArrayView<double>(v._data, v.GetElementCount());
	return {};
}

bool DATOLinearReader::BeginArray(const char* key)
{
	if (auto e = FindEntry(key))
	{
		if (auto arr = e.TryGetArray())
		{
			_stack.Append({ dato::TYPE_Array, 0, {}, arr });
			return true;
		}
	}
	_stack.Append({ dato::TYPE_Null });
	return false;
}

void DATOLinearReader::EndArray()
{
	_stack.RemoveLast();
}

bool DATOLinearReader::BeginObject(const char* key)
{
	if (auto e = FindEntry(key))
	{
		if (auto smap = e.TryGetStringMap())
		{
			_stack.Append({ dato::TYPE_StringMap, 0, smap });
			return true;
		}
	}
	_stack.Append({ dato::TYPE_Null });
	return false;
}

void DATOLinearReader::EndObject()
{
	_stack.RemoveLast();
}

void DATOLinearReader::BeginEntry(const dato::Reader::DynamicAccessor& E)
{
	if (auto smap = E.TryGetStringMap())
		_stack.Append({ dato::TYPE_StringMap, 0, smap });
	else if (auto arr = E.TryGetArray())
		_stack.Append({ dato::TYPE_Array, 0, {}, arr });
	else
		_stack.Append({ dato::TYPE_Null });
}

void DATOLinearReader::EndEntry()
{
	_stack.RemoveLast();
}


void DATOSerializer::BeginObject(const FieldInfo& FI, const char* objname, std::string* outName)
{
	DATOLinearWriter::BeginObject(FI.GetNameOrEmptyStr());
	if (outName)
		WriteString("__", *outName);
}

void DATOSerializer::EndObject()
{
	DATOLinearWriter::EndObject();
}

size_t DATOSerializer::BeginArray(size_t size, const FieldInfo& FI)
{
	DATOLinearWriter::BeginArray(FI.GetNameOrEmptyStr());
	return 0;
}

void DATOSerializer::EndArray()
{
	DATOLinearWriter::EndArray();
}

void DATOSerializer::OnFieldBool(const FieldInfo& FI, bool& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldS8(const FieldInfo& FI, i8& val)
{
	WriteInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldU8(const FieldInfo& FI, u8& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldS16(const FieldInfo& FI, i16& val)
{
	WriteInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldU16(const FieldInfo& FI, u16& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldS32(const FieldInfo& FI, i32& val)
{
	WriteInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldU32(const FieldInfo& FI, u32& val)
{
	WriteUInt32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldS64(const FieldInfo& FI, i64& val)
{
	WriteInt64(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldU64(const FieldInfo& FI, u64& val)
{
	WriteUInt64(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldF32(const FieldInfo& FI, float& val)
{
	WriteFloat32(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldF64(const FieldInfo& FI, double& val)
{
	WriteFloat64(FI.GetNameOrEmptyStr(), val);
}

void DATOSerializer::OnFieldString(const FieldInfo& FI, const IBufferRW& brw)
{
	WriteString(FI.GetNameOrEmptyStr(), brw.Read());
}

void DATOSerializer::OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw)
{
	WriteBytes(FI.GetNameOrEmptyStr(), brw.Read());
}

bool DATOSerializer::OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr)
{
	WriteVectorInt32(FI.GetNameOrEmptyStr(), count, arr);
	return true;
}

bool DATOSerializer::OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr)
{
	WriteVectorFloat32(FI.GetNameOrEmptyStr(), count, arr);
	return true;
}


void DATOUnserializer::BeginObject(const FieldInfo& FI, const char* objname, std::string* outName)
{
	DATOLinearReader::BeginObject(FI.GetNameOrEmptyStr());
	if (outName)
	{
		if (auto s = ReadString("__"))
			*outName = to_string(s.GetValue());
	}
}

void DATOUnserializer::EndObject()
{
	DATOLinearReader::EndObject();
}

size_t DATOUnserializer::BeginArray(size_t size, const FieldInfo& FI)
{
	DATOLinearReader::BeginArray(FI.GetNameOrEmptyStr());
	return GetCurrentArraySize();
}

void DATOUnserializer::EndArray()
{
	DATOLinearReader::EndArray();
}

bool DATOUnserializer::HasMoreArrayElements()
{
	return DATOLinearReader::HasMoreArrayElements();
}

bool DATOUnserializer::HasField(const char* name)
{
	return FindEntry(name).IsValid();
}

void DATOUnserializer::OnFieldBool(const FieldInfo& FI, bool& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = v.GetValue() != 0;
}

void DATOUnserializer::OnFieldS8(const FieldInfo& FI, i8& val)
{
	if (auto v = ReadInt32(FI.GetNameOrEmptyStr()))
		val = i8(v.GetValue());
}

void DATOUnserializer::OnFieldU8(const FieldInfo& FI, u8& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = u8(v.GetValue());
}

void DATOUnserializer::OnFieldS16(const FieldInfo& FI, i16& val)
{
	if (auto v = ReadInt32(FI.GetNameOrEmptyStr()))
		val = i16(v.GetValue());
}

void DATOUnserializer::OnFieldU16(const FieldInfo& FI, u16& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = u16(v.GetValue());
}

void DATOUnserializer::OnFieldS32(const FieldInfo& FI, i32& val)
{
	if (auto v = ReadInt32(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void DATOUnserializer::OnFieldU32(const FieldInfo& FI, u32& val)
{
	if (auto v = ReadUInt32(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void DATOUnserializer::OnFieldS64(const FieldInfo& FI, i64& val)
{
	if (auto v = ReadInt64(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void DATOUnserializer::OnFieldU64(const FieldInfo& FI, u64& val)
{
	if (auto v = ReadUInt64(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void DATOUnserializer::OnFieldF32(const FieldInfo& FI, float& val)
{
	if (auto v = ReadFloat32(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void DATOUnserializer::OnFieldF64(const FieldInfo& FI, double& val)
{
	if (auto v = ReadFloat64(FI.GetNameOrEmptyStr()))
		val = v.GetValue();
}

void DATOUnserializer::OnFieldString(const FieldInfo& FI, const IBufferRW& brw)
{
	if (auto v = ReadString(FI.GetNameOrEmptyStr()))
		brw.Assign(v.GetValue());
}

void DATOUnserializer::OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw)
{
	if (auto v = ReadBytes(FI.GetNameOrEmptyStr()))
		brw.Assign(v.GetValue());
}

bool DATOUnserializer::OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr)
{
	if (auto v = ReadVectorInt32(FI.GetNameOrEmptyStr(), count))
	{
		ImplManyCopy(count, arr, v.GetValue());
		return true;
	}
	return false;
}

bool DATOUnserializer::OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr)
{
	if (auto v = ReadVectorFloat32(FI.GetNameOrEmptyStr(), count))
	{
		ImplManyCopy(count, arr, v.GetValue());
		return true;
	}
	return false;
}

} // ui
