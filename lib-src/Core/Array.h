
#pragma once

#include "Platform.h"

#include <assert.h>
#include <new>
#include <type_traits>


namespace ui {

template <class T>
struct Array
{
	using ValueType = T;

	T* _data = nullptr;
	size_t _size = 0;
	size_t _capacity = 0;

	// whole object ops
	UI_FORCEINLINE Array(size_t capacity = 0)
	{
		if (capacity)
			Reserve(capacity);
	}
	Array(const Array& o) { AppendRange(o); }
	template <class R, typename HasBegin = decltype(std::declval<R>().begin()), typename HasEnd = decltype(std::declval<R>().end())>
	Array(const R& r) { AppendRange(r); }
	Array& operator = (const Array& o)
	{
		Clear();
		AppendRange(o);
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
	UI_FORCEINLINE T* GetDataPtr() { return _data; }
	UI_FORCEINLINE const T* GetDataPtr() const { return _data; }
	UI_FORCEINLINE size_t Size() const { return _size; }
	UI_FORCEINLINE size_t Capacity() const { return _capacity; }

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

	UI_FORCEINLINE bool Contains(const T& v) const { return IndexOf(v) != SIZE_MAX; }
	size_t IndexOf(const T& v, size_t from = 0) const
	{
		for (size_t i = from; i < _size; i++)
			if (_data[i] == v)
				return i;
		return SIZE_MAX;
	}
	size_t LastIndexOf(const T& v) const
	{
		for (size_t i = _size; i > 0; )
		{
			i--;
			if (_data[i] == v)
				return i;
		}
		return SIZE_MAX;
	}

	// simple modification
	void _Realloc(size_t newCap)
	{
		auto* newData = (T*)malloc(sizeof(T) * newCap);
		size_t numToCopy = _size < newCap ? _size : newCap;
		for (size_t i = 0; i < numToCopy; i++)
		{
			new (&newData[i]) T(std::move(_data[i]));
			_data[i].~T();
		}
		for (size_t i = numToCopy; i < _size; i++)
			_data[i].~T();
		free(_data);
		_data = newData;
		_capacity = newCap;
	}

	UI_FORCEINLINE void Reserve(size_t newSize)
	{
		if (newSize > _capacity)
			_Realloc(newSize + _capacity);
	}
	void Resize(size_t newSize)
	{
		Reserve(newSize);
		while (_size < newSize)
			new (&_data[_size++]) T();
		while (_size > newSize)
			_data[--_size].~T();
	}
	void ResizeWith(size_t newSize, const T& v)
	{
		Reserve(newSize);
		while (_size < newSize)
			new (&_data[_size++]) T(v);
		while (_size > newSize)
			_data[--_size].~T();
	}

	inline void Append(const T& v)
	{
		Reserve(_size + 1);
		new (&_data[_size++]) T(v);
	}
	inline void Append(T&& v)
	{
		Reserve(_size + 1);
		new (&_data[_size++]) T(v);
	}
	void AppendMany(const T* p, size_t n)
	{
		size_t newSize = _size + n;
		Reserve(newSize);
		for (size_t i = 0; i < n; i++)
			new (&_data[_size + i]) T(p[i]);
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
		for (size_t i = 0; i < n; i++)
			_data[--_size].~T();
	}

	// complex modification
	UI_FORCEINLINE void _HardMove(size_t to, size_t from)
	{
		new (&_data[to]) T(std::move(_data[from]));
		_data[from].~T();
	}
	inline void UnorderedRemoveAt(size_t at)
	{
		assert(at < _size);
		_data[at].~T();
		--_size;
		if (at < _size)
			_HardMove(at, _size);
	}
	void RemoveAt(size_t at, size_t n = 1)
	{
		assert(at < _size);
		assert(n < _size);
		assert(at + n >= at);
		assert(at + n <= _size); // may overflow

		for (size_t i = 0; i < n; i++)
			_data[at + i].~T();

		if (at + n < _size)
		{
			_size -= n;
			for (size_t p = at; p < _size; p++)
				_HardMove(p, p + n);
		}
	}
	void InsertAt(size_t at, const T& v)
	{
		assert(at <= _size);
		Reserve(_size + 1);
		if (at < _size)
		{
			for (size_t p = _size; p > at; )
			{
				_HardMove(p, p - 1);
				p--;
			}
		}
		new (&_data[at]) T(v);
		_size++;
	}
	UI_FORCEINLINE void Prepend(const T& v)
	{
		InsertAt(0, v);
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
};

} // ui
