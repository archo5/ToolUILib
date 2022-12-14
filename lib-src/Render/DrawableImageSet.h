
#pragma once

#include "DrawableImage.h"

#include "../Core/Image.h"
#include "../Core/VectorImage.h"


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

ImageSetSizeMode GetDefaultImageSetSizeMode(ImageSetType type);
void SetDefaultImageSetSizeMode(ImageSetType type, ImageSetSizeMode sizeMode);
void SetDefaultImageSetSizeModeAll(ImageSetSizeMode sizeMode);

struct ImageSet : RefCountedST
{
	struct BitmapImageEntry : RefCountedST
	{
		ImageHandle image;
		AABB2f innerUV = { 0, 0, 1, 1 };
		AABB2f edgeWidth = {};
	};
	struct VectorImageEntry : RefCountedST
	{
		VectorImageHandle image;
		Vec2f minSizeHint = {}; // the smallest rasterized size at which the image is still readable
		AABB2f innerUV = { 0, 0, 1, 1 };
		AABB2f edgeWidth = {};

		Array<RCHandle<BitmapImageEntry>> _rasterized;

		RCHandle<BitmapImageEntry> RequestImage(Size2i size);
	};

	Array<RCHandle<BitmapImageEntry>> bitmapImageEntries;
	Array<RCHandle<VectorImageEntry>> vectorImageEntries;
	ImageSetType type = ImageSetType::Icon;
	ImageSetSizeMode sizeMode = ImageSetSizeMode::Default;
	AABB2f baseEdgeWidth = {};
	Size2f baseSize = { 1, 1 };

	std::string _cacheKey;

	BitmapImageEntry* FindEntryForSize(Size2i size);
	BitmapImageEntry* FindEntryForEdgeWidth(AABB2f edgeWidth);

	void _DrawAsIcon(AABB2f rect, Color4b color);
	void _DrawAsSliced(AABB2f rect, Color4b color);
	void _DrawAsPattern(AABB2f rect, Color4b color);
	void _DrawAsRawImage(AABB2f rect, Color4b color);
	void Draw(AABB2f rect, Color4b color = {});

	~ImageSet();

	inline void AddBitmapImage(IImage* image, AABB2f innerUV = { 0, 0, 1, 1 }, AABB2f edgeWidth = {})
	{
		auto* E = new BitmapImageEntry;
		E->image = image;
		E->innerUV = innerUV;
		E->edgeWidth = edgeWidth;
		bitmapImageEntries.Append(E);
	}
	inline void AddVectorImage(IVectorImage* image, Vec2f minSizeHint = {}, AABB2f innerUV = { 0, 0, 1, 1 }, AABB2f edgeWidth = {})
	{
		auto* E = new VectorImageEntry;
		E->image = image;
		E->minSizeHint = minSizeHint;
		E->innerUV = innerUV;
		E->edgeWidth = edgeWidth;
		vectorImageEntries.Append(E);
	}
};
using ImageSetHandle = RCHandle<ImageSet>;

ImageSet* ImageSetCacheRead(StringView key);
void ImageSetCacheWrite(ImageSet* set, StringView key);

} // draw
} // ui
