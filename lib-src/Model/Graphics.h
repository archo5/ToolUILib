
#pragma once
#include "Objects.h"
#include "Native.h"
#include "../Core/Image.h"
#include "../Core/3DMath.h"
#include "../Render/Render.h"

// TODO?
#include "Controls.h" // for FrameElement and nothing else


namespace ui {

enum class ScaleMode
{
	None, // do not scale the image
	Stretch, // the whole rectangle is covered by the whole image, aspect may not be preserved
	Fit, // the whole image fits in the rectangle, aspect is preserved
	Fill, // the whole rectangle is covered by some part of the image, aspect is preserved
};

enum class ImageLayoutMode
{
	// require exact size
	PreferredExact,
	// allow stretching if the layout requires that
	PreferredMin,
	// always fill the available space
	Fill,
};

struct PreferredSizeLayout
{
	ImageLayoutMode _layoutMode = ImageLayoutMode::PreferredExact;

	virtual Size2f GetSize() = 0;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type);
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type);
};

struct ColorBlock : FrameElement, PreferredSizeLayout
{
	Color4b _color = Color4b::Black();
	draw::ImageSetHandle _bgImageSet;

	// TODO styled
	Size2f size = { 14, 14 };

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;

	Size2f GetSize() override { return size; }
	ColorBlock& SetSize(Size2f s) { size = s; _OnChangeStyle(); return *this; }
	ColorBlock& SetSize(float x, float y) { size = { x, y }; _OnChangeStyle(); return *this; }
	ColorBlock& SetLayoutMode(ImageLayoutMode mode);

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{ return PreferredSizeLayout::CalcEstimatedWidth(containerSize, type).Add(frameStyle.padding.x0 + frameStyle.padding.x1); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{ return PreferredSizeLayout::CalcEstimatedHeight(containerSize, type).Add(frameStyle.padding.y0 + frameStyle.padding.y1); }

	Color4b GetColor() const { return _color; }
	ColorBlock& SetColor(Color4b col) { _color = col; return *this; }
};

struct ColorInspectBlock : ColorBlock
{
	// TODO styled
	Coord alphaBarHeight = 2;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
};

void DrawImage(UIRect rect, draw::IImage* img, ScaleMode sm = ScaleMode::Fit, Vec2f placement = { 0.5f, 0.5f });

struct ImageElement : UIObjectSingleChild
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	ImageElement& SetImage(draw::IImage* img);
	ImageElement& SetPath(StringView path);
	ImageElement& SetDelayLoadPath(StringView path);
	// range: 0-1 (0.5 = middle)
	ImageElement& SetScaleMode(ScaleMode sm, float ax = 0.5f, float ay = 0.5f);
	ImageElement& SetLayoutMode(ImageLayoutMode mode);
	ImageElement& SetAlphaBackgroundEnabled(bool enabled);

	draw::ImageHandle _image;
	ScaleMode _scaleMode = ScaleMode::Fit;
	ImageLayoutMode _layoutMode = ImageLayoutMode::PreferredExact;
	float _anchorX = 0.5f;
	float _anchorY = 0.5f;

	bool _tryDelayLoad = false;
	std::string _delayLoadPath;

	draw::ImageSetHandle _bgImageSet;
};

struct CursorStyle
{
	static constexpr const char* NAME = "CursorStyle";

	AABB2f expand = {};
	PainterHandle painter;

