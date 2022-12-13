
#pragma once

#include "Image.h"
#include "Math.h"
#include "RefCounted.h"
#include "String.h"


namespace ui {

struct IVectorImage : IRefCounted
{
	virtual Size2f GetSize() const = 0;
	float GetWidth() const { return GetSize().x; }
	float GetHeight() const { return GetSize().y; }
	virtual StringView GetCacheKey() const = 0;

	virtual Canvas GetImageWithHeight(uint16_t h) const = 0;
};
using VectorImageHandle = RCHandle<IVectorImage>;

VectorImageHandle VectorImageLoadFromFile(StringView path);

} // ui
