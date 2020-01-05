
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
			float iasp = _image->GetWidth() / _image->GetHeight();
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
			float iasp = _image->GetWidth() / _image->GetHeight();
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

} // ui
