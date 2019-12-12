
#pragma once
#include "../Core/String.h"

namespace uniparser {

struct Script;

Script* Load(StringView text);
void Free(Script* S);
void RunOnFile(Script* S, StringView file);

}
