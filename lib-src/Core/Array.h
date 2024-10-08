
#pragma once

#include "Platform.h"
#include "Memory.h" // ArrayView
#include "ObjectIterationCore.h"

#include <assert.h>
#include <stdlib.h>
#include <initializer_list>


namespace ui {

template <class T>
struct Array
{
	using ValueType = T;

	T* _data = nullptr;
	size_t _size = 0;
	size_t _capacity = 0;

	// whole object ops
	UI_FORCEINLINE Array() {}
	UI_FORCEINLINE Array(size_t capacity)
	{
		if (capacity)
			Reserve(capacity);
	}

	Array(const Array& o) { AppendRange(o); }
	Array(const std::initializer_list<T>& il) { AppendRange(il); }
	template <class R, typename HasBegin = decltype(std::declval<R>().begin()), typename HasEnd = decltype(std::declval<R>().end())>
	Array(const R& r) { AppendRange(r); }
	Array& operator = (const Array& o)
	{
		AssignRange(o);
		return *this;
	}
	Array& operator = (const std::initializer_list<T>& il)
	{
		AssignRange(il);
		return *this;
	}
	template <class R, typename HasBegin = decltype(std::declval<R>().begin()), typename HasEnd = decltype(std::declval<R>().end())>
	Array& operator = (const R& r)
	{
		AssignRange(r);
		return *this;
	}

	Array(Array&& o) : _data(o._data), _size(o._size), _capacity(o._capacity)
	{
		o._data = nullptr;
		o._size = 0;
		o._capacity = 0;
	}
	Array& operator = (Array&& o)
	{
		Clear();
		free(_data);
		_data = o._data;
		o._data = nullptr;
		_size = o._size;
		o._size = 0;
		_capacity = o._capacity;
		o._capacity = 0;
		return *this;
	}

	UI_FORCEINLINE ~Array()
	{
		Clear();
		if (_data) // optimization
			free(_data);
	}

	void Clear()
	{
		for (size_t i = 0; i < _size; i++)
			_data[i].~T();
		_size = 0;
	}

	void AssignMany(const T* p, size_t n)
	{
		Clear();
		AppendMany(p, n);
	}
	void AssignMany(const T* from, const T* to)
	{
		Clear();
		AppendMany(from, to);
	}
	template <class R>
	void AssignRange(const R& r)
	{
		Clear();
		AppendRange(r);
	}

	// info
	UI_FORCEINLINE size_t size() const { return _size; }
	UI_FORCEINLINE T* data() { return _data; }
	UI_FORCEINLINE T* begin() { return _data; }
	UI_FORCEINLINE T* end() { return _data + _size; }
	UI_FORCEINLINE const T* data() const { return _data; }
	UI_FORCEINLINE const T* begin() const { return _data; }
	UI_FORCEINLINE const T* end() const { return _data + _size; }

	UI_FORCEINLINE bool IsEmpty() const { return _size == 0; }
	UI_FORCEINLINE bool NotEmpty() const { return _size != 0; }
	UI_FORCEINLINE T* Data() { return _data; }
	UI_FORCEINLINE const T* Data() const { return _data; }
	UI_FORCEINLINE size_t Size() const { return _size; }
	UI_FORCEINLINE size_t SizeInBytes() const { return _size * sizeof(T); }
	UI_FORCEINLINE size_t Capacity() const { return _capacity; }
	UI_FORCEINLINE ArrayView<T> View() const { return { _data, _size }; }

	T& At(size_t i)
	{
		assert(i < _size);
		return _data[i];
	}
	UI_FORCEINLINE const T& At(size_t i) const { return const_cast<Array*>(this)->operator[](i); }

	T& operator [] (size_t i)
	{
		assert(i < _size);
		return _data[i];
	}
	UI_FORCEINLINE const T& operator [] (size_t i) const { return const_cast<Array*>(this)->operator[](i); }

	UI_FORCEINLINE size_t GetElementIndex(const T& valFromThisArray) const
	{
		return &valFromThisArray - _data;
	}

	T& First() { assert(NotEmpty()); return *_data; }
	const T& First() const { assert(NotEmpty()); return *_data; }
	T& Last() { assert(NotEmpty()); return _data[_size - 1]; }
	const T& Last() const { assert(NotEmpty()); return _data[_size - 1]; }

	template <class T>
	bool operator == (const Array<T>& b) const
	{
		const Array<T>& a = *this;
		if (a.Size() != b.Size())
			return false;
		for (size_t i = 0; i < a.Size(); i++)
			if (!(a[i] == b[i]))
				return false;
		return true;
	}

