
#pragma once

#if !defined(DATO_MEMCPY) || !defined(DATO_MEMCMP)
#  include <string.h>
#endif

#if !defined(DATO_MALLOC) || !defined(DATO_REALLOC) || !defined(DATO_FREE)
#  include <malloc.h>
#  define DATO_MALLOC malloc
#  define DATO_REALLOC realloc
#  define DATO_FREE free
#endif

#ifdef DATO_USE_STD_SORT // increases compile time, only valuable as a reference/workaround
#  include <algorithm>
#endif

// validation - triggers a code breakpoint when hitting the failure condition

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
extern "C" void __ud2(void);
#  pragma intrinsic(__ud2)
#  define DATO_CRASH _dato_error()
#else
#  define DATO_FORCEINLINE inline __attribute__((always_inline))
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


#if DATO_FAST_UNSAFE
template <class Dst, class Src> DATO_FORCEINLINE Dst BitCast(Src v)
{
	static_assert(sizeof(Dst) == sizeof(Src), "sizes don't match");
	return *(Dst*)&v;
}
#else
template <class Dst, class Src> DATO_FORCEINLINE Dst BitCast(Src v)
{
	static_assert(sizeof(Dst) == sizeof(Src), "sizes don't match");
	Dst ret;
	memcpy(&ret, &v, sizeof(v));
	return ret;
}
#endif

template <class T> inline u32 StrLen(const T* str)
{
	const T* p = str;
	while (*p)
		p++;
	return u32(p - str);
}

struct Builder
{
	char* _data = nullptr;
	u32 _size = 0;
	u32 _mem = 0;
	bool error = false;

	~Builder()
	{
		DATO_FREE(_data);
	}

	DATO_FORCEINLINE const void* GetData() const { return _data; }
	DATO_FORCEINLINE u32 GetSize() const { return _size; }

	void Reserve(u32 atLeast)
	{
		if (_mem < atLeast)
			_ResizeImpl(atLeast);
	}

	void _ResizeImpl(u32 newSize)
	{
		_data = (char*) DATO_REALLOC(_data, newSize);
		_mem = newSize;
	}
	void _ReserveForAppend(u32 sizeToAppend)
	{
		if (_size + sizeToAppend > _mem)
			_ResizeImpl(_size + sizeToAppend + _mem);
	}

	void AddZeroes(u32 num)
	{
		_ReserveForAppend(num);
		for (u32 i = 0; i < num; i++)
			_data[_size++] = 0;
	}
	void AddZeroesUntil(u32 pos)
	{
		if (pos <= _size)
			return;
		_ReserveForAppend(pos - _size);
		while (_size < pos)
			_data[_size++] = 0;
	}
	DATO_FORCEINLINE void AddU32(u32 v)
	{
		AddMem(&v, 4);
	}
	void AddByte(u8 byte)
	{
		_ReserveForAppend(1);
		_data[_size++] = char(byte);
	}
	void AddMem(const void* mem, u32 size)
	{
		_ReserveForAppend(size);
		memcpy(&_data[_size], mem, size);
		_size += size;
	}

	void SetError_ValueOutOfRange() { error = true; }
};

inline u32 WriteSizeU8(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	if (val > 0xff)
	{
		B.SetError_ValueOutOfRange();
		val = 0;
	}
	u32 pos = B.GetSize();
	if (align != 0)
	{
		u32 totalsize = 1 + pfxsize;
		pos = RoundUp(pos + totalsize, align) - totalsize;
		B.AddZeroesUntil(pos);
	}
	if (pfxsize)
		B.AddMem(prefix, pfxsize);
	B.AddByte(u8(val));
	return pos;
}

inline u32 WriteSizeU16(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	if (val > 0xffff)
	{
		B.SetError_ValueOutOfRange();
		val = 0;
	}
	u32 pos = B.GetSize();
	if (align != 0)
	{
		if (align < 2)
			align = 2;
		u32 totalsize = 2 + pfxsize;
		pos = RoundUp(pos + totalsize, align) - totalsize;
		B.AddZeroesUntil(pos);
	}
	if (pfxsize)
		B.AddMem(prefix, pfxsize);
	B.AddMem(&val, 2);
	return pos;
}

inline u32 WriteSizeU32(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	u32 pos = B.GetSize();
	if (align != 0)
	{
		if (align < 4)
			align = 4;
		u32 totalsize = 4 + pfxsize;
		pos = RoundUp(pos + totalsize, align) - totalsize;
		B.AddZeroesUntil(pos);
	}
	if (pfxsize)
		B.AddMem(prefix, pfxsize);
	B.AddMem(&val, 4);
	return pos;
}

