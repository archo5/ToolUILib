
#pragma once
#include <inttypes.h>
#include <string.h>
#include "../Core/Math.h"


namespace ui {

static uint8_t gethex(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

static uint8_t gethex(char c1, char c2)
{
	return (gethex(c1) << 4) | gethex(c2);
}

static char hexchar(uint8_t v)
{
	return "0123456789ABCDEF"[v];
}


struct Color4b
{
	static Color4b Zero() { return { 0 }; }
	static Color4b Black() { return { 0, 255 }; }
	static Color4b White() { return { 255 }; }

	Color4b() : r(255), g(255), b(255), a(255) {}
	Color4b(uint8_t f) : r(f), g(f), b(f), a(f) {}
	Color4b(uint8_t gray, uint8_t alpha) : r(gray), g(gray), b(gray), a(alpha) {}
	Color4b(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) : r(red), g(green), b(blue), a(alpha) {}

	bool operator == (const Color4b& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }

	bool IsOpaque() const { return a == 255; }
	Color4b GetOpaque() const { return { r, g, b, 255 }; }

	uint8_t r, g, b, a;
};


struct Color4f
{
	static Color4f Zero() { return { 0 }; }
	static Color4f Black() { return { 0, 1 }; }
	static Color4f White() { return { 1 }; }
	static Color4f Hex(const char* hex, float a = 1)
	{
		char ntx[6] = {}; // copy hex with null termination to ignore garbage after the first null byte
		strncpy(ntx, hex, 6);
		return { gethex(ntx[0], ntx[1]) / 255.f, gethex(ntx[2], ntx[3]) / 255.f, gethex(ntx[4], ntx[5]) / 255.f, a };
	}
	static Color4f HSV(float h, float s, float v, float a = 1)
	{
		h = fmodf(h, 1);
		h *= 6;
		float c = v * s;
		float x = c * (1 - fabsf(fmodf(h, 2) - 1));
		float m = v - c;
		if (h < 1) return { c + m, x + m, m, a };
		if (h < 2) return { x + m, c + m, m, a };
		if (h < 3) return { m, c + m, x + m, a };
		if (h < 4) return { m, x + m, c + m, a };
		if (h < 5) return { x + m, m, c + m, a };
		return { c + m, m, x + m, a };
	}
	void GetHex(char buf[7])
	{
		uint8_t r = GetRed8(), g = GetGreen8(), b = GetBlue8();
		buf[0] = hexchar(r >> 4);
		buf[1] = hexchar(r & 0xf);
		buf[2] = hexchar(g >> 4);
		buf[3] = hexchar(g & 0xf);
		buf[4] = hexchar(b >> 4);
		buf[5] = hexchar(b & 0xf);
	}
	float GetHue()
	{
		float maxv = GetValue();
		float minv = min(r, min(g, b));
		float c = maxv - minv;
		if (c == 0)
			return 0;
		if (maxv == r)
			return fmodf(((g - b) / c) / 6 + 1, 1);
		if (maxv == g)
			return (2 + (b - r) / c) / 6;
		return (4 + (r - g) / c) / 6;
	}
	float GetSaturation()
	{
		float maxv = GetValue();
		float minv = min(r, min(g, b));
		float c = maxv - minv;
		return maxv == 0 ? 0 : c / maxv;
	}
	float GetValue()
	{
		return max(r, max(g, b));
	}

	Color4f() : r(1), g(1), b(1), a(1) {}
	Color4f(float f) : r(f), g(f), b(f), a(f) {}
	Color4f(float gray, float alpha) : r(gray), g(gray), b(gray), a(alpha) {}
	Color4f(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) {}
	Color4f(const Color4b& c) : r(c.r / 255.f), g(c.g / 255.f), b(c.b / 255.f), a(c.a / 255.f) {}

	bool operator == (const Color4f& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }

	void BlendOver(const Color4f& c)
	{
		float ca = c.a > 0 ? c.a + (1 - c.a) * (1 - a) : 0;
		r = lerp(r, c.r, ca);
		g = lerp(g, c.g, ca);
		b = lerp(b, c.b, ca);
		a = lerp(a, 1, c.a);
	}
	bool IsOpaque() const { return fabsf(a - 1) < 0.0001f; }
	Color4f GetOpaque() const { return { r, g, b, 1 }; }

	operator Color4b() const { return { GetRed8(), GetGreen8(), GetBlue8(), GetAlpha8() }; }

	__forceinline uint8_t GetRed8() const { return uint8_t(clamp(r, 0.0f, 1.0f) * 255); }
	__forceinline uint8_t GetGreen8() const { return uint8_t(clamp(g, 0.0f, 1.0f) * 255); }
	__forceinline uint8_t GetBlue8() const { return uint8_t(clamp(b, 0.0f, 1.0f) * 255); }
	__forceinline uint8_t GetAlpha8() const { return uint8_t(clamp(a, 0.0f, 1.0f) * 255); }
	uint32_t GetColor32()
	{
		auto rb = GetRed8();
		auto gb = GetGreen8();
		auto bb = GetBlue8();
		auto ab = GetAlpha8();
		return (ab << 24) | (bb << 16) | (gb << 8) | rb;
	}

	float r, g, b, a;
};


struct Canvas
{
	Canvas() {}
	Canvas(uint32_t w, uint32_t h) { SetSize(w, h); }
	Canvas(const Canvas& c)
	{
		SetSize(c.GetWidth(), c.GetHeight());
		memcpy(_pixels, c._pixels, _width * _height * 4);
	}
	Canvas(Canvas&& c) : _width(c._width), _height(c._height), _pixels(c._pixels)
	{
		c._width = 0;
		c._height = 0;
		c._pixels = nullptr;
	}
	~Canvas() { delete[] _pixels; }
	Canvas& operator = (const Canvas& c)
	{
		SetSize(c.GetWidth(), c.GetHeight());
		memcpy(_pixels, c._pixels, _width * _height * 4);
		return *this;
	}
	Canvas& operator = (Canvas&& c)
	{
		delete _pixels;
		_width = c._width;
		_height = c._height;
		_pixels = c._pixels;
		c._width = 0;
		c._height = 0;
		c._pixels = nullptr;
		return *this;
	}

	uint32_t GetWidth() const { return _width; }
	uint32_t GetHeight() const { return _height; }
	size_t GetNumPixels() const { return _width * _height; }
	size_t GetSizeBytes() const { return GetNumPixels() * 4; }

	uint32_t* GetPixels() { return _pixels; }
	const uint32_t* GetPixels() const { return _pixels; }
	unsigned char* GetBytes() { return reinterpret_cast<unsigned char*>(_pixels); }
	const unsigned char* GetBytes() const { return reinterpret_cast<unsigned char*>(_pixels); }

	void SetSize(uint32_t w, uint32_t h)
	{
		if (_width == w && _height == h)
			return;
		delete[] _pixels;
		_width = w;
		_height = h;
		_pixels = new uint32_t[w * h];
	}

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t* _pixels = nullptr;
};

} // ui
