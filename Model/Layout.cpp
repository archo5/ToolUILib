
#include "Layout.h"
#include "Theme.h"

#include "Native.h"
#include "Objects.h"

#include <vector>
#include <algorithm>


namespace ui {

void PointAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect) const
{
	UIRect parentRect = outRect;
	Size2f contSize = parentRect.GetSize();

	float w = curObj->GetFullEstimatedWidth(contSize, EstSizeType::Expanding, false).min;
	float h = curObj->GetFullEstimatedHeight(contSize, EstSizeType::Expanding, false).min;

	float xo = 0, yo = 0;
	if (useContentBox)
	{
		auto pr = curObj->styleProps->GetPaddingRect();
		xo = pr.x0;
		yo = pr.y0;
		w -= pr.x0 + pr.x1;
		h -= pr.y0 + pr.y1;
	}

	float x = lerp(parentRect.x0, parentRect.x1, anchor.x) - w * pivot.x - xo + bias.x;
	float y = lerp(parentRect.y0, parentRect.y1, anchor.y) - h * pivot.y - yo + bias.y;
	outRect = { x, y, x + w, y + h };
}

void RectAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect) const
{
	UIRect parentRect = outRect;

	outRect.x0 = lerp(parentRect.x0, parentRect.x1, anchor.x0) + bias.x0;
	outRect.x1 = lerp(parentRect.x0, parentRect.x1, anchor.x1) + bias.x1;
	outRect.y0 = lerp(parentRect.y0, parentRect.y1, anchor.y0) + bias.y0;
	outRect.y1 = lerp(parentRect.y0, parentRect.y1, anchor.y1) + bias.y1;
}