inline u32 WriteSizeU8X32(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	u32 pos = B.GetSize();
	if (val < 0xff)
	{
		if (align != 0)
		{
			u32 totalsize = 1 + pfxsize;
			pos = RoundUp(pos + totalsize, align) - totalsize;
			B.AddZeroesUntil(pos);
		}
		if (pfxsize)
			B.AddMem(prefix, pfxsize);
		B.AddByte(u8(val));
	}
	else
	{
		if (align != 0)
		{
			if (align < 4)
				align = 4;
			u32 totalsize = 5 + pfxsize;
			pos = RoundUp(pos + totalsize, align) - totalsize;
			B.AddZeroesUntil(pos);
		}
		if (pfxsize)
			B.AddMem(prefix, pfxsize);
		B.AddByte(0xff);
		B.AddMem(&val, 4);
	}
	return pos;
}

struct WriterConfig0
{
	static u8 Identifier() { return 0; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteMapSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
};

struct WriterConfig1
{
	static u8 Identifier() { return 1; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteMapSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
};

struct WriterConfig2
{
	static u8 Identifier() { return 2; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteMapSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
};

struct KeyRef
{
	u32 pos;
	u32 dataPos;
	u32 dataLen;
};

struct ValueRef
{
	u8 type;
	u32 pos;
};

struct IntMapEntry
{
	u32 key;
	ValueRef value;
};

struct StringMapEntry
{
	KeyRef key;
	ValueRef value;
};

// both hashing functions are FNV-1a (32-bit)
inline u32 MemHash(const void* rawp, u32 len)
{
	auto* mem = (const char*) rawp;
	u32 nToHash = len < 32 ? len : 32;
	u32 interval = len / nToHash;
	u32 hash = 0x811c9dc5;
	for (u32 i = 0; i < len; i += interval)
	{
		hash ^= mem[i];
		hash *= 0x01000193;
	}
	return hash;
}

inline constexpr u32 StrHash(const char* str)
{
	u32 hash = 0x811c9dc5;
	while (*str)
	{
		hash ^= *str++;
		hash *= 0x01000193;
	}
	return hash;
}

struct MemReuseHashTable
{
	struct Entry
	{
		u32 valuePos;
		u32 dataOff;
		u32 len;
		u32 hash;
	};

	char** _pdata = nullptr; // must be initialized
	Entry* _entries = nullptr;
	u32 _numEntries = 0;
	u32 _memEntries = 0;
	static constexpr const u32 NO_VALUE = 0xffffffff;
	u32* _table = nullptr;
	u32 _numTableSlots = 0;

	MemReuseHashTable(char*& data) : _pdata(&data) {}
	~MemReuseHashTable()
	{
		DATO_FREE(_entries);
		DATO_FREE(_table);
	}

	Entry* Find(const void* mem, u32 len) const
	{
		if (!_numEntries)
			return nullptr;
		char* data = *_pdata;
		u32 hash = MemHash(mem, len);
		u32 ipos = hash % _numTableSlots;
		u32 pos = ipos;
		for (;;)
		{
			u32 p = _table[pos];
			if (p == NO_VALUE)
				return nullptr;
			Entry& e = _entries[p];
			if (e.hash == hash &&
				e.len == len &&
				memcmp(&data[e.dataOff], mem, len) == 0)
				return &e;
			pos = (pos + 1) % _numTableSlots;
			if (pos == ipos)
				return nullptr;
		}
	}

	// must not already exist in the table
	void Insert(u32 valuePos, u32 dataOff, u32 len)
	{
		// keep at least 20% of the hash->pos table free
		if (_numEntries * 5 >= _numTableSlots * 4)
			_Rehash(_numTableSlots == 0 ? 16 : _numTableSlots * 2);

		if (_numEntries >= _memEntries)
		{
			_memEntries = _memEntries == 0 ? 16 : _memEntries * 2;
			_entries = (Entry*) DATO_REALLOC(_entries, _memEntries * sizeof(Entry));
		}

		char* data = *_pdata;
		u32 hash = MemHash(&data[dataOff], len);
		u32 entryIndex = _numEntries++;
		_entries[entryIndex] = { valuePos, dataOff, len, hash };
		_Insert(hash, entryIndex);
	}

