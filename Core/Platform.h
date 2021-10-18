
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



