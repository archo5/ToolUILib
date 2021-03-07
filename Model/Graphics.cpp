
#include "Graphics.h"
#include "Theme.h"
#include "Controls.h"
#include "System.h"
#include "../Render/OpenGL.h"
#include "../Render/Primitives.h"

#include <algorithm>


namespace ui {


void ColorBlock::OnInit()
{
	styleProps = Theme::current->colorBlock;
	_bgImage = Theme::current->GetImage(ThemeImage::CheckerboardBackground);
}

void ColorBlock::OnPaint()
{
	styleProps->paint_func(this);

	auto r = GetContentRect();

	if (!_color.IsOpaque())
	{
		ui::draw::RectTex(r.x0, r.y0, r.x1, r.y1, _bgImage->_texture, 0, 0, r.GetWidth() / _bgImage->GetWidth(), r.GetHeight() / _bgImage->GetHeight());
	}
	
	draw::RectCol(r.x0, r.y0, r.x1, r.y1, _color);

	PaintChildren();
}


void ColorInspectBlock::OnInit()
{
	styleProps = Theme::current->colorInspectBlock;
	_bgImage = Theme::current->GetImage(ThemeImage::CheckerboardBackground);
}

void ColorInspectBlock::OnPaint()
{
	styleProps->paint_func(this);

	auto r = GetContentRect();

	float hbar = roundf(ResolveUnits(alphaBarHeight, r.GetWidth()));
	UIRect r_lf = { r.x0, r.y0, roundf((r.x0 + r.x1) / 2), r.y1 - hbar };
	UIRect r_rt = { r_lf.x1, r.y0, r.x1, r_lf.y1 };

	if (!_color.IsOpaque())
	{
		ui::draw::RectTex(r_rt.x0, r_rt.y0, r_rt.x1, r_rt.y1, _bgImage->_texture, 0, 0, r_rt.GetWidth() / _bgImage->GetWidth(), r_rt.GetHeight() / _bgImage->GetHeight());
	}

	draw::RectCol(r_lf.x0, r_lf.y0, r_lf.x1, r_lf.y1, _color.GetOpaque());
	draw::RectCol(r_rt.x0, r_rt.y0, r_rt.x1, r_rt.y1, _color);

	if (hbar)
	{
		float mx = roundf(lerp(r.x0, r.x1, _color.a / 255.f));
		r_lf.x1 = r_rt.x0 = mx;
		r_lf.y1 = r_rt.y1 = r.y1;
		r_lf.y0 = r_rt.y0 = r.y1 - hbar;
		draw::RectCol(r_lf.x0, r_lf.y0, r_lf.x1, r_lf.y1, Color4b::White());
		draw::RectCol(r_rt.x0, r_rt.y0, r_rt.x1, r_rt.y1, Color4b::Black());
	}

	PaintChildren();
}


void ImageElement::OnInit()
{
	styleProps = Theme::current->image;
}

void ImageElement::OnPaint()
{
	styleProps->paint_func(this);

	auto c = GetContentRect();
	if (_bgImage)
	{
		ui::draw::RectTex(c.x0, c.y0, c.x1, c.y1, _bgImage->_texture, 0, 0, c.GetWidth() / _bgImage->GetWidth(), c.GetHeight() / _bgImage->GetHeight());
	}
	if (_image && _image->GetWidth() && _image->GetHeight() && c.GetWidth() && c.GetHeight())
	{
		switch (_scaleMode)
		{
		case ScaleMode::None: {
			float w = _image->GetWidth(), h = _image->GetHeight();
			float x = roundf(c.x0 + (c.GetWidth() - w) * _anchorX);
			float y = roundf(c.y0 + (c.GetHeight() - h) * _anchorY);
			draw::RectTex(x, y, x + w, y + h, _image->_texture);
			break; }
		case ScaleMode::Stretch:
			draw::RectTex(c.x0, c.y0, c.x1, c.y1, _image->_texture);
			break;
		case ScaleMode::Fit: {
			float iasp = _image->GetWidth() / (float)_image->GetHeight();
			float w = c.GetWidth(), h = c.GetHeight();
			float rasp = w / h;
			if (iasp > rasp) // matched width, adjust height
				h = w / iasp;
			else
				w = h * iasp;
			float x = roundf(c.x0 + (c.GetWidth() - w) * _anchorX);
			float y = roundf(c.y0 + (c.GetHeight() - h) * _anchorY);
			draw::RectTex(x, y, x + w, y + h, _image->_texture);
			break; }
		case ScaleMode::Fill: {
			float iasp = _image->GetWidth() / (float)_image->GetHeight();
			float w = c.GetWidth(), h = c.GetHeight();
			float rasp = w / h;
			if (iasp < rasp) // matched width, adjust height
				h = w / iasp;
			else
				w = h * iasp;
			float x = roundf(c.x0 + (c.GetWidth() - w) * _anchorX);
			float y = roundf(c.y0 + (c.GetHeight() - h) * _anchorY);
			draw::RectTex(c.x0, c.y0, c.x1, c.y1, _image->_texture,
				(c.x0 - x) / w,
				(c.y0 - y) / h,
				(c.x1 - x) / w,
				(c.y1 - y) / h);
			break; }
		}
	}

	PaintChildren();
}

void ImageElement::GetSize(style::Coord& outWidth, style::Coord& outHeight)
{
	if (_image)
	{
		outWidth = _image->GetWidth();
		outHeight = _image->GetHeight();
	}
}

ImageElement* ImageElement::SetImage(Image* img)
{
	_image = img;
	return this;
}

ImageElement* ImageElement::SetScaleMode(ScaleMode sm, float ax, float ay)
{
	_scaleMode = sm;
	_anchorX = ax;
	_anchorY = ay;
	return this;
}

ImageElement* ImageElement::SetAlphaBackgroundEnabled(bool enabled)
{
	_bgImage = enabled ? Theme::current->GetImage(ThemeImage::CheckerboardBackground) : nullptr;
	return this;
}


HueSatPicker::~HueSatPicker()
{
	delete _bgImage;
}

void HueSatPicker::OnInit()
{
	styleProps = Theme::current->selectorContainer;
	selectorStyle = Theme::current->selector;
	SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
}

void HueSatPicker::OnEvent(UIEvent& e)
{
	if ((e.type == UIEventType::MouseMove && IsClicked()) ||
		(e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left))
	{
		auto cr = GetContentRect();
		if (cr.GetWidth() < cr.GetHeight())
			cr.y1 = cr.y0 + cr.GetWidth();
		else
			cr.x1 = cr.x0 + cr.GetHeight();
		float hw = cr.GetWidth() / 2;
		float rx = (e.x - (cr.x0 + hw)) / hw;
		float ry = (e.y - (cr.y0 + hw)) / hw;
		_hue = fmodf(atan2(rx, -ry) / (3.14159f * 2) + 1.0f, 1.0f);
		_sat = clamp(sqrtf(rx * rx + ry * ry), 0.0f, 1.0f);
		e.context->OnChange(this);
	}
}

void HueSatPicker::OnPaint()
{
	auto cr = GetContentRect();
	int tgtw = int(min(cr.GetWidth(), cr.GetHeight()));
	cr.x1 = cr.x0 + tgtw;
	cr.y1 = cr.y0 + tgtw;
	if (!_bgImage || _bgImage->GetWidth() != tgtw)
		_RegenerateBackground(tgtw);

	draw::RectTex(cr.x0, cr.y0, cr.x0 + tgtw, cr.y0 + tgtw, _bgImage->_texture);

	float hw = tgtw / 2.0f;
	float cx = cr.x0 + hw;
	float cy = cr.y0 + hw;
	float sx = cx + sinf(_hue * 3.14159f * 2) * _sat * hw;
	float sy = cy - cosf(_hue * 3.14159f * 2) * _sat * hw;
	float sw = ResolveUnits(selectorStyle->width, cr.GetWidth());
	float sh = ResolveUnits(selectorStyle->height, cr.GetWidth());
	sx = roundf(sx - sw / 2);
	sy = roundf(sy - sh / 2);
	style::PaintInfo info(this);
	info.rect = { sx, sy, sx + sw, sy + sh };
	selectorStyle->paint_func(info);

	PaintChildren();
}

void HueSatPicker::_RegenerateBackground(int w)
{
	int h = w;
	Canvas c(w, h);
	auto* px = c.GetPixels();
	float cx = w / 2.0f;
	float cy = h / 2.0f;
	float dmax = w / 2.0f;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			float d = sqrtf((x - cx) * (x - cx) + (y - cy) * (y - cy));
			float alpha = clamp((d - dmax) * -1, 0.0f, 1.0f);
			float angle = fmodf(atan2(x - cx, cy - y) / (3.14159f * 2) + 1.0f, 1.0f);
			Color4f col = Color4f::HSV(angle, d / dmax, 1.0f, alpha);
			px[x + y * w] = col.GetColor32();
		}
	}

	delete _bgImage;
	_bgImage = new Image(c);
}


