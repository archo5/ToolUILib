
#pragma once

#include "Common.h"
#include "HashUtils.h"
#include "Memory.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <string>


namespace std {
inline size_t HashValue(const std::string& sv)
{
	return ui::HashManyBytesFast(sv.data(), sv.size());
}
} // std

namespace ui {

static inline bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static inline bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline bool IsDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool IsHexDigit(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static inline bool IsAlphaNum(char c) { return IsAlpha(c) || IsDigit(c); }
static inline char ToLower(char c) { return c >= 'A' && c <= 'Z' ? c | 0x20 : c; }
static inline char ToUpper(char c) { return c >= 'a' && c <= 'z' ? c & ~0x20 : c; }

static inline unsigned DecodeHexChar(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

static const constexpr unsigned DECIMAL = 1 << 0;
static const constexpr unsigned HEX = 1 << 1;
static const constexpr unsigned BINARY = 1 << 2;
static const constexpr unsigned OCTAL = 1 << 3;
static const constexpr unsigned ALL_NUMBER_FORMATS = 0xf;


struct StringView
{
	const char* _data;
	size_t _size;

	UI_FORCEINLINE StringView() : _data(nullptr), _size(0) {}
	UI_FORCEINLINE StringView(const char* s) : _data(s), _size(strlen(s)) {}
	UI_FORCEINLINE StringView(char* s) : _data(s), _size(strlen(s)) {}
	UI_FORCEINLINE StringView(const char* ptr, size_t sz) : _data(ptr), _size(sz) {}
	// TODO
	UI_FORCEINLINE StringView(const std::string& o) : _data(o.data()), _size(o.size()) {}
	//StringView(const std::vector<char>& o) : _data(o.data()), _size(o.size()) {}
	UI_FORCEINLINE StringView(const ArrayView<char>& o) : _data(o.data()), _size(o.size()) {}

	UI_FORCEINLINE const char* Data() const { return _data; }
	UI_FORCEINLINE size_t Size() const { return _size; }
	UI_FORCEINLINE bool IsEmpty() const { return _size == 0; }
	UI_FORCEINLINE bool NotEmpty() const { return _size != 0; }
	UI_FORCEINLINE const char* data() const { return _data; }
	UI_FORCEINLINE size_t size() const { return _size; }
	UI_FORCEINLINE const char& First() const { return _data[0]; }
	UI_FORCEINLINE const char& Last() const { return _data[_size - 1]; }
	UI_FORCEINLINE const char* begin() const { return _data; }
	UI_FORCEINLINE const char* end() const { return _data + _size; }
	UI_FORCEINLINE char operator [] (size_t pos) const { return _data[pos]; }

	int compare(const StringView& o) const;

	bool EqualToCI(const StringView& o) const;

	StringView substr(size_t at, size_t size = SIZE_MAX) const
	{
		assert(at <= _size);
		return StringView(_data + at, min(_size - at, size));
	}
	StringView ltrim() const;
	StringView rtrim() const;
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

	UI_FORCEINLINE bool Contains(StringView sub) const { return FindFirstAt(sub) != SIZE_MAX; }
	UI_FORCEINLINE bool ContainsCI(StringView sub) const { return FindFirstAtCI(sub) != SIZE_MAX; }
	int Count(StringView sub, size_t maxpos = SIZE_MAX) const;
	size_t FindFirstAt(StringView sub, size_t from = 0, size_t def = SIZE_MAX) const;
	size_t FindFirstAtCI(StringView sub, size_t from = 0, size_t def = SIZE_MAX) const;
	size_t FindLastAt(StringView sub, size_t from = SIZE_MAX, size_t def = SIZE_MAX) const;

	StringView UntilFirst(StringView sub) const
	{
		size_t at = FindFirstAt(sub);
		return at < SIZE_MAX ? substr(0, at) : *this;
	}
	StringView AfterFirst(StringView sub) const
	{
		size_t at = FindFirstAt(sub);
		return at < SIZE_MAX ? substr(at + sub._size) : StringView();
	}

	StringView AfterLast(StringView sub) const
	{
		size_t at = FindLastAt(sub);
		return substr(at < SIZE_MAX ? at + sub._size : 0);
	}
	StringView SinceLast(StringView sub) const
	{
		size_t at = FindLastAt(sub);
		return substr(at < SIZE_MAX ? at : 0);
	}
	StringView UntilLast(StringView sub) const
	{
		size_t at = FindLastAt(sub, SIZE_MAX, 0);
		return substr(0, at);
	}

	inline struct StringViewSplitPair SplitFirst(StringView sub) const;
	inline struct StringViewSplitPair SplitLast(StringView sub) const;

	bool SkipNumChars(size_t n)
	{
		if (n <= _size)
		{
			_data += n;
			_size -= n;
			return true;
		}
		_data += _size;
		_size = 0;
		return false;
	}

	void SkipCWhitespace(bool single_line_comments = true, bool multiline_comments = true);

	template <class F>
	UI_FORCEINLINE bool FirstCharIs(F f) const
	{
		return _size != 0 && !!f(_data[0]);
	}
	UI_FORCEINLINE bool FirstCharEquals(char c) const
	{
		return _size && c == _data[0];
	}
	char take_char()
	{
		assert(_size);
		_size--;
		return *_data++;
	}
	bool take_if_equal(char c)
	{
		if (FirstCharEquals(c))
		{
			_data += 1;
			_size -= 1;
			return true;
		}
		return false;
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

	int64_t take_int64(unsigned formats = ALL_NUMBER_FORMATS);
	UI_FORCEINLINE int64_t to_int64(unsigned formats = ALL_NUMBER_FORMATS) const { return StringView(*this).take_int64(formats); }
	int32_t take_int32(unsigned formats = ALL_NUMBER_FORMATS) { return int32_t(take_int64(formats)); }
	UI_FORCEINLINE int32_t to_int32(unsigned formats = ALL_NUMBER_FORMATS) const { return int32_t(StringView(*this).take_int64(formats)); }

#if 0 // TODO
	int64_t take_int64_checked(bool* valid = nullptr, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX);
	UI_FORCEINLINE int64_t to_int64_checked(bool* valid = nullptr, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX)
	{ return StringView(*this).take_int64_checked(valid); }
	UI_FORCEINLINE int32_t take_int32_checked(bool* valid = nullptr, int32_t vmin = INT32_MIN, int32_t vmax = INT32_MAX)
	{ return int32_t(take_int64_checked(valid, vmin, vmax)); }
	UI_FORCEINLINE int32_t to_int32_checked(bool* valid = nullptr)
	{ return StringView(*this).take_int32_checked(valid); }
#endif

	double take_float64();
	UI_FORCEINLINE double to_float64() const { return StringView(*this).take_float64(); }
	UI_FORCEINLINE float take_float32() { return float(take_float64()); }
	UI_FORCEINLINE float to_float32() const { return float(StringView(*this).take_float64()); }
};

struct StringViewSplitPair
{
	StringView before;
	StringView after;
};

StringViewSplitPair StringView::SplitFirst(StringView sub) const
{
	size_t pos = FindFirstAt(sub);
	if (pos == SIZE_MAX)
		return { *this, {} };
	return { substr(0, pos), substr(pos + sub.Size()) };
}

StringViewSplitPair StringView::SplitLast(StringView sub) const
{
	size_t pos = FindLastAt(sub);
	if (pos == SIZE_MAX)
		return { {}, *this };
	return { substr(0, pos), substr(pos + sub.Size()) };
}

inline size_t HashValue(StringView sv)
{
	return HashManyBytesFast(sv._data, sv._size);
}


inline std::string to_string(StringView s)
{
	std::string out;
	out.append(s._data, s._size);
	return out;
}
inline std::string to_string(StringView s1, StringView s2)
{
	std::string out;
	out.reserve(s1._size + s2._size);
	out.append(s1._data, s1._size);
	out.append(s2._data, s2._size);
	return out;
}
inline std::string to_string(StringView s1, StringView s2, StringView s3)
{
	std::string out;
	out.reserve(s1._size + s2._size + s3._size);
	out.append(s1._data, s1._size);
	out.append(s2._data, s2._size);
	out.append(s3._data, s3._size);
	return out;
}
inline std::string StrViewArrConcat(ArrayView<StringView> svav)
{
	std::string out;
	size_t size = 0;
	for (const auto& sv : svav)
		size += sv._size;
	out.reserve(size);
	for (const auto& sv : svav)
		out.append(sv._data, sv._size);
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
	UI_FORCEINLINE const char* operator *() const { return _ptr; }
	UI_FORCEINLINE const char* c_str() const { return _ptr; }
	UI_FORCEINLINE operator const char*() const { return _ptr; }

	char* _ptr;
	char _tmp[S];
};

inline bool operator == (const StringView& a, const StringView& b) { return a._size == b._size && memcmp(a._data, b._data, b._size) == 0; }
inline bool operator != (const StringView& a, const StringView& b) { return !(a == b); }
inline bool operator < (const StringView& a, const StringView& b) { return a.compare(b) < 0; }
inline bool operator > (const StringView& a, const StringView& b) { return a.compare(b) > 0; }
inline bool operator <= (const StringView& a, const StringView& b) { return a.compare(b) <= 0; }
inline bool operator >= (const StringView& a, const StringView& b) { return a.compare(b) >= 0; }
inline std::string& operator += (std::string& d, const StringView& s) { d.append(s._data, s._size); return d; }

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

struct UTF8Iterator
{
	static constexpr const uint32_t END = UINT32_MAX;
	static constexpr const uint32_t SAFE_ERROR_VALUE = UINT32_MAX - 1;
	static constexpr const uint32_t REPLACEMENT_CHARACTER = 0xFFFD;

	StringView str;
	size_t pos = 0;
	uint32_t errorReturnValue = REPLACEMENT_CHARACTER;

	UTF8Iterator(StringView s) : str(s) {}
	uint32_t Read();
};

} // ui
