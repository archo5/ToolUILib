
#pragma once

#ifdef _MSC_VER
# ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
# endif
#pragma warning(disable: 4530)
#endif

#include <inttypes.h>


#ifdef _MSC_VER
#define UI_FORCEINLINE __forceinline
# if defined(UI_CHECK_FORCEINLINE) || defined(NDEBUG)
#  pragma warning(error: 4714)
# endif
#endif


namespace ui {

using i8 = int8_t;
using u8 = uint8_t;
using i16 = int16_t;
using u16 = uint16_t;
using i32 = int32_t;
using u32 = uint32_t;
using i64 = int64_t;
using u64 = uint64_t;

template <class T> struct RemoveReference { typedef T Type; };
template <class T> struct RemoveReference<T&> { typedef T Type; };
template <class T> struct RemoveReference<T&&> { typedef T Type; };

template <class T> constexpr typename RemoveReference<T>::Type&& Move(T&& t) noexcept
{ return static_cast<typename RemoveReference<T>::Type&&>(t); }

} // ui