void ColorDragDropData::Render(UIContainer* ctx)
{
	ctx->Make<ui::ColorInspectBlock>()->SetColor(color) + Width(40) + Padding(0);
}


ColorCompPicker2D::ColorCompPicker2D()
{
	styleProps = Theme::current->selectorContainer;
	selectorStyle = Theme::current->selector;
}

ColorCompPicker2D::~ColorCompPicker2D()
{
	delete _bgImage;
}

void ColorCompPicker2D::OnEvent(UIEvent& e)
{
	if ((e.type == UIEventType::MouseMove && IsClicked()) ||
		(e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left))
	{
		auto cr = GetContentRect();
		_x = clamp((e.x - cr.x0) / cr.GetWidth(), 0.0f, 1.0f);
		_y = clamp((e.y - cr.y0) / cr.GetHeight(), 0.0f, 1.0f);
		if (_settings._invx) _x = 1 - _x;
		if (_settings._invy) _y = 1 - _y;
		e.context->OnChange(this);
	}
}

void ColorCompPicker2D::OnPaint()
{
	auto cr = GetContentRect();
	int tgtw = int(cr.GetWidth());
	int tgth = int(cr.GetHeight());

	if (!_bgImage || _bgImage->GetWidth() != tgtw || _bgImage->GetHeight() != tgth || _settings != _curImgSettings)
		_RegenerateBackground(tgtw, tgth);

	draw::RectTex(cr.x0, cr.y0, cr.x1, cr.y1, _bgImage->_texture);

	float sx = lerp(cr.x0, cr.x1, _settings._invx ? 1 - _x : _x);
	float sy = lerp(cr.y0, cr.y1, _settings._invy ? 1 - _y : _y);
	float sw = ResolveUnits(selectorStyle->width, cr.GetWidth());
	float sh = ResolveUnits(selectorStyle->height, cr.GetWidth());
	sx = roundf(sx - sw / 2);
	sy = roundf(sy - sh / 2);
	style::PaintInfo info(this);
	info.rect = { sx, sy, sx + sw, sy + sh };
	selectorStyle->paint_func(info);

	PaintChildren();
}

