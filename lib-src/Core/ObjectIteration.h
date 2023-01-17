
#pragma once

#include "Array.h"
#include "String.h"

#include <inttypes.h>
#include <stdio.h>
#include <type_traits>

#ifdef UI_USE_STD_VECTOR
#  include <vector>
#endif


namespace ui {

struct FieldInfo
{
	enum FLAGS
	{
		AllocateOnly = 1 << 0,
		Preallocated = 1 << 1,
	};

	const char* name = nullptr;
	uint32_t flags = 0;

	UI_FORCEINLINE FieldInfo() {}
	UI_FORCEINLINE FieldInfo(const char* nm, uint32_t f = 0) : name(nm), flags(f) {}

	UI_FORCEINLINE bool NeedObject() const { return name != nullptr; }
	UI_FORCEINLINE const char* GetNameOrEmptyStr() const { return name ? name : ""; }
};

// is the data format binary-optimized
constexpr unsigned OIF_Binary = 1U << 31;
// instead of simple sequential serialization, keys are serialized with values, and value lookups by key are performed when unserializing
constexpr unsigned OIF_KeyMapped = 1U << 30;
constexpr unsigned OI_ALL_FLAGS = 3U << 30;
constexpr unsigned OI_TYPE_Unserializer = 0; // the only type that reads the values
constexpr unsigned OI_TYPE_Serializer = 1;
constexpr unsigned OI_TYPE_InfoDumper = 2;
constexpr unsigned OI_TYPE_Unknown = 3;

// an app-specific interface to use for constructing and finding objects
struct IUnserializeStorage {};

struct IBufferRW
{
	virtual void Assign(StringView sv) const = 0;
	virtual StringView Read() const = 0;
};

struct IObjectIterator
{
	virtual unsigned GetFlags() const = 0;

	virtual void BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) = 0;
	virtual void EndObject() = 0;
	virtual size_t BeginArray(size_t size, const FieldInfo& FI) = 0;
	virtual void EndArray() = 0;

	virtual bool HasMoreArrayElements() { return false; }
	virtual bool HasField(const char* name) { return true; }

	virtual void OnFieldBool(const FieldInfo& FI, bool& val) = 0;
	virtual void OnFieldS8(const FieldInfo& FI, int8_t& val) = 0;
	virtual void OnFieldU8(const FieldInfo& FI, uint8_t& val) = 0;
	virtual void OnFieldS16(const FieldInfo& FI, int16_t& val) = 0;
	virtual void OnFieldU16(const FieldInfo& FI, uint16_t& val) = 0;
	virtual void OnFieldS32(const FieldInfo& FI, int32_t& val) = 0;
	virtual void OnFieldU32(const FieldInfo& FI, uint32_t& val) = 0;
	virtual void OnFieldS64(const FieldInfo& FI, int64_t& val) = 0;
	virtual void OnFieldU64(const FieldInfo& FI, uint64_t& val) = 0;
	virtual void OnFieldF32(const FieldInfo& FI, float& val) = 0;
	virtual void OnFieldF64(const FieldInfo& FI, double& val) = 0;

	virtual void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) = 0;
	virtual void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) = 0;

	// return true if serializing or if successfully unserialized (data had the expected type)
	virtual bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) = 0;
	virtual bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) = 0;

	IUnserializeStorage* unserializeStorage = nullptr;

	// utility functions
	template <class T> T* GetUnserializeStorage() const { return static_cast<T*>(unserializeStorage); }
	bool IsSerializer() const { return (GetFlags() & ~OI_ALL_FLAGS) == OI_TYPE_Serializer; }
	bool IsUnserializer() const { return (GetFlags() & ~OI_ALL_FLAGS) == OI_TYPE_Unserializer; }
	bool IsBinary() const { return (GetFlags() & OIF_Binary) != 0; }
	bool IsKeyMapped() const { return (GetFlags() & OIF_KeyMapped) != 0; }

	template <class T>
	void ImplManyCopy(u32 count, T* dst, ArrayView<T> src)
	{
		// only copy if the number of elements is exactly the same
		//   otherwise it can be assumed that the data is not trivially transferable
		//   in some cases (e.g. vec3 = [vec2 + z]) it might be..
		//   but in others it would not be, e.g.:
		//   - aabb3 = [aabb2.min, +zmin, aabb2.max, +zmax]
		//   - mat4 = mat3 + 7 right/lower column elements
		// > instead create a new key and if it doesn't unserialize and the old one does, manually upgrade from the old data
		if (count == src.Size())
			memcpy(dst, src.Data(), count * sizeof(*dst));
	}
};

template <class T> inline void OnField(IObjectIterator& oi, const FieldInfo& FI, T& val) { val.OnSerialize(oi, FI); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, bool& val) { oi.OnFieldBool(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, int8_t& val) { oi.OnFieldS8(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, uint8_t& val) { oi.OnFieldU8(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, int16_t& val) { oi.OnFieldS16(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, uint16_t& val) { oi.OnFieldU16(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, int32_t& val) { oi.OnFieldS32(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, uint32_t& val) { oi.OnFieldU32(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, int64_t& val) { oi.OnFieldS64(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, uint64_t& val) { oi.OnFieldU64(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, float& val) { oi.OnFieldF32(FI, val); }
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, double& val) { oi.OnFieldF64(FI, val); }

inline bool OnFieldMany(IObjectIterator& oi, const FieldInfo& FI, u32 count, i32* arr) { return oi.OnFieldManyS32(FI, count, arr); }
inline bool OnFieldMany(IObjectIterator& oi, const FieldInfo& FI, u32 count, float* arr) { return oi.OnFieldManyF32(FI, count, arr); }

struct StdStringRW : IBufferRW
{
	std::string* S;
	void Assign(StringView sv) const override
	{
		S->assign(sv.data(), sv.size());
	}
	StringView Read() const override
	{
		return *S;
	}
};
inline void OnField(IObjectIterator& oi, const FieldInfo& FI, std::string& val) { StdStringRW rw; rw.S = &val; oi.OnFieldString(FI, rw); }

template <class T> inline void OnField(IObjectIterator& oi, const FieldInfo& FI, Array<T>& val)
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
			val.Clear();
			val.Reserve(estNewSize);
			while (oi.HasMoreArrayElements())
			{
				val.Append(T());
				OnField(oi, chfi, val.Last());
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
