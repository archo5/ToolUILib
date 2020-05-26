
#include "pch.h"
#include "ImageParsers.h"


static uint32_t RGB5A1_to_RGBA8(uint16_t v)
{
	uint32_t r = ((v >> 0) & 0x1f) << 3;
	uint32_t g = ((v >> 5) & 0x1f) << 3;
	uint32_t b = ((v >> 10) & 0x1f) << 3;
	uint32_t a = (v & 0x8000) ? 255 : 0;
	return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}


struct ReadImageIO
{
	ui::Canvas& canvas;
	uint8_t* bytes;
	uint32_t* pixels;
	IDataSource* ds;
	bool error;
};

typedef void ReadImage(ReadImageIO& io, const ImageInfo& info);
struct ImageFormat
{
	StringView category;
	StringView name;
	ReadImage* readFunc;
};


static void ReadImage_RGBA8(ReadImageIO& io, const ImageInfo& info)
{
	io.ds->Read(info.offImg, info.width * info.height * 4, io.bytes);
}

static void ReadImage_RGBX8(ReadImageIO& io, const ImageInfo& info)
{
	io.ds->Read(info.offImg, info.width * info.height * 4, io.bytes);
	for (uint32_t px = 0; px < info.width * info.height; px++)
	{
		io.bytes[px * 4 + 3] = 0xff;
	}
}

static void ReadImage_RGBo8(ReadImageIO& io, const ImageInfo& info)
{
	io.ds->Read(info.offImg, info.width * info.height * 4, io.bytes);
	for (uint32_t px = 0; px < info.width * info.height; px++)
	{
		io.bytes[px * 4 + 3] = io.bytes[px * 4 + 3] ? 0xff : 0;
	}
}

static void ReadImage_RGB5A1(ReadImageIO& io, const ImageInfo& info)
{
	if (info.width > 4096)
	{
		io.error = true;
		return;
	}

	uint16_t tmp[4096];
	for (uint32_t y = 0; y < info.height; y++)
	{
		io.ds->Read(info.offImg + y * info.width * 2, info.width * 2, tmp);
		for (uint32_t x = 0; x < info.width; x++)
			io.pixels[y * info.width + x] = RGB5A1_to_RGBA8(tmp[x]);
	}
}

static void ReadImage_4BPP_RGB5A1(ReadImageIO& io, const ImageInfo& info)
{
	if (info.width > 4096)
	{
		io.error = true;
		return;
	}

	uint16_t palo[16];
	io.ds->Read(info.offPal, 32, palo);

	uint32_t palc[16];
	for (int i = 0; i < 16; i++)
		palc[i] = RGB5A1_to_RGBA8(palo[i]);

	uint8_t line[2048];
	for (uint32_t y = 0; y < info.height; y++)
	{
		io.ds->Read(info.offImg + info.width / 2 * y, info.width / 2, line);
		for (uint32_t x = 0; x < info.width / 2; x++)
		{
			uint8_t v1 = line[x] & 0xf;
			uint8_t v2 = line[x] >> 4;

			uint32_t px = x * 2 + y * info.width;
			io.pixels[px] = palc[v1];
			px++;
			io.pixels[px] = palc[v2];
		}
	}
}

static void ReadImage_4BPP_RGBo8(ReadImageIO& io, const ImageInfo& info)
{
	if (info.width > 4096)
	{
		io.error = true;
		return;
	}

	uint32_t pal[16 * 4];
	io.ds->Read(info.offPal, 64, pal);
	for (uint32_t i = 0; i < 16; i++)
		if (pal[i] & 0xff000000)
			pal[i] |= 0xff000000;

	uint8_t line[2048];
	for (uint32_t y = 0; y < info.height; y++)
	{
		io.ds->Read(info.offImg + info.width / 2 * y, info.width / 2, line);
		for (uint32_t x = 0; x < info.width / 2; x++)
		{
			uint8_t v1 = line[x] & 0xf;
			uint8_t v2 = line[x] >> 4;

			uint32_t px = x * 2 + y * info.width;
			io.pixels[px] = pal[v1];
			px++;
			io.pixels[px] = pal[v2];
		}
	}
}


static const ImageFormat g_imageFormats[] =
{
	{ "Basic", "RGBA8", ReadImage_RGBA8 },
	{ "Basic", "RGBX8", ReadImage_RGBX8 },
	{ "Basic", "RGBo8", ReadImage_RGBo8 },
	{ "PSX", "RGB5A1", ReadImage_RGB5A1 },
	{ "PSX", "4BPP_RGB5A1", ReadImage_4BPP_RGB5A1 },
	{ "PSX", "4BPP_RGBo8", ReadImage_4BPP_RGBo8 },
};


size_t GetImageFormatCount()
{
	return sizeof(g_imageFormats) / sizeof(g_imageFormats[0]);
}

StringView GetImageFormatCategory(size_t fid)
{
	return g_imageFormats[fid].category;
}

StringView GetImageFormatName(size_t fid)
{
	return g_imageFormats[fid].name;
}

ui::Image* CreateImageFrom(IDataSource* ds, StringView fmt, const ImageInfo& info)
{
	ui::Canvas c(info.width, info.height);
	auto* B = c.GetBytes();
	auto* P = c.GetPixels();
	ReadImageIO io = { c, c.GetBytes(), c.GetPixels(), ds, false };

	bool done = false;
	for (size_t i = 0, count = GetImageFormatCount(); i < count; i++)
	{
		if (fmt == g_imageFormats[i].name)
		{
			g_imageFormats[i].readFunc(io, info);
			done = !io.error;
			break;
		}
	}

	if (!done)
		return nullptr;

	return new ui::Image(c);
}
