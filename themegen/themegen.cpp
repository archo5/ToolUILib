
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#pragma warning (disable:4996)

#define FINLINE __forceinline

struct RRect
{
	int x0, y0, x1, y1, corner00, corner01, corner10, corner11;

	RRect& shrink(int by)
	{
		x0 += by;
		y0 += by;
		x1 -= by;
		y1 -= by;
		return *this;
	}
	RRect getshrunk(int by)
	{
		RRect copy = *this;
		return copy.shrink(by);
	}
	RRect& offset(int x, int y)
	{
		x0 += x;
		y0 += y;
		x1 += x;
		y1 += y;
		return *this;
	}
	void setcorner(int c)
	{
		corner00 = corner01 = corner10 = corner11 = c;
	}
	RRect& diagflip()
	{
		int tmp;
		tmp = x0; x0 = y0; y0 = tmp;
		tmp = x1; x1 = y1; y1 = tmp;
		return *this;
	}
};
RRect combine(const RRect& a, const RRect& b)
{
	return
	{
		a.x0 < b.x0 ? a.x0 : b.x0,
		a.y0 < b.y0 ? a.y0 : b.y0,
		a.x1 > b.x1 ? a.x1 : b.x1,
		a.y1 > b.y1 ? a.y1 : b.y1,
		a.corner00 < b.corner00 ? a.corner00 : b.corner00,
		a.corner01 < b.corner01 ? a.corner01 : b.corner01,
		a.corner10 < b.corner10 ? a.corner10 : b.corner10,
		a.corner11 < b.corner11 ? a.corner11 : b.corner11,
	};
}

struct Color
{
	FINLINE Color() {}
	FINLINE Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) {}

	Color operator + (const Color& o) const { return Color(r + o.r, g + o.g, b + o.b, a + o.a); }

	uint8_t r, g, b, a;
};
uint8_t dechex(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}
uint8_t hex2byte(const char* hex)
{
	return (dechex(hex[0]) << 4) | dechex(hex[1]);
}
Color hexcol(const char* hex, uint8_t a = 255)
{
	return Color{ hex2byte(hex), hex2byte(hex + 2), hex2byte(hex + 4), hex[6] ? hex2byte(hex + 6) : a };
}

Color colorset(Color a, Color b, int amt)
{
	int ca = amt;
	return Color
	(
		(a.r * (255 - ca) + b.r * ca) / 255,
		(a.g * (255 - ca) + b.g * ca) / 255,
		(a.b * (255 - ca) + b.b * ca) / 255,
		(a.a * (255 - ca) + b.a * ca) / 255
	);
}

Color alphablend(Color a, Color b)
{
	int ca = b.a > 0 ? b.a + (255 - b.a) * (255 - a.a) / 255 : 0;
	return Color
	(
		(a.r * (255 - ca) + b.r * ca) / 255,
		(a.g * (255 - ca) + b.g * ca) / 255,
		(a.b * (255 - ca) + b.b * ca) / 255,
		a.a + b.a * (255 - a.a) / 255
	);
}

int lerp(int a, int b, float t)
{
	return (int)roundf(a * (1 - t) + b * t);
}
Color lerp(Color a, Color b, float t)
{
	return Color
	(
		lerp(a.r, b.r, t),
		lerp(a.g, b.g, t),
		lerp(a.b, b.b, t),
		lerp(a.a, b.a, t)
	);
}



constexpr int WIDTH = 256;
constexpr int HEIGHT = 128;
Color theme[WIDTH * HEIGHT];



int aacircle(int x, int y, int cx, int cy, int radius)
{
	int r2 = radius * radius;
	int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
	float q = float(radius) - sqrtf(float(d2)) + 1.0f;
	return q < 0 ? 0 : q > 1 ? 255 : int(q * 255);
}

