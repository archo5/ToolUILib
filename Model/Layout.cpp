
#include "Layout.h"
#include "Theme.h"

#include "Native.h"
#include "Objects.h"

#include <vector>
#include <algorithm>


namespace ui {

void PointAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect)
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

void RectAnchoredPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect)
{
	UIRect parentRect = outRect;

	outRect.x0 = lerp(parentRect.x0, parentRect.x1, anchor.x0) + bias.x0;
	outRect.x1 = lerp(parentRect.x0, parentRect.x1, anchor.x1) + bias.x1;
	outRect.y0 = lerp(parentRect.y0, parentRect.y1, anchor.y0) + bias.y0;
	outRect.y1 = lerp(parentRect.y0, parentRect.y1, anchor.y1) + bias.y1;
}


namespace layouts {

struct InlineBlockLayout : ILayout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		float size = 0;
		for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
		return size;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		float size = curObj->styleProps->font_size;
		for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			size = max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min);
		return size;
	}
	void OnLayout(UIObject* curObj, const UIRect& inrect, LayoutState& state)
	{
		auto contSize = inrect.GetSize();
		float p = inrect.x0;
		float y0 = inrect.y0;
		float maxH = 0;
		for (auto* ch = curObj->firstChild; ch; ch = ch->next)
		{
			float w = ch->GetFullEstimatedWidth(contSize, EstSizeType::Expanding).min;
			float h = ch->GetFullEstimatedHeight(contSize, EstSizeType::Expanding).min;
			ch->PerformLayout({ p, y0, p + w, y0 + h }, contSize);
			p += w;
			maxH = max(maxH, h);
		}
		state.finalContentRect = { inrect.x0, inrect.y0, p, y0 + maxH };
	}
}
g_inlineBlockLayout;
ILayout* InlineBlock() { return &g_inlineBlockLayout; }

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
		case StackingDirection::BottomUp:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min);
			break;
		case StackingDirection::LeftToRight:
		case StackingDirection::RightToLeft:
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
		case StackingDirection::BottomUp:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min;
			break;
		case StackingDirection::LeftToRight:
		case StackingDirection::RightToLeft:
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
		// margins are collapsed
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
			state.finalContentRect = { inrect.x0, inrect.y0, inrect.x1, p };
			break; }
		case StackingDirection::RightToLeft: {
			float p = inrect.x1;
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			{
				float w = ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ p - w, inrect.y0, p, inrect.y1 }, inrect.GetSize());
				p -= w;
			}
			state.finalContentRect = { p, inrect.y0, inrect.x1, inrect.y1 };
			state.scaleOrigin.x = inrect.x1;
			break; }
		case StackingDirection::BottomUp: {
			float p = inrect.y1;
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			{
				float h = ch->GetFullEstimatedHeight(inrect.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ inrect.x0, p - h, inrect.x1, p }, inrect.GetSize());
				p -= h;
			}
			state.finalContentRect = { inrect.x0, p, inrect.x1, inrect.y1 };
			state.scaleOrigin.y = inrect.y1;
			break; }
		case StackingDirection::LeftToRight: {
			float p = inrect.x0;
			float xw = 0;
			auto ha = style.GetHAlign();
			if (ha != HAlign::Undefined && ha != HAlign::Left)
			{
				float tw = 0;
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
					tw += ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
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
					auto cc = curObj->CountChildrenImmediate();
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
			state.finalContentRect = { inrect.x0, inrect.y0, p - xw, inrect.y1 };
			break; }
		}
	}
}
g_stackLayout;
ILayout* Stack() { return &g_stackLayout; }

