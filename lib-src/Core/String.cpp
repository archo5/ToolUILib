
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

bool StringView::EqualToCI(const StringView& o) const
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

int StringView::Count(StringView sub, size_t maxpos) const
{
	int out = 0;
	auto at = FindFirstAt(sub);
	while (at < maxpos)
	{
		out++;
		at = FindFirstAt(sub, at + 1);
	}
	return out;
}

size_t StringView::FindFirstAt(StringView sub, size_t from, size_t def) const
{
	for (size_t i = from; i + sub._size <= _size; i++)
		if (memcmp(&_data[i], sub._data, sub._size) == 0)
			return i;
	return def;
}

size_t StringView::FindFirstAtCI(StringView sub, size_t from, size_t def) const
{
	for (size_t i = from; i + sub._size <= _size; i++)
		if (memicmp(&_data[i], sub._data, sub._size) == 0)
			return i;
	return def;
}

size_t StringView::FindLastAt(StringView sub, size_t from, size_t def) const
{
	for (size_t i = min(_size - sub._size, from) + 1; i <= _size; )
	{
		i--;
		if (memcmp(&_data[i], sub._data, sub._size) == 0)
			return i;
	}
	return def;
}

void StringView::SkipCWhitespace(bool single_line_comments, bool multiline_comments)
{
	bool found = true;
	while (found)
	{
		found = false;
		*this = ltrim();
		if (single_line_comments && starts_with("//"))
		{
			found = true;
			*this = AfterFirst("\n");
		}
		if (multiline_comments && starts_with("/*"))
		{
			found = true;
			*this = AfterFirst("*/");
		}
	}
}

static int ParseHexDigit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

int64_t StringView::take_int64(unsigned formats)
{
	const char* end = _data + _size;

	bool neg = false;
	if (_data != end && *_data == '+')
	{
		neg = false;
		_data++;
	}
	else if (_data != end && *_data == '-')
	{
		neg = true;
		_data++;
	}

	unsigned format = DECIMAL;
	unsigned base = 10;
	if (_data != end && _data != end + 1 && *_data == '0')
	{
		_data++;
		if (*_data == 'x')
		{
			format = HEX;
			base = 16;
		}
		else if (*_data == 'o')
		{
			format = OCTAL;
			base = 8;
		}
		else if (*_data == 'b')
		{
			format = BINARY;
			base = 2;
		}
	}

	if (!(formats & format))
	{
		_size = end - _data;
		return 0;
	}

	if (format != DECIMAL)
		_data++;

	int64_t ret = 0;
	if (base == 16)
	{
		int digit;
		while (_data != end && (digit = ParseHexDigit(*_data)) >= 0)
		{
			ret *= base;
			ret -= digit;
			_data++;
		}
	}
	else
	{
		while (_data != end && *_data >= '0' && *_data <= '0' + base)
		{
			ret *= base;
			ret -= *_data - '0';
			_data++;
		}
	}

	if (!neg)
		ret = -ret;

	_size = end - _data;

	return ret;
}

#if 0 // TODO
int64_t StringView::take_int64_checked(bool* valid, int64_t min, int64_t max)
{
	const char* end = _data + _size;

	bool neg = false;
	if (_data != end && *_data == '+')
	{
		neg = false;
		_data++;
	}
	else if (_data != end && *_data == '-')
	{
		neg = true;
		_data++;
	}

	bool isValid = true;

	int64_t ret = 0;
	while (_data != end && *_data >= '0' && *_data <= '9')
	{
		int64_t pret = ret;
		ret *= 10;
		if (ret / 10 != pret)
			isValid = false;

		pret = ret;
		ret -= *_data - '0';
		if (ret > pret)
			isValid = false;

		_data++;
	}
	if (!neg)
	{
		if (ret == INT64_MIN)
			isValid = false;
		ret = -ret;
	}

	_size = end - _data;

	if (ret < min || ret > max)
		isValid = false;

	if (valid)
		*valid = isValid;

	return ret;
}
#endif

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


uint32_t UTF8Iterator::Read()
{
	if (pos >= str.size())
		return END;

	// ASCII
	char c0 = str[pos++];
	if (!(c0 & 0x80))
		return c0;

	if (pos >= str.size())
		return errorReturnValue;
	char c1 = str[pos++];
	if ((c1 & 0xC0) != 0x80)
		return errorReturnValue;

	if ((c0 & 0xE0) == 0xC0)
		return ((c0 & 0x1F) << 6) | (c1 & 0x3F);

	if (pos >= str.size())
		return errorReturnValue;
	char c2 = str[pos++];
	if ((c2 & 0xC0) != 0x80)
		return errorReturnValue;

	if ((c0 & 0xF0) == 0xE0)
		return ((c0 & 0xF) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);

	if (pos >= str.size())
		return errorReturnValue;
	char c3 = str[pos++];
	if ((c3 & 0xC0) != 0x80)
		return errorReturnValue;

	if ((c0 & 0xF8) == 0xF0)
		return ((c0 & 0x7) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);

	return errorReturnValue;
}

} // ui
