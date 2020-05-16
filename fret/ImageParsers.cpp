
#include "pch.h"
#include "ImageParsers.h"


ui::Image* CreateImageFrom(IDataSource* ds, const char* fmt, int64_t offImg, int64_t offPal, uint32_t width, uint32_t height)
{
	ui::Canvas c(width, height);
	auto* B = c.GetBytes();

	if (strcmp(fmt, "RGBX8") == 0)
	{
		ds->Read(offImg, width * height * 4, B);
		for (uint32_t px = 0; px < width * height; px++)
		{
			B[px * 4 + 3] = 0xff;
		}
	}
	else if (strcmp(fmt, "RGBo8") == 0)
	{
		ds->Read(offImg, width * height * 4, B);
		for (uint32_t px = 0; px < width * height; px++)
		{
			B[px * 4 + 3] = B[px * 4 + 3] ? 0xff : 0;
		}
	}
	else if (strcmp(fmt, "4BPP_RGBo8") == 0)
	{
		if (width > 4096)
			return nullptr;

		uint8_t pal[16 * 4];
		ds->Read(offPal, 64, pal);
		for (uint32_t i = 0; i < 16; i++)
			pal[i * 4 + 3] = pal[i * 4 + 3] ? 255 : 0;

		uint8_t line[2048];
		for (uint32_t y = 0; y < height; y++)
		{
			ds->Read(offImg + width / 2 * y, width / 2, line);
			for (uint32_t x = 0; x < width / 2; x++)
			{
				uint8_t v1 = line[x] & 0xf;
				uint8_t v2 = line[x] >> 4;

				uint32_t px = x * 2 + y * width;
				B[px * 4 + 0] = pal[v1 * 4 + 0];
				B[px * 4 + 1] = pal[v1 * 4 + 1];
				B[px * 4 + 2] = pal[v1 * 4 + 2];
				B[px * 4 + 3] = pal[v1 * 4 + 3];
				px++;
				B[px * 4 + 0] = pal[v2 * 4 + 0];
				B[px * 4 + 1] = pal[v2 * 4 + 1];
				B[px * 4 + 2] = pal[v2 * 4 + 2];
				B[px * 4 + 3] = pal[v2 * 4 + 3];
			}
		}
	}

	return new ui::Image(c);
}
