
#include "Graphics.h"
#include "Theme.h"
#include "Controls.h"
#include "System.h"
#include "../Render/RHI.h"


namespace ui {


static StaticID sid_color_block("color_block");
void ColorBlock::OnInit()
{
	styleProps = Theme::current->GetStyle(sid_color_block);
	_bgImage = Theme::current->GetImage(ThemeImage::CheckerboardBackground);
}

void ColorBlock::OnPaint()
{
	styleProps->background_painter->Paint(this);

	auto r = GetContentRect();

	if (!_color.IsOpaque())
	{
		draw::RectTex(r.x0, r.y0, r.x1, r.y1, _bgImage, 0, 0, r.GetWidth() / _bgImage->GetWidth(), r.GetHeight() / _bgImage->GetHeight());
	}

	draw::RectCol(r.x0, r.y0, r.x1, r.y1, _color);

	PaintChildren();
}


static StaticID sid_color_inspect_block("color_inspect_block");
void ColorInspectBlock::OnInit()
{
	styleProps = Theme::current->GetStyle(sid_color_inspect_block);
	_bgImage = Theme::current->GetImage(ThemeImage::CheckerboardBackground);
}

void ColorInspectBlock::OnPaint()
{
	styleProps->background_painter->Paint(this);

	auto r = GetContentRect();

	float hbar = roundf(ResolveUnits(alphaBarHeight, r.GetWidth()));
	UIRect r_lf = { r.x0, r.y0, roundf((r.x0 + r.x1) / 2), r.y1 - hbar };
	UIRect r_rt = { r_lf.x1, r.y0, r.x1, r_lf.y1 };

	if (!_color.IsOpaque())
	{
		draw::RectTex(r_rt.x0, r_rt.y0, r_rt.x1, r_rt.y1, _bgImage, 0, 0, r_rt.GetWidth() / _bgImage->GetWidth(), r_rt.GetHeight() / _bgImage->GetHeight());
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


void DrawImage(UIRect c, draw::IImage* img, ScaleMode sm, float ax, float ay)
{
	if (img && img->GetWidth() && img->GetHeight() && c.GetWidth() && c.GetHeight())
	{
		switch (sm)
		{
		case ScaleMode::None: {
			float w = img->GetWidth(), h = img->GetHeight();
			float x = roundf(c.x0 + (c.GetWidth() - w) * ax);
			float y = roundf(c.y0 + (c.GetHeight() - h) * ay);
			draw::RectTex(x, y, x + w, y + h, img);
			break; }
		case ScaleMode::Stretch:
			draw::RectTex(c.x0, c.y0, c.x1, c.y1, img);
			break;
		case ScaleMode::Fit: {
			float iasp = img->GetWidth() / (float)img->GetHeight();
			float w = c.GetWidth(), h = c.GetHeight();
			float rasp = w / h;
			if (iasp > rasp) // matched width, adjust height
				h = w / iasp;
			else
				w = h * iasp;
			float x = roundf(c.x0 + (c.GetWidth() - w) * ax);
			float y = roundf(c.y0 + (c.GetHeight() - h) * ay);
			draw::RectTex(x, y, x + w, y + h, img);
			break; }
		case ScaleMode::Fill: {
			float iasp = img->GetWidth() / (float)img->GetHeight();
			float w = c.GetWidth(), h = c.GetHeight();
			float rasp = w / h;
			if (iasp < rasp) // matched width, adjust height
				h = w / iasp;
			else
				w = h * iasp;
			float x = roundf(c.x0 + (c.GetWidth() - w) * ax);
			float y = roundf(c.y0 + (c.GetHeight() - h) * ay);
			draw::RectTex(c.x0, c.y0, c.x1, c.y1, img,
				(c.x0 - x) / w,
				(c.y0 - y) / h,
				(c.x1 - x) / w,
				(c.y1 - y) / h);
			break; }
		}
	}
}


void ImageElement::OnInit()
{
	styleProps = Theme::current->image;
}

void ImageElement::OnPaint()
{
	auto c = GetContentRect();
	if (draw::GetCurrentScissorRectF().Overlaps(c))
	{
		if (_tryDelayLoad)
		{
			_tryDelayLoad = false;
			_image = draw::ImageLoadFromFile(_delayLoadPath, draw::TexFlags::None);
		}

		styleProps->background_painter->Paint(this);

		if (_bgImage)
		{
			draw::RectTex(c.x0, c.y0, c.x1, c.y1, _bgImage, 0, 0, c.GetWidth() / _bgImage->GetWidth(), c.GetHeight() / _bgImage->GetHeight());
		}
		DrawImage(c, _image, _scaleMode, _anchorX, _anchorY);
	}

	PaintChildren();
}

void ImageElement::GetSize(Coord& outWidth, Coord& outHeight)
{
	if (_image)
	{
		outWidth = _image->GetWidth();
		outHeight = _image->GetHeight();
	}
}

ImageElement& ImageElement::SetImage(draw::IImage* img)
{
	_image = img;
	_tryDelayLoad = false;
	_delayLoadPath.clear();
	return *this;
}

ImageElement& ImageElement::SetPath(StringView path)
{
	return SetImage(draw::ImageLoadFromFile(path, draw::TexFlags::None));
}

ImageElement& ImageElement::SetDelayLoadPath(StringView path)
{
	if (_image && _image->GetPath() == path)
		return *this;
	_image = nullptr;
	_tryDelayLoad = true;
	_delayLoadPath = to_string(path);
	return *this;
}

ImageElement& ImageElement::SetScaleMode(ScaleMode sm, float ax, float ay)
{
	_scaleMode = sm;
	_anchorX = ax;
	_anchorY = ay;
	return *this;
}

ImageElement& ImageElement::SetAlphaBackgroundEnabled(bool enabled)
{
	_bgImage = enabled ? Theme::current->GetImage(ThemeImage::CheckerboardBackground) : nullptr;
	return *this;
}


void HueSatPicker::OnInit()
{
	styleProps = Theme::current->selectorContainer;
	selectorStyle = Theme::current->selector;
	SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
}

void HueSatPicker::OnEvent(Event& e)
{
	if ((e.type == EventType::MouseMove && IsClicked()) ||
		(e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left))
	{
		auto cr = GetContentRect();
		if (cr.GetWidth() < cr.GetHeight())
			cr.y1 = cr.y0 + cr.GetWidth();
		else
			cr.x1 = cr.x0 + cr.GetHeight();
		float hw = cr.GetWidth() / 2;
		float rx = (e.position.x - (cr.x0 + hw)) / hw;
		float ry = (e.position.y - (cr.y0 + hw)) / hw;
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

	draw::RectTex(cr.x0, cr.y0, cr.x0 + tgtw, cr.y0 + tgtw, _bgImage);

	float hw = tgtw / 2.0f;
	float cx = cr.x0 + hw;
	float cy = cr.y0 + hw;
	float sx = cx + sinf(_hue * 3.14159f * 2) * _sat * hw;
	float sy = cy - cosf(_hue * 3.14159f * 2) * _sat * hw;
	float sw = ResolveUnits(selectorStyle->width, cr.GetWidth());
	float sh = ResolveUnits(selectorStyle->height, cr.GetWidth());
	sx = roundf(sx - sw / 2);
	sy = roundf(sy - sh / 2);
	PaintInfo info(this);
	info.rect = { sx, sy, sx + sw, sy + sh };
	selectorStyle->background_painter->Paint(info);

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

	_bgImage = draw::ImageCreateFromCanvas(c);
}


void ColorDragDropData::Build()
{
	Make<ColorInspectBlock>().SetColor(color) + SetWidth(40) + SetPadding(0);
}


ColorCompPicker2D::ColorCompPicker2D()
{
	styleProps = Theme::current->selectorContainer;
	selectorStyle = Theme::current->selector;
}

void ColorCompPicker2D::OnEvent(Event& e)
{
	if ((e.type == EventType::MouseMove && IsClicked()) ||
		(e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left))
	{
		auto cr = GetContentRect();
		_x = clamp((e.position.x - cr.x0) / cr.GetWidth(), 0.0f, 1.0f);
		_y = clamp((e.position.y - cr.y0) / cr.GetHeight(), 0.0f, 1.0f);
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

	draw::RectTex(cr.x0, cr.y0, cr.x1, cr.y1, _bgImage);

	float sx = lerp(cr.x0, cr.x1, _settings._invx ? 1 - _x : _x);
	float sy = lerp(cr.y0, cr.y1, _settings._invy ? 1 - _y : _y);
	float sw = ResolveUnits(selectorStyle->width, cr.GetWidth());
	float sh = ResolveUnits(selectorStyle->height, cr.GetWidth());
	sx = roundf(sx - sw / 2);
	sy = roundf(sy - sh / 2);
	PaintInfo info(this);
	info.rect = { sx, sy, sx + sw, sy + sh };
	selectorStyle->background_painter->Paint(info);

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

	_bgImage = draw::ImageCreateFromCanvas(c);
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
	draw::RectGradH(r.x0, r.y0, r.x1, r.y1, a, b);
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

void ColorPicker::Build()
{
	*this + RebuildOnChange();

	PushBox()
		+ SetLayout(layouts::StackExpand())
		+ Set(StackingDirection::LeftToRight);
	{
		// left side
		PushBox()
			+ SetWidth(Coord::Fraction(0)); // TODO any way to make this unnecessary?
		{
			auto& hsp = Make<HueSatPicker>().Init(_color._hue, _color._sat)
				+ SetWidth(240)
				+ SetHeight(240);
			hsp.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateHSV(); };
		}
		Pop();

		FloatLimits limit = { 0, 1 };

		// right side
		PushBox() + SetPadding(8);
		{
		//	PushBox() + SetPadding(8);
			//Text("HSV");

			Property::Begin();
			{
				Text("H") + SetWidth(10);
				auto& sl = Make<Slider>().Init(_color._hue, limit);
				sl.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateHSV(); };
				StyleAccessor a = sl.GetTrackStyle();
				a.SetBackgroundPainter(CreateFunctionPainter([prev{ a.GetBackgroundPainter() }, sat{ _color._sat }, val{ _color._val }](const PaintInfo& info)
				{
					prev->Paint(info);
					for (int i = 0; i < 6; i++)
						DrawHGradQuad(RectHSlice(info.rect, i, 6), Color4f::HSV(i / 6.0f, sat, val), Color4f::HSV((i + 1) / 6.0f, sat, val));
				}));
				sl.GetTrackFillStyle().SetBackgroundPainter(EmptyPainter::Get());
				Make<Textbox>().Init(_color._hue) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateHSV(); });
			}
			Property::End();

			Property::Begin();
			{
				Text("S") + SetWidth(10);
				auto& sl = Make<Slider>().Init(_color._sat, limit);
				sl.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateHSV(); };
				StyleAccessor a = sl.GetTrackStyle();
				a.SetBackgroundPainter(CreateFunctionPainter([prev{ a.GetBackgroundPainter() }, hue{ _color._hue }, val{ _color._val }](const PaintInfo& info)
				{
					prev->Paint(info);
					DrawHGradQuad(info.rect, Color4f::HSV(hue, 0, val), Color4f::HSV(hue, 1, val));
				}));
				sl.GetTrackFillStyle().SetBackgroundPainter(EmptyPainter::Get());
				Make<Textbox>().Init(_color._sat) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateHSV(); });
			}
			Property::End();

			Property::Begin();
			{
				Text("V") + SetWidth(10);
				auto& sl = Make<Slider>().Init(_color._val, limit);
				sl.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateHSV(); };
				StyleAccessor a = sl.GetTrackStyle();
				a.SetBackgroundPainter(CreateFunctionPainter([prev{ a.GetBackgroundPainter() }, hue{ _color._hue }, sat{ _color._sat }](const PaintInfo& info)
				{
					prev->Paint(info);
					DrawHGradQuad(info.rect, Color4f::HSV(hue, sat, 0), Color4f::HSV(hue, sat, 1));
				}));
				sl.GetTrackFillStyle().SetBackgroundPainter(EmptyPainter::Get());
				Make<Textbox>().Init(_color._val) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateHSV(); });
			}
			Property::End();

			Property::Begin();
			{
				Text("R") + SetWidth(10);
				auto& sl = Make<Slider>().Init(_color._rgba.r, limit);
				sl.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateRGB(); };
				StyleAccessor a = sl.GetTrackStyle();
				a.SetBackgroundPainter(CreateFunctionPainter([prev{ a.GetBackgroundPainter() }, col{ _color._rgba }](const PaintInfo& info)
				{
					prev->Paint(info);
					DrawHGradQuad(info.rect, { 0, col.g, col.b }, { 1, col.g, col.b });
				}));
				sl.GetTrackFillStyle().SetBackgroundPainter(EmptyPainter::Get());
				Make<Textbox>().Init(_color._rgba.r) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateRGB(); });
			}
			Property::End();

			Property::Begin();
			{
				Text("G") + SetWidth(10);
				auto& sl = Make<Slider>().Init(_color._rgba.g, limit);
				sl.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateRGB(); };
				StyleAccessor a = sl.GetTrackStyle();
				a.SetBackgroundPainter(CreateFunctionPainter([prev{ a.GetBackgroundPainter() }, col{ _color._rgba }](const PaintInfo& info)
				{
					prev->Paint(info);
					DrawHGradQuad(info.rect, { col.r, 0, col.b }, { col.r, 1, col.b });
				}));
				sl.GetTrackFillStyle().SetBackgroundPainter(EmptyPainter::Get());
				Make<Textbox>().Init(_color._rgba.g) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateRGB(); });
			}
			Property::End();

			Property::Begin();
			{
				Text("B") + SetWidth(10);
				auto& sl = Make<Slider>().Init(_color._rgba.b, limit);
				sl.HandleEvent(EventType::Change) = [this](Event&) { _color._UpdateRGB(); };
				StyleAccessor a = sl.GetTrackStyle();
				a.SetBackgroundPainter(CreateFunctionPainter([prev{ a.GetBackgroundPainter() }, col{ _color._rgba }](const PaintInfo& info)
				{
					prev->Paint(info);
					DrawHGradQuad(info.rect, { col.r, col.g, 0 }, { col.r, col.g, 1 });
				}));
				sl.GetTrackFillStyle().SetBackgroundPainter(EmptyPainter::Get());
				Make<Textbox>().Init(_color._rgba.b) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateRGB(); });
			}
			Property::End();

			Property::Begin();
			{
				Text("A") + SetWidth(10);
				Make<Slider>().Init(_color._rgba.a, limit);
				Make<Textbox>().Init(_color._rgba.a) + SetWidth(50);
			}
			Property::End();

			Property::Begin();
			{
				Push<Panel>()
					+ Set(StackingDirection::LeftToRight)
					+ SetPadding(3)
					+ MakeDraggable([this](Event& e) { DragDrop::SetData(new ColorDragDropData(_color.GetRGBA())); })
					+ AddEventHandler(EventType::DragDrop, [this](Event& e)
				{
					if (auto* cddd = DragDrop::GetData<ColorDragDropData>())
					{
						_color.SetRGBA(cddd->color);
						e.context->OnChange(this);
					}
				});
				Make<ColorBlock>().SetColor(_color.GetRGBA().GetOpaque()) + SetWidth(50) + SetHeight(60) + SetPadding(0);
				Make<ColorBlock>().SetColor(_color.GetRGBA()) + SetWidth(50) + SetHeight(60) + SetPadding(0);
				Pop();

				PushBox();

				PushBox() + SetLayout(layouts::StackExpand()) + Set(StackingDirection::LeftToRight) + SetHeight(22);
				Make<BoxElement>() + SetWidth(Coord::Fraction(1));
				Text("Hex:") + SetHeight(22);
				Make<Textbox>().Init(_color.hex) + SetWidth(50) + AddEventHandler(EventType::Change, [this](Event&) { _color._UpdateHex(); });
				Pop();

				auto& pick = MakeWithText<Button>("Pick");
				pick.HandleEvent() = [this](Event& e)
				{
					if (e.type == EventType::SetCursor)
					{
						// TODO is it possible to set the cursor between click/capture?
						e.context->SetDefaultCursor(DefaultCursor::Crosshair);
						e.StopPropagation();
					}
					if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
					{
						auto col = platform::GetColorAtScreenPos(platform::GetCursorScreenPos());
						_color.SetRGBA(col);
						e.context->OnChange(this);
					}
				};

				Pop();
			}
			Property::End();

		//	Pop();
		}
		Pop();
	}
	Pop();

	PushBox()
		+ SetLayout(layouts::InlineBlock())
		+ Set(StackingDirection::LeftToRight)
		+ SetPadding(8);
	{
		auto& sc = GetSavedColors();
		for (int i = 0; i < 16; i++)
		{
			Make<ColorBlock>().SetColor(sc.colors[i])
				+ MakeDraggable([this, i]() { DragDrop::SetData(new ColorDragDropData(GetSavedColors().colors[i])); })
				+ AddEventHandler(EventType::DragDrop, [this, i](Event&)
			{
				if (auto* cddd = DragDrop::GetData<ColorDragDropData>())
				{
					GetSavedColors().colors[i] = cddd->color;
					Rebuild();
				}
			});
		}
	}
	Pop();
}


