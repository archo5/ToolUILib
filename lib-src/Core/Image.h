
#pragma once

#include "../Core/Math.h"
#include "../Core/ObjectIteration.h"

#include <inttypes.h>
#include <string.h>


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
	UI_FORCEINLINE static Color4b Zero() { return { 0, 0 }; }
	UI_FORCEINLINE static Color4b Black() { return { 0, 255 }; }
	UI_FORCEINLINE static Color4b White() { return { 255 }; }

	UI_FORCEINLINE Color4b(DoNotInitialize) {}
	UI_FORCEINLINE Color4b() : r(255), g(255), b(255), a(255) {}
	UI_FORCEINLINE Color4b(uint8_t f) : r(f), g(f), b(f), a(255) {}
	UI_FORCEINLINE Color4b(uint8_t gray, uint8_t alpha) : r(gray), g(gray), b(gray), a(alpha) {}
	UI_FORCEINLINE Color4b(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) : r(red), g(green), b(blue), a(alpha) {}

	UI_FORCEINLINE bool operator == (const Color4b& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }

	UI_FORCEINLINE bool IsOpaque() const { return a == 255; }
	UI_FORCEINLINE Color4b GetOpaque() const { return { r, g, b, 255 }; }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "Color4b");
		OnField(oi, "r", r);
		OnField(oi, "g", g);
		OnField(oi, "b", b);
		OnField(oi, "a", a);
		oi.EndObject();
	}

	union
	{
		struct
		{
			uint8_t r, g, b, a;
		};
		uint32_t value;
	};
};


struct Color4f
{
	static Color4f Zero() { return { 0, 0 }; }
	static Color4f Black() { return { 0, 1 }; }
	static Color4f White() { return { 1, 1 }; }
	static Color4f Hex(const char* hex, float a = 1)
	{
		char ntx[6] = {}; // copy hex with null termination to ignore garbage after the first null byte
		strncpy(ntx, hex, 6);
		return { gethex(ntx[0], ntx[1]) / 255.f, gethex(ntx[2], ntx[3]) / 255.f, gethex(ntx[4], ntx[5]) / 255.f, a };
	}
	static Color4f HSV(float h, float s, float v, float a = 1);
	void GetHex(char buf[7]);
	float GetHue();
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

	UI_FORCEINLINE Color4f() : r(1), g(1), b(1), a(1) {}
	UI_FORCEINLINE explicit Color4f(float f) : r(f), g(f), b(f), a(f) {}
	UI_FORCEINLINE Color4f(float gray, float alpha) : r(gray), g(gray), b(gray), a(alpha) {}
	UI_FORCEINLINE Color4f(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) {}
	UI_FORCEINLINE Color4f(const Color4b& c) : r(c.r / 255.f), g(c.g / 255.f), b(c.b / 255.f), a(c.a / 255.f) {}

	UI_FORCEINLINE bool operator == (const Color4f& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }

	Color4f operator + (const Color4f& o) const { return { r + o.r, g + o.g, b + o.b, a + o.a }; }
	Color4f operator - (const Color4f& o) const { return { r - o.r, g - o.g, b - o.b, a - o.a }; }
	Color4f operator * (const Color4f& o) const { return { r * o.r, g * o.g, b * o.b, a * o.a }; }
	Color4f operator / (const Color4f& o) const { return { r / o.r, g / o.g, b / o.b, a / o.a }; }

	void BlendOver(const Color4f& c)
	{
		float ca = c.a > 0 ? c.a + (1 - c.a) * (1 - a) : 0;
		r = lerp(r, c.r, ca);
		g = lerp(g, c.g, ca);
		b = lerp(b, c.b, ca);
		a = lerp(a, 1, c.a);
	}
	Color4f Power(float p) const
	{
		return { powf(r, p), powf(g, p), powf(b, p), a };
	}
	bool IsOpaque() const { return fabsf(a - 1) < 0.0001f; }
	UI_FORCEINLINE Color4f GetOpaque() const { return { r, g, b, 1 }; }
	UI_FORCEINLINE Color4f GetPremul() const { return { r * a, g * a, b * a, a }; }
	UI_FORCEINLINE Color4f GetPremulWithAlphaScale(float s) const { float sa = a * s; return { r * sa, g * sa, b * sa, sa }; }

	operator Color4b() const { return { GetRed8(), GetGreen8(), GetBlue8(), GetAlpha8() }; }

	UI_FORCEINLINE uint8_t GetRed8() const { return uint8_t(clamp(r, 0.0f, 1.0f) * 255); }
	UI_FORCEINLINE uint8_t GetGreen8() const { return uint8_t(clamp(g, 0.0f, 1.0f) * 255); }
	UI_FORCEINLINE uint8_t GetBlue8() const { return uint8_t(clamp(b, 0.0f, 1.0f) * 255); }
	UI_FORCEINLINE uint8_t GetAlpha8() const { return uint8_t(clamp(a, 0.0f, 1.0f) * 255); }
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
inline Color4f Color4fLerp(const Color4f& a, const Color4f& b, const Color4f& s)
{
	return
	{
		lerp(a.r, b.r, s.r),
		lerp(a.g, b.g, s.g),
		lerp(a.b, b.b, s.b),
		lerp(a.a, b.a, s.a),
	};
}
UI_FORCEINLINE Color4f Color4fLerp(const Color4f& a, const Color4f& b, float s) { return Color4fLerp(a, b, Color4f(s)); }


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
	Canvas& operator = (const Canvas& c);
	Canvas& operator = (Canvas&& c);

	uint32_t GetWidth() const { return _width; }
	uint32_t GetHeight() const { return _height; }
	size_t GetNumPixels() const { return _width * _height; }
	size_t GetSizeBytes() const { return GetNumPixels() * 4; }

	uint32_t* GetPixels() { return _pixels; }
	const uint32_t* GetPixels() const { return _pixels; }
	unsigned char* GetBytes() { return reinterpret_cast<unsigned char*>(_pixels); }
	const unsigned char* GetBytes() const { return reinterpret_cast<unsigned char*>(_pixels); }

	void SetSize(uint32_t w, uint32_t h);

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t* _pixels = nullptr;
};


// for shadow images
struct SimpleMaskBlurGen
{
	struct Input
	{
		u32 boxSizeX, boxSizeY;
		u32 blurSize;
		u32 cornerLT, cornerRT, cornerLB, cornerRB;

		bool operator == (const Input& o) const
		{
			return boxSizeX == o.boxSizeX
				&& boxSizeY == o.boxSizeY
				&& blurSize == o.blurSize
				&& cornerLT == o.cornerLT
				&& cornerRT == o.cornerRT
				&& cornerLB == o.cornerLB
				&& cornerRB == o.cornerRB;
		}
		Input Normalized() const;
		// generates an equal-comparable version that achieves the same result (after initial normalization)
		Input Canonicalized() const;
	};
	struct Output
	{
		AABB2f innerOffset;
		AABB2f outerOffset;
		AABB2f innerUV;
		AABB2f outerUV;
	};

	static bool OutputWillDiffer(const Input& a, const Input& b);
	static void Generate(const Input& input, Output& output, Canvas& outCanvas);
};

UI_FORCEINLINE size_t HashValue(const SimpleMaskBlurGen::Input& v)
{
	return HashBytesAll(&v, sizeof(v));
}


struct Vertex
{
	float x, y;
	float u, v;
	Color4b col;
};

} // ui
