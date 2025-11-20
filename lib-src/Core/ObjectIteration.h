
#pragma once

#include "ObjectIterationCore.h"
#include "Optional.h"
#include "Array.h"

#include <stdio.h>

#ifdef UI_USE_STD_VECTOR
#  include <vector>
#endif


namespace ui {

#ifdef UI_USE_STD_VECTOR
template <class T> inline void OnField(IObjectIterator& oi, const FieldInfo& FI, std::vector<T>& val)
{
	size_t estNewSize = oi.BeginArray(val.size(), FI);
	if (oi.IsUnserializer())
	{
		FieldInfo chfi(nullptr, FI.flags);
		if (FI.flags & FieldInfo::Preallocated)
		{
			size_t i = 0;
			while (oi.HasMoreArrayElements())
				OnField(oi, chfi, val[i++]);
		}
		else
		{
			val.clear();
			val.reserve(estNewSize);
			while (oi.HasMoreArrayElements())
			{
				val.push_back(T());
				OnField(oi, chfi, val.back());
			}
		}
	}
	else
	{
		for (auto& item : val)
			OnField(oi, {}, item);
	}
	oi.EndArray();
}
#endif // UI_USE_STD_VECTOR

template <class T, class XF, class CF>
inline void OnFieldPtrArray(IObjectIterator& oi, const FieldInfo& FI, Array<T>& val, XF&& transform, CF&& construct)
{
	size_t estNewSize = oi.BeginArray(val.size(), FI);
	if (oi.IsUnserializer())
	{
		FieldInfo chfi(nullptr, FI.flags);
		if (FI.flags & FieldInfo::Preallocated)
		{
			size_t i = 0;
			while (oi.HasMoreArrayElements())
			{
				auto& r = transform(val[i++]);
				OnField(oi, chfi, r);
			}
		}
		else
		{
			val.Clear();
			val.Reserve(estNewSize);
			while (oi.HasMoreArrayElements())
			{
				val.Append(construct());
				auto& r = transform(val.Last());
				OnField(oi, chfi, r);
			}
		}
	}
	else
	{
		for (auto& item : val)
		{
			auto& r = transform(item);
			OnField(oi, {}, r);
		}
	}
	oi.EndArray();
}

#ifdef UI_USE_STD_VECTOR
template <class T, class XF, class CF>
inline void OnFieldPtrVector(IObjectIterator& oi, const FieldInfo& FI, std::vector<T>& val, XF&& transform, CF&& construct)
{
	size_t estNewSize = oi.BeginArray(val.size(), FI);
	if (oi.IsUnserializer())
	{
		FieldInfo chfi(nullptr, FI.flags);
		if (FI.flags & FieldInfo::Preallocated)
		{
			size_t i = 0;
			while (oi.HasMoreArrayElements())
			{
				auto& r = transform(val[i++]);
				OnField(oi, chfi, r);
			}
		}
		else
		{
			val.clear();
			val.reserve(estNewSize);
			while (oi.HasMoreArrayElements())
			{
				val.push_back(construct());
				auto& r = transform(val.back());
				OnField(oi, chfi, r);
			}
		}
	}
	else
	{
		for (auto& item : val)
		{
			auto& r = transform(item);
			OnField(oi, {}, r);
		}
	}
	oi.EndArray();
}
#endif // UI_USE_STD_VECTOR

template <class T, class CT, class XF>
inline void OnFieldArrayValIndex(IObjectIterator& oi, const FieldInfo& FI, T& ptr, CT& cont, XF&& transform)
{
	int32_t idx = -1;
	if (ptr && !oi.IsUnserializer())
	{
		int32_t i = 0;
		for (auto& item : cont)
		{
			if (transform(item) == ptr)
			{
				idx = i;
				break;
			}
			i++;
		}
	}
	OnField(oi, FI, idx);
	if (oi.IsUnserializer())
	{
		if (!oi.HasField(FI.GetNameOrEmptyStr()))
			idx = -1;
		ptr = idx >= 0 && idx < int32_t(cont.size()) ? transform(cont[idx]) : nullptr;
	}
}
template <class T, class CT>
inline void OnFieldArrayValIndex(IObjectIterator& oi, const FieldInfo& FI, T& ptr, CT& cont)
{
	OnFieldArrayValIndex(oi, FI, ptr, cont, [](const T& v) { return v; });
}

template <class E> inline void OnFieldEnumInt(IObjectIterator& oi, const FieldInfo& FI, E& val)
{
	int64_t iv = int64_t(val);
	OnField(oi, FI, iv);
	if (oi.IsUnserializer())
		val = (E)iv;
}


template <class E> struct EnumKeys
{
	static E StringToValue(const char* name) = delete;
	static const char* ValueToString(E e) = delete;
};

inline constexpr size_t CountStrings(const char** list)
{
	const char** it = list;
	while (*it)
		it++;
	return it - list;
}

template <class E, const char** list, E def = (E)0> struct EnumKeysStringList
{
	static E StringToValue(const char* name)
	{
		for (auto it = list; *it; it++)
			if (!strcmp(*it, name))
				return E(uintptr_t(it - list));
		return def;
	}
	static const char* ValueToString(E v)
	{
		static size_t MAX = CountStrings(list);
		if (size_t(v) >= MAX)
			return "";
		return list[size_t(v)];
	}
};

template <class E> inline void OnFieldEnumString(IObjectIterator& oi, const FieldInfo& FI, E& val)
{
	std::string str;
	if (!oi.IsUnserializer())
		str = EnumKeys<E>::ValueToString(val);
	if (oi.IsUnserializer() && !oi.HasField(FI.GetNameOrEmptyStr()))
		return;
	OnField(oi, FI, str);
	if (oi.IsUnserializer())
		val = EnumKeys<E>::StringToValue(str.c_str());
}



struct IObjectIteratorMinTypeSerializeBase : IObjectIterator
{
	bool OnFieldS8(const FieldInfo& FI, int8_t& val) override { int64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp), true; }
	bool OnFieldU8(const FieldInfo& FI, uint8_t& val) override { uint64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp), true; }
	bool OnFieldS16(const FieldInfo& FI, int16_t& val) override { int64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp), true; }
	bool OnFieldU16(const FieldInfo& FI, uint16_t& val) override { uint64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp), true; }
	bool OnFieldS32(const FieldInfo& FI, int32_t& val) override { int64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp), true; }
	bool OnFieldU32(const FieldInfo& FI, uint32_t& val) override { uint64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp), true; }

	bool OnFieldF32(const FieldInfo& FI, float& val) override { double tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldF64(FI, tmp), true; }

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override
	{
		BeginArray(count, FI);
		for (u32 i = 0; i < count; i++)
			OnFieldS32({}, arr[i]);
		EndArray();
		return true;
	}
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override
	{
		BeginArray(count, FI);
		for (u32 i = 0; i < count; i++)
			OnFieldF32({}, arr[i]);
		EndArray();
		return true;
	}
};



