
#pragma once
#include "Platform.h"
#include "String.h"

namespace ui {

struct DynamicLib;

UI_API DynamicLib* DynamicLibLoad(StringView path);
UI_API void DynamicLibUnload(DynamicLib* lib);
UI_API void* DynamicLibGetSymbol(DynamicLib* lib, const char* name);

} // ui
