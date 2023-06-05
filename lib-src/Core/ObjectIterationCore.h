
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

template <class T> UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, T& val) { val.OnSerialize(oi, FI); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, bool& val) { oi.OnFieldBool(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, int8_t& val) { oi.OnFieldS8(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, uint8_t& val) { oi.OnFieldU8(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, int16_t& val) { oi.OnFieldS16(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, uint16_t& val) { oi.OnFieldU16(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, int32_t& val) { oi.OnFieldS32(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, uint32_t& val) { oi.OnFieldU32(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, int64_t& val) { oi.OnFieldS64(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, uint64_t& val) { oi.OnFieldU64(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, float& val) { oi.OnFieldF32(FI, val); }
UI_FORCEINLINE void OnField(IObjectIterator& oi, const FieldInfo& FI, double& val) { oi.OnFieldF64(FI, val); }

UI_FORCEINLINE bool OnFieldMany(IObjectIterator& oi, const FieldInfo& FI, u32 count, i32* arr) { return oi.OnFieldManyS32(FI, count, arr); }
UI_FORCEINLINE bool OnFieldMany(IObjectIterator& oi, const FieldInfo& FI, u32 count, float* arr) { return oi.OnFieldManyF32(FI, count, arr); }

// TODO split String into StringView/String
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

} // ui