	void _Rehash(u32 newSlots)
	{
		DATO_FREE(_table);
		_table = (u32*) DATO_MALLOC(newSlots * sizeof(u32));
		_numTableSlots = newSlots;

		for (u32 i = 0; i < newSlots; i++)
			_table[i] = NO_VALUE;

		for (u32 i = 0; i < _numEntries; i++)
		{
			const Entry& e = _entries[i];
			_Insert(e.hash, i);
		}
	}

	void _Insert(u32 hash, u32 entryIndex)
	{
		u32 ipos = hash % _numTableSlots;
		u32 pos = ipos;
		for (;;)
		{
			u32 p = _table[pos];
			if (p == NO_VALUE)
			{
				_table[pos] = entryIndex;
				return;
			}

			// TODO robin hood hashing

			pos = (pos + 1) % _numTableSlots;
			if (pos == ipos)
			{
				// unexpectedly failed to insert
				DATO_CRASH;
				return;
			}
		}
	}
};

struct WriterBase : Builder
{
	MemReuseHashTable _keyTable { _data };
	u32 _rootPos;
	u32 _rootTypePos;
	u8 _flags;

	WriterBase(const char* prefix, u32 pfxsize, u8 cfgid, u8 flags) : _flags(flags)
	{
		AddMem(prefix, pfxsize);
		AddByte(cfgid);
		AddByte(flags);

		// root type
		_rootTypePos = GetSize();
		AddByte(0);

		// root position
		if (flags & FLAG_Aligned)
			AddZeroesUntil(RoundUp(GetSize(), 4));
		_rootPos = GetSize();
		AddZeroesUntil(GetSize() + 4); // reserve the space
	}

	void SetRoot(ValueRef objRef)
	{
		_data[_rootTypePos] = objRef.type;
		memcpy(&_data[_rootPos], &objRef.pos, 4);
	}

	DATO_FORCEINLINE u8 Align(u8 a)
	{
		return _flags & FLAG_Aligned ? a : 0;
	}

	DATO_FORCEINLINE KeyRef WriteIntKey(u32 k)
	{
		return { k, 0, 0 };
	}

	DATO_FORCEINLINE u32 AddValue8(const void* mem)
	{
		if (_flags & FLAG_Aligned)
			AddZeroesUntil(RoundUp(GetSize(), 8));
		u32 ret = GetSize();
		AddMem(mem, 8);
		return ret;
	}

	DATO_FORCEINLINE ValueRef WriteNull()
	{
		return { TYPE_Null, 0 };
	}
	DATO_FORCEINLINE ValueRef WriteBool(bool v)
	{
		return { TYPE_Bool, u32(v) };
	}
	DATO_FORCEINLINE ValueRef WriteS32(s32 v)
	{
		return { TYPE_S32, u32(v) };
	}
	DATO_FORCEINLINE ValueRef WriteU32(u32 v)
	{
		return { TYPE_U32, v };
	}
	DATO_FORCEINLINE ValueRef WriteF32(f32 v)
	{
		return { TYPE_F32, BitCast<u32>(v) };
	}
	DATO_FORCEINLINE ValueRef WriteS64(s64 v)
	{
		return { TYPE_S64, AddValue8(&v) };
	}
	DATO_FORCEINLINE ValueRef WriteU64(u64 v)
	{
		return { TYPE_U64, AddValue8(&v) };
	}
	DATO_FORCEINLINE ValueRef WriteF64(f64 v)
	{
		return { TYPE_F64, AddValue8(&v) };
	}

	ValueRef WriteVectorRaw(const void* data, u8 subtype, u8 sizeAlign, u16 elemCount)
	{
		DATO_INPUT_EXPECT(elemCount >= 1 && elemCount <= 255);
		if (_flags & FLAG_Aligned)
			AddZeroesUntil(RoundUp(GetSize() + 2, sizeAlign) - 2);
		u32 pos = GetSize();
		AddByte(subtype);
		AddByte(u8(elemCount));
		AddMem(data, sizeAlign * elemCount);
		return { TYPE_Vector, pos };
	}
	template <class T>
	DATO_FORCEINLINE ValueRef WriteVectorT(const T* values, u16 elemCount)
	{
		return WriteVectorRaw(values, SubtypeInfo<T>::Subtype, sizeof(T), elemCount);
	}
};

struct TempMem
{
	void* _data = nullptr;
	u32 _size = 0;

