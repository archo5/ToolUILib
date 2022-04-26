
#include "Layout.h"
#include "Theme.h"

#include "Native.h"
#include "Objects.h"

#include <vector>
#include <algorithm>


namespace ui {

namespace layouts {

struct StackLayout : ILayout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		float size = 0;
		auto dir = StackingDirection::TopDown;
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->CalcEstimatedWidth(containerSize, EstSizeType::Expanding).min);
			break;
		case StackingDirection::LeftToRight:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->CalcEstimatedWidth(containerSize, EstSizeType::Expanding).min;
			break;
		}
		return size;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		float size = 0;
		auto dir = StackingDirection::TopDown;
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->CalcEstimatedHeight(containerSize, EstSizeType::Expanding).min;
			break;
		case StackingDirection::LeftToRight:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->CalcEstimatedHeight(containerSize, EstSizeType::Expanding).min);
			break;
		}
		return size;
	}
	void OnLayout(UIObject* curObj, UIRect& inrect)
	{
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		auto dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown: {
			float p = inrect.y0;
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			{
				float h = ch->CalcEstimatedHeight(inrect.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ inrect.x0, p, inrect.x1, p + h });
				p += h;
			}
			break; }
		case StackingDirection::LeftToRight: {
			float p = inrect.x0;
			float xw = 0;
			auto ha = HAlign::Undefined;
			if (ha != HAlign::Undefined && ha != HAlign::Left)
			{
				float tw = 0;
				int cc = 0;
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				{
					tw += ch->CalcEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
					cc++;
				}
				float diff = inrect.GetWidth() - tw;
				switch (ha)
				{
				case HAlign::Center:
					p += diff / 2;
					break;
				case HAlign::Right:
					p += diff;
					break;
				case HAlign::Justify: {
					if (cc > 1)
						xw += diff / (cc - 1);
					else
						p += diff / 2;
					break; }
				}
			}
			p = floorf(p);
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			{
				float w = ch->CalcEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ p, inrect.y0, p + w, inrect.y1 });
				p += w + xw;
			}
			break; }
		}
	}
}
g_stackLayout;
ILayout* Stack() { return &g_stackLayout; }

} // layouts


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

	const auto& ise = imageSet->entries[0];

	AABB2f r = info.rect;
	r = r.ShrinkBy(AABB2f::UniformBorder(shrink));

	imageSet->Draw(r, color);

	ContentPaintAdvice cpa;
	cpa.offset = contentOffset;
	return cpa;
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
