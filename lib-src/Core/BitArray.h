
#pragma once

#include "Common.h"

#include <string.h>


namespace ui {

struct BitArray
{
	using Word = uintptr_t;
	static const constexpr size_t WORD_BYTES = sizeof(Word);
	static const constexpr size_t WORD_BITS = WORD_BYTES * 8;

	static const constexpr size_t MASK_WORD_BIT = WORD_BITS - 1;
	static const constexpr size_t MASK_WORD_NUM = ~MASK_WORD_BIT;
	static const constexpr size_t SHIFT_WORD_NUM = sizeof(Word) == 4 ? 5 : 6;

	static const constexpr size_t MAX_INLINE_WORDS = 2;
	static const constexpr size_t MAX_INLINE_BITS = MAX_INLINE_WORDS * WORD_BITS;

	// 1st bit: mode selector (0=short, 1=long)
	// size is stored starting from the 2nd bit
	size_t _sized;
	union
	{
		struct
		{
			size_t _capacity; // in words
			Word* _data;
		};
		Word _inld[MAX_INLINE_WORDS];
	};

	static constexpr UI_FORCEINLINE size_t _DivUpPO2(size_t x, size_t dpo2)
	{
		return (x + (dpo2 - 1)) / dpo2;
	}

	UI_FORCEINLINE size_t CapacityInWords() const { return _sized & 1 ? _capacity : MAX_INLINE_WORDS; }
	UI_FORCEINLINE size_t CapacityInBytes() const { return _sized & 1 ? _capacity * WORD_BYTES : MAX_INLINE_WORDS * WORD_BYTES; }
	UI_FORCEINLINE size_t CapacityInBits() const { return _sized & 1 ? _capacity * WORD_BITS : MAX_INLINE_BITS; }
	UI_FORCEINLINE size_t Size() const { return _sized >> 1; }
	UI_FORCEINLINE size_t SizeInBytes() const { return _DivUpPO2(Size(), 8); }
	UI_FORCEINLINE size_t SizeInWords() const { return _DivUpPO2(Size(), WORD_BITS); }
	UI_FORCEINLINE uintptr_t* Data() { return _sized & 1 ? _data : _inld; }
	UI_FORCEINLINE const uintptr_t* Data() const { return _sized & 1 ? _data : _inld; }

	UI_FORCEINLINE ~BitArray()
	{
		Dealloc();
	}
	UI_FORCEINLINE void Clear()
	{
		_sized &= 1;
	}
	UI_FORCEINLINE void Dealloc()
	{
		if (_sized & 1)
		{
			delete[] _data;
			_sized = 0;
		}
	}

	UI_FORCEINLINE BitArray() : _sized(0) {}
	inline BitArray(WithCapacity wc) : _sized(0)
	{
		Reserve(wc.n);
	}
	inline BitArray(WithSize ws) : _sized(ws.n << 1)
	{
		if (ws.n > MAX_INLINE_BITS)
		{
			_sized |= 1;
			size_t cap = _DivUpPO2(ws.n, WORD_BITS);
			_capacity = cap;
			_data = new Word[cap];
		}
	}
	BitArray(size_t n, bool v) : BitArray(WithSize{ n })
	{
		size_t nwords = _DivUpPO2(n, WORD_BITS);
		memset(Data(), v ? -1 : 0, nwords * WORD_BYTES);
	}
	BitArray(const BitArray& o);
	inline BitArray(BitArray&& o) : _sized(o._sized)
	{
		if (_sized & 1) // TODO needed? in practice all compilers should be allowing union cross-member use (e.g. for std::string impls)
		{
			_capacity = o._capacity;
			_data = o._data;
		}
		else
		{
			_inld[0] = o._inld[0];
			_inld[1] = o._inld[1];
		}
		o._sized = 0;
	}
	BitArray& operator = (const BitArray& o)
	{
		Resize(o.Size());
		memcpy(Data(), o.Data(), o.SizeInWords() * WORD_BYTES);
		return *this;
	}
	BitArray& operator = (BitArray&& o)
	{
		Dealloc();
		_sized = o._sized;
		if (_sized & 1) // TODO needed? in practice all compilers should be allowing union cross-member use (e.g. for std::string impls)
		{
			_capacity = o._capacity;
			_data = o._data;
		}
		else
		{
			_inld[0] = o._inld[0];
			_inld[1] = o._inld[1];
		}
		o._sized = 0;
		return *this;
	}

	inline void Resize(size_t sz)
	{
		Reserve(sz);
		_sized = (_sized & 1) | (sz << 1);
	}
	inline void AssignFill(size_t sz, bool v)
	{
		Resize(sz);
		Fill(v);
	}
	inline void Fill(bool v)
	{
		size_t nwords = _DivUpPO2(Size(), WORD_BITS);
		memset(Data(), v ? -1 : 0, nwords * WORD_BYTES);
	}