void ColorCompPicker2D::_RegenerateBackground(int w, int h)
{
	Canvas c(w, h);
	auto* px = c.GetPixels();

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			float fx = x / float(w);
			float fy = y / float(h);
			if (_settings._invx)
				fx = 1 - fx;
			if (_settings._invy)
				fy = 1 - fy;
			Color4f c = _settings._baseColor;
			switch (_settings._mode)
			{
			case CM_RGB:
				switch (_settings._ccx)
				{
				case CC_Red: c.r = fx; break;
				case CC_Green: c.g = fx; break;
				case CC_Blue: c.b = fx; break;
				}
				switch (_settings._ccy)
				{
				case CC_Red: c.r = fy; break;
				case CC_Green: c.g = fy; break;
				case CC_Blue: c.b = fy; break;
				}
				break;
			case CM_HSV: {
				float h = 0, s = 1, v = 1;
				switch (_settings._ccx)
				{
				case CC_Hue: h = fx; break;
				case CC_Sat: s = fx; break;
				case CC_Val: v = fx; break;
				}
				switch (_settings._ccy)
				{
				case CC_Hue: h = fy; break;
				case CC_Sat: s = fy; break;
				case CC_Val: v = fy; break;
				}
				c = Color4f::HSV(h, s, v, c.a);
				break; }
			}
			px[x + y * w] = c.GetColor32();
		}
	}

	delete _bgImage;
	_bgImage = new Image(c);
	_curImgSettings = _settings;
}


MultiFormatColor::MultiFormatColor(const Color4f& c)
{
	SetRGBA(c);
}

MultiFormatColor::MultiFormatColor(Color4b c)
{
	SetRGBA8(c);
}

MultiFormatColor::MultiFormatColor(float h, float s, float v, float a)
{
	SetHSVA(h, s, v, a);
}

float MultiFormatColor::GetAlpha() const
{
	return _rgba.a;
}

Color4f MultiFormatColor::GetRGBA() const
{
	return _rgba;
}

Color4b MultiFormatColor::GetRGBA8() const
{
	return _rgba;
}

void MultiFormatColor::GetHSV(float& h, float& s, float& v) const
{
	h = _hue;
	s = _sat;
	v = _val;
}

void MultiFormatColor::SetAlpha(float a)
{
	_rgba.a = a;
}

void MultiFormatColor::SetRGBA(const Color4f& c)
{
	_rgba = c;
	_UpdateRGB();
}

void MultiFormatColor::SetRGBA8(Color4b c)
{
	_rgba = c;
	_UpdateRGB();
}

void MultiFormatColor::SetHSV(float h, float s, float v)
{
	_hue = h;
	_sat = s;
	_val = v;
	_UpdateHSV();
}

void MultiFormatColor::SetHSVA(float h, float s, float v, float a)
{
	SetHSV(h, s, v);
	_rgba.a = a;
}

void MultiFormatColor::_UpdateRGB()
{
	_RGB2HSV();
	_rgba.GetHex(hex);
}

void MultiFormatColor::_RGB2HSV()
{
	_hue = _rgba.GetHue();
	_sat = _rgba.GetSaturation();
	_val = _rgba.GetValue();
}

void MultiFormatColor::_UpdateHSV()
{
	_rgba = Color4f::HSV(_hue, _sat, _val, _rgba.a);
	_rgba.GetHex(hex);
}

void MultiFormatColor::_UpdateHex()
{
	_rgba = Color4f::Hex(hex, _rgba.a);
	_RGB2HSV();
}


static std::string gstr(float f)
{
	char bfr[64];
	snprintf(bfr, 64, "%g", f);
	return bfr;
}

static void DrawHGradQuad(UIRect r, Color4f a, Color4f b)
{
	ui::draw::RectGradH(r.x0, r.y0, r.x1, r.y1, a, b);
}

static UIRect RectHSlice(UIRect r, float i, float n)
{
	return { lerp(r.x0, r.x1, i / n), r.y0, lerp(r.x0, r.x1, (i + 1) / n), r.y1 };
}

ColorPicker::SavedColors::SavedColors()
{
	for (int i = 0; i < 16; i++)
	{
		colors[i] = Color4f::HSV(i / 2 / 8.0f, i % 2 ? 0.25f : 0.75f, 0.8f);
	}
}

ColorPicker::SavedColors g_savedColors;

ColorPicker::SavedColors& ColorPicker::GetSavedColors()
{
	return g_savedColors;
}