struct StackExpandLayout : ILayout
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
		case StackingDirection::BottomUp:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->GetFullEstimatedWidth(containerSize, EstSizeType::Exact).min);
			break;
		case StackingDirection::LeftToRight:
		case StackingDirection::RightToLeft:
			if (type == EstSizeType::Expanding)
			{
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
					size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
			}
			else
				size = containerSize.x;
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
		case StackingDirection::BottomUp:
			if (type == EstSizeType::Expanding)
			{
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
					size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min;
			}
			else
				size = containerSize.y;
			break;
		case StackingDirection::LeftToRight:
		case StackingDirection::RightToLeft:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min);
			break;
		}
		return size;
	}
	void OnLayout(UIObject* curObj, const UIRect& inrect, LayoutState& state)
	{
		auto style = curObj->GetStyle();
		auto dir = style.GetStackingDirection();
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::LeftToRight: if (curObj->firstChild) {
			float p = inrect.x0;
			float sum = 0, frsum = 0;
			struct Item
			{
				UIObject* ch;
				float minw;
				float maxw;
				float w;
				float fr;
			};
			std::vector<Item> items;
			std::vector<int> sorted;
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			{
				auto s = ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding);
				auto sw = ch->GetStyle().GetWidth();
				float fr = sw.unit == CoordTypeUnit::Fraction ? sw.value : 1;
				items.push_back({ ch, s.min, s.max, s.min, fr });
				sorted.push_back(sorted.size());
				sum += s.min;
				frsum += fr;
			}
			std::sort(sorted.begin(), sorted.end(), [&items](int ia, int ib)
			{
				const auto& a = items[ia];
				const auto& b = items[ib];
				return (a.maxw - a.minw) < (b.maxw - b.minw);
			});
			float leftover = max(inrect.GetWidth() - sum, 0.0f);
			for (auto idx : sorted)
			{
				auto& item = items[idx];
				float mylo = leftover * item.fr / frsum;
				float w = item.minw + mylo;
				if (w > item.maxw)
					w = item.maxw;
				w = roundf(w);
				float actual_lo = w - item.minw;
				leftover -= actual_lo;
				frsum -= item.fr;
				item.w = w;
			}
			for (const auto& item : items)
			{
				item.ch->PerformLayout({ p, inrect.y0, p + item.w, inrect.y1 }, inrect.GetSize());
				p += item.w;
			}
			state.finalContentRect = { inrect.x0, inrect.y0, max(inrect.x1, p), inrect.y1 };
			break; }
		}
	}
}
g_stackExpandLayout;
ILayout* StackExpand() { return &g_stackExpandLayout; }


struct EdgeSliceLayout : ILayout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		return containerSize.x;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size2f& containerSize, EstSizeType type)
	{
		return containerSize.y;
	}
	void OnLayout(UIObject* curObj, const UIRect& inrect, LayoutState& state)
	{
		auto subr = inrect;
		for (auto* ch = curObj->firstChild; ch; ch = ch->next)
		{
			auto e = ch->GetStyle().GetEdge();
			float d;
			switch (e)
			{
			case Edge::Top:
				d = ch->GetFullEstimatedHeight(subr.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ subr.x0, subr.y0, subr.x1, subr.y0 + d }, subr.GetSize());
				subr.y0 += d;
				break;
			case Edge::Bottom:
				d = ch->GetFullEstimatedHeight(subr.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ subr.x0, subr.y1 - d, subr.x1, subr.y1 }, subr.GetSize());
				subr.y1 -= d;
				break;
			case Edge::Left:
				d = ch->GetFullEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ subr.x0, subr.y0, subr.x0 + d, subr.y1 }, subr.GetSize());
				subr.x0 += d;
				break;
			case Edge::Right:
				d = ch->GetFullEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).min;
				ch->PerformLayout({ subr.x1 - d, subr.y0, subr.x1, subr.y1 }, subr.GetSize());
				subr.x1 -= d;
				break;
			}
		}
	}
}
g_edgeSliceLayout;
ILayout* EdgeSlice() { return &g_edgeSliceLayout; }

} // layouts


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
	return _cachedFont.GetCachedFont(font_family.c_str(), int(font_weight), font_style == FontStyle::Italic);
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

ILayout* StyleAccessor::GetLayout() const
{
	return block->layout;
}

void StyleAccessor::SetLayout(ILayout* v)
{
	AccSet(*this, offsetof(StyleBlock, layout), v);
}

IPlacement* StyleAccessor::GetPlacement() const
{
	return block->placement;
}

