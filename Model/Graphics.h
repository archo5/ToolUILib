
#pragma once
#include <memory>
#include "Objects.h"


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
		float max = GetValue();
		float min = std::min(r, std::min(g, b));
		float c = max - min;
		if (c == 0)
			return 0;
		if (max == r)
			return fmodf(((g - b) / c) / 6 + 1, 1);
		if (max == g)
			return (2 + (b - r) / c) / 6;
		return (4 + (r - g) / c) / 6;
	}
	float GetSaturation()
	{
		float max = GetValue();
		float min = std::min(r, std::min(g, b));
		float c = max - min;
		return max == 0 ? 0 : c / max;
	}
	float GetValue()
	{
		return std::max(r, std::max(g, b));
	}

	Color4f() : r(1), g(1), b(1), a(1) {}
	Color4f(float f) : r(f), g(f), b(f), a(f) {}
	Color4f(float gray, float alpha) : r(gray), g(gray), b(gray), a(alpha) {}
	Color4f(float red, float green, float blue, float alpha = 1.0f) : r(red), g(green), b(blue), a(alpha) {}

	bool operator == (const Color4f& o) const { return r == o.r && g == o.g && b == o.b && a == o.a; }

	void BlendOver(const Color4f& c)
	{
		float ca = c.a > 0 ? c.a + (1 - c.a) * (1 - a) : 0;
		r = lerp(r, c.r, ca);
		g = lerp(g, c.g, ca);
		b = lerp(b, c.b, ca);
		a = lerp(a, 1, c.a);
	}

	uint8_t GetRed8() const { return uint8_t(std::max(0.0f, std::min(1.0f, r)) * 255); }
	uint8_t GetGreen8() const { return uint8_t(std::max(0.0f, std::min(1.0f, g)) * 255); }
	uint8_t GetBlue8() const { return uint8_t(std::max(0.0f, std::min(1.0f, b)) * 255); }
	uint8_t GetAlpha8() const { return uint8_t(std::max(0.0f, std::min(1.0f, a)) * 255); }
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
	Image(uint32_t w, uint32_t h, const void* d, bool filtering = true) : _width(w), _height(h)
	{
		_texture = GL::CreateTextureRGBA8(d, w, h, filtering);
	}
	Image(const Canvas& c, bool filtering = true) : _width(c.GetWidth()), _height(c.GetHeight())
	{
		_texture = GL::CreateTextureRGBA8(c.GetBytes(), c.GetWidth(), c.GetHeight(), filtering);
	}
	Image(const Image& img) = delete;
	Image(Image&& img) : _width(img._width), _height(img._height), _texture(img._texture) { img._texture = 0; }
	~Image() { Destroy(); }

	void Destroy() { if (_texture) GL::DestroyTexture(_texture); _texture = 0; }
	uint32_t GetWidth() const { return _width; }
	uint32_t GetHeight() const { return _height; }

	uint32_t _width = 0;
	uint32_t _height = 0;
	GL::Texture2D* _texture = 0;
};

enum class ScaleMode
{
	Stretch, // the whole rectangle is covered by the whole image, aspect may not be preserved
	Fit, // the whole image fits in the rectangle, aspect is preserved
	Fill, // the whole rectangle is covered by some part of the image, aspect is preserved
};

struct ColorBlock : UIElement
{
	void OnInit() override;
	void OnPaint() override;
	void GetSize(style::Coord& outWidth, style::Coord& outHeight) override { outWidth = 20; outHeight = 20; }

	Color4f GetColor() const { return _color; }
	ColorBlock& SetColor(const Color4f& col) { _color = col; return *this; }

	Color4f _color = Color4f::Black();
	std::shared_ptr<Image> _bgImage;
};

struct ImageElement : UIElement
{
	void OnInit() override;
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
	static constexpr bool Persistent = true;

	HueSatPicker();
	~HueSatPicker();
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;

	HueSatPicker& Init(float& hue, float& sat)
	{
		_hue = hue;
		_sat = sat;
		HandleEvent(UIEventType::Change) = [&hue, &sat, this](UIEvent&) { hue = _hue; sat = _sat; };
		return *this;
	}

	void _RegenerateBackground(int w);

	style::BlockRef selectorStyle;
	float _hue = 0;
	float _sat = 0;
	Image* _bgImage = nullptr;
};

enum ColorMode
{
	CM_RGB = 0,
	CM_HSV = 1,
};