void ColorPicker::Render(UIContainer* ctx)
{
	*this + RerenderOnChange();

	ctx->PushBox()
		+ Layout(style::layouts::StackExpand())
		+ StackingDirection(style::StackingDirection::LeftToRight);
	{
		// left side
		ctx->PushBox()
			+ Width(style::Coord::Fraction(0)); // TODO any way to make this unnecessary?
		{
			auto& hsp = ctx->Make<HueSatPicker>()->Init(_color._hue, _color._sat)
				+ Width(240)
				+ Height(240);
			hsp.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateHSV(); };
		}
		ctx->Pop();

		FloatLimits limit = { 0, 1 };

		// right side
		ctx->PushBox() + Padding(8);
		{
		//	ctx->PushBox() + Padding(8);
			//ctx->Text("HSV");

			Property::Begin(ctx);
			{
				ctx->Text("H") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_color._hue, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateHSV(); };
				style::Accessor a = sl.GetTrackStyle();
				a.SetPaintFunc([fn{ a.GetPaintFunc() }, sat{ _color._sat }, val{ _color._val }](const style::PaintInfo& info)
				{
					fn(info);
					for (int i = 0; i < 6; i++)
						DrawHGradQuad(RectHSlice(info.rect, i, 6), Color4f::HSV(i / 6.0f, sat, val), Color4f::HSV((i + 1) / 6.0f, sat, val));
				});
				sl.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
				ctx->Make<Textbox>()->Init(_color._hue) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateHSV(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("S") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_color._sat, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateHSV(); };
				style::Accessor a = sl.GetTrackStyle();
				a.SetPaintFunc([fn{ a.GetPaintFunc() }, hue{ _color._hue }, val{ _color._val }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, Color4f::HSV(hue, 0, val), Color4f::HSV(hue, 1, val));
				});
				sl.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
				ctx->Make<Textbox>()->Init(_color._sat) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateHSV(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("V") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_color._val, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateHSV(); };
				style::Accessor a = sl.GetTrackStyle();
				a.SetPaintFunc([fn{ a.GetPaintFunc() }, hue{ _color._hue }, sat{ _color._sat }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, Color4f::HSV(hue, sat, 0), Color4f::HSV(hue, sat, 1));
				});
				sl.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
				ctx->Make<Textbox>()->Init(_color._val) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateHSV(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("R") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_color._rgba.r, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateRGB(); };
				style::Accessor a = sl.GetTrackStyle();
				a.SetPaintFunc([fn{ a.GetPaintFunc() }, col{ _color._rgba }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, { 0, col.g, col.b }, { 1, col.g, col.b });
				});
				sl.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
				ctx->Make<Textbox>()->Init(_color._rgba.r) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateRGB(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("G") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_color._rgba.g, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateRGB(); };
				style::Accessor a = sl.GetTrackStyle();
				a.SetPaintFunc([fn{ a.GetPaintFunc() }, col{ _color._rgba }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, { col.r, 0, col.b }, { col.r, 1, col.b });
				});
				sl.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
				ctx->Make<Textbox>()->Init(_color._rgba.g) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateRGB(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("B") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_color._rgba.b, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _color._UpdateRGB(); };
				style::Accessor a = sl.GetTrackStyle();
				a.SetPaintFunc([fn{ a.GetPaintFunc() }, col{ _color._rgba }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, { col.r, col.g, 0 }, { col.r, col.g, 1 });
				});
				sl.GetTrackFillStyle().SetPaintFunc([](const style::PaintInfo& info) {});
				ctx->Make<Textbox>()->Init(_color._rgba.b) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateRGB(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("A") + Width(10);
				ctx->Make<Slider>()->Init(_color._rgba.a, limit);
				ctx->Make<Textbox>()->Init(_color._rgba.a) + Width(50);
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				*ctx->Push<Panel>()
					+ StackingDirection(style::StackingDirection::LeftToRight)
					+ Padding(3)
					+ MakeDraggable([this](UIEvent& e) { ui::DragDrop::SetData(new ColorDragDropData(_color.GetRGBA())); })
					+ EventHandler(UIEventType::DragDrop, [this](UIEvent& e)
				{
					if (auto* cddd = ui::DragDrop::GetData<ColorDragDropData>())
					{
						_color.SetRGBA(cddd->color);
						e.context->OnChange(this);
					}
				});
				ctx->Make<ColorBlock>()->SetColor(_color.GetRGBA().GetOpaque()) + Width(50) + Height(60) + Padding(0);
				ctx->Make<ColorBlock>()->SetColor(_color.GetRGBA()) + Width(50) + Height(60) + Padding(0);
				ctx->Pop();

				ctx->PushBox();

				ctx->PushBox() + Layout(style::layouts::StackExpand()) + StackingDirection(style::StackingDirection::LeftToRight) + Height(22);
				*ctx->Make<BoxElement>() + Width(style::Coord::Fraction(1));
				ctx->Text("Hex:") + Height(22);
				ctx->Make<Textbox>()->Init(_color.hex) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateHex(); });
				ctx->Pop();

				auto& pick = *ctx->MakeWithText<Button>("Pick");
				pick.HandleEvent() = [this](UIEvent& e)
				{
					if (e.type == UIEventType::SetCursor)
					{
						// TODO is it possible to set the cursor between click/capture?
						e.context->SetDefaultCursor(DefaultCursor::Crosshair);
						e.StopPropagation();
					}
					if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
					{
						auto col = platform::GetColorAtScreenPos(platform::GetCursorScreenPos());
						_color.SetRGBA(col);
						e.context->OnChange(this);
					}
				};

				ctx->Pop();
			}
			Property::End(ctx);

		//	ctx->Pop();
		}
		ctx->Pop();
	}
	ctx->Pop();

	ctx->PushBox()
		+ Layout(style::layouts::InlineBlock())
		+ StackingDirection(style::StackingDirection::LeftToRight)
		+ Padding(8);
	{
		auto& sc = GetSavedColors();
		for (int i = 0; i < 16; i++)
		{
			ctx->Make<ColorBlock>()->SetColor(sc.colors[i])
				+ MakeDraggable([this, i]() { ui::DragDrop::SetData(new ColorDragDropData(GetSavedColors().colors[i])); })
				+ EventHandler(UIEventType::DragDrop, [this, i](UIEvent&)
			{
				if (auto* cddd = ui::DragDrop::GetData<ColorDragDropData>())
				{
					GetSavedColors().colors[i] = cddd->color;
					Rerender();
				}
			});
		}
	}
	ctx->Pop();
}


ColorPickerWindow::ColorPickerWindow()
{
	SetTitle("Color picker");
	SetSize(500, 300);
}

void ColorPickerWindow::OnRender(UIContainer* ctx)
{
	auto& picker = ctx->Make<ColorPicker>()->SetColor(_color);
	picker.HandleEvent(UIEventType::Change) = [this, &picker](UIEvent&)
	{
		_color = picker.GetColor();
	};
	ctx->Make<DefaultOverlayRenderer>();
}


void ColorEdit::Render(UIContainer* ctx)
{
	auto& cib = ctx->Make<ColorInspectBlock>()->SetColor(_color.GetRGBA());
	cib.SetFlag(UIObject_DB_Button, true);
	cib + EventHandler(UIEventType::Click, [this](UIEvent& e)
	{
		if (e.GetButton() == UIMouseButton::Left)
		{
			ColorPickerWindow cpw;
			cpw.SetColor(_color);
			cpw.Show();
			_color = cpw.GetColor();
			e.context->OnChange(this);
			e.context->OnCommit(this);
		}
	});
}


