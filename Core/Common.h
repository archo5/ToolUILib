
#pragma once

#include "Platform.h"


#define UI_CONCAT_(a, b) a ## b
#define UI_CONCAT(a, b) UI_CONCAT_(a, b)


namespace ui {

struct DoNotInitialize {};

template <class T> UI_FORCEINLINE T min(T a, T b) { return a < b ? a : b; }
template <class T> UI_FORCEINLINE T max(T a, T b) { return a > b ? a : b; }
template <class T> UI_FORCEINLINE T clamp(T x, T vmin, T vmax) { return x < vmin ? vmin : x > vmax ? vmax : x; }

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