int dorectmask(int x, int y, const RRect& rr)
{
	if (x < rr.x0 || x > rr.x1 || y < rr.y0 || y > rr.y1)
		return 0;
	if (x < rr.x0 + rr.corner00 &&
		y < rr.y0 + rr.corner00)
		return aacircle(x, y, rr.x0 + rr.corner00, rr.y0 + rr.corner00, rr.corner00);
	if (x > rr.x1 - rr.corner10 &&
		y < rr.y0 + rr.corner10)
		return aacircle(x, y, rr.x1 - rr.corner10, rr.y0 + rr.corner10, rr.corner10);
	if (x < rr.x0 + rr.corner01 &&
		y > rr.y1 - rr.corner01)
		return aacircle(x, y, rr.x0 + rr.corner01, rr.y1 - rr.corner01, rr.corner01);
	if (x > rr.x1 - rr.corner11 &&
		y > rr.y1 - rr.corner11)
		return aacircle(x, y, rr.x1 - rr.corner11, rr.y1 - rr.corner11, rr.corner11);
	return 255;
}

struct onecolor
{
	Color operator () (int x, int y) const { return color; }
	Color color;
};

struct gradvert
{
	Color operator () (int x, int y) const
	{
		return lerp(ctop, cbtm, (y - ytop) / float(ybtm - ytop));
	}
	Color ctop;
	int ytop;
	Color cbtm;
	int ybtm;
};

struct gradrad
{
	Color operator () (int x, int y) const
	{
		float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy)) / radius;
		if (d > 1)
			return c1;
		return lerp(c0, c1, d);
	}
	float cx;
	float cy;
	float radius;
	Color c0;
	Color c1;
};

struct rectmask
{
	int operator () (int x, int y) const { return dorectmask(x, y, rect); }
	RRect getrect() const { return rect; }
	RRect rect;
};

struct diagrectmask
{
	int operator () (int x, int y) const
	{
		if (x - y > topx - topy) return 0;
		if (x - y < btmx - btmy) return 0;
		if (x + y < topx + topy) return 0;
		if (x + y > btmx + btmy) return 0;
		return 255;
	}
	RRect getrect() const
	{
		return RRect
		{
			0,
			(int) floorf(topy),
			WIDTH - 1,
			(int) ceilf(btmy),
			0,
		};
	}
	diagrectmask& hpo()
	{
		topx -= 0.5f;
		topy -= 0.5f;
		btmx -= 0.5f;
		btmy -= 0.5f;
		return *this;
	}
	float topx, topy;
	float btmx, btmy;
};

template <class TMask>
struct diagflipmask_
{
	int operator () (int x, int y) const { return flip ? mask(y, x) : mask(x, y); }
	RRect getrect() const { return flip ? mask.getrect().diagflip() : mask.getrect(); }
	TMask mask;
	bool flip;
};
template <class TMask> diagflipmask_<TMask> diagflipmask(TMask mask, bool flip) { return { mask, flip }; }

template <class TMask>
struct offsetmask_
{
	int operator () (int x, int y) const { return mask(x - ox, y - oy); }
	RRect getrect() const { return mask.getrect().offset(ox, oy); }
	TMask mask;
	int ox, oy;
};
template <class TMask> offsetmask_<TMask> offsetmask(TMask mask, int ox, int oy) { return { mask, ox, oy }; }

template <class TMaskA, class TMaskB>
struct multmask_
{
	int operator () (int x, int y) const { return maskA(x, y) * maskB(x, y) / 255; }
	RRect getrect() const { return combine(maskA.getrect(), maskB.getrect()); }
	TMaskA maskA;
	TMaskB maskB;
};
template <class TMaskA, class TMaskB> multmask_<TMaskA, TMaskB> multmask(TMaskA maskA, TMaskB maskB) { return { maskA, maskB }; }

template <class TMask>
struct invmask_
{
	int operator () (int x, int y) const { return 255 - mask(x, y); }
	RRect getrect() const { return mask.getrect(); }
	TMask mask;
};
template <class TMask> invmask_<TMask> invmask(TMask mask) { return { mask }; }

template <class TColor, class TMask> void rasterize(TColor color, TMask mask, bool blend = true)
{
	auto rect = mask.getrect();
	if (rect.x0 < 0) rect.x0 = 0;
	if (rect.y0 < 0) rect.y0 = 0;
	if (rect.x1 >= WIDTH) rect.x1 = WIDTH - 1;
	if (rect.y1 >= HEIGHT) rect.y1 = HEIGHT - 1;
	for (int y = rect.y0; y <= rect.y1; y++)
	{
		for (int x = rect.x0; x <= rect.x1; x++)
		{
			Color c = color(x, y);
			if (blend)
				c.a = c.a * mask(x, y) / 255;
			Color pc = theme[x + y * WIDTH];
			theme[x + y * WIDTH] = blend ? alphablend(pc, c) : colorset(pc, c, mask(x, y));
		}
	}
}