	~TempMem()
	{
		DATO_FREE(_data);
	}
	void* GetDataBytes(u32 atLeast)
	{
		if (_size < atLeast)
		{
			_size += atLeast;
			DATO_FREE(_data);
			_data = DATO_MALLOC(_size);
		}
		return _data;
	}
	template <class T>
	DATO_FORCEINLINE T* GetData(u32 atLeast)
	{
		return (T*) GetDataBytes(atLeast * sizeof(T));
	}
	template <class T>
	DATO_FORCEINLINE T* CopyData(const T* from, u32 count)
	{
		T* data = GetData<T>(count);
		memcpy(data, from, sizeof(T) * count);
		return data;
	}
};

#ifndef DATO_USE_STD_SORT
template <class T> DATO_FORCEINLINE void PODSwap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

inline void SortEntriesByKeyInt_Insertion(TempMem&, IntMapEntry* entries, u32 count)
{
	// insertion sort
	if (count < 2)
		return;
	auto* end = entries + count;
	for (auto* i = entries + 1; i != end; i++)
	{
		auto cur = *i;
		auto* j = i;
		while (j != entries && cur.key < j[-1].key)
		{
			auto* oldj = j--;
			*oldj = *j;
		}
		*j = cur;
	}
}

inline void SortEntriesByKeyInt_Radix(TempMem& tempMem, IntMapEntry* entries, u32 count)
{
	// radix sort
	IntMapEntry* from = entries;
	IntMapEntry* to = tempMem.GetData<IntMapEntry>(count);
	for (int part = 0; part < 4; part++)
	{
		u32 shift = part * 8;

		// count the number of elements in each bucket
		u32 numElements[256] = {};
		for (u32 i = 0; i < count; i++)
			numElements[(from[i].key >> shift) & 0xff]++;

		// allocate ranges for each bucket
		u32 offsets[256];
		u32 o = 0;
		for (u32 i = 0; i < 256; i++)
		{
			offsets[i] = o;
			o += numElements[i];
		}

		// copy elements according to their assigned offsets
		for (u32 i = 0; i < count; i++)
			to[offsets[(from[i].key >> shift) & 0xff]++] = from[i];

		PODSwap(from, to);
	}
}

DATO_FORCEINLINE void SortEntriesByKeyInt(TempMem& tempMem, IntMapEntry* entries, u32 count)
{
	if (count <= 58)
		SortEntriesByKeyInt_Insertion(tempMem, entries, count);
	else
		SortEntriesByKeyInt_Radix(tempMem, entries, count);
}

inline int Q3SS_CharAt(const char* mem, const StringMapEntry& e, u32 at)
{
	if (at < e.key.dataLen)
		return u8(mem[e.key.dataPos + at]);
	return -1;
}

inline bool SIN_LessThan(const char* mem, const KeyRef& a, const KeyRef& b, u32 which)
{
	u32 minLen = a.dataLen < b.dataLen ? a.dataLen : b.dataLen;
	int diff = memcmp(mem + a.dataPos + which, mem + b.dataPos + which, minLen - which);
	if (diff)
		return diff < 0;
	return a.dataLen < b.dataLen;
}

inline void InsertionStringSort(const char* mem, StringMapEntry* entries, int low, int high, u32 which)
{
	StringMapEntry* start = entries + low;
	StringMapEntry* end = entries + high + 1;
	for (auto* i = start + 1; i != end; i++)
	{
		auto cur = *i;
		auto* j = i;
		while (j != start && SIN_LessThan(mem, cur.key, j[-1].key, which))
		{
			auto* oldj = j--;
			*oldj = *j;
		}
		*j = cur;
	}
}

// three-way string quicksort
inline void Quick3StringSort(
	const char* mem, StringMapEntry* entries, int low, int high, u32 which, int IST = 64)
{
	if (high <= low)
		return;

	if (high - low < IST)
	{
		InsertionStringSort(mem, entries, low, high, which);
		return;
	}

	int sublow = low;
	int subhigh = high;
	int ch = Q3SS_CharAt(mem, entries[low], which);
	for (int i = low + 1; i <= subhigh; )
	{
		int nch = Q3SS_CharAt(mem, entries[i], which);
		if (nch < ch)
			PODSwap(entries[sublow++], entries[i++]);
		else if (nch > ch)
			PODSwap(entries[i], entries[subhigh--]);
		else
			i++;
	}
	Quick3StringSort(mem, entries, low, sublow - 1, which);
	if (ch >= 0)
		Quick3StringSort(mem, entries, sublow, subhigh, which + 1);
	Quick3StringSort(mem, entries, subhigh + 1, high, which);
}

inline void SortEntriesByKeyString(const char* mem, StringMapEntry* entries, u32 count)
{
	Quick3StringSort(mem, entries, 0, int(count - 1), 0);
}
#endif // DATO_USE_STD_SORT

struct DATO_CONCAT(Writer, DATO_CONFIG) : WriterBase
{
	using Config = DATO_CONCAT(WriterConfig, DATO_CONFIG);
	TempMem _sortableEntries;
	TempMem _sortCopyEntries;
	bool _skipDuplicateKeys;

