
#pragma once

#include "Platform.h"
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

	UI_FORCEINLINE FieldInfo() {}
	UI_FORCEINLINE FieldInfo(const char* nm, uint32_t f = 0) : name(nm), flags(f) {}

	UI_FORCEINLINE bool NeedObject() const { return name != nullptr; }
	UI_FORCEINLINE const char* GetNameOrEmptyStr() const { return name ? name : ""; }
};

// - type flags
// is the data format binary-optimized
constexpr unsigned OITF_Binary = 1U << 31;
// instead of simple sequential serialization, keys are serialized with values, and value lookups by key are performed when unserializing
constexpr unsigned OITF_KeyMapped = 1U << 30;
constexpr unsigned OI_TYPE_PART = 0xffffU;
constexpr unsigned OI_FLAG_PART = ~OI_TYPE_PART;
constexpr unsigned OI_TYPE_Unserializer = 0; // the only type that reads the values
constexpr unsigned OI_TYPE_Serializer = 1;
constexpr unsigned OI_TYPE_InfoDumper = 2;
constexpr unsigned OI_TYPE_Unknown = 3;
// - usage flags
// is the data editor-only (not for runtime use)
constexpr unsigned OIUF_EditorOnly = 1U << 29;

// an app-specific interface to use for constructing and finding objects
struct IUnserializeStorage {};

struct IBufferRW
{
	virtual void Assign(StringView sv) const = 0;
	virtual StringView Read() const = 0;
};

template <class AssignFunc>
struct BRWImpl : IBufferRW
{
	StringView bufferSourceRef;
	AssignFunc assignFunc;

	BRWImpl(StringView src, AssignFunc&& af) : bufferSourceRef(src), assignFunc(af) {}

	void Assign(StringView sv) const override { assignFunc(sv); }
	StringView Read() const override { return bufferSourceRef; }
};
template <class AssignFunc>
BRWImpl<AssignFunc> BRW(StringView src, AssignFunc&& af) { return BRWImpl<AssignFunc>(src, Move(af)); }

struct IObjectIterator
{
	virtual unsigned GetFlags() const = 0;

	// EndObject needs to be called regardless of the return value!
	virtual bool BeginObject(const FieldInfo& FI, const char* objname, std::string* outName = nullptr) = 0;
	virtual void EndObject() = 0;
	virtual size_t BeginArray(size_t size, const FieldInfo& FI) = 0;
	virtual void EndArray() = 0;

	virtual bool HasMoreArrayElements() { return false; }
	virtual bool HasField(const char* name) { return true; }

	virtual bool OnFieldNull(const FieldInfo& FI) = 0;
	virtual bool OnFieldBool(const FieldInfo& FI, bool& val) = 0;
	virtual bool OnFieldS8(const FieldInfo& FI, int8_t& val) = 0;
	virtual bool OnFieldU8(const FieldInfo& FI, uint8_t& val) = 0;
	virtual bool OnFieldS16(const FieldInfo& FI, int16_t& val) = 0;
	virtual bool OnFieldU16(const FieldInfo& FI, uint16_t& val) = 0;
	virtual bool OnFieldS32(const FieldInfo& FI, int32_t& val) = 0;
	virtual bool OnFieldU32(const FieldInfo& FI, uint32_t& val) = 0;
	virtual bool OnFieldS64(const FieldInfo& FI, int64_t& val) = 0;
	virtual bool OnFieldU64(const FieldInfo& FI, uint64_t& val) = 0;
	virtual bool OnFieldF32(const FieldInfo& FI, float& val) = 0;
	virtual bool OnFieldF64(const FieldInfo& FI, double& val) = 0;

	virtual bool OnFieldString(const FieldInfo& FI, const IBufferRW& brw) = 0;
	virtual bool OnFieldBytes(const FieldInfo& FI, const IBufferRW& brw) = 0;

	// return true if serializing or if successfully unserialized (data had the expected type)
	virtual bool OnFieldManyS32(const FieldInfo& FI, u32 count, i32* arr) = 0;
	virtual bool OnFieldManyF32(const FieldInfo& FI, u32 count, float* arr) = 0;

	IUnserializeStorage* unserializeStorage = nullptr;
	u32 usageFlags = 0;

	// utility functions
	template <class T> T* GetUnserializeStorage() const { return static_cast<T*>(unserializeStorage); }
	bool IsSerializer() const { return (GetFlags() & OI_TYPE_PART) == OI_TYPE_Serializer; }
	bool IsUnserializer() const { return (GetFlags() & OI_TYPE_PART) == OI_TYPE_Unserializer; }
	bool IsBinary() const { return (GetFlags() & OITF_Binary) != 0; }
	bool IsKeyMapped() const { return (GetFlags() & OITF_KeyMapped) != 0; }
	bool IsEditorOnly() const { return (usageFlags & OIUF_EditorOnly) != 0; }

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

template <class T> UI_FORCEINLINE auto OnField(IObjectIterator& oi, const FieldInfo& FI, T& val) { return val.OnSerialize(oi, FI); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, bool& val) { return oi.OnFieldBool(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, int8_t& val) { return oi.OnFieldS8(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, uint8_t& val) { return oi.OnFieldU8(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, int16_t& val) { return oi.OnFieldS16(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, uint16_t& val) { return oi.OnFieldU16(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, int32_t& val) { return oi.OnFieldS32(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, uint32_t& val) { return oi.OnFieldU32(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, int64_t& val) { return oi.OnFieldS64(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, uint64_t& val) { return oi.OnFieldU64(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, float& val) { return oi.OnFieldF32(FI, val); }
UI_FORCEINLINE bool OnField(IObjectIterator& oi, const FieldInfo& FI, double& val) { return oi.OnFieldF64(FI, val); }

UI_FORCEINLINE bool OnFieldMany(IObjectIterator& oi, const FieldInfo& FI, u32 count, i32* arr) { return oi.OnFieldManyS32(FI, count, arr); }
UI_FORCEINLINE bool OnFieldMany(IObjectIterator& oi, const FieldInfo& FI, u32 count, float* arr) { return oi.OnFieldManyF32(FI, count, arr); }

// TODO split String into StringView/String
struct StdStringRW : IBufferRW
{
	std::string* S = nullptr;
	UI_FORCEINLINE StdStringRW() {}
	UI_FORCEINLINE StdStringRW(std::string& s) : S(&s) {}
	void Assign(StringView sv) const override
	{
		S->assign(sv.data(), sv.size());
	}
	StringView Read() const override
	{
		return *S;
	}
};
inline bool OnField(IObjectIterator& oi, const FieldInfo& FI, std::string& val) { return oi.OnFieldString(FI, StdStringRW(val)); }

} // ui
