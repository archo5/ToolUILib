
#pragma once

#include "Array.h"


namespace ui {

// for plain old data

struct DataWriter
{
	static constexpr const bool IsReader = false;

	Array<char>& _data;

	UI_FORCEINLINE DataWriter(Array<char>& data) : _data(data) {}

	bool Marker(const char* text, size_t count = SIZE_MAX) const
	{
		if (count == SIZE_MAX)
			count = strlen(text);
		_data.AppendMany(text, count);
		return true;
	}

	template <class T> UI_FORCEINLINE void RawData(const T& src) const
	{
		_data.AppendMany((char*)&src, sizeof(src));
	}
	
	template <class T> UI_FORCEINLINE void RawDataArray(const T* ptr, size_t sz) const
	{
		_data.AppendMany((char*)ptr, sizeof(*ptr) * sz);
	}

	template <class T> UI_FORCEINLINE void RefView(const ArrayView<T>& src) const
	{
		u32 size = u32(src.Size());
		RawData(size);
		RawDataArray(src.Data(), src.Size());
	}
};

template <class T> UI_FORCEINLINE DataWriter& operator << (DataWriter& dw, const T& src)
{
	dw.RawData(src);
	return dw;
}

struct DataReader
{
	static constexpr const bool IsReader = true;

	ArrayView<char> _data;
	size_t _off = 0;

	UI_FORCEINLINE DataReader(ArrayView<char> data) : _data(data) {}

	size_t Remaining() const
	{
		return _off <= _data.Size() ? _data.Size() - _off : 0;
	}

	UI_FORCEINLINE void Reset()
	{
		_off = 0;
	}

	UI_FORCEINLINE void SkipBytes(size_t n)
	{
		_off += n;
	}

	bool Marker(const char* text, size_t count = SIZE_MAX)
	{
		if (count == SIZE_MAX)
			count = strlen(text);
		if (_off + count > _data.Size())
			return false;

		int ret = memcmp(_data.Data() + _off, text, count);
		_off += count;
		return ret == 0;
	}

	template <class T> UI_FORCEINLINE void Skip(size_t count = 1)
	{
		_off += sizeof(T) * count;
	}

	template <class T> void RawData(T& o)
	{
		if (_off + sizeof(T) <= _data.Size())
		{
			memcpy(&o, &_data[_off], sizeof(T));
			_off += sizeof(T);
		}
		else
		{
			memset(&o, 0, sizeof(T));
		}
	}

	template <class T> void RawDataArray(T* o, size_t n)
	{
		size_t tsz = sizeof(T) * n;
		if (_off + tsz <= _data.Size())
		{
			memcpy(o, _data.Data() + _off, tsz);
			_off += tsz;
		}
		else
		{
			memset(o, 0, tsz);
		}
	}

	template <class T> UI_FORCEINLINE void RefView(ArrayView<T>& o)
	{
		u32 size = 0;
		RawData(size);
		if (_off + size * sizeof(T) <= _data.Size())
		{
			o = ArrayView<T>(_data.Data() + (_off + size * sizeof(T)), size);
		}
		else
		{
			o = {};
		}
	}
};

template <class T> UI_FORCEINLINE DataReader& operator << (DataReader& dr, T& o)
{
	dr.RawData(o);
	return dr;
}

} // ui