void View2D::OnPaint()
{
	styleProps->paint_func(this);

	auto r = finalRectC;
	if (draw::PushScissorRect(r.x0, r.y0, r.x1, r.y1))
	{
		if (onPaint)
			onPaint(r);
	}
	draw::PopScissorRect();

	PaintChildren();
}


void View3D::OnPaint()
{
	styleProps->paint_func(this);

	auto r = finalRectC;
	if (draw::PushScissorRect(r.x0, r.y0, r.x1, r.y1))
	{
		rhi::Begin3DMode(r.x0, r.y0, r.x1, r.y1);

		if (onRender)
			onRender(r);

		rhi::End3DMode();

		if (onPaintOverlay)
			onPaintOverlay(r);
	}
	draw::PopScissorRect();

	PaintChildren();
}


void CameraBase::_UpdateViewProjMatrix()
{
	_mtxViewProj = _mtxView * _mtxProj;
	_mtxInvViewProj = _mtxViewProj.Inverted();
}

void CameraBase::SetViewMatrix(const Mat4f& m)
{
	_mtxView = m;
	_mtxInvView = m.Inverted();
	_UpdateViewProjMatrix();
}

void CameraBase::SetProjectionMatrix(const Mat4f& m)
{
	_mtxProj = m;
	_mtxInvProj = m.Inverted();
	_UpdateViewProjMatrix();
}

void CameraBase::SetWindowRect(const UIRect& rect)
{
	_windowRect = rect;
}

Point<float> CameraBase::WindowToNormalizedPoint(Point<float> p) const
{
	auto& cr = _windowRect;
	return
	{
		lerp(-1, 1, invlerp(cr.x0, cr.x1, p.x)),
		lerp(1, -1, invlerp(cr.y0, cr.y1, p.y)),
	};
}

Point<float> CameraBase::NormalizedToWindowPoint(Point<float> p) const
{
	auto& cr = _windowRect;
	return
	{
		lerp(cr.x0, cr.x1, invlerp(-1, 1, p.x)),
		lerp(cr.y0, cr.y1, invlerp(1, -1, p.y)),
	};
}

Point<float> CameraBase::WorldToNormalizedPoint(const Vec3f& p) const
{
	auto np = GetViewProjectionMatrix().TransformPoint(p);
	return { np.x, np.y };
}

Point<float> CameraBase::WorldToWindowPoint(const Vec3f& p) const
{
	return NormalizedToWindowPoint(WorldToNormalizedPoint(p));
}

Ray3f CameraBase::GetRayNP(Point<float> p) const
{
	return GetCameraRay(GetInverseViewProjectionMatrix(), p.x, p.y);
}

Ray3f CameraBase::GetLocalRayNP(Point<float> p, const Mat4f& world2local) const
{
	return GetCameraRay(GetInverseViewProjectionMatrix() * world2local, p.x, p.y);
}

Ray3f CameraBase::GetRayWP(Point<float> p) const
{
	return GetRayNP(WindowToNormalizedPoint(p));
}

Ray3f CameraBase::GetLocalRayWP(Point<float> p, const Mat4f& world2local) const
{
	return GetLocalRayNP(WindowToNormalizedPoint(p), world2local);
}

Ray3f CameraBase::GetRayEP(const UIEvent& e) const
{
	return GetRayNP(WindowToNormalizedPoint({ e.x, e.y }));
}

Ray3f CameraBase::GetLocalRayEP(const UIEvent& e, const Mat4f& world2local) const
{
	return GetLocalRayNP(WindowToNormalizedPoint({ e.x, e.y }), world2local);
}


OrbitCamera::OrbitCamera()
{
	_UpdateViewMatrix();
}

bool OrbitCamera::OnEvent(UIEvent& e)
{
	if (e.IsPropagationStopped())
		return false;
	if (e.type == UIEventType::ButtonDown)
	{
		if (e.GetButton() == rotateButton)
		{
			rotating = true;
			e.StopPropagation();
		}
		else if (e.GetButton() == panButton)
		{
			panning = true;
			e.StopPropagation();
		}
	}
	if (e.type == UIEventType::ButtonUp)
	{
		if (e.GetButton() == rotateButton)
		{
			rotating = false;
		}
		else if (e.GetButton() == panButton)
		{
			panning = false;
		}
	}
	if (e.type == UIEventType::MouseMove)
	{
		if (rotating)
			Rotate(e.dx * rotationSpeed, e.dy * rotationSpeed);
		if (panning)
			Pan(e.dx / e.current->GetContentRect().GetWidth(), e.dy / e.current->GetContentRect().GetHeight());
		return rotating || panning;
	}
	if (e.type == UIEventType::MouseScroll)
	{
		Zoom(e.dy == 0 ? 0 : e.dy < 0 ? 1 : -1);
		return true;
	}
	return false;
}

void OrbitCamera::Rotate(float dx, float dy)
{
	yaw += dx;
	pitch += dy;
	pitch = clamp(pitch, minPitch, maxPitch);

	_UpdateViewMatrix();
}

void OrbitCamera::Pan(float dx, float dy)
{
	float scale = distance * 2;
	dx *= scale / GetProjectionMatrix().m[0][0];
	dy *= scale / GetProjectionMatrix().m[1][1];
	auto vm = GetViewMatrix();
	Vec3f right = { vm.v00, vm.v01, vm.v02 };
	Vec3f up = { vm.v10, vm.v11, vm.v12 };
	pivot -= right * dx - up * dy;

	_UpdateViewMatrix();
}

void OrbitCamera::Zoom(float delta)
{
	distance *= powf(distanceScale, delta);

	_UpdateViewMatrix();
}

void OrbitCamera::_UpdateViewMatrix()
{
	float cp = cosf(pitch * DEG2RAD);
	float sp = sinf(pitch * DEG2RAD);
	float cy = cosf(yaw * DEG2RAD);
	float sy = sinf(yaw * DEG2RAD);
	Vec3f dir = { cy * cp, sy * cp, sp };
	Mat4f vm = Mat4f::LookAtLH(pivot + dir * distance, pivot, { 0, 0, 1 });

	SetViewMatrix(vm);
}


