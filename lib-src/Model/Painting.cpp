
#include "Painting.h"

#include "Theme.h"
#include "Native.h"
#include "Objects.h"


namespace ui {

FontWeight EnumKeys<FontWeight>::StringToValue(const char* name)
{
	if (!strcmp(name, "thin")) return FontWeight::Thin;
	if (!strcmp(name, "extraLight")) return FontWeight::ExtraLight;
	if (!strcmp(name, "light")) return FontWeight::Light;
	if (!strcmp(name, "normal")) return FontWeight::Normal;
	if (!strcmp(name, "medium")) return FontWeight::Medium;
	if (!strcmp(name, "semibold")) return FontWeight::Semibold;
	if (!strcmp(name, "bold")) return FontWeight::Bold;
	if (!strcmp(name, "extraBold")) return FontWeight::ExtraBold;
	if (!strcmp(name, "black")) return FontWeight::Black;
	return FontWeight::Normal;
}

const char* EnumKeys<FontWeight>::ValueToString(FontWeight e)
{
	switch (e)
	{
	case FontWeight::Thin: return "thin";
	case FontWeight::ExtraLight: return "extraLight";
	case FontWeight::Light: return "light";
	case FontWeight::Normal: return "normal";
	case FontWeight::Medium: return "medium";
	case FontWeight::Semibold: return "semibold";
	case FontWeight::Bold: return "bold";
	case FontWeight::ExtraBold: return "extraBold";
	case FontWeight::Black: return "black";
	default: return "";
	}
}

const char* EnumKeys_FontStyle[] =
{
	"normal",
	"italic",
	nullptr,
};


struct CoordBufferRW : IBufferRW
{
	Coord* coord;
	mutable char buf[32];

	void Assign(StringView sv) const
	{
		if (sv == "")
			*coord = Coord::Undefined();
		else if (sv == "auto")
			*coord = Coord(0, CoordTypeUnit::Auto);
		else if (sv == "inherit")
			*coord = Coord(0, CoordTypeUnit::Inherit);
		else
		{
			Coord c;
			if (sv.ends_with("%"))
				c.unit = CoordTypeUnit::Percent;
			else if (sv.ends_with("fr"))
				c.unit = CoordTypeUnit::Fraction;
			else
				c.unit = CoordTypeUnit::Pixels;
			c.value = sv.to_float64();
			*coord = c;
		}
	}
	StringView Read() const
	{
		const char* type = "";
		switch (coord->unit)
		{
		case CoordTypeUnit::Undefined: return "";
		case CoordTypeUnit::Inherit: return "inherit";
		case CoordTypeUnit::Auto: return "auto";

		case CoordTypeUnit::Pixels: type = "px"; break;
		case CoordTypeUnit::Percent: type = "%"; break;
		case CoordTypeUnit::Fraction: type = "fr"; break;
		}
		snprintf(buf, sizeof(buf), "%g%s", coord->value, type);
		return buf;
	}
};

void Coord::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	CoordBufferRW rw;
	rw.coord = this;
	oi.OnFieldString(FI, rw);
}


PaintInfo::PaintInfo(const UIObject* o) : obj(o)
{
	rect = o->GetFinalRect();
	if (o->IsHovered())
		state |= PS_Hover;
	if (o->IsPressed())
		state |= PS_Down;
	if (o->IsInputDisabled())
		state |= PS_Disabled;
	if (o->IsFocused())
		state |= PS_Focused;
	if (o->flags & UIObject_IsChecked)
	{
		state |= PS_Checked;
		checkState = 1;
	}
}

void PaintInfo::SetChecked(bool checked)
{
	SetCheckState(checked);
}

void PaintInfo::SetCheckState(uint8_t cs)
{
	if (cs)
		state |= PS_Checked;
	else
		state &= ~PS_Checked;
	checkState = cs;
}


ContentPaintAdvice EmptyPainter::Paint(const PaintInfo&)
{
	// do nothing
	return {};
}

EmptyPainter* EmptyPainter::Get()
{
	static EmptyPainter ep;
	return &ep;
}

EmptyPainter::EmptyPainter()
{
	AddRef();
}


static StaticID_ImageSet sid_bgr_checkerboard("bgr-checkerboard");
ContentPaintAdvice CheckerboardPainter::Paint(const PaintInfo& info)
{
	GetCurrentTheme()->GetImageSet(sid_bgr_checkerboard)->Draw(info.rect);
	return {};
}

CheckerboardPainter* CheckerboardPainter::Get()
{
	static CheckerboardPainter ep;
	return &ep;
}

CheckerboardPainter::CheckerboardPainter()
{
	AddRef();
}


ContentPaintAdvice LayerPainter::Paint(const PaintInfo& info)
{
	ContentPaintAdvice ret;
	for (const auto& h : layers)
		ret.MergeOver(h->Paint(info));
	return ret;
}

RCHandle<LayerPainter> LayerPainter::Create()
{
	return new LayerPainter;
}


ContentPaintAdvice ConditionalPainter::Paint(const PaintInfo& info)
{
	if (painter && (condition & info.state) == condition)
		return painter->Paint(info);
	return {};
}


