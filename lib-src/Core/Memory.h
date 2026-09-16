
#pragma once

#include "Platform.h"

#include <assert.h>
#include <initializer_list>


namespace ui {

template <class T>
struct ArrayView
{
	const T* _data;
	size_t _size;

	UI_FORCEINLINE ArrayView() : _data(nullptr), _size(0) {}
	UI_FORCEINLINE ArrayView(const T* data, size_t size) : _data(data), _size(size) {}
	UI_FORCEINLINE ArrayView(const std::initializer_list<T>& src) : _data(src.begin()), _size(src.size()) {}
	template <size_t N>
	UI_FORCEINLINE ArrayView(const T(&arr)[N]) : _data(arr), _size(N) {}
	template <class U>
	UI_FORCEINLINE ArrayView(const U& o) : _data(o.data()), _size(o.size()) {}

	UI_FORCEINLINE const T* Data() const { return _data; }
	UI_FORCEINLINE size_t Size() const { return _size; }
	UI_FORCEINLINE size_t SizeInBytes() const { return _size * sizeof(T); }
	UI_FORCEINLINE bool IsEmpty() const { return _size == 0; }
	UI_FORCEINLINE bool NotEmpty() const { return _size != 0; }
	UI_FORCEINLINE const T* data() const { return _data; }
	UI_FORCEINLINE size_t size() const { return _size; }
	UI_FORCEINLINE const T* begin() const { return _data; }
	UI_FORCEINLINE const T* end() const { return _data + _size; }
	UI_FORCEINLINE const T& operator [] (size_t i) const
	{
		assert(i < _size);
		return _data[i];
	}
	UI_FORCEINLINE size_t GetElementIndex(const T& valFromThisArrayView) const
	{
		return &valFromThisArrayView - _data;
	}
	UI_FORCEINLINE const T& First() const
	{
		assert(_size);
		return _data[0];
	}
	UI_FORCEINLINE ArrayView WithoutFirst() const
	{
		assert(_size);
		return { _data + 1, _size - 1 };
	}
	UI_FORCEINLINE const T& Last() const
	{
		assert(_size);
		return _data[_size - 1];
	}
	UI_FORCEINLINE ArrayView WithoutLast() const
	{
		assert(_size);
		return { _data, _size - 1 };
	}

	const T& PrevWrap(size_t i) const
	{
		assert(i < _size);
		return _data[(i + _size - 1) % _size];
	}
	const T& NextWrap(size_t i) const
	{
		assert(i < _size);
		return _data[(i + 1) % _size];
	}

	const T& PrevClamp(size_t i) const
	{
		assert(i < _size);
		return _data[i ? i - 1 : 0];
	}
	const T& NextClamp(size_t i) const
	{
		assert(i < _size);
		return _data[i + 1 == _size ? i : i + 1];
	}

	// subSize is clamped instead of range-checked
	UI_FORCEINLINE ArrayView Subview(size_t subOff = 0, size_t subSize = SIZE_MAX) const
	{
		assert(subOff <= _size);
		if (subSize > _size - subOff)
			subSize = _size - subOff;
		return { _data + subOff, subSize };
	}

	UI_FORCEINLINE ArrayView<char> AsChar() const { return { (const char*)_data, _size * sizeof(T) }; }
	UI_FORCEINLINE ArrayView<i8> AsI8() const { return { (const i8*)_data, _size * sizeof(T) }; }
	UI_FORCEINLINE ArrayView<u8> AsU8() const { return { (const u8*)_data, _size * sizeof(T) }; }

	template <class T2>
	bool ContainsT(const T2& what) const
	{
		for (size_t i = 0; i < _size; i++)
			if (_data[i] == what)
				return true;
		return false;
	}
	template <class T2>
	size_t IndexOfT(const T2& what) const
	{
		for (size_t i = 0; i < _size; i++)
			if (_data[i] == what)
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

	UI_FORCEINLINE bool Contains(const T& what) const { return ContainsT(what); }
	UI_FORCEINLINE size_t IndexOf(const T& what) const { return IndexOfT(what); }
	UI_FORCEINLINE size_t LastIndexOf(const T& what) const { return LastIndexOfT(what); }
};

template <class T1, class T2>
bool operator == (const ArrayView<T1>& a, const ArrayView<T2>& b)
{
	if (a._size != b._size)
		return false;
	for (size_t i = 0; i < a._size; i++)
		if (a._data[i] != b._data[i])
			return false;
	return true;
}
template <class T1, class T2>
UI_FORCEINLINE bool operator != (const ArrayView<T1>& a, const ArrayView<T2>& b) { return !(a == b); }

} // ui