void StyleAccessor::SetPlacement(IPlacement* v)
{
	AccSet(*this, offsetof(StyleBlock, placement), v);
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

Edge StyleAccessor::GetEdge() const
{
	return block->edge;
}

void StyleAccessor::SetEdge(Edge v)
{
	AccSet(*this, offsetof(StyleBlock, edge), v);
}

BoxSizing StyleAccessor::GetBoxSizing(BoxSizingTarget t) const
{
	return BoxSizing((block->boxSizing >> (unsigned(t) << 1)) & 3);
}

void StyleAccessor::SetBoxSizing(BoxSizingTarget t, BoxSizing v)
{
	auto f = block->boxSizing;
	f &= ~(3 << (unsigned(t) << 1));
	f |= (unsigned(v) << (unsigned(t) << 1));
	AccSet(*this, offsetof(StyleBlock, boxSizing), f);
}

HAlign StyleAccessor::GetHAlign() const
{
	return block->h_align;
}

void StyleAccessor::SetHAlign(HAlign a)
{
	AccSet(*this, offsetof(StyleBlock, h_align), a);
}


const std::string& StyleAccessor::GetFontFamily() const
{
	return block->font_family;
}

void StyleAccessor::SetFontFamily(const std::string& v)
{
	AccSet(*this, offsetof(StyleBlock, font_family), v);
}

FontWeight StyleAccessor::GetFontWeight() const
{
	return block->font_weight;
}

void StyleAccessor::SetFontWeight(FontWeight v)
{
	AccSet(*this, offsetof(StyleBlock, font_weight), v);
}

FontStyle StyleAccessor::GetFontStyle() const
{
	return block->font_style;
}

void StyleAccessor::SetFontStyle(FontStyle v)
{
	AccSet(*this, offsetof(StyleBlock, font_style), v);
}

int StyleAccessor::GetFontSize() const
{
	return block->font_size;
}

void StyleAccessor::SetFontSize(int v)
{
	AccSet(*this, offsetof(StyleBlock, font_size), v);
}

Color4b StyleAccessor::GetTextColor() const
{
	return block->text_color;
}

void StyleAccessor::SetTextColor(Color4b v)
{
	AccSet(*this, offsetof(StyleBlock, text_color), v);
}


Coord StyleAccessor::GetWidth() const
{
	return block->width;
}

void StyleAccessor::SetWidth(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, width), v);
}

Coord StyleAccessor::GetHeight() const
{
	return block->height;
}

void StyleAccessor::SetHeight(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, height), v);
}

Coord StyleAccessor::GetMinWidth() const
{
	return block->min_width;
}

void StyleAccessor::SetMinWidth(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, min_width), v);
}

Coord StyleAccessor::GetMinHeight() const
{
	return block->min_height;
}

void StyleAccessor::SetMinHeight(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, min_height), v);
}

Coord StyleAccessor::GetMaxWidth() const
{
	return block->max_width;
}

void StyleAccessor::SetMaxWidth(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, max_width), v);
}

Coord StyleAccessor::GetMaxHeight() const
{
	return block->max_height;
}

void StyleAccessor::SetMaxHeight(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, max_height), v);
}

float StyleAccessor::GetMarginLeft() const
{
	return block->margin_left;
}

void StyleAccessor::SetMarginLeft(float v)
{
	AccSet(*this, offsetof(StyleBlock, margin_left), v);
}

float StyleAccessor::GetMarginRight() const
{
	return block->margin_right;
}

void StyleAccessor::SetMarginRight(float v)
{
	AccSet(*this, offsetof(StyleBlock, margin_right), v);
}

float StyleAccessor::GetMarginTop() const
{
	return block->margin_top;
}

void StyleAccessor::SetMarginTop(float v)
{
	AccSet(*this, offsetof(StyleBlock, margin_top), v);
}

float StyleAccessor::GetMarginBottom() const
{
	return block->margin_bottom;
}

void StyleAccessor::SetMarginBottom(float v)
{
	AccSet(*this, offsetof(StyleBlock, margin_bottom), v);
}

void StyleAccessor::SetMargin(float t, float r, float b, float l)
{
	// TODO?
#if 0
	GetOrCreate();
	block->margin_top = t;
	block->margin_right = r;
	block->margin_bottom = b;
	block->margin_left = l;
#endif
	AccSet(*this, offsetof(StyleBlock, margin_top), t);
	AccSet(*this, offsetof(StyleBlock, margin_right), r);
	AccSet(*this, offsetof(StyleBlock, margin_bottom), b);
	AccSet(*this, offsetof(StyleBlock, margin_left), l);
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
static StyleBlockRef g_textStyle;

void _InitStyles()
{
	g_objectStyle = new StyleBlock;
	g_objectStyle->background_painter = EmptyPainter::Get();

	g_textStyle = new StyleBlock;
	g_textStyle->layout = layouts::InlineBlock();
	g_textStyle->background_painter = EmptyPainter::Get();
}

void _FreeStyles()
{
	g_objectStyle = nullptr;
	g_textStyle = nullptr;
}

StyleBlockRef GetObjectStyle()
{
	return g_objectStyle;
}

StyleBlockRef GetTextStyle()
{
	return g_textStyle;
}


} // ui
