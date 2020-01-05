
#pragma once

#include "Objects.h"


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

} // ui
