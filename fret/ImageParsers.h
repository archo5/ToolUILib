
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
ui::StringView GetImageFormatCategory(size_t fid);
ui::StringView GetImageFormatName(size_t fid);
ui::draw::ImageHandle CreateImageFrom(IDataSource* ds, ui::StringView fmt, const ImageInfo& info);
