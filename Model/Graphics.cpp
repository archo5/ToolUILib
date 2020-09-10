
#include "Graphics.h"
#include "Theme.h"
#include "Controls.h"
#include "System.h"


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
	if (_image && _image->GetWidth() && _image->GetHeight() && c.GetWidth() && c.GetHeight())
	{
		switch (_scaleMode)
		{
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
			float x = c.x0 + (c.GetWidth() - w) * (_anchorX + 1) * 0.5f;
			float y = c.y0 + (c.GetHeight() - h) * (_anchorY + 1) * 0.5f;
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
			float x = c.x0 + (c.GetWidth() - w) * (_anchorX + 1) * 0.5f;
			float y = c.y0 + (c.GetHeight() - h) * (_anchorY + 1) * 0.5f;
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


HueSatPicker::HueSatPicker()
{
	styleProps = Theme::current->selectorContainer;
	selectorStyle = Theme::current->selector;
}

HueSatPicker::~HueSatPicker()
{
	delete _bgImage;
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
		_sat = std::max(0.0f, std::min(1.0f, sqrtf(rx * rx + ry * ry)));
		e.context->OnChange(this);
	}
}

void HueSatPicker::OnPaint()
{
	auto cr = GetContentRect();
	int tgtw = int(std::min(cr.GetWidth(), cr.GetHeight()));
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
			float alpha = std::max(0.0f, std::min(1.0f, (d - dmax) * -1));
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
		_x = std::max(0.0f, std::min(1.0f, (e.x - cr.x0) / cr.GetWidth()));
		_y = std::max(0.0f, std::min(1.0f, (e.y - cr.y0) / cr.GetHeight()));
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
	HandleEvent(UIEventType::Change) = [this](UIEvent&) { Rerender(); };
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
					+ EventHandler(UIEventType::DragDrop, [this](UIEvent&)
				{
					if (auto* cddd = ui::DragDrop::GetData<ColorDragDropData>())
					{
						_color.SetRGBA(cddd->color);
						Rerender();
					}
				});
				ctx->Make<ColorBlock>()->SetColor(_color.GetRGBA().GetOpaque()) + Width(50) + Height(60) + Padding(0);
				ctx->Make<ColorBlock>()->SetColor(_color.GetRGBA()) + Width(50) + Height(60) + Padding(0);
				ctx->Pop();

				//ctx->PushBox() + Layout(style::layouts::StackExpand()) + StackingDirection(style::StackingDirection::LeftToRight) + Height(22);
				ctx->Text("Hex:") + Width(30) + Height(22);
				ctx->Make<Textbox>()->Init(_color.hex) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _color._UpdateHex(); });
				//ctx->Pop();
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
	ctx->Make<ColorInspectBlock>()->SetColor(_color.GetRGBA())
		+ EventHandler(UIEventType::Click, [this](UIEvent& e)
	{
		if (e.GetButton() == UIMouseButton::Left)
		{
			ColorPickerWindow cpw;
			cpw.SetColor(_color);
			cpw.Show();
			_color = cpw.GetColor();
			e.context->OnChange(this);
		}
	});
}

} // ui