enum MovingGizmoParts
{
	MGP_NONE = -1,
	MGP_XAxis,
	MGP_YAxis,
	MGP_ZAxis,
	MGP_XPlane,
	MGP_YPlane,
	MGP_ZPlane,
};

struct SDFRet
{
	float dist;
	int part;
};

static float ConeSDF(const Vec3f& p, const Vec3f& slope)
{
	float q = sqrtf(p.x * p.x + p.y * p.y);
	return slope.x * q + slope.y * p.z;
}

static float BoxSDF(const Vec3f& p, const Vec3f& bbmin, const Vec3f& bbmax)
{
	Vec3f c = p;
	if (c.x < bbmin.x) c.x = bbmin.x; else if (c.x > bbmax.x) c.x = bbmax.x;
	if (c.y < bbmin.y) c.y = bbmin.y; else if (c.y > bbmax.y) c.y = bbmax.y;
	if (c.z < bbmin.z) c.z = bbmin.z; else if (c.z > bbmax.z) c.z = bbmax.z;
	return (c - p).Length();
}

static SDFRet MovingGizmoSDF(const Vec3f& pos)
{
	auto coneSlope = Vec3f{ 2, 1 }.Normalized();

	float coneDistX = max(ConeSDF({ pos.y, pos.z, pos.x - 1.1f }, coneSlope), 0.9f - pos.x);
	float coneDistY = max(ConeSDF({ pos.z, pos.x, pos.y - 1.1f }, coneSlope), 0.9f - pos.y);
	float coneDistZ = max(ConeSDF({ pos.x, pos.y, pos.z - 1.1f }, coneSlope), 0.9f - pos.z);

	float axisDistX = BoxSDF(pos, { 0, 0, 0 }, { 0.9f, 0, 0 }) - 0.05f;
	float axisDistY = BoxSDF(pos, { 0, 0, 0 }, { 0, 0.9f, 0 }) - 0.05f;
	float axisDistZ = BoxSDF(pos, { 0, 0, 0 }, { 0, 0, 0.9f }) - 0.05f;

	float planeDistX = BoxSDF(pos, { 0, 0, 0 }, { 0, 0.3f, 0.3f });
	float planeDistY = BoxSDF(pos, { 0, 0, 0 }, { 0.3f, 0, 0.3f });
	float planeDistZ = BoxSDF(pos, { 0, 0, 0 }, { 0.3f, 0.3f, 0 });

	SDFRet ret = { FLT_MAX, MGP_NONE };
	if (ret.dist > coneDistX) ret = { coneDistX, MGP_XAxis };
	if (ret.dist > coneDistY) ret = { coneDistY, MGP_YAxis };
	if (ret.dist > coneDistZ) ret = { coneDistZ, MGP_ZAxis };
	if (ret.dist > axisDistX) ret = { axisDistX, MGP_XAxis };
	if (ret.dist > axisDistY) ret = { axisDistY, MGP_YAxis };
	if (ret.dist > axisDistZ) ret = { axisDistZ, MGP_ZAxis };
	if (ret.dist > planeDistX) ret = { planeDistX, MGP_XPlane };
	if (ret.dist > planeDistY) ret = { planeDistY, MGP_YPlane };
	if (ret.dist > planeDistZ) ret = { planeDistZ, MGP_ZPlane };
	return ret;
}

int Gizmo_Moving::_GetIntersectingPart(const Ray3f& ray)
{
	Vec3f pos = ray.origin;

	for (int i = 0; i < 64; i++)
	{
		auto nearest = MovingGizmoSDF(pos);
		if (nearest.dist < 0.001f)
			return nearest.part;

		pos += ray.direction * nearest.dist;
	}

	return MGP_NONE;
}

void Gizmo_Moving::SetTransform(const Mat4f& base)
{
	_xf = base;
}