enum ColorComponent
{
	CC_0 = 0,
	CC_1 = 1,
	CC_2 = 2,
	CC_Red = 0,
	CC_Green = 1,
	CC_Blue = 2,
	CC_Hue = 0,
	CC_Sat = 1,
	CC_Val = 2,
};

struct ColorCompPicker2DSettings
{
	ColorCompPicker2DSettings() : _mode(CM_HSV), _ccx(CC_Hue), _ccy(CC_Sat), _invx(false), _invy(true), _baseColor(1, 0, 0) {}
	ColorCompPicker2DSettings(ColorMode cm, ColorComponent ccx, ColorComponent ccy, Color4f baseColor = Color4f(1, 0, 0)) :
		ColorCompPicker2DSettings(cm, ccx, false, ccy, true, baseColor) {}
	ColorCompPicker2DSettings(ColorMode cm, ColorComponent ccx, bool invertX, ColorComponent ccy, bool invertY, Color4f baseColor = Color4f(1, 0, 0)) :
		_mode(cm), _ccx(ccx), _ccy(ccy), _invx(invertX), _invy(invertY), _baseColor(baseColor)
	{
	}
	bool operator == (const ColorCompPicker2DSettings& o) const
	{
		return _mode == o._mode && _ccx == o._ccx && _ccy == o._ccy && _invx == o._invx && _invy == o._invy && _baseColor == o._baseColor;
	}
	bool operator != (const ColorCompPicker2DSettings& o) const { return !(*this == o); }

	ColorMode _mode;
	ColorComponent _ccx;
	ColorComponent _ccy;
	bool _invx;
	bool _invy;
	Color4f _baseColor;
};

struct ColorDragDropData : DragDropData
{
	ColorDragDropData(const Color4f& c) : DragDropData("color"), color(c) {}
	void Render(UIContainer* ctx) override;
	Color4f color;
};

struct ColorCompPicker2D : UIElement
{
	ColorCompPicker2D();
	~ColorCompPicker2D();
	void OnEvent(UIEvent& e) override;
	void OnPaint() override;

	ColorCompPicker2DSettings GetSettings() const { return _settings; }
	ColorCompPicker2D& SetSettings(const ColorCompPicker2DSettings& s) { _settings = s; return *this; }

	float GetX() const { return _x; }
	ColorCompPicker2D& SetX(float v) { _x = v; return *this; }

	float GetY() const { return _y; }
	ColorCompPicker2D& SetY(float v) { _y = v; return *this; }

	ColorCompPicker2D& Init(float& x, float& y, ColorCompPicker2DSettings s = {})
	{
		SetSettings(s);
		SetX(x);
		SetY(y);
		HandleEvent(UIEventType::Change) = [&x, &y, this](UIEvent&) { x = _x; y = _y; RerenderNode(); };
		return *this;
	}

	void _RegenerateBackground(int w, int h);

	style::BlockRef selectorStyle;
	ColorCompPicker2DSettings _settings, _curImgSettings;
	float _x = 0;
	float _y = 0;
	Image* _bgImage = nullptr;
};

struct ColorPicker : Node
{
	void Render(UIContainer* ctx) override;

	Color4f GetColor() const { return _rgba; }
	ColorPicker& SetColor(const Color4f& c)
	{
		_rgba = c;
		_UpdateRGB();
		return *this;
	}
	ColorPicker& SetColor(float h, float s, float v, float a = 1)
	{
		_hue = h;
		_sat = s;
		_val = v;
		_rgba.a = a;
		_UpdateHSV();
		return *this;
	}

	void _UpdateRGB()
	{
		_RGB2HSV();
		_rgba.GetHex(hex);
	}
	void _RGB2HSV()
	{
		_hue = _rgba.GetHue();
		_sat = _rgba.GetSaturation();
		_val = _rgba.GetValue();
	}
	void _UpdateHSV()
	{
		_rgba = Color4f::HSV(_hue, _sat, _val, _rgba.a);
		_rgba.GetHex(hex);
	}
	void _UpdateHex()
	{
		_rgba = Color4f::Hex(hex, _rgba.a);
		_RGB2HSV();
	}

	struct SavedColors
	{
		SavedColors();

		Color4f colors[16];
	};
	static SavedColors& GetSavedColors();

	Color4f _rgba = Color4f::White();
	float _hue = 0;
	float _sat = 0;
	float _val = 1;
	char hex[7] = "FFFFFF";
};

} // ui