	void Paint(UIObject* obj, Vec2f pos);
	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct HueSatPicker : FrameElement, PreferredSizeLayout
{
	Size2f size = { 256, 256 };

	void OnReset() override;
	void OnEvent(Event& e) override;
	void OnPaint(const UIPaintContext& ctx) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{ return PreferredSizeLayout::CalcEstimatedWidth(containerSize, type).Add(frameStyle.padding.x0 + frameStyle.padding.x1); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{ return PreferredSizeLayout::CalcEstimatedHeight(containerSize, type).Add(frameStyle.padding.y0 + frameStyle.padding.y1); }

	Size2f GetSize() override { return size; }
	HueSatPicker& SetSize(Size2f s) { size = s; _OnChangeStyle(); return *this; }
	HueSatPicker& SetSize(float x, float y) { size = { x, y }; _OnChangeStyle(); return *this; }
	HueSatPicker& SetLayoutMode(ImageLayoutMode mode);

	HueSatPicker& Init(float& hue, float& sat)
	{
		_hue = hue;
		_sat = sat;
		HandleEvent(EventType::Change) = [&hue, &sat, this](Event&) { hue = _hue; sat = _sat; };
		return *this;
	}

	void _RegenerateBackground(int w);

	CursorStyle selectorStyle;
	float _hue = 0;
	float _sat = 0;
	draw::ImageHandle _bgImage;
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
	void Build() override;
	Color4f color;
};

struct ColorCompPicker2D : FrameElement, PreferredSizeLayout
{
	Size2f size = { 256, 256 };

	void OnReset() override;
	void OnEvent(Event& e) override;
	void OnPaint(const UIPaintContext& ctx) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override
	{ return PreferredSizeLayout::CalcEstimatedWidth(containerSize, type).Add(frameStyle.padding.x0 + frameStyle.padding.x1); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{ return PreferredSizeLayout::CalcEstimatedHeight(containerSize, type).Add(frameStyle.padding.y0 + frameStyle.padding.y1); }

	Size2f GetSize() override { return size; }
	ColorCompPicker2D& SetSize(Size2f s) { size = s; _OnChangeStyle(); return *this; }
	ColorCompPicker2D& SetSize(float x, float y) { size = { x, y }; _OnChangeStyle(); return *this; }
	ColorCompPicker2D& SetLayoutMode(ImageLayoutMode mode);

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
		HandleEvent(EventType::Change) = [&x, &y, this](Event&) { x = _x; y = _y; Rebuild(); };
		return *this;
	}

	void _RegenerateBackground(int w, int h);

	CursorStyle selectorStyle;
	ColorCompPicker2DSettings _settings, _curImgSettings;
	float _x = 0;
	float _y = 0;
	draw::ImageHandle _bgImage;
};

struct MultiFormatColor
{
	MultiFormatColor() {}
	MultiFormatColor(const Color4f& c);
	MultiFormatColor(Color4b c);
	MultiFormatColor(float h, float s, float v, float a = 1);

	float GetAlpha() const;
	Color4f GetRGBA() const;
	Color4b GetRGBA8() const;
	void GetHSV(float& h, float& s, float& v) const;

	void SetAlpha(float a);
	void SetRGBA(const Color4f& c);
	void SetRGBA8(Color4b c);
	void SetHSV(float h, float s, float v);
	void SetHSVA(float h, float s, float v, float a = 1);

	void _UpdateRGB();
	void _RGB2HSV();
	void _UpdateHSV();
	void _UpdateHex();

	Color4f _rgba = Color4f::White();
	float _hue = 0;
	float _sat = 0;
	float _val = 1;
	char hex[7] = "FFFFFF";
};

struct ColorPicker : Buildable
{
	void OnReset() override;
	void Build() override;

	const MultiFormatColor& GetColor() const { return _color; }
	ColorPicker& SetColor(const MultiFormatColor& c)
	{
		_color = c;
		return *this;
	}

	struct SavedColors
	{
		static constexpr int COUNT = 16;

		SavedColors();

		Color4f colors[COUNT];
	};
	static SavedColors& GetSavedColors();

	MultiFormatColor _color;
};

struct ColorPickerWindow : NativeDialogWindow
{
	ColorPickerWindow();

	const MultiFormatColor& GetColor() const { return _color; }
	void SetColor(const MultiFormatColor& c) { _color = c; }

	MultiFormatColor _color;
};

struct IColorEdit : Buildable
{
	virtual const MultiFormatColor& GetColor() const = 0;
	virtual void SetColorAny(const MultiFormatColor& c) = 0;
};

struct ColorEdit : IColorEdit
{
	MultiFormatColor _color;

	void Build() override;

	const MultiFormatColor& GetColor() const override { return _color; }
	void SetColorAny(const MultiFormatColor& c) override { SetColor(c); }

	ColorEdit& SetColor(const MultiFormatColor& c);
};

struct ColorEditRT : IColorEdit
{
	MultiFormatColor _color;
	struct ColorPickerWindowRT* _rtWindow = nullptr;

	void Build() override;
	void OnDisable() override;

	const MultiFormatColor& GetColor() const override { return _color; }
	void SetColorAny(const MultiFormatColor& c) override { SetColor(c); }

	ColorEditRT& SetColor(const MultiFormatColor& c);
};

struct View2D : FillerElement
{
	std::function<void(UIRect)> onPaint;

	void OnReset() override
	{
		FillerElement::OnReset();

		onPaint = {};
	}

	void OnPaint(const UIPaintContext& ctx) override;
};

struct View3D : FillerElement
{
	std::function<void(UIRect)> onRender;
	std::function<void(UIRect)> onPaintOverlay;

	void OnReset() override
	{
		FillerElement::OnReset();

		onRender = {};
		onPaintOverlay = {};
	}

	void OnPaint(const UIPaintContext& ctx) override;
};

} // ui
