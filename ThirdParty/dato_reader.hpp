
#pragma once

#if !defined(DATO_MEMCPY) || !defined(DATO_MEMCMP) || !defined(DATO_STRCMP)
#  include <string.h>
#endif

// validation - triggers a code breakpoint when hitting the failure condition

// whether to validate the accessed parts of the buffer
#ifndef DATO_VALIDATE_BUFFERS
#  ifdef NDEBUG
#    define DATO_VALIDATE_BUFFERS 0
#  else
#    define DATO_VALIDATE_BUFFERS 1
#  endif
#endif

#if DATO_VALIDATE_BUFFERS
#  define DATO_BUFFER_EXPECT(x) if (!(x)) DATO_CRASH
#else
#  define DATO_BUFFER_EXPECT(x)
#endif

// whether to validate inputs
#ifndef DATO_VALIDATE_INPUTS
#  ifdef NDEBUG
#    define DATO_VALIDATE_INPUTS 0
#  else
#    define DATO_VALIDATE_INPUTS 1
#  endif
#endif

#if DATO_VALIDATE_INPUTS
#  define DATO_INPUT_EXPECT(x) if (!(x)) DATO_CRASH
#else
#  define DATO_INPUT_EXPECT(x)
#endif

#ifndef DATO_CONFIG
#  define DATO_CONFIG 0
#endif


#ifdef _MSC_VER
#  define DATO_FORCEINLINE __forceinline
#  define DATO_NOINLINE __declspec(noinline)
extern "C" void __ud2(void);
#  pragma intrinsic(__ud2)
#  define DATO_CRASH _dato_error()
#else
#  define DATO_FORCEINLINE inline __attribute__((always_inline))
#  define DATO_NOINLINE __attribute__((noinline))
#  define DATO_CRASH __builtin_trap()
#endif

#define _DATO_CONCAT(a, b) a ## b
#define DATO_CONCAT(a, b) _DATO_CONCAT(a, b)


namespace dato {

#ifndef DATO_COMMON_DEFS
#define DATO_COMMON_DEFS

#ifdef _MSC_VER
[[noreturn]] __forceinline void _dato_error() { __ud2(); }
#endif

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

static const u8 TYPE_Null = 0;
// embedded value types
static const u8 TYPE_Bool = 1;
static const u8 TYPE_S32 = 2;
static const u8 TYPE_U32 = 3;
static const u8 TYPE_F32 = 4;
// external value reference types
// - one value
static const u8 TYPE_S64 = 5;
static const u8 TYPE_U64 = 6;
static const u8 TYPE_F64 = 7;
// - generic containers
static const u8 TYPE_Array = 8;
static const u8 TYPE_StringMap = 9;
static const u8 TYPE_IntMap = 10;
// - raw arrays (identified by purpose)
// (strings contain an extra 0-termination value not included in their size)
static const u8 TYPE_String8 = 11; // ASCII/UTF-8
static const u8 TYPE_String16 = 12; // UTF-16
static const u8 TYPE_String32 = 13; // UTF-32
static const u8 TYPE_ByteArray = 14;
static const u8 TYPE_Vector = 15;
static const u8 TYPE_VectorArray = 16;

static const u8 SUBTYPE_S8 = 0;
static const u8 SUBTYPE_U8 = 1;
static const u8 SUBTYPE_S16 = 2;
static const u8 SUBTYPE_U16 = 3;
static const u8 SUBTYPE_S32 = 4;
static const u8 SUBTYPE_U32 = 5;
static const u8 SUBTYPE_S64 = 6;
static const u8 SUBTYPE_U64 = 7;
static const u8 SUBTYPE_F32 = 8;
static const u8 SUBTYPE_F64 = 9;

template <class T> struct SubtypeInfo;
template <> struct SubtypeInfo<s8> { enum { Subtype = SUBTYPE_S8 }; };
template <> struct SubtypeInfo<u8> { enum { Subtype = SUBTYPE_U8 }; };
template <> struct SubtypeInfo<s16> { enum { Subtype = SUBTYPE_S16 }; };
template <> struct SubtypeInfo<u16> { enum { Subtype = SUBTYPE_U16 }; };
template <> struct SubtypeInfo<s32> { enum { Subtype = SUBTYPE_S32 }; };
template <> struct SubtypeInfo<u32> { enum { Subtype = SUBTYPE_U32 }; };
template <> struct SubtypeInfo<s64> { enum { Subtype = SUBTYPE_S64 }; };
template <> struct SubtypeInfo<u64> { enum { Subtype = SUBTYPE_U64 }; };
template <> struct SubtypeInfo<f32> { enum { Subtype = SUBTYPE_F32 }; };
template <> struct SubtypeInfo<f64> { enum { Subtype = SUBTYPE_F64 }; };

static const u8 FLAG_Aligned = 1 << 0;
static const u8 FLAG_SortedKeys = 1 << 1;

#ifndef DATO_MEMCPY
#  define DATO_MEMCPY memcpy
#endif

#ifndef DATO_MEMCMP
#  define DATO_MEMCMP memcmp
#endif

// override this if you're adding inline types
#ifndef DATO_IS_REFERENCE_TYPE
#define DATO_IS_REFERENCE_TYPE(t) ((t) >= TYPE_S64)
#endif

DATO_FORCEINLINE bool IsReferenceType(u8 t)
{
	return DATO_IS_REFERENCE_TYPE(t);
}

inline u32 RoundUp(u32 x, u32 n)
{
	return (x + n - 1) / n * n;
}

#endif // DATO_COMMON_DEFS

#ifndef DATO_STRCMP
#  define DATO_STRCMP strcmp
#endif


#if DATO_FAST_UNSAFE
// only for use with aligned data or platforms that support unaligned loads (x86/x64/ARM64)
// not guaranteed to work correctly if strict aliasing is enabled in compiler options!
template <class T> DATO_FORCEINLINE T ReadT(const void* ptr)
{
	return *(const T*)ptr;
}
#else
template <class T> DATO_FORCEINLINE T ReadT(const void* ptr)
{
	T v;
	DATO_MEMCPY(&v, ptr, sizeof(T));
	return v;
}
template <> DATO_FORCEINLINE char ReadT(const void* ptr) { return *(const char*)ptr; }
template <> DATO_FORCEINLINE signed char ReadT(const void* ptr) { return *(const signed char*)ptr; }
template <> DATO_FORCEINLINE unsigned char ReadT(const void* ptr) { return *(const unsigned char*)ptr; }
#endif

inline u32 SubtypeGetSize(u8 subtype)
{
	switch (subtype)
	{
	case SUBTYPE_S8: return 1;
	case SUBTYPE_U8: return 1;
	case SUBTYPE_S16: return 2;
	case SUBTYPE_U16: return 2;
	case SUBTYPE_S32: return 4;
	case SUBTYPE_U32: return 4;
	case SUBTYPE_S64: return 8;
	case SUBTYPE_U64: return 8;
	case SUBTYPE_F32: return 4;
	case SUBTYPE_F64: return 8;
	default: return 0;
	}
}

#define DATO_READSIZE_ARGS const char* data, u32 len, u32& pos
#define DATO_READSIZE_PASS data, len, pos

inline u32 ReadSizeU8(DATO_READSIZE_ARGS)
{
	(void)len;
	DATO_BUFFER_EXPECT(pos + 1 <= len);
	return u8(data[pos++]);
}

inline u32 ReadSizeU16(DATO_READSIZE_ARGS)
{
	(void)len;
	DATO_BUFFER_EXPECT(pos + 2 <= len);
	u32 v = ReadT<u16>(data + pos);
	pos += 2;
	return v;
}

inline u32 ReadSizeU32(DATO_READSIZE_ARGS)
{
	(void)len;
	DATO_BUFFER_EXPECT(pos + 4 <= len);
	u32 v = ReadT<u32>(data + pos);
	pos += 4;
	return v;
}

inline u32 ReadSizeU8X32(DATO_READSIZE_ARGS)
{
	(void)len;
	DATO_BUFFER_EXPECT(pos + 1 <= len);
	u32 v = u8(data[pos++]);
	if (v == 255)
	{
		DATO_BUFFER_EXPECT(pos + 4 <= len);
		v = ReadT<u32>(data + pos);
		pos += 4;
	}
	return v;
}

struct ReaderConfig0
{
	bool InitForReading(u8 id) { return id == 0; }

