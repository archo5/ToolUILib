
#include "Graphics.h"
#include "Theme.h"


namespace ui {

ImageElement::ImageElement()
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
		float hueval = fmodf(atan2(rx, -ry) / (3.14159f * 2) + 1.0f, 1.0f);
		float satval = std::max(0.0f, std::min(1.0f, sqrtf(rx * rx + ry * ry)));
		if (_hue)
			*_hue = hueval;
		if (_sat)
			*_sat = satval;
		e.context->OnChange(this);
	}
}

void HueSatPicker::OnPaint()
{
	auto cr = GetContentRect();
	int tgtw = std::min(cr.GetWidth(), cr.GetHeight());
	cr.x1 = cr.x0 + tgtw;
	cr.y1 = cr.y0 + tgtw;
	if (!_bgImage || _bgImage->GetWidth() != tgtw)
		_RegenerateBackground(tgtw);

	GL::SetTexture(_bgImage->_texture);
	GL::BatchRenderer br;
	br.Begin();
	br.Quad(cr.x0, cr.y0, cr.x0 + tgtw, cr.y0 + tgtw, 0, 0, 1, 1);
	br.End();

	float hw = tgtw / 2.0f;
	float hue = _hue ? *_hue : 0;
	float sat = _sat ? *_sat : 0;
	float cx = cr.x0 + hw;
	float cy = cr.y0 + hw;
	float sx = cx + sinf(*_hue * 3.14159f * 2) * sat * hw;
	float sy = cy - cosf(*_hue * 3.14159f * 2) * sat * hw;
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


ColorPicker2D::ColorPicker2D()
{
	styleProps = Theme::current->selectorContainer;
	selectorStyle = Theme::current->selector;
}

ColorPicker2D::~ColorPicker2D()
{
	delete _bgImage;
}

void ColorPicker2D::OnEvent(UIEvent& e)
{
	if ((e.type == UIEventType::MouseMove && IsClicked()) ||
		(e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left))
	{
		auto cr = GetContentRect();
		float xval = std::max(0.0f, std::min(1.0f, (e.x - cr.x0) / cr.GetWidth()));
		float yval = std::max(0.0f, std::min(1.0f, (e.y - cr.y0) / cr.GetHeight()));
		if (_x)
			*_x = xval;
		if (_y)
			*_y = yval;
		e.context->OnChange(this);
	}
}

void ColorPicker2D::OnPaint()
{
	auto cr = GetContentRect();
	int tgtw = cr.GetWidth();
	int tgth = cr.GetHeight();

	if (!_bgImage || _bgImage->GetWidth() != tgtw || _bgImage->GetHeight() != tgth)
		_RegenerateBackground(tgtw, tgth);

	GL::SetTexture(_bgImage->_texture);
	GL::BatchRenderer br;
	br.Begin();
	br.Quad(cr.x0, cr.y0, cr.x1, cr.y1, 0, 0, 1, 1);
	br.End();

	float sx = lerp(cr.x0, cr.x1, _x ? *_x : 0);
	float sy = lerp(cr.y0, cr.y1, _y ? *_y : 0);
	float sw = ResolveUnits(selectorStyle->width, cr.GetWidth());
	float sh = ResolveUnits(selectorStyle->height, cr.GetWidth());
	sx = roundf(sx - sw / 2);
	sy = roundf(sy - sh / 2);
	style::PaintInfo info(this);
	info.rect = { sx, sy, sx + sw, sy + sh };
	selectorStyle->paint_func(info);

	PaintChildren();
}

void ColorPicker2D::_RegenerateBackground(int w, int h)
{
	Canvas c(w, h);
	auto* px = c.GetPixels();

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			float fx = x / float(w);
			float fy = y / float(h);
			Color4f c(0, 1);
			switch (_mode)
			{
			case CM_RGB:
				switch (_cx)
				{
				case CC_Red: c.r = fx; break;
				case CC_Green: c.g = fx; break;
				case CC_Blue: c.b = fx; break;
				}
				switch (_cy)
				{
				case CC_Red: c.r = fy; break;
				case CC_Green: c.g = fy; break;
				case CC_Blue: c.b = fy; break;
				}
				break;
			case CM_HSV: {
				float h = 0, s = 1, v = 1;
				switch (_cx)
				{
				case CC_Hue: h = fx; break;
				case CC_Sat: s = fx; break;
				case CC_Val: v = fx; break;
				}
				switch (_cy)
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
}


} // ui