void save(const char* path)
{
	FILE* f = fopen(path, "wb");
	putc(0, f); // id
	putc(0, f); // cmtype
	putc(2, f); // format
	putc(0, f); // null colormap
	putc(0, f);
	putc(0, f);
	putc(0, f);
	putc(0, f);
	short originsize[4] = { 0, HEIGHT, WIDTH, HEIGHT };
	fwrite(originsize, 2, 4, f);
	putc(32, f); // bpp
	putc(0x28, f); // imgdesc (direction, alpha depth)
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			const auto& c = theme[(/*HEIGHT - 1 - */y) * WIDTH + x];
			putc(c.b, f);
			putc(c.g, f);
			putc(c.r, f);
			putc(c.a, f);
		}
	}
	fclose(f);
}

enum State
{
	Normal = 1 << 0,
	Hover = 1 << 1,
	Down = 1 << 2,
	Disabled = 1 << 3,
};
State operator | (State a, State b)
{
	return (State)(int(a) | int(b));
}

const Color bgcolor = hexcol("202020");

void theme_panel(RRect rr)
{
	rr.setcorner(2);
	rasterize(onecolor{ hexcol("ffffff", 30) }, multmask(rectmask{ rr }, invmask(rectmask{ rr.getshrunk(1) })), false);
	rr.shrink(1);
	rasterize(onecolor{ hexcol("000000", 80) }, multmask(rectmask{ rr }, invmask(rectmask{ rr.getshrunk(1) })), false);
	rr.shrink(1);
	rasterize(onecolor{ hexcol("ffffff", 30) }, multmask(rectmask{ rr }, invmask(rectmask{ rr.getshrunk(1) })), false);
	rr.shrink(1);
	//rasterize(onecolor{ bgcolor }, rectmask{ rr });
}

void theme_button(RRect rr, State state)
{
	rr.setcorner(1);
	rasterize(onecolor{ hexcol("1a1a1a") }, rectmask{ rr });
	rr.shrink(1);
	rasterize(onecolor{ state & Disabled ? hexcol("202020") : state == Down ? hexcol("303030") : hexcol("4d4d4d") }, rectmask{ rr });
	rr.shrink(1);
	if (state == Disabled)
		rasterize(gradvert{ hexcol("252525"), rr.y0, hexcol("222222"), rr.y1 }, rectmask{ rr });
	else if (state == Down)
		rasterize(gradvert{ hexcol("292929"), rr.y0, hexcol("2b2b2b"), rr.y1 }, rectmask{ rr });
	else if (state == (Disabled | Down))
		rasterize(gradvert{ hexcol("202020"), rr.y0, hexcol("242424"), rr.y1 }, rectmask{ rr });
	else if (state == Hover)
		rasterize(gradvert{ hexcol("3a3a3a"), rr.y0, hexcol("2b2b2b"), rr.y1 }, rectmask{ rr });
	else
		rasterize(gradvert{ hexcol("343434"), rr.y0, hexcol("272727"), rr.y1 }, rectmask{ rr });
}

void theme_textbox(RRect rr, State state)
{
	rr.setcorner(3);
	rasterize(onecolor{ state == Disabled ? hexcol("202020") : hexcol("2d2d2d") }, rectmask{ rr });
	rr.shrink(1);
	rasterize(onecolor{ hexcol("1a1a1a") }, rectmask{ rr });
	rr.shrink(1);
	rasterize(onecolor{ hexcol("1e1e1e") }, rectmask{ rr });
}

