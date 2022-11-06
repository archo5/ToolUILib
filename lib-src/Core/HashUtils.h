
#pragma once

#include "Platform.h"

#include <type_traits>
#include <new>


namespace ui {

static constexpr const size_t HASH_OFFSET_BASIS = 2166136261;
static constexpr const size_t HASH_PRIME = 16777619;

inline size_t HashBytesAll(const void* bytes, size_t num)
{
	auto* arr = (const char*)bytes;
	size_t h = HASH_OFFSET_BASIS;
	for (size_t i = 0; i < num; i++)
	{
		h ^= arr[i];
		h *= HASH_PRIME;
	}
	return h;
}

inline size_t HashManyBytesFast(const void* bytes, size_t num, size_t maxsteps = 64)
{
	auto* arr = (const char*)bytes;
	size_t h = HASH_OFFSET_BASIS;
	size_t step = num / maxsteps;
	if (step == 0)
		step = 1;
	for (size_t i = 0; i < num; i += step)
	{
		h ^= arr[i];
		h *= HASH_PRIME;
	}
	return h;
}

UI_FORCEINLINE size_t HashValue(unsigned char v) { return v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(signed char v) { return (unsigned char)v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(unsigned short v) { return v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(signed short v) { return (unsigned short)v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(unsigned int v) { return v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(signed int v) { return (unsigned int)v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(unsigned long v) { return v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(signed long v) { return (unsigned long)v * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(unsigned long long v) { return size_t(v * HASH_PRIME); }
UI_FORCEINLINE size_t HashValue(signed long long v) { return size_t((unsigned long long)v * HASH_PRIME); }

UI_FORCEINLINE size_t HashValue(float v) { return reinterpret_cast<u32&>(v) * HASH_PRIME; }
UI_FORCEINLINE size_t HashValue(double v) { return size_t(reinterpret_cast<u64&>(v) * HASH_PRIME); }

template <class T>
UI_FORCEINLINE size_t HashValue(const T* p) { return size_t(uintptr_t(p) * HASH_PRIME); }

template <class T>
struct HashEqualityComparer
{
	UI_FORCEINLINE static bool AreEqual(const T& a, const T& b)
	{
		return a == b;
	}
	UI_FORCEINLINE static size_t GetHash(const T& v)
	{
		return HashValue(v);
	}
};

} // ui