	DATO_FORCEINLINE DATO_CONCAT(Writer, DATO_CONFIG)
	(
		const char* prefix = "DATO",
		u32 pfxsize = 4,
		u8 flags = FLAG_Aligned | FLAG_SortedKeys,
		bool skipDuplicateKeys = true
	)
		: WriterBase(prefix, pfxsize, Config::Identifier(), flags)
		, _skipDuplicateKeys(skipDuplicateKeys)
	{}

	KeyRef WriteStringKey(const char* str, u32 size)
	{
		if (_skipDuplicateKeys)
		{
			if (auto* e = _keyTable.Find(str, size))
				return { e->valuePos, e->dataOff, e->len };
		}

		u32 pos = Config::WriteKeyLength(*this, size, 0, nullptr, 0);
		u32 dataPos = GetSize();
		AddMem(str, size);
		AddByte(0);

		if (_skipDuplicateKeys)
		{
			_keyTable.Insert(pos, dataPos, size);
		}

		return { pos, dataPos, size };
	}
	DATO_FORCEINLINE KeyRef WriteStringKey(const char* str) { return WriteStringKey(str, StrLen(str)); }

	ValueRef WriteStringMap(const StringMapEntry* entries, u32 count)
	{
		if (_flags & FLAG_SortedKeys)
		{
			StringMapEntry* sea = _sortableEntries.CopyData(entries, count);
			_SortStringMapEntries(sea, count);
			entries = sea;
		}
		return _WriteStringMapImpl(entries, count);
	}

	ValueRef WriteStringMapInlineSort(StringMapEntry* entries, u32 count)
	{
		if (_flags & FLAG_SortedKeys)
		{
			_SortStringMapEntries(entries, count);
		}
		return _WriteStringMapImpl(entries, count);
	}

	void _SortStringMapEntries(StringMapEntry* entries, u32 count)
	{
#ifdef DATO_USE_STD_SORT
		std::sort(
			entries,
			entries + count,
			[this](const StringMapEntry& a, const StringMapEntry& b)
		{
			u32 minSize = a.key.dataLen < b.key.dataLen ? a.key.dataLen : b.key.dataLen;
			const char* ka = _data + a.key.dataPos;
			const char* kb = _data + b.key.dataPos;
			if (int diff = memcmp(ka, kb, minSize))
				return diff < 0;
			return a.key.dataLen < b.key.dataLen;
		});
#else
		SortEntriesByKeyString(_data, entries, count);
#endif
	}

	ValueRef _WriteStringMapImpl(const StringMapEntry* entries, u32 count)
	{
		u32 pos = Config::WriteMapSize(*this, count, Align(4), nullptr, 0);
		u32 basepos = GetSize();
		for (u32 i = 0; i < count; i++)
			AddU32(entries[i].key.pos);
		_WriteMapValuesAndTypes(entries, count, basepos);
		return { TYPE_StringMap, pos };
	}

	ValueRef WriteIntMap(const IntMapEntry* entries, u32 count)
	{
		if (_flags & FLAG_SortedKeys)
		{
			IntMapEntry* sea = _sortableEntries.CopyData(entries, count);
			_SortIntMapEntries(sea, count);
			entries = sea;
		}
		return _WriteIntMapImpl(entries, count);
	}

	ValueRef WriteIntMapInlineSort(IntMapEntry* entries, u32 count)
	{
		if (_flags & FLAG_SortedKeys)
		{
			_SortIntMapEntries(entries, count);
		}
		return _WriteIntMapImpl(entries, count);
	}

	void _SortIntMapEntries(IntMapEntry* entries, u32 count)
	{
#ifdef DATO_USE_STD_SORT
		std::sort(
			entries,
			entries + count,
			[](const IntMapEntry& a, const IntMapEntry& b)
		{
			return a.key.pos < b.key.pos;
		});
#else
		SortEntriesByKeyInt(_sortCopyEntries, entries, count);
#endif
	}

