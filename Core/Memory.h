
#pragma once
#include <assert.h>

template <class T>
struct ArrayView
{
	ArrayView() : _data(nullptr), _size(0) {}
	ArrayView(const T* data, size_t size) : _data(data), _size(size) {}
	template <size_t N>
	ArrayView(const T(&arr)[N]) : _data(arr), _size(N) {}
	template <class U>
	ArrayView(const U& o) : _data(o.data()), _size(o.size()) {}

	const T* data() const { return _data; }
	size_t size() const { return _size; }
	bool empty() const { return _size == 0; }
	const T* begin() const { return _data; }
	const T* end() const { return _data + _size; }
	const T& operator [] (size_t i) const
	{
		assert(i < _size);
		return _data[i];
	}
	const T& last() const
	{
		assert(_size);
		return _data[_size - 1];
	}
	ArrayView without_last() const
	{
		assert(_size);
		return { _data, _size - 1 };
	}

	const T* _data;
	size_t _size;
};
