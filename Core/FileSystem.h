
#pragma once

#include "String.h"


namespace ui {

std::string ReadTextFile(const char* path);
bool WriteTextFile(const char* path, StringView text);

} // ui