void theme_checkbox(RRect rr, bool radio, State state)
{
	rr.setcorner(1);
	if (radio)
		rr.setcorner((rr.x1 - rr.x0) / 2);
	rasterize(onecolor{ hexcol("1a1a1a") }, rectmask{ rr });
	rr.shrink(1);
#if 0
	if (radio)
		rr.corner = (rr.x1 - rr.x0) / 2;
	rasterize(onecolor{ hexcol("4d4d4d") }, rectmask{ rr });
	rr.shrink(1);
#endif
	if (radio)
		rr.setcorner((rr.x1 - rr.x0) / 2);
	if (state == Disabled)
		rasterize(gradrad{ (rr.x0 + rr.x1) / 2.f, (rr.y0 + rr.y1) / 2.f, (rr.x1 - rr.x0) / 2.f, hexcol("222222"), hexcol("252525") }, rectmask{ rr });
	else if (state == Down)
		rasterize(gradrad{ (rr.x0 + rr.x1) / 2.f, (rr.y0 + rr.y1) / 2.f, (rr.x1 - rr.x0) / 2.f, hexcol("141414"), hexcol("1d1d1d") }, rectmask{ rr });
	else if (state == Hover)
		rasterize(gradrad{ (rr.x0 + rr.x1) / 2.f, (rr.y0 + rr.y1) / 2.f, (rr.x1 - rr.x0) / 2.f, hexcol("222222"), hexcol("2d2d2d") }, rectmask{ rr });
	else
		rasterize(gradrad{ (rr.x0 + rr.x1) / 2.f, (rr.y0 + rr.y1) / 2.f, (rr.x1 - rr.x0) / 2.f, hexcol("171717"), hexcol("232323") }, rectmask{ rr });
	rasterize(onecolor{ hexcol("1a1a1a", state == Disabled ? 30 : 127) }, multmask(rectmask{ rr }, invmask(rectmask{ rr.getshrunk(1) })));
}

void blur5(const RRect& rr)
{
	Color theme2[WIDTH * HEIGHT];
	memcpy(theme2, theme, sizeof(theme2));
	// horizontal
	for (int y = rr.y0; y <= rr.y1; y++)
	{
		for (int x = rr.x0; x <= rr.x1; x++)
		{
			theme[x + y * WIDTH].r = (theme2[x + y * WIDTH].r * 7 +
				(theme2[x - 1 + y * WIDTH].r + theme2[x + 1 + y * WIDTH].r) * 4 +
				(theme2[x - 2 + y * WIDTH].r + theme2[x + 2 + y * WIDTH].r) * 1) / (1 + 4 + 7 + 4 + 1);
			theme[x + y * WIDTH].g = (theme2[x + y * WIDTH].g * 7 +
				(theme2[x - 1 + y * WIDTH].g + theme2[x + 1 + y * WIDTH].g) * 4 +
				(theme2[x - 2 + y * WIDTH].g + theme2[x + 2 + y * WIDTH].g) * 1) / (1 + 4 + 7 + 4 + 1);
			theme[x + y * WIDTH].b = (theme2[x + y * WIDTH].b * 7 +
				(theme2[x - 1 + y * WIDTH].b + theme2[x + 1 + y * WIDTH].b) * 4 +
				(theme2[x - 2 + y * WIDTH].b + theme2[x + 2 + y * WIDTH].b) * 1) / (1 + 4 + 7 + 4 + 1);
			theme[x + y * WIDTH].a = (theme2[x + y * WIDTH].a * 7 +
				(theme2[x - 1 + y * WIDTH].a + theme2[x + 1 + y * WIDTH].a) * 4 +
				(theme2[x - 2 + y * WIDTH].a + theme2[x + 2 + y * WIDTH].a) * 1) / (1 + 4 + 7 + 4 + 1);
		}
	}
	memcpy(theme2, theme, sizeof(theme2));
	// vertical
	for (int y = rr.y0; y <= rr.y1; y++)
	{
		for (int x = rr.x0; x <= rr.x1; x++)
		{
			theme[x + y * WIDTH].r = (theme2[x + y * WIDTH].r * 7 +
				(theme2[x + (y - 1) * WIDTH].r + theme2[x + (y + 1) * WIDTH].r) * 4 +
				(theme2[x + (y - 2) * WIDTH].r + theme2[x + (y + 2) * WIDTH].r) * 1) / (1 + 4 + 7 + 4 + 1);
			theme[x + y * WIDTH].g = (theme2[x + y * WIDTH].g * 7 +
				(theme2[x + (y - 1) * WIDTH].g + theme2[x + (y + 1) * WIDTH].g) * 4 +
				(theme2[x + (y - 2) * WIDTH].g + theme2[x + (y + 2) * WIDTH].g) * 1) / (1 + 4 + 7 + 4 + 1);
			theme[x + y * WIDTH].b = (theme2[x + y * WIDTH].b * 7 +
				(theme2[x + (y - 1) * WIDTH].b + theme2[x + (y + 1) * WIDTH].b) * 4 +
				(theme2[x + (y - 2) * WIDTH].b + theme2[x + (y + 2) * WIDTH].b) * 1) / (1 + 4 + 7 + 4 + 1);
			theme[x + y * WIDTH].a = (theme2[x + y * WIDTH].a * 7 +
				(theme2[x + (y - 1) * WIDTH].a + theme2[x + (y + 1) * WIDTH].a) * 4 +
				(theme2[x + (y - 2) * WIDTH].a + theme2[x + (y + 2) * WIDTH].a) * 1) / (1 + 4 + 7 + 4 + 1);
		}
	}
}

