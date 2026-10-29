
#include "Image_Load.h"

#include "Core/FileSystem.h"
#include "Core/Logging.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../ThirdParty/stb_image.h"


namespace ui {

LogCategory LOG_IMAGE_LOAD("ImageLoad", LogLevel::Info);


bool ImageDataRef::LoadFromBytes(const void* srcdata, size_t srcsize)
{
	int w = 0, h = 0, n = 0;
	auto* imgData = stbi_load_from_memory((const stbi_uc*)srcdata, srcsize, &w, &h, &n, 4);
	width = w;
	height = h;
	pixels = imgData;
	return !!imgData;
}

bool ImageDataRef::LoadFromFile(StringView path)
{
	auto frr = FSReadBinaryFile(path);
	if (!frr.data)
	{
		LogError(LOG_IMAGE_LOAD, "ImageDataRef::LoadFromFile: failed to load %s - could not read file", to_string(path).c_str());
		return false;
	}
	return LoadFromBytes(frr.data->Data(), frr.data->Size());
}

void ImageDataRef::Free()
{
	stbi_image_free(pixels);
	pixels = nullptr;
	width = 0;
	height = 0;
}

} // ui