ColorPickerWindow::ColorPickerWindow()
{
	SetTitle("Color picker");
	SetSize(500, 300);
}

void ColorPickerWindow::OnBuild()
{
	auto& picker = Make<ColorPicker>().SetColor(_color);
	picker.HandleEvent(EventType::Change) = [this, &picker](Event&)
	{
		_color = picker.GetColor();
	};
	Make<DefaultOverlayBuilder>();
}


void ColorEdit::Build()
{
	auto& cib = Make<ColorInspectBlock>().SetColor(_color.GetRGBA());
	cib.SetFlag(UIObject_DB_Button, true);
	cib + AddEventHandler(EventType::Click, [this](Event& e)
	{
		if (e.GetButton() == MouseButton::Left)
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

ColorEdit& ColorEdit::SetColor(const MultiFormatColor& c)
{
	_color = c;
	return *this;
}


struct ColorPickerWindowRT : NativeWindowBase
{
	ColorPickerWindowRT()
	{
		SetTitle("Color picker");
		SetSize(500, 300);
	}
	void OnBuild() override
	{
		auto& picker = Make<ColorPicker>().SetColor(_editor->GetColor());
		picker.HandleEvent(EventType::Change) = [this, &picker](Event& e)
		{
			_editor->_color = picker.GetColor();
			e.context->OnChange(_editor);
			e.context->OnCommit(_editor);
		};
		Make<DefaultOverlayBuilder>();
	}

	ColorEditRT* _editor = nullptr;
};


void ColorEditRT::Build()
{
	auto& cib = Make<ColorInspectBlock>().SetColor(_color.GetRGBA());
	cib.SetFlag(UIObject_DB_Button, true);
	cib + AddEventHandler(EventType::Click, [this](Event& e)
	{
		if (e.GetButton() == MouseButton::Left)
		{
			if (!_rtWindow)
				_rtWindow = new ColorPickerWindowRT;
			_rtWindow->_editor = this;
			_rtWindow->SetVisible(true);
		}
	});
}

void ColorEditRT::OnDestroy()
{
	delete _rtWindow;
	_rtWindow = nullptr;
}

ColorEditRT& ColorEditRT::SetColor(const MultiFormatColor& c)
{
	_color = c;
	if (_rtWindow)
		_rtWindow->Rebuild();
	return *this;
}


void View2D::OnPaint()
{
	styleProps->background_painter->Paint(this);

	auto r = finalRectC;
	if (draw::PushScissorRect(r.Cast<int>()))
	{
		if (onPaint)
			onPaint(r);
	}
	draw::PopScissorRect();

	PaintChildren();
}


void View3D::OnPaint()
{
	styleProps->background_painter->Paint(this);

	auto r = finalRectC;
	if (draw::PushScissorRect(r.Cast<int>()))
	{
		rhi::Begin3DMode(r.Cast<int>());

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

Point2f CameraBase::WindowToNormalizedPoint(Point2f p) const
{
	auto& cr = _windowRect;
	return
	{
		lerp(-1, 1, invlerp(cr.x0, cr.x1, p.x)),
		lerp(1, -1, invlerp(cr.y0, cr.y1, p.y)),
	};
}

Point2f CameraBase::NormalizedToWindowPoint(Point2f p) const
{
	auto& cr = _windowRect;
	return
	{
		lerp(cr.x0, cr.x1, invlerp(-1, 1, p.x)),
		lerp(cr.y0, cr.y1, invlerp(1, -1, p.y)),
	};
}

Point2f CameraBase::WorldToNormalizedPoint(const Vec3f& p) const
{
	auto np = GetViewProjectionMatrix().TransformPoint(p);
	return { np.x, np.y };
}

Point2f CameraBase::WorldToWindowPoint(const Vec3f& p) const
{
	return NormalizedToWindowPoint(WorldToNormalizedPoint(p));
}

float CameraBase::WorldToWindowSize(float size, const Vec3f& refp) const
{
	float dist = GetViewMatrix().TransformPoint(refp).z;
	return size * GetWindowRect().GetHeight() * GetProjectionMatrix().m[1][1] * 0.5f / dist;
}

float CameraBase::WindowToWorldSize(float size, const Vec3f& refp) const
{
	return size / WorldToWindowSize(1.0f, refp);
}

Ray3f CameraBase::GetRayNP(Point2f p) const
{
	return GetCameraRay(GetInverseViewProjectionMatrix(), p.x, p.y);
}

Ray3f CameraBase::GetLocalRayNP(Point2f p, const Mat4f& world2local) const
{
	return GetCameraRay(GetInverseViewProjectionMatrix() * world2local, p.x, p.y);
}

Ray3f CameraBase::GetRayWP(Point2f p) const
{
	return GetRayNP(WindowToNormalizedPoint(p));
}

Ray3f CameraBase::GetLocalRayWP(Point2f p, const Mat4f& world2local) const
{
	return GetLocalRayNP(WindowToNormalizedPoint(p), world2local);
}

Ray3f CameraBase::GetRayEP(const Event& e) const
{
	return GetRayNP(WindowToNormalizedPoint(e.position));
}

Ray3f CameraBase::GetLocalRayEP(const Event& e, const Mat4f& world2local) const
{
	return GetLocalRayNP(WindowToNormalizedPoint(e.position), world2local);
}


OrbitCamera::OrbitCamera(bool rh) : rightHanded(rh)
{
	_UpdateViewMatrix();
}

bool OrbitCamera::OnEvent(Event& e)
{
	if (e.IsPropagationStopped())
		return false;
	if (e.type == EventType::ButtonDown)
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
	if (e.type == EventType::ButtonUp)
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
	if (e.type == EventType::MouseMove)
	{
		if (rotating)
			Rotate(e.delta.x * rotationSpeed, e.delta.y * rotationSpeed);
		if (panning)
			Pan(e.delta.x / e.current->GetContentRect().GetWidth(), e.delta.y / e.current->GetContentRect().GetHeight());
		return rotating || panning;
	}
	if (e.type == EventType::MouseScroll)
	{
		Zoom(e.delta.y == 0 ? 0 : e.delta.y < 0 ? 1 : -1);
		return true;
	}
	return false;
}

void OrbitCamera::SerializeState(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "OrbitCamera(state)");

	OnField(oi, "pivot", pivot);
	OnField(oi, "yaw", yaw);
	OnField(oi, "pitch", pitch);
	OnField(oi, "distance", distance);

	oi.EndObject();
}

void OrbitCamera::Rotate(float dx, float dy)
{
	yaw += dx * (rightHanded ? -1 : 1);
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

void OrbitCamera::ResetState()
{
	pivot = {};
	yaw = 45;
	pitch = 45;
	distance = 1;

	_UpdateViewMatrix();
}

void OrbitCamera::_UpdateViewMatrix()
{
	float cp = cosf(pitch * DEG2RAD);
	float sp = sinf(pitch * DEG2RAD);
	float cy = cosf(yaw * DEG2RAD);
	float sy = sinf(yaw * DEG2RAD);
	Vec3f dir = { cy * cp, sy * cp, sp };
	Vec3f pos = pivot + dir * distance;
	Mat4f vm = rightHanded
		? Mat4f::LookAtRH(pos, pivot, { 0, 0, 1 })
		: Mat4f::LookAtLH(pos, pivot, { 0, 0, 1 });

	SetViewMatrix(vm);
}

} // ui