	UI_FORCEINLINE bool Get(size_t at) const
	{
		assert(at < Size());
		return GetUnchecked(at);
	}
	inline bool GetUnchecked(size_t at) const
	{
		size_t word = (at & MASK_WORD_NUM) >> SHIFT_WORD_NUM;
		size_t bit = at & MASK_WORD_BIT;
		uintptr_t w = Data()[word];
		Word bm = Word(1) << bit;
		return (w & bm) != 0;
	}
	UI_FORCEINLINE void Set(size_t at, bool v)
	{
		assert(at < Size());
		SetUnchecked(at, v);
	}
	inline void SetUnchecked(size_t at, bool v)
	{
		size_t word = (at & MASK_WORD_NUM) >> SHIFT_WORD_NUM;
		size_t bit = at & MASK_WORD_BIT;
		uintptr_t& w = Data()[word];
		Word bm = Word(1) << bit;
		if (v)
			w |= bm;
		else
			w &= ~bm;
	}
	UI_FORCEINLINE void Set0(size_t at)
	{
		assert(at < Size());
		Set0Unchecked(at);
	}
	inline void Set0Unchecked(size_t at)
	{
		size_t word = (at & MASK_WORD_NUM) >> SHIFT_WORD_NUM;
		size_t bit = at & MASK_WORD_BIT;
		uintptr_t& w = Data()[word];
		Word bm = Word(1) << bit;
		w &= ~bm;
	}
	UI_FORCEINLINE void Set1(size_t at)
	{
		assert(at < Size());
		Set1Unchecked(at);
	}
	inline void Set1Unchecked(size_t at)
	{
		size_t word = (at & MASK_WORD_NUM) >> SHIFT_WORD_NUM;
		size_t bit = at & MASK_WORD_BIT;
		uintptr_t& w = Data()[word];
		Word bm = Word(1) << bit;
		w |= bm;
	}
	UI_FORCEINLINE void SetInverse(size_t at)
	{
		assert(at < Size());
		SetInverseUnchecked(at);
	}
	inline void SetInverseUnchecked(size_t at)
	{
		size_t word = (at & MASK_WORD_NUM) >> SHIFT_WORD_NUM;
		size_t bit = at & MASK_WORD_BIT;
		uintptr_t& w = Data()[word];
		Word bm = Word(1) << bit;
		w ^= bm;
	}
	void SetRange(size_t from, size_t to, bool v)
	{
		size_t sz = Size();
		assert(from <= sz);
		assert(to <= sz);
		// TODO optimize
		if (v)
		{
			for (size_t i = from; i < to; i++)
				Set1Unchecked(i);
		}
		else
		{
			for (size_t i = from; i < to; i++)
				Set0Unchecked(i);
		}
	}

	inline void Reserve(size_t sz)
	{
		size_t newCap = _DivUpPO2(sz, WORD_BITS);
		size_t cap = CapacityInWords();
		if (newCap <= cap)
			return;
		_ReallocBigger(newCap);
	}
	inline void ReserveForAppend(size_t numAdded)
	{
		size_t newCap = _DivUpPO2(Size() + numAdded, WORD_BITS);
		size_t cap = CapacityInWords();
		if (newCap <= cap)
			return;
		_ReallocBigger(newCap + cap); // size >= 2x cap and newCap
	}
	// only allowed with inputs exceeding MAX_INLINE_BITS
	void _ReallocBigger(size_t newCap)
	{
		Word* ndata = new Word[newCap];
		// copy whole words (easier to optimize)
		memcpy(ndata, Data(), _DivUpPO2(Size(), WORD_BITS) * WORD_BYTES);
		if (_sized & 1)
			delete[] _data;
		else
			_sized |= 1;
		_data = ndata;
		_capacity = newCap;
	}
	inline void Append(bool v)
	{
		// split size into word/bit
		size_t at = Size();
		size_t word = (at & MASK_WORD_NUM) >> SHIFT_WORD_NUM;
		size_t bit = at & MASK_WORD_BIT;
		// check if resizing is needed
		if (bit == 0) // current size is n*wordsize
		{
			size_t capw = CapacityInWords();
			if (word == capw)
				_ReallocBigger(capw << 1);
		}
		// write the value
		Word& w = Data()[word];
		size_t bm = size_t(1) << bit;
		if (v)
			w |= bm;
		else
			w &= ~bm;
		// increment bit-shifted size
		_sized += 2;
	}
	// replace n with last value if removed wasn't last
	inline void UnorderedRemoveAt(size_t n)
	{
		assert(n < Size());
		_sized -= 2;
		size_t nsz = Size();
		if (n == nsz)
			return;
		bool v = GetUnchecked(nsz);
		SetUnchecked(n, v);
	}

	// these are slow, avoid
	void Insert(size_t at, bool v);
	void InsertN(size_t at, bool v, size_t n);
	void _MoveBitsUp(size_t dst, size_t src, size_t n);
	void RemoveAt(size_t at);
	void RemoveAtN(size_t at, size_t n);
	void RemoveRange(size_t from, size_t to);
	void _MoveBitsDown(size_t dst, size_t src, size_t n);
};

} // ui