	DATO_FORCEINLINE u32 ReadKeyLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadMapSize(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadArrayLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadValueLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
};

struct ReaderConfig1
{
	bool InitForReading(u8 id) { return id == 1; }

	DATO_FORCEINLINE u32 ReadKeyLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadMapSize(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadArrayLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadValueLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU8X32(DATO_READSIZE_PASS); }
};

struct ReaderConfig2
{
	bool InitForReading(u8 id) { return id == 2; }

	DATO_FORCEINLINE u32 ReadKeyLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadMapSize(DATO_READSIZE_ARGS) const
	{ return ReadSizeU8X32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadArrayLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU8X32(DATO_READSIZE_PASS); }
	DATO_FORCEINLINE u32 ReadValueLength(DATO_READSIZE_ARGS) const
	{ return ReadSizeU8X32(DATO_READSIZE_PASS); }
};

struct ReaderConfigAdaptive
{
	typedef u32 ReadFunc(DATO_READSIZE_ARGS);

	ReadFunc* keyLength = nullptr;
	ReadFunc* mapSize = nullptr;
	ReadFunc* arrayLength = nullptr;
	ReadFunc* valueLength = nullptr;

	bool InitForReading(u8 id)
	{
		switch (id)
		{
		case 0:
			keyLength = &ReadSizeU32;
			mapSize = &ReadSizeU32;
			arrayLength = &ReadSizeU32;
			valueLength = &ReadSizeU32;
			return true;
		case 1:
			keyLength = &ReadSizeU32;
			mapSize = &ReadSizeU32;
			arrayLength = &ReadSizeU32;
			valueLength = &ReadSizeU8X32;
			return true;
		case 2:
			keyLength = &ReadSizeU32;
			mapSize = &ReadSizeU8X32;
			arrayLength = &ReadSizeU8X32;
			valueLength = &ReadSizeU8X32;
			return true;
		}
		return false;
	}

	u32 ReadKeyLength(DATO_READSIZE_ARGS) const { return keyLength(DATO_READSIZE_PASS); }
	u32 ReadMapSize(DATO_READSIZE_ARGS) const { return mapSize(DATO_READSIZE_PASS); }
	u32 ReadArrayLength(DATO_READSIZE_ARGS) const { return arrayLength(DATO_READSIZE_PASS); }
	u32 ReadValueLength(DATO_READSIZE_ARGS) const { return valueLength(DATO_READSIZE_PASS); }
};

struct IValueIterator
{
	virtual void BeginMap(u8 type, u32 size) = 0;
	virtual void EndMap(u8 type) = 0;
	virtual void BeginStringKey(const char* key, u32 length) = 0;
	virtual void EndStringKey() = 0;
	virtual void BeginIntKey(u32 key) = 0;
	virtual void EndIntKey() = 0;