bool Gizmo_Moving::OnEvent(UIEvent& e, const CameraBase& cam, const IGizmoEditable& editable)
{
	if (e.IsPropagationStopped())
		return false;

	auto& editableNC = const_cast<IGizmoEditable&>(editable);
	if (e.type == UIEventType::MouseMove)
	{
		if (_selectedPart == MGP_NONE)
		{
			_hoveredPart = _GetIntersectingPart(cam.GetLocalRayEP(e, _combinedXF.Inverted()));
		}
		else
		{
			Point<float> newPointWP = { e.x + _origDiffWP.x, e.y + _origDiffWP.y };
			Ray3f ray = cam.GetRayWP(newPointWP);

			Vec3f axis = {};
			switch (_selectedPart)
			{
			case MGP_XAxis: case MGP_XPlane: axis = { 1, 0, 0 }; break;
			case MGP_YAxis: case MGP_YPlane: axis = { 0, 1, 0 }; break;
			case MGP_ZAxis: case MGP_ZPlane: axis = { 0, 0, 1 }; break;
			}
			Vec3f worldAxis = (_combinedXF.TransformPoint(axis) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized();

			Vec3f worldOrigPos = _origPos;

			switch (_selectedPart)
			{
			case MGP_XAxis:
			case MGP_YAxis:
			case MGP_ZAxis: {
				// generate a plane
				Vec3f cam2center = (_combinedXF.TransformPoint({ 0, 0, 0 }) - cam.GetInverseViewProjectionMatrix().TransformPoint({ 0, 0, -10000 })).Normalized();
				Vec3f rt = Vec3Cross(cam2center, worldAxis);
				Vec3f pn = Vec3Cross(rt, worldAxis);
				auto rpir = RayPlaneIntersect(ray.origin, ray.direction, { pn.x, pn.y, pn.z, Vec3Dot(pn, worldOrigPos) });
				if (rpir.angcos)
				{
					Vec3f isp = ray.GetPoint(rpir.dist);
					float diff = Vec3Dot(worldAxis, isp) - Vec3Dot(worldAxis, worldOrigPos);

					auto xf = Mat4f::Translate(worldAxis * diff);

					ui::DataReader dr(_origData);
					editableNC.Transform(dr, &xf);
					return true;
				}
				break; }
			case MGP_XPlane:
			case MGP_YPlane:
			case MGP_ZPlane: {
				auto rpir = RayPlaneIntersect(ray.origin, ray.direction, { worldAxis.x, worldAxis.y, worldAxis.z, Vec3Dot(worldAxis, worldOrigPos) });
				if (rpir.angcos)
				{
					Vec3f diff = ray.GetPoint(rpir.dist) - _origPos;

					auto xf = Mat4f::Translate(diff);

					ui::DataReader dr(_origData);
					editableNC.Transform(dr, &xf);
					return true;
				}
				break; }
			}
		}
	}
	else if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && _hoveredPart != MGP_NONE)
	{
		Vec3f cam2center = (_combinedXF.TransformPoint({ 0, 0, 0 }) - cam.GetInverseViewProjectionMatrix().TransformPoint({ 0, 0, -10000 })).Normalized();
		float dotX = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 1, 0, 0 }) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized(), cam2center));
		float dotY = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 0, 1, 0 }) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized(), cam2center));
		float dotZ = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 0, 0, 1 }) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized(), cam2center));

		float visX = clamp(invlerp(0.99f, 0.97f, dotX), 0.0f, 1.0f);
		float visY = clamp(invlerp(0.99f, 0.97f, dotY), 0.0f, 1.0f);
		float visZ = clamp(invlerp(0.99f, 0.97f, dotZ), 0.0f, 1.0f);
		float visPX = clamp(invlerp(0.05f, 0.21f, dotX), 0.0f, 1.0f);
		float visPY = clamp(invlerp(0.05f, 0.21f, dotY), 0.0f, 1.0f);
		float visPZ = clamp(invlerp(0.05f, 0.21f, dotZ), 0.0f, 1.0f);

		switch (_hoveredPart)
		{
		case MGP_XAxis: if (visX <= 0) return false; break;
		case MGP_YAxis: if (visY <= 0) return false; break;
		case MGP_ZAxis: if (visZ <= 0) return false; break;
		case MGP_XPlane: if (visPX <= 0) return false; break;
		case MGP_YPlane: if (visPY <= 0) return false; break;
		case MGP_ZPlane: if (visPZ <= 0) return false; break;
		}

		_selectedPart = _hoveredPart;

		_origPos = _combinedXF.TransformPoint({ 0, 0, 0 });

		auto pointWP = cam.WorldToWindowPoint(_origPos);
		_origDiffWP = { pointWP.x - e.x, pointWP.y - e.y };

		_origData.clear();
		ui::DataWriter dw(_origData);
		editable.Backup(dw);

		e.StopPropagation();
	}
	else if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Right && _selectedPart != MGP_NONE)
	{
		_selectedPart = MGP_NONE;
		ui::DataReader dr(_origData);
		editableNC.Transform(dr, nullptr);
		return true;
	}
	else if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
	{
		_selectedPart = MGP_NONE;
	}
	// TODO esc
	return false;
}

