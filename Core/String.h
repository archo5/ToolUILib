
#pragma once

#include "Common.h"
#include "Memory.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>


namespace ui {

static inline bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static inline bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline bool IsDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool IsHexDigit(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static inline bool IsAlphaNum(char c) { return IsAlpha(c) || IsDigit(c); }


struct StringView
{
	StringView() : _data(nullptr), _size(0) {}
	StringView(const char* s) : _data(s), _size(strlen(s)) {}
	StringView(char* s) : _data(s), _size(strlen(s)) {}
	StringView(const char* ptr, size_t sz) : _data(ptr), _size(sz) {}
	// TODO
	StringView(const std::string& o) : _data(o.data()), _size(o.size()) {}
	//StringView(const std::vector<char>& o) : _data(o.data()), _size(o.size()) {}
	StringView(const ArrayView<char>& o) : _data(o.data()), _size(o.size()) {}

	const char* data() const { return _data; }
	size_t size() const { return _size; }
	bool empty() const { return _size == 0; }
	const char& first() const { return _data[0]; }
	const char& last() const { return _data[_size - 1]; }
	const char* begin() const { return _data; }
	const char* end() const { return _data + _size; }
	char operator [] (size_t pos) const { return _data[pos]; }

	int compare(const StringView& o) const
	{
		size_t minSize = min(_size, o._size);
		int diff = memcmp(_data, o._data, minSize);
		if (diff != 0)
			return diff;
		return _size == o._size
			? 0
			: _size < o._size ? -1 : 1;
	}

	StringView substr(size_t at, size_t size = SIZE_MAX) const
	{
		assert(at <= _size);
		return StringView(_data + at, min(_size - at, size));
	}
	StringView ltrim() const
	{
		StringView out = *this;
		while (out._size && IsSpace(*out._data))
		{
			out._data++;
			out._size--;
		}
		return out;
	}
	StringView rtrim() const
	{
		StringView out = *this;
		while (out._size && IsSpace(out._data[out._size - 1]))
		{
			out._size--;
		}
		return out;
	}
	StringView trim() const
	{
		return ltrim().rtrim();
	}

	bool starts_with(StringView sub) const
	{
		return _size >= sub._size && memcmp(_data, sub._data, sub._size) == 0;
	}
	bool ends_with(StringView sub) const
	{
		return _size >= sub._size && memcmp(_data + _size - sub._size, sub._data, sub._size) == 0;
	}

	int count(StringView sub, size_t maxpos = SIZE_MAX) const
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
	size_t find_first_at(StringView sub, size_t from = 0, size_t def = SIZE_MAX) const
	{
		for (size_t i = from; i <= _size - sub._size; i++)
			if (memcmp(&_data[i], sub._data, sub._size) == 0)
				return i;
		return def;
	}
	size_t find_last_at(StringView sub, size_t from = SIZE_MAX, size_t def = SIZE_MAX) const
	{
		for (size_t i = min(_size - sub._size, from) + 1; i <= _size; )
		{
			i--;
			if (memcmp(&_data[i], sub._data, sub._size) == 0)
				return i;
		}
		return def;
	}

	StringView until_first(StringView sub) const
	{
		size_t at = find_first_at(sub);
		return at < SIZE_MAX ? substr(0, at) : StringView();
	}
	StringView after_first(StringView sub) const
	{
		size_t at = find_first_at(sub);
		return at < SIZE_MAX ? substr(at + sub._size) : StringView();
	}

	StringView after_last(StringView sub) const
	{
		size_t at = find_last_at(sub);
		return substr(at < SIZE_MAX ? at + sub._size : 0);
	}
	StringView until_last(StringView sub) const
	{
		size_t at = find_last_at(sub, SIZE_MAX, 0);
		return substr(0, at);
	}

	void skip_c_whitespace(bool single_line_comments = true, bool multiline_comments = true)
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
	template <class F>
	bool first_char_is(F f)
	{
		return _size != 0 && !!f(_data[0]);
	}
	char take_char()
	{
		assert(_size);
		_size--;
		return *_data++;
	}
	bool take_if_equal(StringView s)
	{
		if (starts_with(s))
		{
			_data += s._size;
			_size -= s._size;
			return true;
		}
		return false;
	}
	template <class F>
	StringView take_while(F f)
	{
		const char* start = _data;
		while (_size && f(_data[0]))
		{
			_data++;
			_size--;
		}
		return StringView(start, _data - start);
	}

	const char* _data;
	size_t _size;
};

inline std::string to_string(StringView s)
{
	std::string out;
	out.append(s.data(), s.size());
	return out;
}
inline std::string to_string(StringView s1, StringView s2)
{
	std::string out;
	out.reserve(s1.size() + s2.size());
	out.append(s1.data(), s1.size());
	out.append(s2.data(), s2.size());
	return out;
}
inline std::string to_string(StringView s1, StringView s2, StringView s3)
{
	std::string out;
	out.reserve(s1.size() + s2.size() + s3.size());
	out.append(s1.data(), s1.size());
	out.append(s2.data(), s2.size());
	out.append(s3.data(), s3.size());
	return out;
}

template <size_t S>
struct CStr
{
	CStr(StringView s)
	{
		if (s.size() + 1 <= S)
			_ptr = _tmp;
		else
			_ptr = new char[s.size() + 1];

		memcpy(_ptr, s.data(), s.size());
		_ptr[s.size()] = 0;
	}
	~CStr()
	{
		if (_ptr != _tmp)
			delete[] _ptr;
	}
	const char* c_str() const { return _ptr; }
	operator const char*() const { return _ptr; }

	char* _ptr;
	char _tmp[S];
};

inline bool operator == (const StringView& a, const StringView& b) { return a._size == b._size && memcmp(a._data, b._data, b._size) == 0; }
inline bool operator != (const StringView& a, const StringView& b) { return !(a == b); }
inline bool operator < (const StringView& a, const StringView& b) { return a.compare(b) < 0; }
inline bool operator > (const StringView& a, const StringView& b) { return a.compare(b) > 0; }
inline bool operator <= (const StringView& a, const StringView& b) { return a.compare(b) <= 0; }
inline bool operator >= (const StringView& a, const StringView& b) { return a.compare(b) >= 0; }

} // ui
namespace std {
template <>
struct hash<ui::StringView>
{
	size_t operator () (const ui::StringView& v) const
	{
		uint64_t hash = 0xcbf29ce484222325;
		for (char c : v)
			hash = (hash ^ c) * 1099511628211;
		return size_t(hash);
	}
};
} // std
namespace ui {

inline std::string FormatVA(const char* fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	int len = vsnprintf(nullptr, 0, fmt, args2);
	va_end(args2);
	if (len > 0)
	{
		std::string ret;
		ret.resize(len + 1);
		va_copy(args2, args);
		int len2 = vsnprintf(&ret[0], ret.size(), fmt, args2);
		va_end(args2);
		if (len2 > 0)
		{
			ret.resize(len2);
			return ret;
		}
	}
	return {};
}

inline std::string Format(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	auto ret = FormatVA(fmt, args);
	va_end(args);
	return ret;
}

} // ui