	virtual void BeginArray(u32 size) = 0;
	virtual void EndArray() = 0;
	virtual void BeginArrayIndex(u32 i) = 0;
	virtual void EndArrayIndex() = 0;

	virtual void OnValueNull() = 0;
	virtual void OnValueBool(bool value) = 0;
	virtual void OnValueS32(s32 value) = 0;
	virtual void OnValueU32(u32 value) = 0;
	virtual void OnValueF32(f32 value) = 0;
	virtual void OnValueS64(s64 value) = 0;
	virtual void OnValueU64(u64 value) = 0;
	virtual void OnValueF64(f64 value) = 0;
	virtual void OnValueString(const char* data, u32 length) = 0;
	// assume the pointer is not readable without ReadT:
	virtual void OnValueString(const u16* data, u32 length) = 0;
	virtual void OnValueString(const u32* data, u32 length) = 0;

	virtual void OnValueByteArray(const void* data, u32 length) = 0;
	virtual void OnValueVector(u8 subtype, u8 elemCount, const void* data) = 0;
	virtual void OnValueVectorArray(u8 subtype, u8 elemCount, const void* data, u32 length) = 0;
	virtual void OnUnknownValue(u8 type, u32 embedded, const char* buffer, u32 length) = 0;
};

struct DATO_CONCAT(Reader, DATO_CONFIG)
{
private:
	using Reader = DATO_CONCAT(Reader, DATO_CONFIG);
	DATO_CONCAT(ReaderConfig, DATO_CONFIG) _cfg = {};
	const char* _data = nullptr;
	u32 _len = 0;
	u8 _flags = 0;
	u8 _rootType = 0;
	u32 _root = 0;

	template <class T> DATO_FORCEINLINE T RD(u32 pos) const { return ReadT<T>(_data + pos); }

	// compares the bytes until the first 0-char
	bool KeyEquals(u32 kpos, const char* str) const
	{
		u32 len = _cfg.ReadKeyLength(_data, _len, kpos);
		(void)len;
		DATO_BUFFER_EXPECT(kpos + len + 1 <= _len);
		return DATO_STRCMP(str, &_data[kpos]) == 0;
	}
	int KeyCompare(u32 kpos, const char* str) const
	{
		u32 len = _cfg.ReadKeyLength(_data, _len, kpos);
		(void)len;
		DATO_BUFFER_EXPECT(kpos + len + 1 <= _len);
		return DATO_STRCMP(str, &_data[kpos]);
	}
	// compares the size first, then all of bytes
	bool KeyEquals(u32 kpos, const void* mem, size_t lenMem) const
	{
		u32 len = _cfg.ReadKeyLength(_data, _len, kpos);
		if (len != lenMem)
			return false;
		DATO_BUFFER_EXPECT(kpos + len + 1 <= _len);
		return DATO_MEMCMP(mem, &_data[kpos], len) == 0;
	}
	int KeyCompare(u32 kpos, const void* mem, size_t lenMem) const
	{
		u32 len = _cfg.ReadKeyLength(_data, _len, kpos);
		DATO_BUFFER_EXPECT(kpos + len + 1 <= _len);
		u32 testLen = u32(len < lenMem ? len : lenMem);
		if (int bc = DATO_MEMCMP(mem, &_data[kpos], testLen))
			return bc;
		if (lenMem == len)
			return 0;
		return lenMem < len ? -1 : 1;
	}

public:
	struct DynamicAccessor;
	struct MapAccessor
	{
		Reader* _r;
		u32 _size;
		u32 _objpos;

		DATO_FORCEINLINE MapAccessor() : _r(nullptr), _size(0), _objpos(0) {}
		MapAccessor(Reader* r, u32 pos) : _r(r)
		{
			_objpos = pos;
			_size = r->_cfg.ReadMapSize(r->_data, r->_len, _objpos);
			DATO_BUFFER_EXPECT(pos + 9 * _size <= r->_len);
		}

		DATO_FORCEINLINE operator const void* () const { return _r; } // to support `if (init)` exprs
		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		// retrieving values
		DATO_FORCEINLINE DynamicAccessor TryGetValueByIndex(size_t i) const
		{
			if (i >= _size)
				return {};
			return GetValueByIndex(i);
		}
		DATO_FORCEINLINE DynamicAccessor GetValueByIndex(size_t i) const
		{
			DATO_INPUT_EXPECT(i < _size);
			u32 vpos = _objpos + _size * 4 + u32(i) * 4;
			u32 tpos = _objpos + _size * 8 + u32(i);
			u32 val = _r->RD<u32>(vpos);
			u8 type = _r->RD<u8>(tpos);
			if (IsReferenceType(type))
				val = _objpos - val;
			return { _r, val, type };
		}
	};
	struct StringMapAccessor : MapAccessor
	{
		struct Iterator
		{
			const StringMapAccessor* _obj;
			u32 _i;

			DATO_FORCEINLINE const Iterator& operator * () const { return *this; }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _i != o._i; }
			DATO_FORCEINLINE void operator ++ () { ++_i; }

			DATO_FORCEINLINE const char* GetKeyCStr(u32* pOutLen = nullptr) const
			{ return _obj->GetKeyCStr(_i, pOutLen); }
			DATO_FORCEINLINE u32 GetKeyLength() const { return _obj->GetKeyLength(_i); }

			DATO_FORCEINLINE DynamicAccessor GetValue() const { return _obj->GetValueByIndex(_i); }
		};

		using MapAccessor::MapAccessor;

		DATO_FORCEINLINE Iterator begin() const { return { this, 0 }; }
		DATO_FORCEINLINE Iterator end() const { return { this, _size }; }

