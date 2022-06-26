
#pragma once
#include "Platform.h"
#include "String.h"

namespace ui {

struct DynamicLib;

DynamicLib* DynamicLibLoad(StringView path);
void DynamicLibUnload(DynamicLib* lib);
void* DynamicLibGetSymbol(DynamicLib* lib, const char* name);

} // ui