const Color col_tick = hexcol("cd1a00");
const Color col_tickdis = hexcol("555555");
void theme_cbtick(int x, int y, State state)
{
	auto mask1 = diagrectmask{ 232.5f - 224 + x, 45.5f - 32 + y, 238.f - 224 + x, 55.f - 32 + y }.hpo();
	auto mask2 = diagrectmask{ 247.5f - 224 + x, 41.5f - 32 + y, 238.f - 224 + x, 55.f - 32 + y }.hpo();
	rasterize(onecolor{ hexcol("000000") }, mask1);
	rasterize(onecolor{ hexcol("000000") }, mask2);
	blur5(RRect{ x + 3, y + 3, x + 32 - 3, y + 32 - 3 });

	auto col = state == Disabled ? col_tickdis : col_tick;
	rasterize(onecolor{ col }, mask1);
	rasterize(onecolor{ col }, mask2);
}

void theme_cbbox(int x, int y, State state)
{
	RRect rr = RRect{ x, y, x + 31, y + 31 }.shrink(9);
	rasterize(onecolor{ hexcol("000000") }, rectmask{ rr });
	blur5(RRect{ x + 3, y + 3, x + 32 - 3, y + 32 - 3 });
	rasterize(onecolor{ state == Disabled ? col_tickdis : col_tick }, rectmask{ rr });
}

void theme_radiotick(int x, int y, State state)
{
	RRect rr = RRect{ x, y, x + 31, y + 31 }.shrink(9);
	rr.setcorner((rr.x1 - rr.x0) / 2);
	rasterize(onecolor{ hexcol("000000") }, rectmask{ rr });
	blur5(RRect{ x + 3, y + 3, x + 32 - 3, y + 32 - 3 });

	rasterize(onecolor{ state == Disabled ? col_tickdis : col_tick }, rectmask{ rr });
}

void theme_selector(RRect rr)
{
	rr.setcorner((rr.x1 - rr.x0) / 2);
	rasterize(onecolor{ hexcol("000000") }, rectmask{ rr });
	rr.shrink(1);
	rr.setcorner((rr.x1 - rr.x0) / 2);
	rasterize(onecolor{ hexcol("ffffff") }, rectmask{ rr });
	rr.shrink(1);
	rr.setcorner((rr.x1 - rr.x0) / 2);
	rasterize(onecolor{ hexcol("000000") }, rectmask{ rr });
	rr.shrink(1);
	rr.setcorner((rr.x1 - rr.x0) / 2);
	rasterize(onecolor{ hexcol("000000", 0) }, rectmask{ rr }, false);
}

void theme_treetick(int x, int y, State state, bool open)
{
	auto bmask1 = diagflipmask(offsetmask(diagrectmask{ 8.5f, 13.5f, 14.f, 23.f }.hpo(), 2, -1), !open);
	auto bmask2 = diagflipmask(offsetmask(diagrectmask{ 19.5f, 13.5f, 14.f, 23.f }.hpo(), 2, -1), !open);
	auto mask1 = offsetmask(bmask1, x, y);
	auto mask2 = offsetmask(bmask2, x, y);
	rasterize(onecolor{ hexcol("000000") }, mask1);
	rasterize(onecolor{ hexcol("000000") }, mask2);
	blur5(RRect{ x + 3, y + 3, x + 32 - 3, y + 32 - 3 });

	auto col = state == Hover ? hexcol("cd1a00") : hexcol("999999");
	rasterize(onecolor{ col }, mask1);
	rasterize(onecolor{ col }, mask2);
}

