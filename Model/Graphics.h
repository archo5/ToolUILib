
#pragma once
#include "Objects.h"
#include "../Core/Image.h"


namespace ui {

struct Image
{
	Image() {}
	Image(uint32_t w, uint32_t h, const void* d, bool filtering = draw::DEFAULT_FILTERING) : _width(w), _height(h)
	{
		_texture = draw::TextureCreateRGBA8(w, h, d, filtering);
	}
	Image(const Canvas& c, bool filtering = draw::DEFAULT_FILTERING) : _width(c.GetWidth()), _height(c.GetHeight())
	{
		_texture = draw::TextureCreateRGBA8(c.GetWidth(), c.GetHeight(), c.GetBytes(), filtering);
	}
	Image(const Image& img) = delete;
	Image(Image&& img) : _width(img._width), _height(img._height), _texture(img._texture) { img._texture = nullptr; }
	~Image() { Destroy(); }

	void Destroy() { if (_texture) draw::TextureRelease(_texture); _texture = nullptr; }
	uint32_t GetWidth() const { return _width; }
	uint32_t GetHeight() const { return _height; }

	uint32_t _width = 0;
	uint32_t _height = 0;
	draw::Texture* _texture = nullptr;
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

	Color4b GetColor() const { return _color; }
	ColorBlock& SetColor(Color4b col) { _color = col; return *this; }

	Color4b _color = Color4b::Black();
	std::shared_ptr<Image> _bgImage;
};

struct ColorInspectBlock : UIElement
{
	void OnInit() override;
	void OnPaint() override;

	Color4b GetColor() const { return _color; }
	ColorInspectBlock& SetColor(Color4b col) { _color = col; return *this; }

	Color4b _color = Color4b::Black();
	std::shared_ptr<Image> _bgImage;

	style::Coord alphaBarHeight = 2;
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
	static constexpr const char* NAME = "color";
	ColorDragDropData(const Color4f& c) : DragDropData(NAME), color(c) {}
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

struct ColorEdit : Node
{
	void Render(UIContainer* ctx) override;

	Color4f GetColor() const { return _rgba; }
	void SetColor(Color4f c);

	Color4f _rgba = Color4f::White();
	bool _editorOpened = false;
};

} // ui
