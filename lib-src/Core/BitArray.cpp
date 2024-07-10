
#include "BitArray.h"


namespace ui {

BitArray::BitArray(const BitArray& o)
{
	size_t newSize = o.Size();
	if (newSize <= MAX_INLINE_BITS)
	{
		_sized = o._sized & ~size_t(1);
		auto* src = o.Data();
		_inld[0] = src[0];
		_inld[1] = src[1];
	}
	else
	{
		_sized = o._sized | 1;
		size_t nwords = _DivUpPO2(newSize, WORD_BITS);
		_capacity = nwords;
		_data = new Word[nwords];
		memcpy(_data, o._data, WORD_BYTES * nwords);
	}
}

void BitArray::Insert(size_t at, bool v)
{
	size_t sz = Size();
	assert(at <= sz);
	Reserve(sz + 1);
	_MoveBitsUp(at + 1, at, sz - at);
	SetUnchecked(at, v);
	_sized += 2;
}

void BitArray::InsertN(size_t at, bool v, size_t n)
{
	size_t sz = Size();
	assert(at <= sz);
	Resize(sz + n);
	_MoveBitsUp(at + n, at, sz - at);
	SetRange(at, at + n, v);
}

void BitArray::_MoveBitsUp(size_t dst, size_t src, size_t n)
{
	for (size_t i = n; i > 0; )
	{
		i--;
		bool v = GetUnchecked(src + i);
		SetUnchecked(dst + i, v);
	}
}

void BitArray::RemoveAt(size_t at)
{
	size_t sz = Size();
	assert(at < sz);
	_MoveBitsDown(at, at + 1, sz - (at + 1));
	_sized -= 2;
}

void BitArray::RemoveAtN(size_t at, size_t n)
{
	size_t sz = Size();
	assert(at <= sz);
	assert(at + n <= sz);
	_MoveBitsDown(at, at + n, sz - (at + n));
	_sized -= n << 1;
}

void BitArray::RemoveRange(size_t from, size_t to)
{
	size_t sz = Size();
	assert(from <= sz);
	assert(to <= sz);
	_MoveBitsDown(from, to, sz - to);
	_sized -= (to - from) << 1;
}

void BitArray::_MoveBitsDown(size_t dst, size_t src, size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		bool v = GetUnchecked(src + i);
		SetUnchecked(dst + i, v);
	}
}


#if UI_BUILD_TESTS
#include "Test.h"

DEFINE_TEST_CATEGORY(BitArray, 300);

static size_t SensSizes[] = { 0, 1, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129 };

static void CheckCapacity(BitArray& a, size_t sz)
{
	ASSERT_EQUAL(true, BitArray::_DivUpPO2(sz, BitArray::WORD_BITS) <= a.CapacityInWords());
	ASSERT_EQUAL(true, BitArray::_DivUpPO2(sz, 8) <= a.CapacityInBytes());
	ASSERT_EQUAL(true, sz <= a.CapacityInBits());
}

static void CheckSize(BitArray& a, size_t sz)
{
	ASSERT_EQUAL(sz, a.Size());
	ASSERT_EQUAL(sz > BitArray::MAX_INLINE_BITS, (a._sized & 1) != 0);
	CheckCapacity(a, sz);
}

DEFINE_TEST(BitArray, Size)
{
	printf("- ctor(WithCapacity{N}):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a(WithCapacity{ S });
		CheckCapacity(a, S);
	}
	puts("");
	printf("- Reserve(N):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a;
		a.Reserve(S);
		ASSERT_EQUAL(0, a.Size());
		CheckCapacity(a, S);
	}
	puts("");
	printf("- ReserveForAppend(N):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a;
		a.ReserveForAppend(S);
		ASSERT_EQUAL(0, a.Size());
		CheckCapacity(a, S);
	}
	puts("");
	printf("- ctor(WithSize{N}):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a(WithSize{ S });
		CheckSize(a, S);
	}
	puts("");
	printf("- ctor(N, bool):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a(S, false);
		CheckSize(a, S);
	}
	puts("");
	printf("- Resize(N):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a;
		a.Resize(S);
		CheckSize(a, S);
	}
	puts("");
	printf("- AssignFill(N, bool):");
	for (size_t S : SensSizes)
	{
		printf("%zu,", S);
		BitArray a;
		a.AssignFill(S, false);
		CheckSize(a, S);
	}
	puts("");
}

DEFINE_TEST(BitArray, Append)
{
	BitArray a;
	for (size_t i = 0; i <= 129; i++)
	{
		a.Append(i % 2);
	}
	for (size_t i = 0; i <= 129; i++)
	{
		ASSERT_EQUAL((i % 2) != 0, a.Get(i));
	}
}

static void CheckSeq(BitArray& a, const char* vals)
{
	size_t n = strlen(vals);
	ASSERT_EQUAL(n, a.Size());
	for (size_t i = 0; i < n; i++)
		ASSERT_EQUAL(vals[i] == '1', a.Get(i));
}

DEFINE_TEST(BitArray, GetSetFill)
{
	BitArray a(1, false);
	CheckSeq(a, "0");
	a.AssignFill(2, true);
	CheckSeq(a, "11");
	a.Set0(0);
	CheckSeq(a, "01");
	a.SetInverse(0);
	CheckSeq(a, "11");
	a.SetInverse(1);
	CheckSeq(a, "10");
	a.Set1(1);
	CheckSeq(a, "11");
	a.Set(1, false);
	CheckSeq(a, "10");
	a.Set(1, true);
	CheckSeq(a, "11");
	a.Fill(false);
	CheckSeq(a, "00");
}

DEFINE_TEST(BitArray, InsertRemove)
{
	BitArray a;
	a.Insert(0, true);
	CheckSeq(a, "1");
	a.Insert(0, false);
	CheckSeq(a, "01");
	a.Insert(2, true);
	CheckSeq(a, "011");
	a.Insert(2, false);
	CheckSeq(a, "0101");
	a.Insert(1, false);
	CheckSeq(a, "00101");
	a.Insert(1, true);
	CheckSeq(a, "010101");
	a.InsertN(6, false, 2);
	CheckSeq(a, "01010100");
	a.InsertN(8, true, 2);
	CheckSeq(a, "0101010011");
	a.InsertN(0, true, 2);
	CheckSeq(a, "110101010011");
	a.InsertN(0, false, 2);
	CheckSeq(a, "00110101010011");
	a.RemoveAt(13);
	CheckSeq(a, "0011010101001");
	a.RemoveAt(10);
	CheckSeq(a, "001101010101");
	a.RemoveAt(0);
	CheckSeq(a, "01101010101");
	a.RemoveAt(3);
	CheckSeq(a, "0111010101");
	a.RemoveAtN(1, 3);
	CheckSeq(a, "0010101");
	a.RemoveAtN(5, 2);
	CheckSeq(a, "00101");
	a.RemoveRange(0, 2);
	CheckSeq(a, "101");
	a.RemoveRange(3, 3);
	CheckSeq(a, "101");
	a.RemoveRange(2, 3);
	CheckSeq(a, "10");
	a.Append(true);
	a.Append(false);
	a.Append(true);
	CheckSeq(a, "10101");
	a.UnorderedRemoveAt(1);
	CheckSeq(a, "1110");
	a.UnorderedRemoveAt(0);
	CheckSeq(a, "011");
	a.UnorderedRemoveAt(2);
	CheckSeq(a, "01");
	a.Resize(1);
	CheckSeq(a, "0");
}
#endif

} // ui
