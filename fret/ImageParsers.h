
#pragma once
#include "pch.h"
#include "FileReaders.h"


struct ImageInfo
{
	int64_t offImg;
	int64_t offPal;
	uint32_t width;
	uint32_t height;
};

size_t GetImageFormatCount();
StringView GetImageFormatCategory(size_t fid);
StringView GetImageFormatName(size_t fid);
ui::Image* CreateImageFrom(IDataSource* ds, StringView fmt, const ImageInfo& info);
