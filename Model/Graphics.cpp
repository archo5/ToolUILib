
#include "Graphics.h"
#include "Theme.h"
#include "Controls.h"
#include "System.h"


namespace ui {


void ColorBlock::OnInit()
{
	styleProps = Theme::current->colorBlock;
}

void ColorBlock::OnPaint()
{
	styleProps->paint_func(this);

	GL::SetTexture(0);
	GL::BatchRenderer br;
	br.Begin();
	br.SetColor(_color.r, _color.g, _color.b, _color.a);
	auto r = GetContentRect();
	br.Quad(r.x0, r.y0, r.x1, r.y1, 0, 0, 1, 1);
	br.End();

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
		GL::SetTexture(_image->_texture);
		GL::BatchRenderer br;
		br.Begin();
		br.SetColor(1, 1, 1);
		switch (_scaleMode)
		{
		case ScaleMode::Stretch:
			br.Quad(c.x0, c.y0, c.x1, c.y1, 0, 0, 1, 1);
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
			br.Quad(x, y, x + w, y + h, 0, 0, 1, 1);
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
			br.Quad(c.x0, c.y0, c.x1, c.y1,
				(c.x0 - x) / w,
				(c.y0 - y) / h,
				(c.x1 - x) / w,
				(c.y1 - y) / h);
			break; }
		}
		br.End();
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

	GL::SetTexture(_bgImage->_texture);
	GL::BatchRenderer br;
	br.Begin();
	br.SetColor(1, 1, 1, 1);
	br.Quad(cr.x0, cr.y0, cr.x0 + tgtw, cr.y0 + tgtw, 0, 0, 1, 1);
	br.End();

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

	GL::SetTexture(_bgImage->_texture);
	GL::BatchRenderer br;
	br.Begin();
	br.Quad(cr.x0, cr.y0, cr.x1, cr.y1, 0, 0, 1, 1);
	br.End();

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


static std::string gstr(float f)
{
	char bfr[64];
	snprintf(bfr, 64, "%g", f);
	return bfr;
}

static void DrawHGradQuad(UIRect r, Color4f a, Color4f b)
{
	GL::SetTexture(0);
	GL::BatchRenderer br;
	br.Begin();
	br.SetColor(a.r, a.g, a.b, a.a);
	br.Pos(r.x0, r.y1);
	br.Pos(r.x0, r.y0);
	br.SetColor(b.r, b.g, b.b, b.a);
	br.Pos(r.x1, r.y0);
	br.Pos(r.x1, r.y0);
	br.Pos(r.x1, r.y1);
	br.SetColor(a.r, a.g, a.b, a.a);
	br.Pos(r.x0, r.y1);
	br.End();
}

static UIRect RectHSlice(UIRect r, float i, float n)
{
	return { lerp(r.x0, r.x1, i / n), r.y0, lerp(r.x0, r.x1, (i + 1) / n), r.y1 };
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
			auto& hsp = ctx->Make<HueSatPicker>()->Init(_hue, _sat)
				+ Width(240)
				+ Height(240);
			hsp.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateHSV(); };
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
				auto& sl = ctx->Make<Slider>()->Init(_hue, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateHSV(); };
				auto& fn = style::Accessor(sl.trackStyle).MutablePaintFunc();
				fn = [fn, sat{ _sat }, val{ _val }](const style::PaintInfo& info)
				{
					fn(info);
					for (int i = 0; i < 6; i++)
						DrawHGradQuad(RectHSlice(info.rect, i, 6), Color4f::HSV(i / 6.0f, sat, val), Color4f::HSV((i + 1) / 6.0f, sat, val));
				};
				style::Accessor(sl.trackFillStyle).MutablePaintFunc() = [](const style::PaintInfo& info) {};
				ctx->Make<Textbox>()->Init(_hue) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateHSV(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("S") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_sat, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateHSV(); };
				auto& fn = style::Accessor(sl.trackStyle).MutablePaintFunc();
				fn = [fn, hue{ _hue }, val{ _val }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, Color4f::HSV(hue, 0, val), Color4f::HSV(hue, 1, val));
				};
				style::Accessor(sl.trackFillStyle).MutablePaintFunc() = [](const style::PaintInfo& info) {};
				ctx->Make<Textbox>()->Init(_sat) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateHSV(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("V") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_val, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateHSV(); };
				auto& fn = style::Accessor(sl.trackStyle).MutablePaintFunc();
				fn = [fn, hue{ _hue }, sat{ _sat }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, Color4f::HSV(hue, sat, 0), Color4f::HSV(hue, sat, 1));
				};
				style::Accessor(sl.trackFillStyle).MutablePaintFunc() = [](const style::PaintInfo& info) {};
				ctx->Make<Textbox>()->Init(_val) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateHSV(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("R") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_rgba.r, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateRGB(); };
				auto& fn = style::Accessor(sl.trackStyle).MutablePaintFunc();
				fn = [fn, col{ _rgba }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, { 0, col.g, col.b }, { 1, col.g, col.b });
				};
				style::Accessor(sl.trackFillStyle).MutablePaintFunc() = [](const style::PaintInfo& info) {};
				ctx->Make<Textbox>()->Init(_rgba.r) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateRGB(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("G") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_rgba.g, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateRGB(); };
				auto& fn = style::Accessor(sl.trackStyle).MutablePaintFunc();
				fn = [fn, col{ _rgba }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, { col.r, 0, col.b }, { col.r, 1, col.b });
				};
				style::Accessor(sl.trackFillStyle).MutablePaintFunc() = [](const style::PaintInfo& info) {};
				ctx->Make<Textbox>()->Init(_rgba.g) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateRGB(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("B") + Width(10);
				auto& sl = ctx->Make<Slider>()->Init(_rgba.b, limit);
				sl.HandleEvent(UIEventType::Change) = [this](UIEvent&) { _UpdateRGB(); };
				auto& fn = style::Accessor(sl.trackStyle).MutablePaintFunc();
				fn = [fn, col{ _rgba }](const style::PaintInfo& info)
				{
					fn(info);
					DrawHGradQuad(info.rect, { col.r, col.g, 0 }, { col.r, col.g, 1 });
				};
				style::Accessor(sl.trackFillStyle).MutablePaintFunc() = [](const style::PaintInfo& info) {};
				ctx->Make<Textbox>()->Init(_rgba.b) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateRGB(); });
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				ctx->Text("A") + Width(10);
				ctx->Make<Slider>()->Init(_rgba.a, limit);
				ctx->Make<Textbox>()->Init(_rgba.a) + Width(50);
			}
			Property::End(ctx);

			Property::Begin(ctx);
			{
				*ctx->Push<Panel>() + StackingDirection(style::StackingDirection::LeftToRight) + Padding(3);
				ctx->Make<ColorBlock>()->SetColor({ _rgba.r, _rgba.g, _rgba.b, 1 }) + Width(50) + Height(60) + Padding(0);
				ctx->Make<ColorBlock>()->SetColor(_rgba) + Width(50) + Height(60) + Padding(0);
				ctx->Pop();

				ctx->Text("Hex:") + Width(30);
				ctx->Make<Textbox>()->Init(hex) + Width(50) + EventHandler(UIEventType::Change, [this](UIEvent&) { _UpdateHex(); });
			}
			Property::End(ctx);

		//	ctx->Pop();
		}
		ctx->Pop();
	}
	ctx->Pop();
}


} // ui
