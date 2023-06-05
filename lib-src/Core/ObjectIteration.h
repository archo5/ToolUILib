
#pragma once

#include "ObjectIterationCore.h"
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
	void OnFieldS8(const FieldInfo& FI, int8_t& val) override { int64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); }
	void OnFieldU8(const FieldInfo& FI, uint8_t& val) override { uint64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); }
	void OnFieldS16(const FieldInfo& FI, int16_t& val) override { int64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); }
	void OnFieldU16(const FieldInfo& FI, uint16_t& val) override { uint64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); }
	void OnFieldS32(const FieldInfo& FI, int32_t& val) override { int64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); }
	void OnFieldU32(const FieldInfo& FI, uint32_t& val) override { uint64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); }

	void OnFieldF32(const FieldInfo& FI, float& val) override { double tmp = val; static_cast<IObjectIterator*>(this)->OnFieldF64(FI, tmp); }

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
	void OnFieldS8(const FieldInfo& FI, int8_t& val) override { int64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); val = static_cast<int8_t>(tmp); }
	void OnFieldU8(const FieldInfo& FI, uint8_t& val) override { uint64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); val = static_cast<uint8_t>(tmp); }
	void OnFieldS16(const FieldInfo& FI, int16_t& val) override { int64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); val = static_cast<int16_t>(tmp); }
	void OnFieldU16(const FieldInfo& FI, uint16_t& val) override { uint64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); val = static_cast<uint16_t>(tmp); }
	void OnFieldS32(const FieldInfo& FI, int32_t& val) override { int64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); val = static_cast<int32_t>(tmp); }
	void OnFieldU32(const FieldInfo& FI, uint32_t& val) override { uint64_t tmp = val; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); val = static_cast<uint32_t>(tmp); }

	void OnFieldF32(const FieldInfo& FI, float& val) override { double tmp = val; static_cast<IObjectIterator*>(this)->OnFieldF64(FI, tmp); val = static_cast<float>(tmp); }

	bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) override
	{
		BeginArray(count, FI);
		bool ret = HasMoreArrayElements();
		for (u32 i = 0; i < count; i++)
			OnFieldS32({}, arr[i]);
		EndArray();
		return ret;
	}
	bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) override
	{
		BeginArray(count, FI);
		bool ret = HasMoreArrayElements();
		for (u32 i = 0; i < count; i++)
			OnFieldF32({}, arr[i]);
		EndArray();
		return ret;
	}
};



struct IObjectStringWriterIteratorBase : IObjectIteratorMinTypeSerializeBase
{
	virtual void OnFieldAsString(const FieldInfo& FI, const char* str) = 0;

	void OnFieldBool(const FieldInfo& FI, bool& val) override { OnFieldAsString(FI, val ? "true" : "false"); }
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%lld", val);
		OnFieldAsString(FI, bfr);
	}
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%llu", val);
		OnFieldAsString(FI, bfr);
	}
	void OnFieldF64(const FieldInfo& FI, double& val) override
	{
		char bfr[32];
		snprintf(bfr, 32, "%g", val);
		OnFieldAsString(FI, bfr);
	}
};



struct IObjectStringReaderIteratorBase : IObjectIteratorMinTypeUnserializeBase
{
	virtual std::string GetFieldString(const FieldInfo& FI) = 0;

	void OnFieldBool(const FieldInfo& FI, bool& val) override
	{
		auto str = GetFieldString(FI);
		val = str == "true" || atoi(str.c_str()) != 0;
	}
	void OnFieldS64(const FieldInfo& FI, int64_t& val) override
	{
		val = strtoll(GetFieldString(FI).c_str(), nullptr, 10);
	}
	void OnFieldU64(const FieldInfo& FI, uint64_t& val) override
	{
		val = strtoull(GetFieldString(FI).c_str(), nullptr, 10);
	}
	void OnFieldF64(const FieldInfo& FI, double& val) override
	{
		val = atof(GetFieldString(FI).c_str());
	}
};

} // ui