namespace layouts {

struct StackLayout : ILayout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		auto style = curObj->GetStyle();
		float size = 0;
		auto dir = style.GetStackingDirection();
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min);
			break;
		case StackingDirection::LeftToRight:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
			break;
		}
		return size;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		auto style = curObj->GetStyle();
		float size = 0;
		auto dir = style.GetStackingDirection();
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min;
			break;
		case StackingDirection::LeftToRight:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min);
			break;
		}
		return size;
	}
	void OnLayout(UIObject* curObj, const UIRect& inrect, LayoutState& state)
	{
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		auto style = curObj->GetStyle();
		auto dir = style.GetStackingDirection();
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown: {
			float p = inrect.y0;
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			{
				float h = ch->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ inrect.x0, p, inrect.x1, p + h }, inrect.GetSize());
				p += h;
			}
			break; }
		case StackingDirection::LeftToRight: {
			float p = inrect.x0;
			float xw = 0;
			auto ha = style.GetHAlign();
			if (ha != HAlign::Undefined && ha != HAlign::Left)
			{
				float tw = 0;
				int cc = 0;
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				{
					tw += ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
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
				float w = ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ p, inrect.y0, p + w, inrect.y1 }, inrect.GetSize());
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


int g_numStyleBlocks;


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
	rect = o->GetPaddingRect();
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


StyleBlock::~StyleBlock()
{
	for (auto* ch = _firstChild; ch; ch = ch->_next)
	{
		ch->_parent = nullptr;
	}

	if (_prev)
		_prev->_next = _next;
	else if (_parent)
		_parent->_firstChild = _next;

	if (_next)
		_next->_prev = _prev;
	else if (_parent)
		_parent->_lastChild = _prev;

	if (_parent)
	{
		_parent->_numChildren--;
		_parent->_Release();
	}
}

void StyleBlock::_Release()
{
	if (--_refCount <= 0)
		delete this;
}

StyleBlock* StyleBlock::_GetWithChange(int off, FnIsPropEqual feq, FnPropCopy fcopy, const void* ref)
{
	for (auto* ch = _firstChild; ch; ch = ch->_next)
	{
		if (ch->_parentDiffOffset == off && feq(reinterpret_cast<char*>(ch) + off, ref))
		{
			return ch;
		}
	}

	// create new
	StyleBlock* copy = new StyleBlock(*this);
	copy->_refCount = 0;
	copy->_numChildren = 0;
	copy->_prev = nullptr;
	copy->_next = nullptr;
	copy->_firstChild = nullptr;
	copy->_lastChild = nullptr;
	// copy prop
	fcopy(reinterpret_cast<char*>(copy) + off, ref);
	// link
	copy->_parent = this;
	if (_firstChild)
	{
		_firstChild->_prev = copy;
		copy->_next = _firstChild;
	}
	else
		_lastChild = copy;
	_firstChild = copy;
	//copy->_parentDiffFunc = feq; TODO needed?
	copy->_parentDiffOffset = off;
	_refCount++;
	_numChildren++;
	return copy;
}

Font* StyleBlock::GetFont()
{
	return font.GetFont();
}


StyleAccessor::StyleAccessor(StyleBlock* b) : block(b), blkref(nullptr), owner(nullptr)
{
}

StyleAccessor::StyleAccessor(StyleBlockRef& r, UIObject* o) : block(r), blkref(&r), owner(o)
{
}

template <class T> void AccSet(StyleAccessor& a, int off, T v)
{
	if (a.blkref)
	{
		if (auto* it = a.block->GetWithChange(off, v))
		{
			*a.blkref = a.block = it;
			if (a.owner)
				a.owner->_OnChangeStyle();
			return;
		}
	}
	PropFuncs<T>::Copy(reinterpret_cast<char*>(a.block) + off, &v);
	if (a.owner)
		a.owner->_OnChangeStyle();
}

PainterHandle StyleAccessor::GetBackgroundPainter() const
{
	return block->background_painter;
}

void StyleAccessor::SetBackgroundPainter(const PainterHandle& h)
{
	AccSet(*this, offsetof(StyleBlock, background_painter), h);
}


StackingDirection StyleAccessor::GetStackingDirection() const
{
	return block->stacking_direction;
}

void StyleAccessor::SetStackingDirection(StackingDirection v)
{
	AccSet(*this, offsetof(StyleBlock, stacking_direction), v);
}

HAlign StyleAccessor::GetHAlign() const
{
	return block->h_align;
}

void StyleAccessor::SetHAlign(HAlign a)
{
	AccSet(*this, offsetof(StyleBlock, h_align), a);
}


void StyleAccessor::SetFontFamily(const std::string& v)
{
	AccSet(*this, offsetof(StyleBlock, font.family), v);
}

void StyleAccessor::SetFontWeight(FontWeight v)
{
	AccSet(*this, offsetof(StyleBlock, font.weight), v);
}

void StyleAccessor::SetFontStyle(FontStyle v)
{
	AccSet(*this, offsetof(StyleBlock, font.style), v);
}

void StyleAccessor::SetFontSize(int v)
{
	AccSet(*this, offsetof(StyleBlock, font.size), v);
}

void StyleAccessor::SetTextColor(Color4b v)
{
	AccSet(*this, offsetof(StyleBlock, text_color), v);
}


float StyleAccessor::GetPaddingLeft() const
{
	return block->padding_left;
}

void StyleAccessor::SetPaddingLeft(float v)
{
	AccSet(*this, offsetof(StyleBlock, padding_left), v);
}

float StyleAccessor::GetPaddingRight() const
{
	return block->padding_right;
}

void StyleAccessor::SetPaddingRight(float v)
{
	AccSet(*this, offsetof(StyleBlock, padding_right), v);
}

float StyleAccessor::GetPaddingTop() const
{
	return block->padding_top;
}

void StyleAccessor::SetPaddingTop(float v)
{
	AccSet(*this, offsetof(StyleBlock, padding_top), v);
}

float StyleAccessor::GetPaddingBottom() const
{
	return block->padding_bottom;
}

void StyleAccessor::SetPaddingBottom(float v)
{
	AccSet(*this, offsetof(StyleBlock, padding_bottom), v);
}

void StyleAccessor::SetPadding(float t, float r, float b, float l)
{
	// TODO?
#if 0
	GetOrCreate();
	block->padding_top = t;
	block->padding_right = r;
	block->padding_bottom = b;
	block->padding_left = l;
#endif
	AccSet(*this, offsetof(StyleBlock, padding_top), t);
	AccSet(*this, offsetof(StyleBlock, padding_right), r);
	AccSet(*this, offsetof(StyleBlock, padding_bottom), b);
	AccSet(*this, offsetof(StyleBlock, padding_left), l);
}


static StyleBlockRef g_objectStyle;

void _InitStyles()
{
	g_objectStyle = new StyleBlock;
	g_objectStyle->background_painter = EmptyPainter::Get();
}

void _FreeStyles()
{
	g_objectStyle = nullptr;
}

StyleBlockRef GetObjectStyle()
{
	return g_objectStyle;
}


} // ui
