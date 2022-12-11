
#pragma once

#include "DrawableImage.h"

#include "../Core/Image.h"


namespace ui {
namespace draw {

enum class ImageSetType : uint8_t
{
	Icon = 0,
	Sliced,
	Pattern,
	RawImage,
};

enum class ImageSetSizeMode : uint8_t
{
	Default = 0, // image set defaults to app-wide setting, which defaults to nearest scale down
	NearestScaleDown,
	NearestScaleUp,
	NearestNoScale,
};

struct ImageSet : RefCountedST
{
	struct Entry
	{
		draw::ImageHandle image;
		AABB2f innerUV = { 0, 0, 1, 1 };
		AABB2f edgeWidth = {};
	};

	Array<Entry> entries;
	ImageSetType type = ImageSetType::Icon;
	ImageSetSizeMode sizeMode = ImageSetSizeMode::Default;
	AABB2f baseEdgeWidth = {};
	Size2f baseSize = { 1, 1 };

	Entry* FindEntryForSize(Size2f size);
	Entry* FindEntryForEdgeWidth(AABB2f edgeWidth);

	void _DrawAsIcon(AABB2f rect, Color4b color);
	void _DrawAsSliced(AABB2f rect, Color4b color);
	void _DrawAsPattern(AABB2f rect, Color4b color);
	void _DrawAsRawImage(AABB2f rect, Color4b color);
	void Draw(AABB2f rect, Color4b color = {});
};
using ImageSetHandle = RCHandle<ImageSet>;

ImageSetSizeMode GetDefaultImageSetSizeMode(ImageSetType type);
void SetDefaultImageSetSizeMode(ImageSetType type, ImageSetSizeMode sizeMode);
void SetDefaultImageSetSizeModeAll(ImageSetSizeMode sizeMode);

} // draw
} // ui
