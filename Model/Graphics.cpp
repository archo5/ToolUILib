
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


} // ui
