
#pragma once

#include "Platform.h"

#include <assert.h>


#define UI_CONCAT_(a, b) a ## b
#define UI_CONCAT(a, b) UI_CONCAT_(a, b)


namespace ui {

struct NoValue {};

struct DoNotInitialize {};

template <class T> UI_FORCEINLINE T min(T a, T b) { return a < b ? a : b; }
template <class T> UI_FORCEINLINE T max(T a, T b) { return a > b ? a : b; }
template <class T> UI_FORCEINLINE T clamp(T x, T vmin, T vmax) { return x < vmin ? vmin : x > vmax ? vmax : x; }

template <typename T> struct ReverseIterable { T& iterable; };
template <typename T> inline auto begin(ReverseIterable<T> w) { return std::rbegin(w.iterable); }
template <typename T> inline auto end(ReverseIterable<T> w) { return std::rend(w.iterable); }
template <typename T> inline ReverseIterable<T> ReverseIterate(T&& iterable) { return { iterable }; }

template <class T>
struct Optional
{
	T _value{};
	bool _hasValue = false;

	UI_FORCEINLINE Optional() {}
	UI_FORCEINLINE Optional(const T& v) : _value(v), _hasValue(true) {}

	UI_FORCEINLINE bool operator == (const Optional& o) const
	{
		return _hasValue == o._hasValue && (_hasValue ? _value == o._value : true);
	}
	UI_FORCEINLINE bool operator != (const Optional& o) const { return !(*this == o); }
	UI_FORCEINLINE operator void*() const { return (void*)_hasValue; }

	UI_FORCEINLINE bool HasValue() const
	{
		return _hasValue;
	}
	UI_FORCEINLINE const T& GetValue() const
	{
		assert(_hasValue);
		return _value;
	}
	UI_FORCEINLINE T GetValueOrDefault(const T& def) const
	{
		return _hasValue ? _value : def;
	}
	UI_FORCEINLINE void SetValue(const T& v)
	{
		_value = v;
		_hasValue = true;
	}
	UI_FORCEINLINE void ClearValue()
	{
		_value = {};
		_hasValue = false;
	}
	template <class R>
	UI_FORCEINLINE Optional<R> StaticCast() const
	{
		if (_hasValue)
			return static_cast<R>(_value);
		return {};
	}
};

template <class T>
struct TmpEdit
{
	TmpEdit(T& dst, T src) : dest(dst), backup(dst)
	{
		dst = src;
	}
	~TmpEdit()
	{
		dest = backup;
	}
	T& dest;
	T backup;
};

template <int& at>
struct InstanceCounter
{
	InstanceCounter() { at++; }
	InstanceCounter(const InstanceCounter&) { at++; }
	~InstanceCounter() { at--; }
};

template <class F>
struct DeferImpl
{
	F& func;
	UI_FORCEINLINE ~DeferImpl()
	{
		func();
	}
};
#define UI_DEFER(fn) auto UI_CONCAT(__deferimpl_, __LINE__) = [&](){ fn; }; \
	::ui::DeferImpl<decltype(UI_CONCAT(__deferimpl_, __LINE__))> UI_CONCAT(__defer_, __LINE__) = { UI_CONCAT(__deferimpl_, __LINE__) };

} // ui