	ValueRef _WriteIntMapImpl(const IntMapEntry* entries, u32 count)
	{
		u32 pos = Config::WriteMapSize(*this, count, Align(4), nullptr, 0);
		u32 basepos = GetSize();
		for (u32 i = 0; i < count; i++)
			AddU32(entries[i].key);
		_WriteMapValuesAndTypes(entries, count, basepos);
		return { TYPE_IntMap, pos };
	}

	template <class EntryT> void _WriteMapValuesAndTypes(const EntryT* entries, u32 count, u32 basepos)
	{
		for (u32 i = 0; i < count; i++)
		{
			u32 vp = entries[i].value.pos;
			if (IsReferenceType(entries[i].value.type))
				vp = basepos - vp;
			AddU32(vp);
		}

		for (u32 i = 0; i < count; i++)
			AddByte(entries[i].value.type);
	}

	ValueRef WriteArray(const ValueRef* values, u32 count)
	{
		u32 pos = Config::WriteArrayLength(*this, count, Align(4), nullptr, 0);
		u32 basepos = GetSize();

		for (u32 i = 0; i < count; i++)
		{
			u32 vp = values[i].pos;
			if (IsReferenceType(values[i].type))
				vp = basepos - vp;
			AddU32(vp);
		}

		for (u32 i = 0; i < count; i++)
			AddByte(values[i].type);
		return { TYPE_Array, pos };
	}

	ValueRef WriteString8(const char* str, u32 size)
	{
		u32 pos = Config::WriteValueLength(*this, size, 0, nullptr, 0);
		AddMem(str, size);
		AddByte(0);
		return { TYPE_String8, pos };
	}
	DATO_FORCEINLINE ValueRef WriteString8(const char* str) { return WriteString8(str, StrLen(str)); }

	ValueRef WriteString16(const u16* str, u32 size)
	{
		u32 pos = Config::WriteValueLength(*this, size, Align(2), nullptr, 0);
		AddMem(str, size * sizeof(*str));
		AddZeroes(2);
		return { TYPE_String16, pos };
	}
	DATO_FORCEINLINE ValueRef WriteString16(const u16* str) { return WriteString16(str, StrLen(str)); }
	DATO_FORCEINLINE ValueRef WriteString16(const char16_t* str, u32 size)
	{
		static_assert(sizeof(u16) == sizeof(char16_t), "unexpected type size difference");
		return WriteString16((const u16*) str, size);
	}
	DATO_FORCEINLINE ValueRef WriteString16(const char16_t* str)
	{ return WriteString16(str, StrLen(str)); }

	ValueRef WriteString32(const u32* str, u32 size)
	{
		u32 pos = Config::WriteValueLength(*this, size, Align(4), nullptr, 0);
		AddMem(str, size * sizeof(*str));
		AddZeroes(4);
		return { TYPE_String32, pos };
	}
	DATO_FORCEINLINE ValueRef WriteString32(const u32* str) { return WriteString32(str, StrLen(str)); }
	DATO_FORCEINLINE ValueRef WriteString32(const char32_t* str, u32 size)
	{
		static_assert(sizeof(u32) == sizeof(char32_t), "unexpected type size difference");
		return WriteString32((const u32*) str, size);
	}
	DATO_FORCEINLINE ValueRef WriteString32(const char32_t* str)
	{ return WriteString32(str, StrLen(str)); }

	ValueRef WriteByteArray(const void* data, u32 size, u32 align = 0)
	{
		u32 pos = Config::WriteValueLength(*this, size, align, nullptr, 0);
		AddMem(data, size);
		return { TYPE_ByteArray, pos };
	}

	ValueRef WriteVectorArrayRaw(const void* data, u8 subtype, u8 sizeAlign, u16 elemCount, u32 length)
	{
		DATO_INPUT_EXPECT(elemCount >= 1 && elemCount <= 255);
		u8 prefix[] = { subtype, u8(elemCount) };
		u32 pos = Config::WriteValueLength(
			*this,
			length,
			Align(length ? sizeAlign : 1),
			prefix,
			sizeof(prefix));
		AddMem(data, sizeAlign * elemCount * length);
		return { TYPE_VectorArray, pos };
	}
	template <class T>
	DATO_FORCEINLINE ValueRef WriteVectorArrayT(const T* values, u16 elemCount, u32 length)
	{
		return WriteVectorArrayRaw(values, SubtypeInfo<T>::Subtype, sizeof(T), elemCount, length);
	}
};
using Writer = DATO_CONCAT(Writer, DATO_CONFIG);

} // dato
