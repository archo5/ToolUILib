
#pragma once
#include "pch.h"
#include "FileReaders.h"


ui::Image* CreateImageFrom(IDataSource* ds, const char* fmt, int64_t offImg, int64_t offPal, uint32_t width, uint32_t height);
