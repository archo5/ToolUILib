
#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <type_traits>
#include <vector>
#include "String.h"


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

	FieldInfo() {}
	FieldInfo(const char* nm, uint32_t f = 0) : name(nm), flags(f) {}

	bool NeedObject() const { return name != nullptr; }
	const char* GetNameOrEmptyStr() const { return name ? name : ""; }
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
	virtual void OnFieldString(const FieldInfo& FI, const IBufferRW& brw) { OnFieldBytes(FI, brw); }
	virtual void OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) = 0;

	IUnserializeStorage* unserializeStorage = nullptr;

	// utility functions
	template <class T> T* GetUnserializeStorage() const { return static_cast<T*>(unserializeStorage); }
	bool IsSerializer() const { return (GetFlags() & ~OI_ALL_FLAGS) == OI_TYPE_Serializer; }
	bool IsUnserializer() const { return (GetFlags() & ~OI_ALL_FLAGS) == OI_TYPE_Unserializer; }
	bool IsBinary() const { return (GetFlags() & OIF_Binary) != 0; }
	bool IsKeyMapped() const { return (GetFlags() & OIF_KeyMapped) != 0; }
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

template <class T, class CT, class XF>
inline void OnFieldVectorValIndex(IObjectIterator& oi, const FieldInfo& FI, T& ptr, CT& cont, XF&& transform)
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
inline void OnFieldVectorValIndex(IObjectIterator& oi, const FieldInfo& FI, T& ptr, CT& cont)
{
	OnFieldVectorValIndex(oi, FI, ptr, cont, [](const T& v) { return v; });
}

template <class E> inline void OnFieldEnumInt(IObjectIterator& oi, const FieldInfo& FI, E& val)
{
	int64_t iv = int64_t(val);
	OnField(oi, FI, iv);
	if (oi.IsUnserializer())
		val = (E)iv;
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
};



struct IObjectIteratorMinTypeUnserializeBase : IObjectIterator
{
	void OnFieldS8(const FieldInfo& FI, int8_t& val) override { int64_t tmp; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }
	void OnFieldU8(const FieldInfo& FI, uint8_t& val) override { uint64_t tmp; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }
	void OnFieldS16(const FieldInfo& FI, int16_t& val) override { int64_t tmp; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }
	void OnFieldU16(const FieldInfo& FI, uint16_t& val) override { uint64_t tmp; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }
	void OnFieldS32(const FieldInfo& FI, int32_t& val) override { int64_t tmp; static_cast<IObjectIterator*>(this)->OnFieldS64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }
	void OnFieldU32(const FieldInfo& FI, uint32_t& val) override { uint64_t tmp; static_cast<IObjectIterator*>(this)->OnFieldU64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }

	void OnFieldF32(const FieldInfo& FI, float& val) override { double tmp; static_cast<IObjectIterator*>(this)->OnFieldF64(FI, tmp); val = static_cast<typename std::remove_reference<decltype(val)>::type>(tmp); }
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
