
#pragma once

#include "Core/Platform.h"
#include "Core/String.h"


namespace ui {

struct ImageDataRef
{
	u8* pixels = nullptr;
	u32 width = 0;
	u32 height = 0;

	bool LoadFromBytes(const void* srcdata, size_t srcsize);
	bool LoadFromFile(StringView path);
	void Free();
};

} // ui
