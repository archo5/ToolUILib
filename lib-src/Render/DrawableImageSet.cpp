
#include "DrawableImageSet.h"

#include "Render.h"

#include "../Core/HashMap.h"


namespace ui {
namespace draw {


HashMap<StringView, ImageSet*> g_loadedImageSets;
ImageSetSizeMode g_imageSetSizeModes[3] =
{
	ImageSetSizeMode::NearestScaleDown,
	ImageSetSizeMode::NearestScaleDown,
	ImageSetSizeMode::NearestScaleDown,
};


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


static inline ImageSetSizeMode GetFinalSizeMode(ImageSetType type, ImageSetSizeMode mode)
{
	if (mode == ImageSetSizeMode::Default)
		mode = g_imageSetSizeModes[unsigned(type)];
	return mode;
}

static Size2i GetClosestLargerSize(Size2i input, Size2f nominal, Size2i maxSize/*, ArrayView<Size2i> specialSizes*/)
{
	float expX = logf(input.x / nominal.x) / logf(2);
	float expY = logf(input.y / nominal.y) / logf(2);
	float expMax = max(expX, expY);
	float expMaxUp = ceilf(expMax);
	float mult = powf(2, expMaxUp);
	float x = mult * nominal.x;
	float y = mult * nominal.y;
	// unlikely to be a practical image
	//maxSize.x = min(maxSize.x, 4096); -- TODO needed?
	//maxSize.y = min(maxSize.y, 4096);
	if (x > maxSize.x || y > maxSize.y)
	{
		float xds = x / maxSize.x;
		float yds = y / maxSize.y;
		if (xds > yds) // limited by x
		{
			y = x * maxSize.y / maxSize.x;
			x = float(maxSize.x);
		}
		else // limited by y
		{
			x = y * maxSize.x / maxSize.y;
			y = float(maxSize.y);
		}
	}
	if (x < 1)
		x = 1;
	if (y < 1)
		y = 1;
#if 0
	for (auto ssz : specialSizes)
	{
		if (input.x <= ssz.x && ssz.x <= x &&
			input.y <= ssz.y && ssz.y <= y)
			return ssz;
	}
#endif
	return { int(x), int(y) };
}

static inline float SizeDiff(ImageSet::VectorImageEntry* e, Size2i size)
{
	return abs(e->minSizeHint.x - size.x) + abs(e->minSizeHint.y - size.y);
}

static inline float EdgeWidthDiff(ImageSet::VectorImageEntry* e, AABB2f edgeWidth)
{
	return abs(e->edgeWidth.x0 - edgeWidth.x0) + abs(e->edgeWidth.y0 - edgeWidth.y0)
		+ abs(e->edgeWidth.x1 - edgeWidth.x1) + abs(e->edgeWidth.y1 - edgeWidth.y1);
}

RCHandle<ImageSet::BitmapImageEntry> ImageSet::VectorImageEntry::RequestImage(Size2i size)
{
	for (auto r : _rasterized)
		if (r->image->GetSize().y == size.y) // currently requesting by y-size
			return r;

	auto* E = new BitmapImageEntry;
	E->image = ImageCreateFromCanvas(image->GetImageWithHeight(size.y));
	E->image->SetExclDebugName(image->GetCacheKey());
	E->innerUV = innerUV;
	E->edgeWidth = edgeWidth;
	auto rs = image->GetSize();
	auto scale = size.Cast<float>();
	scale.x /= rs.x;
	scale.y /= rs.y;
	E->edgeWidth.x0 *= scale.x;
	E->edgeWidth.y0 *= scale.y;
	E->edgeWidth.x1 *= scale.x;
	E->edgeWidth.y1 *= scale.y;
	_rasterized.Append(E);
	return E;
}

ImageSet::BitmapImageEntry* ImageSet::FindEntryForSize(Size2i size)
{
	if (bitmapImageEntries.IsEmpty() && vectorImageEntries.IsEmpty())
		return nullptr;

	auto mode = GetFinalSizeMode(type, sizeMode);

	BitmapImageEntry* best = nullptr;
	if (bitmapImageEntries.NotEmpty())
		best = bitmapImageEntries.First();
	if (mode == ImageSetSizeMode::NearestScaleUp || mode == ImageSetSizeMode::NearestNoScale)
	{
		for (auto& e : bitmapImageEntries)
		{
			if (!e->image)
				continue;
			if (e->image->GetWidth() > size.x || e->image->GetHeight() > size.y)
				break;
			best = e;
		}

		// exact size found
		if (best && best->image->GetSize() == size)
			return best;

		if (vectorImageEntries.IsEmpty())
			return best;
	}
	else
	{
		for (auto& e : bitmapImageEntries)
		{
			if (!e->image)
				continue;
			best = e;
			if (e->image->GetWidth() >= size.x && e->image->GetHeight() >= size.y)
				break;
		}

		// exact size found
		if (best && best->image->GetSize() == size)
			return best;

		if (vectorImageEntries.IsEmpty())
			return best;

		VectorImageEntry* bestVEntry = nullptr;
		VectorImageEntry* backupVEntry = nullptr;
		for (auto ve : vectorImageEntries)
		{
			if (!ve->image)
				continue;
			VectorImageEntry*& bve = ve->minSizeHint.x >= size.x && ve->minSizeHint.y >= size.y
				? bestVEntry
				: backupVEntry;
			if (!bve || SizeDiff(bve, size) > SizeDiff(ve, size))
				bve = ve;
		}
		if (!bestVEntry)
		{
			if (!backupVEntry)
				return nullptr;
			bestVEntry = backupVEntry;
		}
		Size2i clvsize = GetClosestLargerSize(size, bestVEntry->image->GetSize(), maxSize);

		// raster image still better
		if (best && (clvsize.x > best->image->GetWidth() || clvsize.y > best->image->GetHeight()))
			return best;

		return bestVEntry->RequestImage(clvsize);
	}
	return best;
}

ImageSet::BitmapImageEntry* ImageSet::FindEntryForEdgeWidth(AABB2f edgeWidth)
{
	if (bitmapImageEntries.IsEmpty() && vectorImageEntries.IsEmpty())
		return nullptr;

	auto mode = GetFinalSizeMode(type, sizeMode);

	BitmapImageEntry* best = nullptr;
	if (bitmapImageEntries.NotEmpty())
		best = bitmapImageEntries.First();
	if (mode == ImageSetSizeMode::NearestScaleUp || mode == ImageSetSizeMode::NearestNoScale)
	{
		for (auto& e : bitmapImageEntries)
		{
			if (e->edgeWidth.x0 >= edgeWidth.x0 || e->edgeWidth.y0 >= edgeWidth.y0 ||
				e->edgeWidth.x1 >= edgeWidth.x1 || e->edgeWidth.y1 >= edgeWidth.y1)
				break;
			best = e;
		}

		// exact size found
		if (best && best->edgeWidth == edgeWidth)
			return best;

		if (vectorImageEntries.IsEmpty())
			return best;
	}
	else
	{
		for (auto& e : bitmapImageEntries)
		{
			if (!e->image)
				continue;
			best = e;
			if (e->edgeWidth.x0 >= edgeWidth.x0 || e->edgeWidth.y0 >= edgeWidth.y0 ||
				e->edgeWidth.x1 >= edgeWidth.x1 || e->edgeWidth.y1 >= edgeWidth.y1)
				break;
		}

		// exact size found
		if (best && best->edgeWidth == edgeWidth)
			return best;

		if (vectorImageEntries.IsEmpty())
			return best;

		VectorImageEntry* bestVEntry = nullptr;
		VectorImageEntry* backupVEntry = nullptr;
		for (auto ve : vectorImageEntries)
		{
			if (!ve->image)
				continue;
			VectorImageEntry*& bve =
				ve->edgeWidth.x0 >= edgeWidth.x0 && ve->edgeWidth.y0 >= edgeWidth.y0 &&
				ve->edgeWidth.x1 >= edgeWidth.x1 && ve->edgeWidth.y1 >= edgeWidth.y1
				? bestVEntry
				: backupVEntry;
			if (!bve || EdgeWidthDiff(bve, edgeWidth) > EdgeWidthDiff(ve, edgeWidth))
				bve = ve;
		}
		if (!bestVEntry)
		{
			if (!backupVEntry)
				return nullptr;
			bestVEntry = backupVEntry;
		}
		Vec2f tgtEWA = edgeWidth.GetCenter();
		Vec2f imgEWA = bestVEntry->edgeWidth.GetCenter();
		Vec2f neededSizeF = tgtEWA / imgEWA * bestVEntry->image->GetSize().ToVec2();
		Size2i neededSize = { int(roundf(neededSizeF.x)), int(roundf(neededSizeF.y)) };
		Size2i clvsize = GetClosestLargerSize(neededSize, bestVEntry->image->GetSize(), maxSize);

		// raster image still better
		if (best && (clvsize.x > best->image->GetWidth() || clvsize.y > best->image->GetHeight()))
			return best;

		return bestVEntry->RequestImage(clvsize);
	}
	return best;
}

void ImageSet::_DrawAsIcon(AABB2f rect, Color4b color)
{
	if (auto e = FindEntryForSize(rect.GetSize().Cast<int>()))
	{
		if (sizeMode == ImageSetSizeMode::NearestNoScale)
			rect = RectGenCentered(rect, e->image->GetSizeF());
		else
			rect = RectGenFitSize(rect, baseSize);
		draw::RectColTex(rect.x0, rect.y0, rect.x1, rect.y1, color, e->image);
	}
}

void ImageSet::_DrawAsSliced(AABB2f rect, Color4b color)
{
	if (auto e = FindEntryForEdgeWidth(baseEdgeWidth))
	{
		AABB2f inner = rect.ShrinkBy(baseEdgeWidth);

		draw::RectColTex9Slice(rect, inner, color, e->image, { 0, 0, 1, 1 }, e->innerUV);
	}
}

void ImageSet::_DrawAsPattern(AABB2f rect, Color4b color)
{
	if (auto e = FindEntryForSize(baseSize.Cast<int>()))
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
	if (auto e = FindEntryForSize(rect.GetSize().Cast<int>()))
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

ImageSet::~ImageSet()
{
	if (!_cacheKey.empty())
		g_loadedImageSets.Remove(_cacheKey);
}


ImageSet* ImageSetCacheRead(StringView key)
{
	return g_loadedImageSets.GetValueOrDefault(key);
}

void ImageSetCacheWrite(ImageSet* set, StringView key)
{
	if (auto* prev = ImageSetCacheRead(key))
		prev->_cacheKey.clear();

	set->_cacheKey <<= key;
	g_loadedImageSets[set->_cacheKey] = set;
}


} // draw
} // ui
