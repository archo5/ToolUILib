
#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb_truetype.h"

#include "OpenGL.h"
#include "Render.h"


unsigned char ttf_buffer[1 << 20];
unsigned char temp_bitmap[512 * 512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GL::TexID g_fontTexture;

void InitFont()
{
	fread(ttf_buffer, 1, 1 << 20, fopen("c:/windows/fonts/segoeui.ttf", "rb"));
	stbtt_BakeFontBitmap(ttf_buffer, 0, -12.0f, temp_bitmap, 512, 512, 32, 96, cdata); // no guarantee this fits!
	g_fontTexture = GL::CreateTextureA8(temp_bitmap, 512, 512);
}

float GetTextWidth(const char* text, size_t num)
{
	float out = 0;
	for (size_t i = 0; num != SIZE_MAX ? i < num : *text; i++)
	{
		if (*text >= 32 && *text < 128)
			out += cdata[*text - 32].xadvance;
		text++;
	}
	return out;
}

float GetFontHeight()
{
	return 12;
}

void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a)
{
	// assume orthographic projection with units = screen pixels, origin at top left
	GL::SetTexture(g_fontTexture);
	GL::BatchRenderer br;
	br.Begin();
	br.SetColor(r, g, b, a);
	while (*text)
	{
		if (*text >= 32 && *text < 128)
		{
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata, 512, 512, *text - 32, &x, &y, &q, 1);//1=opengl & d3d10+,0=d3d9
			br.Quad(q.x0, q.y0, q.x1, q.y1, q.s0, q.t0, q.s1, q.t1);
		}
		++text;
	}
	br.End();
}


unsigned char* LoadTGA(const char* img, int size[2])
{
	FILE* f = fopen(img, "rb");
	char idlen = getc(f);
	assert(idlen == 0); // no id
	char cmtype = getc(f);
	assert(cmtype == 0); // no colormap
	char imgtype = getc(f);
	assert(imgtype == 2); // only uncompressed true color image supported
	fseek(f, 5, SEEK_CUR); // skip colormap

	fseek(f, 4, SEEK_CUR); // skip x/y origin
	short fsize[2];
	fread(&fsize, 2, 2, f); // x/y size
	char bpp = getc(f);
	char imgdesc = getc(f);

	// image id here?
	// colormap here?
	// image data
	size[0] = fsize[0];
	size[1] = fsize[1];
	unsigned char* out = new unsigned char[fsize[0] * fsize[1] * 4];
	for (int i = 0; i < fsize[0] * fsize[1]; i++)
	{
		out[i * 4 + 2] = getc(f);
		out[i * 4 + 1] = getc(f);
		out[i * 4 + 0] = getc(f);
		out[i * 4 + 3] = getc(f);
	}
	return out;
}

Sprite g_themeSprites[TE__COUNT] =
{
#if 1
	// button
	{ 0, 0, 32, 32, 5, 5, 5, 5 },
	{ 32, 0, 64, 32, 5, 5, 5, 5 },
	{ 64, 0, 96, 32, 5, 5, 5, 5 },
	{ 96, 0, 128, 32, 5, 5, 5, 5 },
	{ 128, 0, 160, 32, 5, 5, 5, 5 },

	// panel
	{ 160, 0, 192, 32, 6, 6, 6, 6 },

	// textbox
	{ 192, 0, 224, 32, 5, 5, 5, 5 },
	{ 224, 0, 256, 32, 5, 5, 5, 5 },

	// checkbgr
	{ 0, 32, 32, 64, 5, 5, 5, 5 },
	{ 32, 32, 64, 64, 5, 5, 5, 5 },
	{ 64, 32, 96, 64, 5, 5, 5, 5 },
	{ 96, 32, 128, 64, 5, 5, 5, 5 },

	// checkmarks
	{ 128, 32, 160, 64, 5, 5, 5, 5 },
	{ 160, 32, 192, 64, 5, 5, 5, 5 },
	{ 192, 32, 224, 64, 5, 5, 5, 5 },
	{ 224, 32, 256, 64, 5, 5, 5, 5 },

	// radiobgr
	{ 0, 64, 32, 96, 0, 0, 0, 0 },
	{ 32, 64, 64, 96, 0, 0, 0, 0 },
	{ 64, 64, 96, 96, 0, 0, 0, 0 },
	{ 96, 64, 128, 96, 0, 0, 0, 0 },

	// radiomark
	{ 128, 64, 160, 96, 0, 0, 0, 0 },
	{ 160, 64, 192, 96, 0, 0, 0, 0 },

	// selector
	{ 192, 64, 224, 96, 0, 0, 0, 0 },
	{ 224, 64, 240, 80, 0, 0, 0, 0 },
	{ 240, 64, 252, 76, 0, 0, 0, 0 },
	// outline
	{ 224, 80, 240, 96, 4, 4, 4, 4 },

	// tab
	{ 0, 96, 32, 128, 5, 5, 5, 5 },
	{ 32, 96, 64, 128, 5, 5, 5, 5 },
	{ 64, 96, 96, 128, 5, 5, 5, 5 },
	{ 96, 96, 128, 128, 5, 5, 5, 5 },

	// treetick
	{ 128, 96, 160, 128, 0, 0, 0, 0 },
	{ 160, 96, 192, 128, 0, 0, 0, 0 },
	{ 192, 96, 224, 128, 0, 0, 0, 0 },
	{ 224, 96, 256, 128, 0, 0, 0, 0 },

#else
	{ 0, 0, 64, 64, 5, 5, 5, 5 },
	{ 64, 0, 128, 64, 5, 5, 5, 5 },
	{ 128, 0, 192, 64, 5, 5, 5, 5 },

	{ 192, 0, 224, 32, 5, 5, 5, 5 },
	{ 224, 0, 256, 32, 5, 5, 5, 5 },
	{ 192, 32, 224, 64, 5, 5, 5, 5 },
	{ 224, 32, 256, 64, 5, 5, 5, 5 },
#endif
};