	template <class T>
	UI_FORCEINLINE bool operator != (const Array<T>& o) const
	{
		return !(*this == o);
	}

	template <class T2>
	bool ContainsT(const T2& v, size_t from = 0) const
	{
		for (size_t i = from; i < _size; i++)
			if (_data[i] == v)
				return true;
		return false;
	}
	template <class T2>
	size_t IndexOfT(const T2& v, size_t from = 0) const
	{
		for (size_t i = from; i < _size; i++)
			if (_data[i] == v)
				return i;
		return SIZE_MAX;
	}
	template <class T2>
	size_t LastIndexOfT(const T2& v) const
	{
		for (size_t i = _size; i > 0; )
		{
			i--;
			if (_data[i] == v)
				return i;
		}
		return SIZE_MAX;
	}

	UI_FORCEINLINE bool Contains(const T& what, size_t from = 0) const { return ContainsT(what, from); }
	UI_FORCEINLINE size_t IndexOf(const T& what, size_t from = 0) const { return IndexOfT(what, from); }
	UI_FORCEINLINE size_t LastIndexOf(const T& what) const { return LastIndexOfT(what); }

	// simple modification
	void _Realloc(size_t newCap)
	{
		auto* newData = (T*)malloc(sizeof(T) * newCap);
		size_t numToCopy = _size < newCap ? _size : newCap;

		UI_IF_MAYBE_CONSTEXPR(std::is_trivially_copyable<T>::value)
		{
			memcpy(newData, _data, numToCopy * sizeof(T));
		}
		else
		{
			for (size_t i = 0; i < numToCopy; i++)
			{
				new (&newData[i]) T(Move(_data[i]));
				_data[i].~T();
			}
		}

		UI_IF_MAYBE_CONSTEXPR(!std::is_trivially_destructible<T>::value)
		{
			for (size_t i = numToCopy; i < _size; i++)
				_data[i].~T();
		}

		free(_data);
		_data = newData;
		_capacity = newCap;
	}

	UI_FORCEINLINE void Reserve(size_t newSize)
	{
		if (newSize > _capacity)
			_Realloc(newSize);
	}
	UI_FORCEINLINE void ReserveForAppend(size_t newSize)
	{
		if (newSize > _capacity)
			_Realloc(newSize + _capacity);
	}
	void Resize(size_t newSize)
	{
		Reserve(newSize);

		UI_IF_MAYBE_CONSTEXPR(std::is_trivially_constructible<T>::value)
		{
			if (_size < newSize)
				_size = newSize;
		}
		else
		{
			while (_size < newSize)
				new (&_data[_size++]) T();
		}

		UI_IF_MAYBE_CONSTEXPR(std::is_trivially_destructible<T>::value)
		{
			if (_size > newSize)
				_size = newSize;
		}
		else
		{
			while (_size > newSize)
				_data[--_size].~T();
		}
	}
	void ResizeWith(size_t newSize, const T& v)
	{
		Reserve(newSize);

		while (_size < newSize)
			new (&_data[_size++]) T(v);

		UI_IF_MAYBE_CONSTEXPR(std::is_trivially_destructible<T>::value)
		{
			if (_size > newSize)
				_size = newSize;
		}
		else
		{
			while (_size > newSize)
				_data[--_size].~T();
		}
	}
	// ignores the constructor
	void ResizeWithZeroes(size_t newSize)
	{
		Reserve(newSize);

		if (_size < newSize)
		{
			memset(&_data[_size], 0, (newSize - _size) * sizeof(T));
		}
		_size = newSize;
	}

	inline void Append(const T& v)
	{
		ReserveForAppend(_size + 1);
		new (&_data[_size++]) T(v);
	}
	inline void Append(T&& v)
	{
		ReserveForAppend(_size + 1);
		new (&_data[_size++]) T(Move(v));
	}
	void AppendMany(const T* p, size_t n)
	{
		size_t newSize = _size + n;
		ReserveForAppend(newSize);
		UI_IF_MAYBE_CONSTEXPR(std::is_trivially_copy_constructible<T>::value)
		{
			memcpy(&_data[_size], p, sizeof(T) * n);
		}
		else
		{
			for (size_t i = 0; i < n; i++)
				new (&_data[_size + i]) T(p[i]);
		}
		_size = newSize;
	}
	UI_FORCEINLINE void AppendMany(const T* bp, const T* ep)
	{
		AppendMany(bp, ep - bp);
	}
	template <class R>
	UI_FORCEINLINE void AppendRange(const R& r)
	{
		AppendMany(r.begin(), r.end() - r.begin());
	}