ContentPaintAdvice SelectFirstPainter::Paint(const PaintInfo& info)
{
	for (const auto& item : items)
	{
		if (item.painter &&
			(item.condition & info.state) == item.condition &&
			(item.checkState == 0xff || item.checkState == info.checkState))
		{
			return item.painter->Paint(info);
		}
	}
	return {};
}


ContentPaintAdvice TextStyleModPainter::Paint(const PaintInfo& info)
{
	ContentPaintAdvice cpa = {};
	if (painter)
		cpa = painter->Paint(info);
	cpa.SetTextColor(textColor);
	cpa.offset *= contentOffsetInherit;
	cpa.offset += contentOffset;
	return cpa;
}


ContentPaintAdvice PointAnchoredPlacementRectModPainter::Paint(const PaintInfo& info)
{
	auto ro = info.rect;

	float w = ro.GetWidth() * sizeAddFraction.x + size.x;
	float h = ro.GetHeight() * sizeAddFraction.y + size.y;
	float x = lerp(ro.x0, ro.x1, anchor.x) - w * pivot.x + bias.x;
	float y = lerp(ro.y0, ro.y1, anchor.y) - h * pivot.y + bias.y;
	AABB2f rnew = { x, y, x + w, y + h };
	const_cast<AABB2f&>(info.rect) = rnew;

	auto cpa = painter->Paint(info);

	const_cast<AABB2f&>(info.rect) = ro;
	return cpa;
}


ContentPaintAdvice ColorFillPainter::Paint(const PaintInfo& info)
{
	AABB2f r = info.rect;
	r = r.ShrinkBy(AABB2f::UniformBorder(shrink));

	if (r.GetWidth() > 0 && r.GetHeight() > 0)
	{
		auto baseColor = color;
		float w = borderWidth;
		bool drawBorder = w > 0 && borderColor.a > 0;

		if (borderWidth * 2 >= r.GetWidth() ||
			borderWidth * 2 >= r.GetHeight())
		{
			drawBorder = false;
			baseColor = borderColor;
		}

		if (drawBorder)
		{
			draw::RectCol(r.x0 + w, r.y0 + w, r.x1 - w, r.y1 - w, color);
			draw::RectCol(r.x0, r.y0, r.x1, r.y0 + w, borderColor);
			draw::RectCol(r.x0, r.y0, r.x0 + w, r.y1, borderColor);
			draw::RectCol(r.x0, r.y1 - w, r.x1, r.y1, borderColor);
			draw::RectCol(r.x1 - w, r.y0, r.x1, r.y1, borderColor);
		}
		else
		{
			draw::RectCol(r.x0, r.y0, r.x1, r.y1, baseColor);
		}
	}

	ContentPaintAdvice cpa;
	cpa.offset = contentOffset;
	return cpa;
}


ContentPaintAdvice ImageSetPainter::Paint(const PaintInfo& info)
{
	if (!imageSet)
		return {};

	AABB2f r = info.rect;
	r = r.ShrinkBy(AABB2f::UniformBorder(shrink));

	imageSet->Draw(r, color);

	ContentPaintAdvice cpa;
	cpa.offset = contentOffset;
	if (overrideTextColor)
		cpa.SetTextColor(textColor);
	return cpa;
}


BoxShadowPainter::CacheValue* BoxShadowPainter::GetOrCreate(Size2f size)
{
	SimpleMaskBlurGen::Input config = {};
	config.boxSizeX = u32(size.x);
	config.boxSizeY = u32(size.y);
	config.blurSize = blurSize;
	config.cornerLT = cornerLT;
	config.cornerLB = cornerLB;
	config.cornerRT = cornerRT;
	config.cornerRB = cornerRB;
	config = config.Canonicalized();

	CacheValue* ptr = cache.GetValuePtr(config);
	if (!ptr)
	{
		// TODO cache/share generated images?
		CacheValue val;
		Canvas canvas;
		SimpleMaskBlurGen::Generate(config, val.output, canvas);
		val.image = draw::ImageCreateFromCanvas(canvas);

		ptr = &cache.Insert(config, val)->value;
	}
	return ptr;
}

ContentPaintAdvice BoxShadowPainter::Paint(const PaintInfo& info)
{
	auto* ptr = GetOrCreate(info.rect.GetSize());
	auto rect = info.rect.MoveBy(offset.x, offset.y);
	draw::RectColTex9Slice
	(
		rect.ExtendBy(ptr->output.outerOffset),
		rect.ShrinkBy(ptr->output.innerOffset),
		color,
		ptr->image,
		ptr->output.outerUV,
		ptr->output.innerUV
	);
	return {};
}


void FontSettings::_SerializeContents(IObjectIterator& oi)
{
	OnField(oi, "family", family);
	OnFieldEnumString(oi, "weight", weight);
	OnFieldEnumString(oi, "style", style);
	OnField(oi, "size", size);
}

void FontSettings::Serialize(ThemeData& td, IObjectIterator& oi)
{
	_SerializeContents(oi);
}

void FontSettings::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "FontSettings");
	_SerializeContents(oi);
	oi.EndObject();
}

Font* FontSettings::GetFont() const
{
	return _cachedFont.GetCachedFont(family.c_str(), int(weight), style == FontStyle::Italic);
}


} // ui