GL::TexID g_themeTexture;
int g_themeWidth, g_themeHeight;

void InitTheme()
{
	int size[2];
	auto* data = LoadTGA("gui-theme2.tga", size);
	g_themeTexture = GL::CreateTextureRGBA8(data, size[0], size[1]);
	g_themeWidth = size[0];
	g_themeHeight = size[1];
	delete[] data;
}

static void Draw9Slice(GL::BatchRenderer& br, const Sprite& s, int w, int h, float ox0, float oy0, float ox1, float oy1)
{
	float ifw = 1.0f / w, ifh = 1.0f / h;
	int s_ix0 = s.ox0 + s.bx0, s_iy0 = s.oy0 + s.by0, s_ix1 = s.ox1 - s.bx1, s_iy1 = s.oy1 - s.by1;
	float tox0 = s.ox0 * ifw, toy0 = s.oy0 * ifh, tox1 = s.ox1 * ifw, toy1 = s.oy1 * ifh;
	float tix0 = s_ix0 * ifw, tiy0 = s_iy0 * ifh, tix1 = s_ix1 * ifw, tiy1 = s_iy1 * ifh;
	float ix0 = ox0 + s.bx0;
	float iy0 = oy0 + s.by0;
	float ix1 = ox1 - s.bx1;
	float iy1 = oy1 - s.by1;

	br.Quad(ox0, oy0, ix0, iy0, tox0, toy0, tix0, tiy0);
	br.Quad(ix0, oy0, ix1, iy0, tix0, toy0, tix1, tiy0);
	br.Quad(ix1, oy0, ox1, iy0, tix1, toy0, tox1, tiy0);

	br.Quad(ox0, iy0, ix0, iy1, tox0, tiy0, tix0, tiy1);
	br.Quad(ix0, iy0, ix1, iy1, tix0, tiy0, tix1, tiy1);
	br.Quad(ix1, iy0, ox1, iy1, tix1, tiy0, tox1, tiy1);

	br.Quad(ox0, iy1, ix0, oy1, tox0, tiy1, tix0, toy1);
	br.Quad(ix0, iy1, ix1, oy1, tix0, tiy1, tix1, toy1);
	br.Quad(ix1, iy1, ox1, oy1, tix1, tiy1, tox1, toy1);
}

AABB<float> GetThemeElementBorderWidths(EThemeElement e)
{
	auto& s = g_themeSprites[e];
	return { float(s.bx0), float(s.by0), float(s.bx1), float(s.by1) };
}

void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1)
{
	GL::SetTexture(g_themeTexture);
	GL::BatchRenderer br;
	br.Begin();
	br.SetColor(1, 1, 1);
	Draw9Slice(br, g_themeSprites[e], g_themeWidth, g_themeHeight, x0, y0, x1, y1);
	br.End();
}
