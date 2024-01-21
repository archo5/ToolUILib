
#pragma once
#include "Platform.h"


namespace ui {

void TweakableValueSetMinReloadInterval(int ms);
void TweakableValueForceSubsequentReloads();

// macroNamePfx = a fairly unique string to look for to find the macro, doesn't need to be the entire macro
// uid = if not null, a unique string to look for inside the macro that identifies the specific macro
double TweakableValue(const char* file, int line, const char* macroNamePfx, double original, const char* uid = nullptr);
UI_FORCEINLINE double TweakableValueSkip(double original, const char* = nullptr) { return original; }

// example macro implementation (copy and rename to shorten)
#ifdef NDEBUG
# define UI_TWEAKABLE_VALUE(...) ::ui::TweakableValueSkip(__VA_ARGS__)
#else
# define UI_TWEAKABLE_VALUE(...) ::ui::TweakableValue(__FILE__, __LINE__, "UI_TWEAKABLE_VALUE", __VA_ARGS__)
#endif
// usage:
// - UI_TWEAKABLE_VALUE(5)
// - UI_TWEAKABLE_VALUE(5, "x")

} // ui
