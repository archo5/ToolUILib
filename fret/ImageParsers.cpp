
#include "pch.h"
#include "ImageParsers.h"


ui::Image* CreateImageFrom(IDataSource* ds, const char* fmt, int64_t offImg, int64_t offPal, uint32_t width, uint32_t height)
{
	assert(strcmp(fmt, "RGBX8") == 0);

	ui::Canvas c(width, height);
	auto* B = c.GetBytes();
	ds->Read(offImg, width * height * 4, B);
	for (uint32_t px = 0; px < width * height; px++)
	{
		B[px * 4 + 3] = 0xff;
	}
	return new ui::Image(c);
}