		DATO_FORCEINLINE Iterator operator [](size_t i) const
		{
			DATO_INPUT_EXPECT(i < _size);
			return { this, u32(i) };
		}

		// retrieving keys
		const char* GetKeyCStr(size_t i, u32* pOutLen = nullptr) const
		{
			auto* BR = _r;
			DATO_INPUT_EXPECT(i < _size);
			u32 kpos = BR->RD<u32>(_objpos + u32(i) * 4);
			u32 L = BR->_cfg.ReadKeyLength(BR->_data, BR->_len, kpos);
			if (pOutLen)
				*pOutLen = L;
			DATO_BUFFER_EXPECT(kpos + L + 1 <= _r->_len);
			return &BR->_data[kpos];
		}
		DATO_FORCEINLINE u32 GetKeyLength(size_t i) const
		{
			u32 ret;
			GetKeyCStr(i, &ret);
			return ret;
		}

		// searching for values
		DATO_NOINLINE DynamicAccessor FindValueByKey(const char* keyToFind) const
		{
			auto* BR = _r;
			if (BR->_flags & FLAG_SortedKeys)
			{
				u32 L = 0, R = _size;
				while (L < R)
				{
					u32 M = (L + R) / 2;
					u32 keyM = BR->RD<u32>(_objpos + M * 4);
					int diff = BR->KeyCompare(keyM, keyToFind);
					if (diff == 0)
						return GetValueByIndex(M);
					if (diff < 0)
						R = M;
					else
						L = M + 1;
				}
			}
			else
			{
				for (u32 i = 0; i < _size; i++)
				{
					u32 key = BR->RD<u32>(_objpos + i * 4);
					if (BR->KeyEquals(key, keyToFind))
						return GetValueByIndex(i);
				}
			}
			return {};
		}
		DATO_NOINLINE DynamicAccessor FindValueByKey(const void* keyToFind, size_t lenKeyToFind) const
		{
			auto* BR = _r;
			if (BR->_flags & FLAG_SortedKeys)
			{
				u32 L = 0, R = _size;
				while (L < R)
				{
					u32 M = (L + R) / 2;
					u32 keyM = BR->RD<u32>(_objpos + M * 4);
					int diff = BR->KeyCompare(keyM, keyToFind, lenKeyToFind);
					if (diff == 0)
						return GetValueByIndex(M);
					if (diff < 0)
						R = M;
					else
						L = M + 1;
				}
			}
			else
			{
				for (u32 i = 0; i < _size; i++)
				{
					u32 key = BR->RD<u32>(_objpos + i * 4);
					if (BR->KeyEquals(key, keyToFind, lenKeyToFind))
						return GetValueByIndex(i);
				}
			}
			return {};
		}

		DATO_NOINLINE void Iterate(IValueIterator& it)
		{
			it.BeginMap(TYPE_StringMap, _size);
			for (u32 i = 0; i < _size; i++)
			{
				u32 keyLength;
				const char* key = GetKeyCStr(i, &keyLength);
				it.BeginStringKey(key, keyLength);
				{
					GetValueByIndex(i).Iterate(it);
				}
				it.EndStringKey();
			}
			it.EndMap(TYPE_StringMap);
		}
	};
	struct IntMapAccessor : MapAccessor
	{
		struct Iterator
		{
			const IntMapAccessor* _obj;
			u32 _i;

			DATO_FORCEINLINE const Iterator& operator * () const { return *this; }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _i != o._i; }
			DATO_FORCEINLINE void operator ++ () { ++_i; }

			DATO_FORCEINLINE u32 GetKey() const { return _obj->GetKey(_i); }
			DATO_FORCEINLINE DynamicAccessor GetValue() const { return _obj->GetValueByIndex(_i); }
		};

		using MapAccessor::MapAccessor;

		DATO_FORCEINLINE Iterator begin() const { return { this, 0 }; }
		DATO_FORCEINLINE Iterator end() const { return { this, _size }; }

		DATO_FORCEINLINE Iterator operator [](size_t i) const
		{
			DATO_INPUT_EXPECT(i < _size);
			return { this, u32(i) };
		}

		// retrieving keys
		u32 GetKey(size_t i) const
		{
			DATO_INPUT_EXPECT(i < _size);
			return _r->RD<u32>(_objpos + u32(i) * 4);
		}

		// searching for values
		DATO_NOINLINE DynamicAccessor FindValueByKey(u32 keyToFind) const
		{
			if (_r->_flags & FLAG_SortedKeys)
			{
				u32 L = 0, R = _size;
				while (L < R)
				{
					u32 M = (L + R) / 2;
					u32 keyM = _r->RD<u32>(_objpos + M * 4);
					if (keyToFind == keyM)
						return GetValueByIndex(M);
					if (keyToFind < keyM)
						R = M;
					else
						L = M + 1;
				}
			}
			else
			{
				for (u32 i = 0; i < _size; i++)
				{
					u32 key = _r->RD<u32>(_objpos + i * 4);
					if (key == keyToFind)
						return GetValueByIndex(i);
				}
			}
			return {};
		}

		DATO_NOINLINE void Iterate(IValueIterator& it)
		{
			it.BeginMap(TYPE_IntMap, _size);
			for (u32 i = 0; i < _size; i++)
			{
				it.BeginIntKey(GetKey(i));
				{
					GetValueByIndex(i).Iterate(it);
				}
				it.EndIntKey();
			}
			it.EndMap(TYPE_IntMap);
		}
	};
	struct ArrayAccessor
	{
		struct Iterator
		{
			const ArrayAccessor* _obj;
			u32 _i;