	inline void RemoveLast()
	{
		assert(_size);
		_data[--_size].~T();
	}
	void RemoveLast(size_t n)
	{
		assert(_size >= n);
		UI_IF_MAYBE_CONSTEXPR(std::is_trivially_destructible<T>::value)
		{
			_size -= n;
		}
		else
		{
			for (size_t i = 0; i < n; i++)
				_data[--_size].~T();
		}
	}

	// complex modification
	inline void _HardMove(size_t to, size_t from)
	{
		new (&_data[to]) T(Move(_data[from]));
		_data[from].~T();
	}
	inline bool UnorderedRemoveAt(size_t at)
	{
		assert(at < _size);
		_data[at].~T();
		--_size;
		if (at < _size)
			_HardMove(at, _size);
		return at < _size;
	}
	void RemoveAt(size_t at, size_t n = 1)
	{
		assert(at < _size);
		assert(n <= _size);
		assert(at + n >= at);
		assert(at + n <= _size); // may overflow

		for (size_t i = 0; i < n; i++)
			_data[at + i].~T();

		_size -= n;
		if (at < _size)
		{
			for (size_t p = at; p < _size; p++)
				_HardMove(p, p + n);
		}
	}
	void InsertAt(size_t at, const T& v)
	{
		assert(at <= _size);
		ReserveForAppend(_size + 1);
		if (at < _size)
		{
			for (size_t p = _size; p > at; )
			{
				p--;
				_HardMove(p + 1, p);
			}
		}
		new (&_data[at]) T(v);
		_size++;
	}
	UI_FORCEINLINE void Prepend(const T& v)
	{
		InsertAt(0, v);
	}
	void InsertManyAt(size_t at, const T* p, size_t n)
	{
		assert(at <= _size);
		ReserveForAppend(_size + n);
		if (at < _size)
		{
			for (size_t p = _size; p > at; )
			{
				p--;
				_HardMove(p + n, p);
			}
		}
		for (size_t i = 0; i < n; i++)
			new (&_data[at + i]) T(p[i]);
		_size += n;
	}
	UI_FORCEINLINE void InsertManyAt(size_t at, const T* from, const T* end)
	{
		InsertManyAt(at, from, end - from);
	}
	template <class R>
	UI_FORCEINLINE void InsertRangeAt(size_t at, const R& r)
	{
		InsertManyAt(at, r.begin(), r.end() - r.begin());
	}
	UI_FORCEINLINE void InsertRangeAt(size_t at, const std::initializer_list<T>& il)
	{
		InsertManyAt(at, il.begin(), il.size());
	}
	void RemoveAllOf(const T& v)
	{
		size_t diff = 0;
		for (size_t i = 0; i < _size; i++)
		{
			if (_data[i] == v)
			{
				diff++;
				_data[i].~T();
				continue;
			}
			if (diff)
				_HardMove(i - diff, i);
		}
		_size -= diff;
	}

	// combined shorthands
	bool RemoveFirstOf(const T& v)
	{
		size_t i = IndexOf(v);
		if (i != SIZE_MAX)
			RemoveAt(i);
		return i != SIZE_MAX;
	}
	bool UnorderedRemoveFirstOf(const T& v)
	{
		size_t i = IndexOf(v);
		if (i != SIZE_MAX)
			UnorderedRemoveAt(i);
		return i != SIZE_MAX;
	}

	template <class Func>
	void OnSerializeCustom(IObjectIterator& oi, const FieldInfo& FI, Func&& func)
	{
		size_t estNewSize = oi.BeginArray(_size, FI);
		FieldInfo chfi("", FI.flags);
		if (oi.IsUnserializer())
		{
			if (FI.flags & FieldInfo::Preallocated)
			{
				size_t i = 0;
				while (oi.HasMoreArrayElements())
					func(oi, chfi, (*this)[i++]);
			}
			else
			{
				Clear();
				Reserve(estNewSize);
				while (oi.HasMoreArrayElements())
				{
					Append(T());
					func(oi, chfi, Last());
				}
			}
		}
		else
		{
			for (auto& item : *this)
				func(oi, chfi, item);
		}
		oi.EndArray();
	}

	UI_FORCEINLINE void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		// TODO force-inline the lambda
		OnSerializeCustom(oi, FI, [](IObjectIterator& oi, const FieldInfo& FI, T& val) { OnField(oi, FI, val); });
	}
};

} // ui