void Gizmo_Moving::Render(const CameraBase& cam, float size, GizmoSizeMode sizeMode)
{
	using namespace ui::rhi;

	if (sizeMode != GizmoSizeMode::Scene)
	{
		Vec3f pos = _xf.TransformPoint({ 0, 0, 0 });
		float q = max(0.00001f, cam.GetViewMatrix().TransformPoint(pos).z);
		size *= q;
		if (sizeMode == GizmoSizeMode::ViewPixelsY)
			size /= cam.GetWindowRect().GetHeight();
	}
	_combinedXF = Mat4f::Scale(size) * _xf;

	constexpr ui::prim::PlaneSettings PS = {};
	constexpr uint16_t planeVC = PS.CalcVertexCount();
	constexpr uint16_t planeIC = PS.CalcIndexCount();
	ui::Vertex_PF3CB4 planeVerts[planeVC];
	uint16_t planeIndices[planeIC];
	ui::prim::GeneratePlane(PS, planeVerts, planeIndices);

	constexpr uint16_t planeFrameIC = 5;
	uint16_t planeFrameIndices[planeFrameIC] = { 0, 1, 3, 2, 0 };

	constexpr ui::prim::ConeSettings CS = {};
	constexpr uint16_t coneVC = CS.CalcVertexCount();
	constexpr uint16_t coneIC = CS.CalcIndexCount();
	ui::Vertex_PF3CB4 coneVerts[coneVC];
	uint16_t coneIndices[coneIC];
	ui::prim::GenerateCone(CS, coneVerts, coneIndices);

	ui::Vertex_PF3CB4 tmpVerts[2] = {};

	Vec3f cam2center = (_combinedXF.TransformPoint({ 0, 0, 0 }) - cam.GetInverseViewProjectionMatrix().TransformPoint({ 0, 0, -10000 })).Normalized();
	float dotX = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 1, 0, 0 }) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized(), cam2center));
	float dotY = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 0, 1, 0 }) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized(), cam2center));
	float dotZ = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 0, 0, 1 }) - _combinedXF.TransformPoint({ 0, 0, 0 })).Normalized(), cam2center));

	float visX = clamp(invlerp(0.99f, 0.97f, dotX), 0.0f, 1.0f);
	float visY = clamp(invlerp(0.99f, 0.97f, dotY), 0.0f, 1.0f);
	float visZ = clamp(invlerp(0.99f, 0.97f, dotZ), 0.0f, 1.0f);
	float visPX = clamp(invlerp(0.05f, 0.21f, dotX), 0.0f, 1.0f);
	float visPY = clamp(invlerp(0.05f, 0.21f, dotY), 0.0f, 1.0f);
	float visPZ = clamp(invlerp(0.05f, 0.21f, dotZ), 0.0f, 1.0f);

	constexpr int NUM_PARTS = 9;
	Vec3f centers[NUM_PARTS] =
	{
		{ 0, 0.15f, 0.15f }, // x plane
		{ 0.15f, 0, 0.15f }, // y plane
		{ 0.15f, 0.15f, 0 }, // z plane
		{ 0.45f, 0, 0 }, // x line
		{ 0, 0.45f, 0 }, // y line
		{ 0, 0, 0.45f }, // z line
		{ 0.95f, 0, 0 }, // x cone
		{ 0, 0.95f, 0 }, // y cone
		{ 0, 0, 0.95f }, // z cone
	};
	float distances[NUM_PARTS];
	Mat4f matWorldView = _combinedXF * cam.GetViewMatrix();
	for (int i = 0; i < NUM_PARTS; i++)
	{
		distances[i] = matWorldView.TransformPoint(centers[i]).z;
	}

	int sortedParts[NUM_PARTS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	std::sort(std::begin(sortedParts), std::end(sortedParts), [&distances](int a, int b) { return distances[a] > distances[b]; });

	constexpr unsigned DF_PRIMARY = DF_AlphaBlended | DF_ZTestOff | DF_ZWriteOff;

	SetTexture(nullptr);

	for (int i = 0; i < NUM_PARTS; i++)
	{
		switch (sortedParts[i])
		{
		case 0:
			SetRenderState(DF_PRIMARY);
			ui::prim::SetVertexColor(planeVerts, planeVC, ui::Color4f(0.9f, 0.1f, 0.0f, (_hoveredPart == MGP_XPlane ? 0.7f : 0.3f) * visPX));
			Mat4f mtxXPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.15f, 0.15f, 0) * Mat4f::RotateY(90) * _combinedXF;
			DrawIndexed(mtxXPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxXPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 1:
			SetRenderState(DF_PRIMARY);
			ui::prim::SetVertexColor(planeVerts, planeVC, ui::Color4f(0.1f, 0.9f, 0.0f, (_hoveredPart == MGP_YPlane ? 0.7f : 0.3f) * visPY));
			Mat4f mtxYPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.15f, 0.15f, 0) * Mat4f::RotateX(-90) * _combinedXF;
			DrawIndexed(mtxYPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxYPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 2:
			SetRenderState(DF_PRIMARY);
			ui::prim::SetVertexColor(planeVerts, planeVC, ui::Color4f(0.0f, 0.1f, 0.9f, (_hoveredPart == MGP_ZPlane ? 0.7f : 0.3f) * visPZ));
			Mat4f mtxZPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.15f, 0.15f, 0) * _combinedXF;
			DrawIndexed(mtxZPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxZPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 3:
			SetRenderState(DF_PRIMARY);
			tmpVerts[1].pos = { 0.9f, 0, 0 };
			tmpVerts[0].col = tmpVerts[1].col = ui::Color4f(0.9f, 0.1f, 0.0f, (_hoveredPart == MGP_XAxis ? 1.0f : 0.5f) * visX);
			Draw(_combinedXF, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 4:
			SetRenderState(DF_PRIMARY);
			tmpVerts[1].pos = { 0, 0.9f, 0 };
			tmpVerts[0].col = tmpVerts[1].col = ui::Color4f(0.1f, 0.9f, 0.0f, (_hoveredPart == MGP_YAxis ? 1.0f : 0.5f) * visY);
			Draw(_combinedXF, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 5:
			SetRenderState(DF_PRIMARY);
			tmpVerts[1].pos = { 0, 0, 0.9f };
			tmpVerts[0].col = tmpVerts[1].col = ui::Color4f(0.0f, 0.1f, 0.9f, (_hoveredPart == MGP_ZAxis ? 1.0f : 0.5f) * visZ);
			Draw(_combinedXF, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 6:
			SetRenderState(DF_PRIMARY | DF_Cull);
			ui::prim::SetVertexColor(coneVerts, coneVC, ui::Color4f(0.9f, 0.1f, 0.0f, (_hoveredPart == MGP_XAxis ? 1.0f : 0.5f) * visX));
			DrawIndexed(Mat4f::Scale(0.1f, 0.1f, 0.2f) * Mat4f::Translate(0, 0, 0.9f) * Mat4f::RotateY(-90) * _combinedXF,
				PT_Triangles, VF_Color, coneVerts, coneVC, coneIndices, coneIC);
			break;

		case 7:
			SetRenderState(DF_PRIMARY | DF_Cull);
			ui::prim::SetVertexColor(coneVerts, coneVC, ui::Color4f(0.1f, 0.9f, 0.0f, (_hoveredPart == MGP_YAxis ? 1.0f : 0.5f) * visY));
			DrawIndexed(Mat4f::Scale(0.1f, 0.1f, 0.2f) * Mat4f::Translate(0, 0, 0.9f) * Mat4f::RotateX(90) * _combinedXF,
				PT_Triangles, VF_Color, coneVerts, coneVC, coneIndices, coneIC);
			break;

		case 8:
			SetRenderState(DF_PRIMARY | DF_Cull);
			ui::prim::SetVertexColor(coneVerts, coneVC, ui::Color4f(0.0f, 0.1f, 0.9f, (_hoveredPart == MGP_ZAxis ? 1.0f : 0.5f) * visZ));
			DrawIndexed(Mat4f::Scale(0.1f, 0.1f, 0.2f) * Mat4f::Translate(0, 0, 0.9f) * _combinedXF,
				PT_Triangles, VF_Color, coneVerts, coneVC, coneIndices, coneIC);
			break;
		}
	}
}

} // ui