			DATO_FORCEINLINE DynamicAccessor operator * () const { return _obj->GetValueByIndex(_i); }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _i != o._i; }
			DATO_FORCEINLINE void operator ++ () { ++_i; }
		};

		Reader* _r;
		u32 _size;
		u32 _arrpos;

		DATO_FORCEINLINE ArrayAccessor() : _r(nullptr), _size(0), _arrpos(0) {}
		ArrayAccessor(Reader* r, u32 pos) : _r(r)
		{
			_arrpos = pos;
			_size = r->_cfg.ReadArrayLength(r->_data, r->_len, _arrpos);
			DATO_BUFFER_EXPECT(pos + 5 * _size <= r->_len);
		}

		DATO_FORCEINLINE operator const void* () const { return _r; } // to support `if (init)` exprs
		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		DATO_FORCEINLINE Iterator begin() const { return { this, 0 }; }
		DATO_FORCEINLINE Iterator end() const { return { this, _size }; }

		DATO_FORCEINLINE DynamicAccessor operator [](size_t i) const { return GetValueByIndex(i); }

		// retrieving values
		DATO_FORCEINLINE DynamicAccessor TryGetValueByIndex(size_t i) const
		{
			if (i >= _size)
				return {};
			return GetValueByIndex(i);
		}
		DATO_FORCEINLINE DynamicAccessor GetValueByIndex(size_t i) const
		{
			DATO_INPUT_EXPECT(i < _size);
			u32 vpos = _arrpos + u32(i) * 4;
			u32 tpos = _arrpos + _size * 4 + u32(i);
			u32 val = _r->RD<u32>(vpos);
			u8 type = _r->RD<u8>(tpos);
			if (IsReferenceType(type))
				val = _arrpos - val;
			return { _r, val, type };
		}