struct IObjectIteratorMinTypeUnserializeBase : IObjectIterator
{
	bool OnFieldS8(const FieldInfo& FI, int8_t& val) override { int64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp) ? val = static_cast<int8_t>(tmp), true : false; }
	bool OnFieldU8(const FieldInfo& FI, uint8_t& val) override { uint64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp) ? val = static_cast<uint8_t>(tmp), true : false; }
	bool OnFieldS16(const FieldInfo& FI, int16_t& val) override { int64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp) ? val = static_cast<int16_t>(tmp), true : false; }
	bool OnFieldU16(const FieldInfo& FI, uint16_t& val) override { uint64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp) ? val = static_cast<uint16_t>(tmp), true : false; }
	bool OnFieldS32(const FieldInfo& FI, int32_t& val) override { int64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp) ? val = static_cast<int32_t>(tmp), true : false; }
	bool OnFieldU32(const FieldInfo& FI, uint32_t& val) override { uint64_t tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp) ? val = static_cast<uint32_t>(tmp), true : false; }

	bool OnFieldF32(const FieldInfo& FI, float& val) override { double tmp = val; return static_cast<IObjectIterator*>(this)->OnFieldF64(FI, tmp) ? val = static_cast<float>(tmp), true : false; }

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override
	{
		BeginArray(count, FI);
		bool ret = true;
		for (u32 i = 0; i < count; i++)
			ret &= OnFieldS32({}, arr[i]);
		EndArray();
		return ret;
	}
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override
	{
		BeginArray(count, FI);
		bool ret = true;
		for (u32 i = 0; i < count; i++)
			ret &= OnFieldF32({}, arr[i]);
		EndArray();
		return ret;
	}
};



struct IObjectStringWriterIteratorBase : IObjectIteratorMinTypeSerializeBase
{
	virtual void OnFieldAsString(const FieldInfo& FI, const char* str) = 0;

	bool OnFieldBool(const FieldInfo& FI, bool& val) override { return OnFieldAsString(FI, val ? "true" : "false"), true; }
	bool OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%lld", val);
		return OnFieldAsString(FI, bfr), true;
	}
	bool OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%llu", val);
		return OnFieldAsString(FI, bfr), true;
	}
	bool OnFieldF32(const FieldInfo& FI, float& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%.9g", val);
		return OnFieldAsString(FI, bfr), true;
	}
	bool OnFieldF64(const FieldInfo& FI, double& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%.17g", val);
		return OnFieldAsString(FI, bfr), true;
	}
};



struct IObjectStringReaderIteratorBase : IObjectIteratorMinTypeUnserializeBase
{
	virtual Optional<std::string> GetFieldString(const FieldInfo& FI) = 0;

	bool OnFieldBool(const FieldInfo& FI, bool& val) override
	{
		if (auto str = GetFieldString(FI))
		{
			if (str.GetValue() == "true" || atoi(str.GetValue().c_str()))
				return val = true, true;
			if (str.GetValue() == "false" || str.GetValue() == "0")
				return val = false, true;
		}
		return false;
	}
	bool OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		if (auto str = GetFieldString(FI))
		{
			char* ret = nullptr;
			val = strtoll(str.GetValue().c_str(), &ret, 10);
			return *ret == 0 && ret != str.GetValue().c_str();
		}
		return false;
	}
	bool OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		if (auto str = GetFieldString(FI))
		{
			char* ret = nullptr;
			val = strtoull(str.GetValue().c_str(), &ret, 10);
			return *ret == 0 && ret != str.GetValue().c_str();
		}
		return false;
	}
	bool OnFieldF64(const FieldInfo& FI, double& val) override
	{
		if (auto str = GetFieldString(FI))
		{
			char* ret = nullptr;
			val = strtod(str.GetValue().c_str(), &ret);
			return *ret == 0 && ret != str.GetValue().c_str();
		}
		return false;
	}
};

} // ui
