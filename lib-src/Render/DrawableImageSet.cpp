
#include "DrawableImageSet.h"

#include "Render.h"


namespace ui {
namespace draw {

ImageSetSizeMode g_imageSetSizeModes[3] =
{
	ImageSetSizeMode::NearestScaleDown,
	ImageSetSizeMode::NearestScaleDown,
	ImageSetSizeMode::NearestScaleDown,
};

static inline ImageSetSizeMode GetFinalSizeMode(ImageSetType type, ImageSetSizeMode mode)
{
	if (mode == ImageSetSizeMode::Default)
		mode = g_imageSetSizeModes[unsigned(type)];
	return mode;
}

ImageSet::Entry* ImageSet::FindEntryForSize(Size2f size)
{
	if (entries.IsEmpty())
		return nullptr;

	auto mode = GetFinalSizeMode(type, sizeMode);

	Entry* last = entries.Data();
	if (mode == ImageSetSizeMode::NearestScaleUp || mode == ImageSetSizeMode::NearestNoScale)
	{
		for (auto& e : entries)
		{
			if (e.image->GetWidth() > size.x || e.image->GetHeight() > size.y)
				break;
			last = &e;
		}
	}
	else
	{
		for (auto& e : entries)
		{
			last = &e;
			if (e.image->GetWidth() >= size.x && e.image->GetHeight() >= size.y)
				break;
		}
	}
	return last;
}

ImageSet::Entry* ImageSet::FindEntryForEdgeWidth(AABB2f edgeWidth)
{
	if (entries.IsEmpty())
		return nullptr;

	Entry* last = entries.Data();
	// TODO
	return last;
}

void ImageSet::_DrawAsIcon(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForSize(rect.GetSize()))
	{
		if (sizeMode == ImageSetSizeMode::NearestNoScale)
			rect = RectGenCentered(rect, e->image->GetSizeF());
		else
			rect = RectGenFit(rect, baseSize);
		draw::RectColTex(rect.x0, rect.y0, rect.x1, rect.y1, color, e->image);
	}
}

void ImageSet::_DrawAsSliced(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForEdgeWidth(baseEdgeWidth))
	{
		AABB2f inner = rect.ShrinkBy(baseEdgeWidth);

		draw::RectColTex9Slice(rect, inner, color, e->image, { 0, 0, 1, 1 }, e->innerUV);
	}
}

void ImageSet::_DrawAsPattern(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForSize(baseSize))
	{
		draw::RectColTex(
			rect.x0, rect.y0, rect.x1, rect.y1,
			color,
			e->image,
			0, 0, rect.GetWidth() / baseSize.x, rect.GetHeight() / baseSize.y);
	}
}

void ImageSet::_DrawAsRawImage(AABB2f rect, Color4b color)
{
	if (auto* e = FindEntryForSize(rect.GetSize()))
	{
		draw::RectColTex(rect.x0, rect.y0, rect.x1, rect.y1, color, e->image);
	}
}

void ImageSet::Draw(AABB2f rect, Color4b color)
{
	if (rect.GetWidth() <= 0 || rect.GetHeight() <= 0)
		return;
	switch (type)
	{
	case ImageSetType::Icon:
		_DrawAsIcon(rect, color);
		break;
	case ImageSetType::Sliced:
		_DrawAsSliced(rect, color);
		break;
	case ImageSetType::Pattern:
		_DrawAsPattern(rect, color);
		break;
	case ImageSetType::RawImage:
		_DrawAsRawImage(rect, color);
		break;
	}
}

ImageSetSizeMode GetDefaultImageSetSizeMode(ImageSetType type)
{
	auto i = unsigned(type);
	assert(i < 3);
	return g_imageSetSizeModes[i];
}

void SetDefaultImageSetSizeMode(ImageSetType type, ImageSetSizeMode sizeMode)
{
	auto i = unsigned(type);
	assert(i < 3);
	if (sizeMode == ImageSetSizeMode::Default)
		sizeMode = ImageSetSizeMode::NearestScaleDown;
	g_imageSetSizeModes[i] = sizeMode;
}

void SetDefaultImageSetSizeModeAll(ImageSetSizeMode sizeMode)
{
	if (sizeMode == ImageSetSizeMode::Default)
		sizeMode = ImageSetSizeMode::NearestScaleDown;
	for (auto& m : g_imageSetSizeModes)
		m = sizeMode;
}


} // draw
} // ui