void theme_tab(int x, int y, State state, bool tab)
{
	RRect rr{ x, y, x + 31, y + 31 };
	if (tab && state != Down)
		rr.y0++;
	//rr.shrink(1);
	rr.setcorner(1);
	if (tab)
	{
		rr.corner01 = rr.corner11 = 0;
		rr.y1--;
	}
	rasterize(onecolor{ hexcol("1a1a1a") }, rectmask{ rr });
	rr.shrink(1);
	if (tab)
		rr.y1 += 2;
	rasterize(onecolor{ state == Down ? hexcol("606060") : hexcol("6d6d6d") }, rectmask{ rr });
	rr.shrink(1);
	if (tab)
		rr.y1++;
	if (!tab)
		rasterize(onecolor{ hexcol("4b4b4b") }, rectmask{ rr });
	else if (state == Down)
		rasterize(gradvert{ hexcol("494949"), rr.y0, hexcol("4b4b4b"), rr.y1 }, rectmask{ rr });
	else if (state == Hover)
		rasterize(gradvert{ hexcol("5a4a4a"), rr.y0, hexcol("4b3b3b"), rr.y1 }, rectmask{ rr });
	else
		rasterize(gradvert{ hexcol("444444"), rr.y0, hexcol("373737"), rr.y1 }, rectmask{ rr });
}

int main()
{
#if 0
	for (int y = 0; y < 30; y++)
	{
		for (int x = 0; x < 30; x++)
		{
			const char* table = " .-+oO@#";
			//putchar(table[aacircle(x, y, 15, 15, 14) / 32]);
			putchar(table[dorectmask(x, y, { 1, 1, 29, 29, 5 }) / 32]);
			//theme[x + y * 256].a = dorectmask(x, y, { 1, 1, 29, 29, 5 });
		}
		putchar('\n');
	}
#endif

	int x, y = 0;

	x = 0;
	theme_button(RRect{ x, y, x + 31, y + 31 }.shrink(1), Normal);
	x += 32;
	theme_button(RRect{ x, y, x + 31, y + 31 }.shrink(1), Hover);
	x += 32;
	theme_button(RRect{ x, y, x + 31, y + 31 }.shrink(1), Down);
	x += 32;
	theme_button(RRect{ x, y, x + 31, y + 31 }.shrink(1), Disabled);
	x += 32;
	theme_button(RRect{ x, y, x + 31, y + 31 }.shrink(1), Disabled | Down);

	x += 32;
	theme_panel(RRect{ x, y, x + 31, y + 31 }.shrink(1));

	x += 32;
	theme_textbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), Normal);
	x += 32;
	theme_textbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), Disabled);

	x = 0;
	y = 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), false, Normal);
	x += 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), false, Hover);
	x += 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), false, Down);
	x += 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), false, Disabled);

	x += 32;
	theme_cbtick(x, y, Normal);
	x += 32;
	theme_cbtick(x, y, Disabled);

	x += 32;
	theme_cbbox(x, y, Normal);
	x += 32;
	theme_cbbox(x, y, Disabled);

	x = 0;
	y = 64;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), true, Normal);
	x += 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), true, Hover);
	x += 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), true, Down);
	x += 32;
	theme_checkbox(RRect{ x, y, x + 31, y + 31 }.shrink(1), true, Disabled);

	x += 32;
	theme_radiotick(x, y, Normal);
	x += 32;
	theme_radiotick(x, y, Disabled);

	x += 32;
	theme_selector(RRect{ x, y, x + 31, y + 31 }.shrink(1));
	x += 32;
	theme_selector(RRect{ x, y, x + 15, y + 15 }.shrink(1));
	x += 16;
	theme_selector(RRect{ x, y, x + 11, y + 11 }.shrink(1));

	x = 0;
	y = 96;
	theme_tab(x, y, Normal, true);
	x += 32;
	theme_tab(x, y, Hover, true);
	x += 32;
	theme_tab(x, y, Down, true);
	x += 32;
	theme_tab(x, y, Down, false);

	x = 128;
	theme_treetick(x, y, Normal, false);
	x += 32;
	theme_treetick(x, y, Hover, false);
	x += 32;
	theme_treetick(x, y, Normal, true);
	x += 32;
	theme_treetick(x, y, Hover, true);

	save("gui-theme2.tga");
	return 0;
}
