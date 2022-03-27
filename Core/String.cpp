
#include "String.h"


namespace ui {

int StringView::compare(const StringView& o) const
{
	size_t minSize = min(_size, o._size);
	int diff = memcmp(_data, o._data, minSize);
	if (diff != 0)
		return diff;
	return _size == o._size
		? 0
		: _size < o._size ? -1 : 1;
}

bool StringView::equal_to_ci(const StringView& o) const
{
	if (_size != o._size)
		return false;
	for (size_t i = 0; i < _size; i++)
	{
		if (_data[i] != o._data[i])
		{
			char low1 = ToLower(_data[i]);
			char low2 = ToLower(o._data[i]);
			if (low1 != low2)
				return false;
		}
	}
	return true;
}

StringView StringView::ltrim() const
{
	StringView out = *this;
	while (out._size && IsSpace(*out._data))
	{
		out._data++;
		out._size--;
	}
	return out;
}

StringView StringView::rtrim() const
{
	StringView out = *this;
	while (out._size && IsSpace(out._data[out._size - 1]))
	{
		out._size--;
	}
	return out;
}

int StringView::count(StringView sub, size_t maxpos) const
{
	int out = 0;
	auto at = find_first_at(sub);
	while (at < maxpos)
	{
		out++;
		at = find_first_at(sub, at + 1);
	}
	return out;
}

size_t StringView::find_first_at(StringView sub, size_t from, size_t def) const
{
	for (size_t i = from; i + sub._size <= _size; i++)
		if (memcmp(&_data[i], sub._data, sub._size) == 0)
			return i;
	return def;
}

size_t StringView::find_last_at(StringView sub, size_t from, size_t def) const
{
	for (size_t i = min(_size - sub._size, from) + 1; i <= _size; )
	{
		i--;
		if (memcmp(&_data[i], sub._data, sub._size) == 0)
			return i;
	}
	return def;
}

void StringView::skip_c_whitespace(bool single_line_comments, bool multiline_comments)
{
	bool found = true;
	while (found)
	{
		found = false;
		*this = ltrim();
		if (single_line_comments && starts_with("//"))
		{
			found = true;
			*this = after_first("\n");
		}
		if (multiline_comments && starts_with("/*"))
		{
			found = true;
			*this = after_first("*/");
		}
	}
}

double StringView::take_float64()
{
	const char* end = _data + _size;

	double sign;
	if (_data != end && *_data == '+')
	{
		sign = 1;
		_data++;
	}
	else if (_data != end && *_data == '-')
	{
		sign = -1;
		_data++;
	}
	else
		sign = 1;

	double ret = 0;
	while (_data != end && *_data >= '0' && *_data <= '9')
	{
		ret *= 10;
		ret += *_data - '0';
		_data++;
	}

	if (_data != end && *_data == '.')
	{
		_data++;

		double fractpos = 1;
		while (_data != end && *_data >= '0' && *_data <= '9')
		{
			fractpos *= 0.1;
			ret += fractpos * (*_data - '0');
			_data++;
		}
	}

	if (_data + 2 < end && _data[0] == 'e' && (_data[1] == '+' || _data[1] == '-') && _data[2] >= '0' && _data[2] <= '9')
	{
		double esign = _data[1] == '+' ? 1 : -1;
		_data += 2;
		double exp = 0;
		while (_data != end && *_data >= '0' && *_data <= '9')
		{
			exp *= 10;
			exp += *_data - '0';
			_data++;
		}
		ret *= pow(10, exp * esign);
	}

	_size = end - _data;

	return ret * sign;
}

} // ui
