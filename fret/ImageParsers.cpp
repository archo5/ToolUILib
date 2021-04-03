
#include "pch.h"
#include "ImageParsers.h"


static uint32_t divup(uint32_t x, uint32_t d)
{
	return (x + d - 1) / d;
}


static uint32_t RGB5A1_to_RGBA8(uint16_t v)
{
	uint32_t r = ((v >> 0) & 0x1f) << 3;
	uint32_t g = ((v >> 5) & 0x1f) << 3;
	uint32_t b = ((v >> 10) & 0x1f) << 3;
	uint32_t a = (v & 0x8000) ? 255 : 0;
	return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

static uint32_t RGB565_to_RGBA8(uint16_t v)
{
	uint32_t b = ((v >> 0) & 0x1f) << 3;
	uint32_t g = ((v >> 5) & 0x3f) << 2;
	uint32_t r = ((v >> 11) & 0x1f) << 3;
	return (r << 0) | (g << 8) | (b << 16) | (0xffUL << 24);
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
	ui::StringView category;
	ui::StringView name;
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

static void ReadImage_G8(ReadImageIO& io, const ImageInfo& info)
{
	if (info.width > 4096)
	{
		io.error = true;
		return;
	}

	uint8_t tmp[4096];
	for (uint32_t y = 0; y < info.height; y++)
	{
		io.ds->Read(info.offImg + y * info.width, info.width, tmp);
		for (uint32_t x = 0; x < info.width; x++)
			io.pixels[y * info.width + x] = tmp[x] | (tmp[x] << 8) | (tmp[x] << 16) | 0xff000000UL;
	}
}

static void ReadImage_G1(ReadImageIO& io, const ImageInfo& info)
{
	if (info.width > 4096)
	{
		io.error = true;
		return;
	}

	int w = divup(info.width, 8);
	uint8_t tmp[4096 / 8];
	for (uint32_t y = 0; y < info.height; y++)
	{
		io.ds->Read(info.offImg + y * w, w, tmp);
		for (uint32_t x = 0; x < info.width; x++)
		{
			bool set = (tmp[x / 8] & (1 << (x % 8))) != 0;
			io.pixels[y * info.width + x] = set ? 0xffffffffUL : 0xff000000UL;
		}
	}
}

static void ReadImage_8BPP_RGBA8(ReadImageIO& io, const ImageInfo& info)
{
	if (info.width > 4096)
	{
		io.error = true;
		return;
	}

	uint32_t pal[256];
	io.ds->Read(info.offPal, 256 * 4, pal);

	uint8_t line[4096];
	for (uint32_t y = 0; y < info.height; y++)
	{
		io.ds->Read(info.offImg + info.width * y, info.width, line);
		for (uint32_t x = 0; x < info.width; x++)
		{
			io.pixels[x + y * info.width] = pal[line[x]];
		}
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

struct DXT1Block
{
	uint16_t c0, c1;
	uint32_t pixels;

	void GenerateColors(uint32_t c[4])
	{
		uint8_t r0 = (c[0] >> 0) & 0xff;
		uint8_t g0 = (c[0] >> 8) & 0xff;
		uint8_t b0 = (c[0] >> 16) & 0xff;
		uint8_t r1 = (c[1] >> 0) & 0xff;
		uint8_t g1 = (c[1] >> 8) & 0xff;
		uint8_t b1 = (c[1] >> 16) & 0xff;
		uint8_t r2 = r0 * 2 / 3 + r1 / 3;
		uint8_t g2 = g0 * 2 / 3 + g1 / 3;
		uint8_t b2 = b0 * 2 / 3 + b1 / 3;
		uint8_t r3 = r0 / 3 + r1 * 2 / 3;
		uint8_t g3 = g0 / 3 + g1 * 2 / 3;
		uint8_t b3 = b0 / 3 + b1 * 2 / 3;
		c[2] = r2 | (g2 << 8) | (b2 << 16) | 0xff000000UL;
		c[3] = r3 | (g3 << 8) | (b3 << 16) | 0xff000000UL;
	}
	uint32_t GetColor(int x, int y)
	{
		uint32_t c[4] = { RGB565_to_RGBA8(c0), RGB565_to_RGBA8(c1) };
		if (c0 > c1)
		{
			GenerateColors(c);
		}
		else
		{
			c[2] = (c[0] >> 1) + (c[1] >> 1);
			c[3] = 0;
		}

		int at = y * 4 + x;
		int idx = (pixels >> (at * 2)) & 0x3;
		return c[idx];
	}
	uint32_t GetColorDXT3(int x, int y, uint8_t alpha)
	{
		uint32_t c[4] = { RGB565_to_RGBA8(c0), RGB565_to_RGBA8(c1) };
		GenerateColors(c);

		int at = y * 4 + x;
		int idx = (pixels >> (at * 2)) & 0x3;
		auto cc = c[idx];
		return (cc & 0xffffff) | (uint32_t(alpha) << 24);
	}
};

static const uint8_t g_DXT3AlphaTable[16] =
{
	255 * 0 / 15,
	255 * 1 / 15,
	255 * 2 / 15,
	255 * 3 / 15,
	255 * 4 / 15,
	255 * 5 / 15,
	255 * 6 / 15,
	255 * 7 / 15,
	255 * 8 / 15,
	255 * 9 / 15,
	255 * 10 / 15,
	255 * 11 / 15,
	255 * 12 / 15,
	255 * 13 / 15,
	255 * 14 / 15,
	255 * 15 / 15,
};

struct DXT3AlphaBlock
{
	uint64_t pixels;

	uint8_t GetAlpha(int x, int y)
	{
		int at = y * 4 + x;
		int idx = (pixels >> (at * 4)) & 0xf;
		return g_DXT3AlphaTable[idx];
	}
};

static void ReadImage_DXT1(ReadImageIO& io, const ImageInfo& info)
{
	uint32_t nbx = divup(info.width, 4);
	uint32_t nby = divup(info.height, 4);

	uint64_t at = info.offImg;
	for (uint32_t by = 0; by < nby; by++)
	{
		for (uint32_t bx = 0; bx < nbx; bx++)
		{
			DXT1Block block;
			io.ds->Read(at, sizeof(block), &block);
			at += sizeof(block);

			for (uint32_t y = by * 4; y < std::min((by + 1) * 4, info.height); y++)
			{
				for (uint32_t x = bx * 4; x < std::min((bx + 1) * 4, info.width); x++)
				{
					io.pixels[x + info.width * y] = block.GetColor(x - bx * 4, y - by * 4);
				}
			}
		}
	}
}

static void ReadImage_DXT3(ReadImageIO& io, const ImageInfo& info)
{
	uint32_t nbx = divup(info.width, 4);
	uint32_t nby = divup(info.height, 4);

	uint64_t at = info.offImg;
	for (uint32_t by = 0; by < nby; by++)
	{
		for (uint32_t bx = 0; bx < nbx; bx++)
		{
			DXT1Block block;
			DXT3AlphaBlock ablock;
			io.ds->Read(at, sizeof(ablock), &ablock);
			at += sizeof(ablock);
			io.ds->Read(at, sizeof(block), &block);
			at += sizeof(block);

			for (uint32_t y = by * 4; y < std::min((by + 1) * 4, info.height); y++)
			{
				for (uint32_t x = bx * 4; x < std::min((bx + 1) * 4, info.width); x++)
				{
					uint8_t alpha = ablock.GetAlpha(x - bx * 4, y - by * 4);
					io.pixels[x + info.width * y] = block.GetColorDXT3(x - bx * 4, y - by * 4, alpha);
				}
			}
		}
	}
}


static const ImageFormat g_imageFormats[] =
{
	{ "Basic", "RGBA8", ReadImage_RGBA8 },
	{ "Basic", "RGBX8", ReadImage_RGBX8 },
	{ "Basic", "RGBo8", ReadImage_RGBo8 },
	{ "Basic", "G8", ReadImage_G8 },
	{ "Basic", "G1", ReadImage_G1 },
	{ "Basic", "8BPP_RGBA8", ReadImage_8BPP_RGBA8 },
	{ "PSX", "RGB5A1", ReadImage_RGB5A1 },
	{ "PSX", "4BPP_RGB5A1", ReadImage_4BPP_RGB5A1 },
	{ "PSX", "4BPP_RGBo8", ReadImage_4BPP_RGBo8 },
	{ "S3TC", "DXT1", ReadImage_DXT1 },
	{ "S3TC", "DXT3", ReadImage_DXT3 },
};


size_t GetImageFormatCount()
{
	return sizeof(g_imageFormats) / sizeof(g_imageFormats[0]);
}

ui::StringView GetImageFormatCategory(size_t fid)
{
	return g_imageFormats[fid].category;
}

ui::StringView GetImageFormatName(size_t fid)
{
	return g_imageFormats[fid].name;
}

ui::draw::ImageHandle CreateImageFrom(IDataSource* ds, ui::StringView fmt, const ImageInfo& info)
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

	return ui::draw::ImageCreateFromCanvas(c, ui::draw::TexFlags::Repeat | ui::draw::TexFlags::NoFilter);
}
