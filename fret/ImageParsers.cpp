
#include "pch.h"
#include "ImageParsers.h"


static uint32_t RGB555_to_RGBA32(uint16_t v)
{
	//v = (v >> 8) | (v << 8);
	uint32_t r = ((v >> 0) & 0x1f) << 3;
	uint32_t g = ((v >> 5) & 0x1f) << 3;
	uint32_t b = ((v >> 10) & 0x1f) << 3;
	uint32_t a = (v & 0x8000) ? 255 : 0;
	return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

ui::Image* CreateImageFrom(IDataSource* ds, const char* fmt, int64_t offImg, int64_t offPal, uint32_t width, uint32_t height)
{
	ui::Canvas c(width, height);
	auto* B = c.GetBytes();
	auto* P = c.GetPixels();

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
	else if (strcmp(fmt, "RGB5") == 0)
	{
		if (width > 4096)
			return nullptr;

		uint16_t tmp[4096];
		for (uint32_t y = 0; y < height; y++)
		{
			ds->Read(offImg + y * width * 2, width * 2, tmp);
			for (uint32_t x = 0; x < width; x++)
				P[y * width + x] = RGB555_to_RGBA32(tmp[x]);
		}
	}
	else if (strcmp(fmt, "4BPP_RGB5") == 0)
	{
		if (width > 4096)
			return nullptr;

		uint16_t palo[16];
		ds->Read(offPal, 32, palo);

		uint32_t palc[16];
		for (int i = 0; i < 16; i++)
			palc[i] = RGB555_to_RGBA32(palo[i]);

		uint8_t line[2048];
		for (uint32_t y = 0; y < height; y++)
		{
			ds->Read(offImg + width / 2 * y, width / 2, line);
			for (uint32_t x = 0; x < width / 2; x++)
			{
				uint8_t v1 = line[x] & 0xf;
				uint8_t v2 = line[x] >> 4;

				uint32_t px = x * 2 + y * width;
				P[px] = palc[v1];
				px++;
				P[px] = palc[v2];
			}
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
