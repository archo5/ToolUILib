
#pragma once

#include "Objects.h"


struct Color4f
{
	static Color4f Zero() { return { 0 }; }
	static Color4f Black() { return { 0, 1 }; }
	static Color4f White() { return { 1 }; }
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

	Color4f(float f) : r(f), g(f), b(f), a(f) {}
	Color4f(float gray, float alpha) : r(gray), g(gray), b(gray), a(alpha) {}
	Color4f(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) {}

	void BlendOver(const Color4f& c)
	{
		float ca = c.a > 0 ? c.a + (1 - c.a) * (1 - a) : 0;
		r = lerp(r, c.r, ca);
		g = lerp(g, c.g, ca);
		b = lerp(b, c.b, ca);
		a = lerp(a, 1, c.a);
	}

	uint32_t GetColor32()
	{
		uint8_t rb = std::max(0.0f, std::min(1.0f, r)) * 255;
		uint8_t gb = std::max(0.0f, std::min(1.0f, g)) * 255;
		uint8_t bb = std::max(0.0f, std::min(1.0f, b)) * 255;
		uint8_t ab = std::max(0.0f, std::min(1.0f, a)) * 255;
		return (ab << 24) | (bb << 16) | (gb << 8) | rb;
	}

	float r, g, b, a;
};


namespace ui {

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

struct Image
{
	Image() {}
	Image(uint32_t w, uint32_t h, const void* d) : _width(w), _height(h) { _texture = GL::CreateTextureRGBA8(d, w, h); }
	Image(const Canvas& c) : _width(c.GetWidth()), _height(c.GetHeight()) { _texture = GL::CreateTextureRGBA8(c.GetBytes(), c.GetWidth(), c.GetHeight()); }
	Image(const Image& img) = delete;
	Image(Image&& img) : _width(img._width), _height(img._height), _texture(img._texture) { img._texture = 0; }
	~Image() { Destroy(); }

	void Destroy() { if (_texture) GL::DestroyTexture(_texture); _texture = 0; }
	uint32_t GetWidth() const { return _width; }
	uint32_t GetHeight() const { return _height; }

	uint32_t _width = 0;
	uint32_t _height = 0;
	GL::TexID _texture = 0;
};

enum class ScaleMode
{
	Stretch, // the whole rectangle is covered by the whole image, aspect may not be preserved
	Fit, // the whole image fits in the rectangle, aspect is preserved
	Fill, // the whole rectangle is covered by some part of the image, aspect is preserved
};

struct ImageElement : UIElement
{
	ImageElement();
	void OnPaint() override;
	void GetSize(style::Coord& outWidth, style::Coord& outHeight) override;

	ImageElement* SetImage(Image* img);
	ImageElement* SetScaleMode(ScaleMode sm, float ax = 0, float ay = 0);

	Image* _image = nullptr;
	ScaleMode _scaleMode = ScaleMode::Fit;
	float _anchorX = 0;
	float _anchorY = 0;
};

struct HueSatPicker : UIElement
{
	HueSatPicker();
	~HueSatPicker();
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;

	HueSatPicker& Init(float& hue, float& sat)
	{
		_hue = &hue;
		_sat = &sat;
		return *this;
	}

	void _RegenerateBackground(int w);

	style::BlockRef selectorStyle;
	float* _hue = nullptr;
	float* _sat = nullptr;
	Image* _bgImage = nullptr;
};

} // ui