		DATO_NOINLINE void Iterate(IValueIterator& it)
		{
			it.BeginArray(_size);
			for (u32 i = 0; i < _size; i++)
			{
				it.BeginArrayIndex(i);
				{
					GetValueByIndex(i).Iterate(it);
				}
				it.EndArrayIndex();
			}
			it.EndArray();
		}
	};
	template <class T>
	struct TypedArrayAccessor
	{
		struct Iterator
		{
			const T* _ptr;

			DATO_FORCEINLINE T operator * () const { return ReadT<T>(_ptr); }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _ptr != o._ptr; }
			DATO_FORCEINLINE void operator ++ () { ++_ptr; }
		};

		const T* _data;
		u32 _size;

		DATO_FORCEINLINE TypedArrayAccessor() : _data(nullptr), _size(0) {}
		TypedArrayAccessor(Reader* r, u32 pos)
		{
			_size = r->_cfg.ReadValueLength(r->_data, r->_len, pos);
			DATO_BUFFER_EXPECT(pos + sizeof(T) * _size <= r->_len);
			// will not be dereferenced until ReadT but casting early for simplified code
			_data = (const T*) (const void*) (r->_data + pos);
		}

		DATO_FORCEINLINE operator const void* () const { return _data; } // to support `if (init)` exprs
		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		DATO_FORCEINLINE Iterator begin() const { return { _data }; }
		DATO_FORCEINLINE Iterator end() const { return { _data + _size }; }

		DATO_FORCEINLINE T operator [](size_t i) const { return ReadT<T>(&_data[i]); }
	};
	struct ByteArrayAccessor : TypedArrayAccessor<u8>
	{
		using TypedArrayAccessor<u8>::TypedArrayAccessor;

		DATO_FORCEINLINE const u8* GetData() const { return _data; }
		DATO_FORCEINLINE const u8* begin() const { return _data; }
		DATO_FORCEINLINE const u8* end() const { return _data + _size; }

		void Iterate(IValueIterator& it)
		{
			it.OnValueByteArray(this->_data, this->_size);
		}
	};
	struct String8Accessor : TypedArrayAccessor<char>
	{
		using TypedArrayAccessor<char>::TypedArrayAccessor;

		DATO_FORCEINLINE const char* GetData() const { return _data; }
		DATO_FORCEINLINE const char* begin() const { return _data; }
		DATO_FORCEINLINE const char* end() const { return _data + _size; }

		void Iterate(IValueIterator& it)
		{
			it.OnValueString(_data, _size);
		}
	};
	template <class T> struct StringAccessor : TypedArrayAccessor<T>
	{
		using TypedArrayAccessor<T>::TypedArrayAccessor;

		void Iterate(IValueIterator& it)
		{
			it.OnValueString(this->_data, this->_size);
		}
	};
	DATO_FORCEINLINE void ParseVectorAccessorPrefix(u32& pos, u8& outSubtype, u8& outElemCount)
	{
		DATO_BUFFER_EXPECT(pos + 2 <= _len);
		outSubtype = RD<u8>(pos++);
		outElemCount = RD<u8>(pos++);
	}
	template <class T>
	struct VectorAccessor
	{
		const T* _data;
		u8 _subtype;
		u8 _elemCount;

		DATO_FORCEINLINE VectorAccessor() : _data(nullptr), _subtype(0), _elemCount(0) {}
		VectorAccessor(Reader* r, u32 pos, u8 st, u8 ec) : _subtype(st), _elemCount(ec)
		{
			DATO_BUFFER_EXPECT(pos + sizeof(T) * ec <= r->_len);
			// will not be dereferenced until ReadT but casting early for simplified code
			_data = (const T*) (const void*) (r->_data + pos);
		}

		DATO_FORCEINLINE operator const void* () const { return _data; } // to support `if (init)` exprs
		DATO_FORCEINLINE u8 GetElementCount() const { return _elemCount; }

		DATO_FORCEINLINE T operator [](size_t i) const { return ReadT<T>(&_data[i]); }
		DATO_FORCEINLINE void CopyTo(T* ret, u32 N) const
		{
			DATO_INPUT_EXPECT(N <= _elemCount);
			CopyTo_SkipChecks(ret, N);
		}
		DATO_FORCEINLINE void CopyTo_SkipChecks(T* ret, u32 N) const
		{
			memcpy(ret, _data, sizeof(T) * N);
		}

		void Iterate(IValueIterator& it)
		{
			it.OnValueVector(_subtype, _elemCount, _data);
		}
	};
	template <class T>
	struct VectorArrayAccessor
	{
		struct Iterator
		{
			const T* _ptr;
			u8 elemCount;

			DATO_FORCEINLINE T operator [](size_t i) const
			{
				DATO_INPUT_EXPECT(i < elemCount);
				return ReadT<T>(&_ptr[i]);
			}
			DATO_FORCEINLINE const Iterator& operator * () const { return *this; }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _ptr != o._ptr; }
			DATO_FORCEINLINE void operator ++ () { _ptr += elemCount; }
		};

		const T* _data;
		u8 _subtype;
		u8 _elemCount;
		u32 _size;

		DATO_FORCEINLINE VectorArrayAccessor() : _data(nullptr), _subtype(0), _elemCount(0), _size(0) {}
		VectorArrayAccessor(Reader* r, u32 pos, u8 st, u8 ec) : _subtype(st), _elemCount(ec)
		{
			_size = r->_cfg.ReadValueLength(r->_data, r->_len, pos);
			DATO_BUFFER_EXPECT(pos + sizeof(T) * _elemCount * _size <= r->_len);
			// will not be dereferenced until ReadT but casting early for simplified code
			_data = (const T*) (const void*) (r->_data + pos);
		}

		DATO_FORCEINLINE operator const void* () const { return _data; } // to support `if (init)` exprs
		DATO_FORCEINLINE u8 GetElementCount() const { return _elemCount; }
		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		DATO_FORCEINLINE Iterator begin() const { return { _data, _elemCount }; }
		DATO_FORCEINLINE Iterator end() const { return { _data + _size * _elemCount, _elemCount }; }

		DATO_FORCEINLINE T operator [](size_t i) const { return ReadT<T>(&_data[i]); }
		DATO_FORCEINLINE void CopyTo(T* ret, u32 N, u32 elem = 0) const
		{
			DATO_INPUT_EXPECT(N + elem * _elemCount <= _size * _elemCount);
			CopyTo_SkipChecks(ret, N, elem);
		}
		DATO_FORCEINLINE void CopyTo_SkipChecks(T* ret, u32 N, u32 elem = 0) const
		{
			memcpy(ret, _data, sizeof(T) * (N + elem * _elemCount));
		}

		void Iterate(IValueIterator& it)
		{
			it.OnValueVectorArray(_subtype, _elemCount, _data, _size);
		}
	};
	struct DynamicAccessor
	{
		Reader* _r;
		u32 _pos;
		u8 _type;

		DATO_FORCEINLINE DynamicAccessor() : _r(nullptr), _pos(0), _type(TYPE_Null) {}
		DATO_FORCEINLINE DynamicAccessor(Reader* r, u32 pos, u8 type)
			: _r(r), _pos(pos), _type(type) {}

		DATO_FORCEINLINE bool IsValid() const { return !!_r; }
		DATO_FORCEINLINE operator const void* () const { return _r; } // to support `if (init)` exprs

		DATO_NOINLINE void Iterate(IValueIterator& it)
		{
			switch (_type)
			{
			case TYPE_Null: it.OnValueNull(); break;
			case TYPE_Bool: it.OnValueBool(_pos != 0); break;
			case TYPE_S32: it.OnValueS32(_pos); break;
			case TYPE_U32: it.OnValueU32(_pos); break;
			case TYPE_F32: it.OnValueF32(ReadT<f32>(&_pos)); break;
			case TYPE_S64: it.OnValueS64(_r->RD<s64>(_pos)); break;
			case TYPE_U64: it.OnValueU64(_r->RD<u64>(_pos)); break;
			case TYPE_F64: it.OnValueF64(_r->RD<f64>(_pos)); break;
			case TYPE_Array: ArrayAccessor(_r, _pos).Iterate(it); break;
			case TYPE_StringMap: StringMapAccessor(_r, _pos).Iterate(it); break;
			case TYPE_IntMap: IntMapAccessor(_r, _pos).Iterate(it); break;
			case TYPE_String8: String8Accessor(_r, _pos).Iterate(it); break;
			case TYPE_String16: StringAccessor<u16>(_r, _pos).Iterate(it); break;
			case TYPE_String32: StringAccessor<u32>(_r, _pos).Iterate(it); break;
			case TYPE_ByteArray: ByteArrayAccessor(_r, _pos).Iterate(it); break;
			case TYPE_Vector: {
				DATO_BUFFER_EXPECT(_pos + 2 <= _r->_len);
				u8 subtype = _r->RD<u8>(_pos);
				u8 elemCount = _r->RD<u8>(_pos + 1);
				it.OnValueVector(subtype, elemCount, _r->_data + _pos + 2);
				break; }
			case TYPE_VectorArray: {
				DATO_BUFFER_EXPECT(_pos + 2 <= _r->_len);
				u32 vpos = _pos;
				u8 subtype = _r->RD<u8>(vpos++);
				u8 elemCount = _r->RD<u8>(vpos++);
				u32 _size = _r->_cfg.ReadValueLength(_r->_data, _r->_len, vpos);
				DATO_BUFFER_EXPECT(vpos + SubtypeGetSize(subtype) * elemCount * _size <= _r->_len);
				it.OnValueVectorArray(subtype, elemCount, _r->_data + vpos, _size);
				break; }
			default: it.OnUnknownValue(_type, _pos, _r->_data, _r->_len); break;
			}
		}

		// reading type info
		DATO_FORCEINLINE u8 GetType() const { return _type; }
		inline u8 GetSubtype() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_Vector || _type == TYPE_VectorArray);
			DATO_BUFFER_EXPECT(_pos + 2 <= _r->_len);
			return _r->RD<u8>(_pos);
		}
		inline u8 GetElementCount() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_Vector || _type == TYPE_VectorArray);
			DATO_BUFFER_EXPECT(_pos + 2 <= _r->_len);
			return _r->RD<u8>(_pos + 1);
		}

		// type checks
		DATO_FORCEINLINE bool IsNull() const { return _type == TYPE_Null; }
		DATO_FORCEINLINE bool IsBool() const { return _type == TYPE_Bool; }
		bool IsInteger() const
		{
			return _type == TYPE_S32 || _type == TYPE_U32
				|| _type == TYPE_S64 || _type == TYPE_U64;
		}
		bool IsFloat() const { return _type == TYPE_F32 || _type == TYPE_F64; }
		bool IsNumber() const
		{
			return _type == TYPE_S32 || _type == TYPE_U32 || _type == TYPE_F32
				|| _type == TYPE_S64 || _type == TYPE_U64 || _type == TYPE_F64;
		}
		bool IsVector(u8 subtype, u8 elemCount) const
		{
			if (_type == TYPE_Vector)
			{
				DATO_BUFFER_EXPECT(_pos + 2 <= _r->_len);
				return _r->RD<u8>(_pos) == subtype
					&& _r->RD<u8>(_pos + 1) == elemCount;
			}
			return false;
		}
		template <class T> DATO_FORCEINLINE bool IsVectorT(u8 elemCount) const
		{
			return IsVector(SubtypeInfo<T>::Subtype, elemCount);
		}
		bool IsVectorArray(u8 subtype, u8 elemCount) const
		{
			if (_type == TYPE_VectorArray)
			{
				DATO_BUFFER_EXPECT(_pos + 2 <= _r->_len);
				return _r->RD<u8>(_pos) == subtype
					&& _r->RD<u8>(_pos + 1) == elemCount;
			}
			return false;
		}
		template <class T> DATO_FORCEINLINE bool IsVectorArrayT(u8 elemCount) const
		{
			return IsVectorArray(SubtypeInfo<T>::Subtype, elemCount);
		}

		// reading the assumed exact data
		inline bool AsBool() const { DATO_INPUT_EXPECT(_type == TYPE_Bool); return _pos != 0; }
		inline s32 AsS32() const { DATO_INPUT_EXPECT(_type == TYPE_S32); return _pos; }
		inline u32 AsU32() const { DATO_INPUT_EXPECT(_type == TYPE_U32); return _pos; }
		inline f32 AsF32() const { DATO_INPUT_EXPECT(_type == TYPE_F32); return ReadT<f32>(&_pos); }
		inline s64 AsS64() const { DATO_INPUT_EXPECT(_type == TYPE_S64); return _r->RD<s64>(_pos); }
		inline u64 AsU64() const { DATO_INPUT_EXPECT(_type == TYPE_U64); return _r->RD<u64>(_pos); }
		inline f64 AsF64() const { DATO_INPUT_EXPECT(_type == TYPE_F64); return _r->RD<f64>(_pos); }

		inline StringMapAccessor AsStringMap() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_StringMap);
			return { _r, _pos };
		}
		inline IntMapAccessor AsIntMap() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_IntMap);
			return { _r, _pos };
		}
		inline ArrayAccessor AsArray() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_Array);
			return { _r, _pos };
		}

		inline String8Accessor AsString8() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_String8);
			return { _r, _pos };
		}
		inline StringAccessor<u16> AsString16() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_String16);
			return { _r, _pos };
		}
		inline StringAccessor<u32> AsString32() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_String32);
			return { _r, _pos };
		}
		inline ByteArrayAccessor AsByteArray() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_ByteArray);
			return { _r, _pos };
		}

		template <class T> inline VectorAccessor<T> AsVector() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_Vector);
			u32 pos = _pos;
			u8 subtype, elemCount;
			_r->ParseVectorAccessorPrefix(pos, subtype, elemCount);
			DATO_INPUT_EXPECT(subtype == SubtypeInfo<T>::Subtype);
			return { _r, pos, subtype, elemCount };
		}
		template <class T> inline VectorAccessor<T> AsVector(u16 expectedElemCount) const
		{
			DATO_INPUT_EXPECT(_type == TYPE_Vector);
			u32 pos = _pos;
			u8 subtype, elemCount;
			_r->ParseVectorAccessorPrefix(pos, subtype, elemCount);
			DATO_INPUT_EXPECT(subtype == SubtypeInfo<T>::Subtype);
			(void)expectedElemCount;
			DATO_INPUT_EXPECT(elemCount == expectedElemCount);
			return { _r, pos, subtype, elemCount };
		}
		template <class T> inline VectorArrayAccessor<T> AsVectorArray() const
		{
			DATO_INPUT_EXPECT(_type == TYPE_VectorArray);
			u32 pos = _pos;
			u8 subtype, elemCount;
			_r->ParseVectorAccessorPrefix(pos, subtype, elemCount);
			DATO_INPUT_EXPECT(subtype == SubtypeInfo<T>::Subtype);
			return { _r, pos, subtype, elemCount };
		}
		template <class T> inline VectorArrayAccessor<T> AsVectorArray(u16 expectedElemCount) const
		{
			DATO_INPUT_EXPECT(_type == TYPE_VectorArray);
			u32 pos = _pos;
			u8 subtype, elemCount;
			_r->ParseVectorAccessorPrefix(pos, subtype, elemCount);
			DATO_INPUT_EXPECT(subtype == SubtypeInfo<T>::Subtype);
			(void)expectedElemCount;
			DATO_INPUT_EXPECT(elemCount == expectedElemCount);
			return { _r, pos, subtype, elemCount };
		}

		// reading data with validation (to avoid double validation)
		inline StringMapAccessor TryGetStringMap() const
		{
			if (_type == TYPE_StringMap)
				return { _r, _pos };
			return {};
		}
		inline IntMapAccessor TryGetIntMap() const
		{
			if (_type == TYPE_IntMap)
				return { _r, _pos };
			return {};
		}
		inline ArrayAccessor TryGetArray() const
		{
			if (_type == TYPE_Array)
				return { _r, _pos };
			return {};
		}

		inline String8Accessor TryGetString8() const
		{
			if (_type == TYPE_String8)
				return { _r, _pos };
			return {};
		}
		inline StringAccessor<u16> TryGetString16() const
		{
			if (_type == TYPE_String16)
				return { _r, _pos };
			return {};
		}
		inline StringAccessor<u32> TryGetString32() const
		{
			if (_type == TYPE_String32)
				return { _r, _pos };
			return {};
		}
		inline ByteArrayAccessor TryGetByteArray() const
		{
			if (_type == TYPE_ByteArray)
				return { _r, _pos };
			return {};
		}

		template <class T> inline VectorAccessor<T> TryGetVector() const
		{
			if (_type == TYPE_Vector)
			{
				u32 pos = _pos;
				u8 st, ec;
				_r->ParseVectorAccessorPrefix(pos, st, ec);
				if (st == SubtypeInfo<T>::Subtype)
					return { _r, pos, st, ec };
			}
			return {};
		}
		template <class T> inline VectorAccessor<T> TryGetVector(u16 expectedElemCount) const
		{
			if (_type == TYPE_Vector)
			{
				u32 pos = _pos;
				u8 st, ec;
				_r->ParseVectorAccessorPrefix(pos, st, ec);
				if (st == SubtypeInfo<T>::Subtype && ec == expectedElemCount)
					return { _r, pos, st, ec };
			}
			return {};
		}
		template <class T> inline VectorArrayAccessor<T> TryGetVectorArray() const
		{
			if (_type == TYPE_VectorArray)
			{
				u32 pos = _pos;
				u8 st, ec;
				_r->ParseVectorAccessorPrefix(pos, st, ec);
				if (st == SubtypeInfo<T>::Subtype)
					return { _r, pos, st, ec };
			}
			return {};
		}
		template <class T> inline VectorArrayAccessor<T> TryGetVectorArray(u16 expectedElemCount) const
		{
			if (_type == TYPE_VectorArray)
			{
				u32 pos = _pos;
				u8 st, ec;
				_r->ParseVectorAccessorPrefix(pos, st, ec);
				if (st == SubtypeInfo<T>::Subtype && ec == expectedElemCount)
					return { _r, pos, st, ec };
			}
			return {};
		}

		// casts
		template <class T> DATO_NOINLINE T CastToNumber() const
		{
			switch (_type)
			{
			case TYPE_Bool: return T(_pos ? 1 : 0);
			case TYPE_S32: return T(s32(_pos));
			case TYPE_U32: return T(_pos);
			case TYPE_F32: return T(ReadT<f32>(&_pos));
			case TYPE_S64: return T(_r->RD<s64>(_pos));
			case TYPE_U64: return T(_r->RD<u64>(_pos));
			case TYPE_F64: return T(_r->RD<f64>(_pos));
			default: return T(0);
			}
		}
		DATO_NOINLINE bool CastToBool() const
		{
			switch (_type)
			{
			case TYPE_Bool:
			case TYPE_S32:
			case TYPE_U32: return _pos != 0;
			case TYPE_F32: return ReadT<f32>(&_pos) != 0;
			case TYPE_S64:
			case TYPE_U64: return _r->RD<u64>(_pos) != 0;
			case TYPE_F64: return _r->RD<f64>(_pos) != 0;
			default: return false;
			}
		}
	};

	DATO_NOINLINE bool Init(const void* data, u32 len, const void* prefix = "DATO", u32 prefix_len = 4)
	{
		if (prefix_len + 3 > len)
			return false;

		if (0 != DATO_MEMCMP(data, prefix, prefix_len))
			return false;

		const char* cdata = (const char*) data;
		u32 rootpos = prefix_len + 3;
		if (cdata[prefix_len + 1] & FLAG_Aligned)
			rootpos = RoundUp(rootpos, 4);
		DATO_BUFFER_EXPECT(rootpos + 4 <= len);
		if (!(rootpos + 4 <= len))
			return false;

		u32 root = ReadT<u32>(cdata + rootpos);

		if (!_cfg.InitForReading(cdata[prefix_len]))
			return false;

		// safe to init
		_data = cdata;
		_len = len;
		_flags = cdata[prefix_len + 1];
		_root = root;
		_rootType = cdata[prefix_len + 2];
		return true;
	}

	DynamicAccessor GetRoot()
	{
		return { this, _root, _rootType };
	}
};

using Reader = DATO_CONCAT(Reader, DATO_CONFIG);

} // dato
