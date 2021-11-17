
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
		auto pr = curObj->GetPaddingRect(curObj->styleProps, contSize.x);
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
			if (e == Edge::Undefined)
				e = Edge::Top;
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
		checkState = 1;
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


ContentPaintAdvice CheckerboardPainter::Paint(const PaintInfo& info)
{
	auto bgr = Theme::current->GetImage(ThemeImage::CheckerboardBackground);

	auto r = info.rect;

	draw::RectTex(r.x0, r.y0, r.x1, r.y1, bgr, 0, 0, r.GetWidth() / bgr->GetWidth(), r.GetHeight() / bgr->GetHeight());

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
		ret = h->Paint(info);
	return ret;
}

RCHandle<LayerPainter> LayerPainter::Create()
{
	return new LayerPainter;
}


void StyleBlock::MergeDirect(const StyleBlock& o)
{
	//if (display == Display::Undefined) display = o.display;
	//if (position == Position::Undefined) position = o.position;
	if (layout == nullptr) layout = o.layout;
	if (stacking_direction == StackingDirection::Undefined) stacking_direction = o.stacking_direction;
	if (edge == Edge::Undefined) edge = o.edge;
	if (box_sizing == BoxSizing::Undefined) box_sizing = o.box_sizing;

	if (!width.IsDefined()) width = o.width;
	if (!height.IsDefined()) height = o.height;
	if (!min_width.IsDefined()) min_width = o.min_width;
	if (!min_height.IsDefined()) min_height = o.min_height;
	if (!max_width.IsDefined()) max_width = o.max_width;
	if (!max_height.IsDefined()) max_height = o.max_height;

	if (!left.IsDefined()) left = o.left;
	if (!right.IsDefined()) right = o.right;
	if (!top.IsDefined()) top = o.top;
	if (!bottom.IsDefined()) bottom = o.bottom;
	if (!margin_left.IsDefined()) margin_left = o.margin_left;
	if (!margin_right.IsDefined()) margin_right = o.margin_right;
	if (!margin_top.IsDefined()) margin_top = o.margin_top;
	if (!margin_bottom.IsDefined()) margin_bottom = o.margin_bottom;
	if (!padding_left.IsDefined()) padding_left = o.padding_left;
	if (!padding_right.IsDefined()) padding_right = o.padding_right;
	if (!padding_top.IsDefined()) padding_top = o.padding_top;
	if (!padding_bottom.IsDefined()) padding_bottom = o.padding_bottom;
}

void StyleBlock::MergeParent(const StyleBlock& o)
{
	//if (layout == Layout::Inherit) layout = o.layout;
	if (stacking_direction == StackingDirection::Inherit) stacking_direction = o.stacking_direction;
	if (edge == Edge::Inherit) edge = o.edge;
	if (box_sizing == BoxSizing::Inherit) box_sizing = o.box_sizing;

	if (width.unit == CoordTypeUnit::Inherit) width = o.width;
	if (height.unit == CoordTypeUnit::Inherit) height = o.height;
	if (min_width.unit == CoordTypeUnit::Inherit) min_width = o.min_width;
	if (min_height.unit == CoordTypeUnit::Inherit) min_height = o.min_height;
	if (max_width.unit == CoordTypeUnit::Inherit) max_width = o.max_width;
	if (max_height.unit == CoordTypeUnit::Inherit) max_height = o.max_height;

	if (left.unit == CoordTypeUnit::Inherit) left = o.left;
	if (right.unit == CoordTypeUnit::Inherit) right = o.right;
	if (top.unit == CoordTypeUnit::Inherit) top = o.top;
	if (bottom.unit == CoordTypeUnit::Inherit) bottom = o.bottom;
	if (margin_left.unit == CoordTypeUnit::Inherit) margin_left = o.margin_left;
	if (margin_right.unit == CoordTypeUnit::Inherit) margin_right = o.margin_right;
	if (margin_top.unit == CoordTypeUnit::Inherit) margin_top = o.margin_top;
	if (margin_bottom.unit == CoordTypeUnit::Inherit) margin_bottom = o.margin_bottom;
	if (padding_left.unit == CoordTypeUnit::Inherit) padding_left = o.padding_left;
	if (padding_right.unit == CoordTypeUnit::Inherit) padding_right = o.padding_right;
	if (padding_top.unit == CoordTypeUnit::Inherit) padding_top = o.padding_top;
	if (padding_bottom.unit == CoordTypeUnit::Inherit) padding_bottom = o.padding_bottom;
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
	return _cachedFont.GetCachedFont(FONT_FAMILY_SANS_SERIF, int(font_weight), font_style == FontStyle::Italic);
}


StyleAccessor::StyleAccessor(StyleBlock* b) : block(b), blkref(nullptr), owner(nullptr)
{
}

StyleAccessor::StyleAccessor(StyleBlockRef& r, UIObject* o) : block(r), blkref(&r), owner(o)
{
}

#if 0
Display StyleAccessor::GetDisplay() const
{
	return obj->styleProps ? obj->styleProps->display : Display::Undefined;
}

void StyleAccessor::SetDisplay(Display display)
{
	CreateStyleBlock(obj);
	obj->styleProps->display = display;
}

Position StyleAccessor::GetPosition() const
{
	return obj->styleProps ? obj->styleProps->position : Position::Undefined;
}

void StyleAccessor::SetPosition(Position position)
{
	CreateStyleBlock(obj);
	obj->styleProps->position = position;
}
#endif

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

BoxSizing StyleAccessor::GetBoxSizing() const
{
	return block->box_sizing;
}

void StyleAccessor::SetBoxSizing(BoxSizing v)
{
	AccSet(*this, offsetof(StyleBlock, box_sizing), v);
}

HAlign StyleAccessor::GetHAlign() const
{
	return block->h_align;
}

void StyleAccessor::SetHAlign(HAlign a)
{
	AccSet(*this, offsetof(StyleBlock, h_align), a);
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

Coord StyleAccessor::GetLeft() const
{
	return block->left;
}

void StyleAccessor::SetLeft(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, left), v);
}

Coord StyleAccessor::GetRight() const
{
	return block->right;
}

void StyleAccessor::SetRight(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, right), v);
}

Coord StyleAccessor::GetTop() const
{
	return block->top;
}

void StyleAccessor::SetTop(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, top), v);
}

Coord StyleAccessor::GetBottom() const
{
	return block->bottom;
}

void StyleAccessor::SetBottom(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, bottom), v);
}

Coord StyleAccessor::GetMarginLeft() const
{
	return block->margin_left;
}

void StyleAccessor::SetMarginLeft(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, margin_left), v);
}

Coord StyleAccessor::GetMarginRight() const
{
	return block->margin_right;
}

void StyleAccessor::SetMarginRight(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, margin_right), v);
}

Coord StyleAccessor::GetMarginTop() const
{
	return block->margin_top;
}

void StyleAccessor::SetMarginTop(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, margin_top), v);
}

Coord StyleAccessor::GetMarginBottom() const
{
	return block->margin_bottom;
}

void StyleAccessor::SetMarginBottom(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, margin_bottom), v);
}

void StyleAccessor::SetMargin(Coord t, Coord r, Coord b, Coord l)
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

Coord StyleAccessor::GetPaddingLeft() const
{
	return block->padding_left;
}

void StyleAccessor::SetPaddingLeft(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, padding_left), v);
}

Coord StyleAccessor::GetPaddingRight() const
{
	return block->padding_right;
}

void StyleAccessor::SetPaddingRight(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, padding_right), v);
}

Coord StyleAccessor::GetPaddingTop() const
{
	return block->padding_top;
}

void StyleAccessor::SetPaddingTop(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, padding_top), v);
}

Coord StyleAccessor::GetPaddingBottom() const
{
	return block->padding_bottom;
}

void StyleAccessor::SetPaddingBottom(Coord v)
{
	AccSet(*this, offsetof(StyleBlock, padding_bottom), v);
}

void StyleAccessor::SetPadding(Coord t, Coord r, Coord b, Coord l)
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


// https://www.w3.org/TR/css-flexbox-1/#line-sizing
void FlexLayout(UIObject* obj)
{
	// TODO 2 - available main and cross space for the flex items
	float maxMain = FLT_MAX;
	float maxCross = FLT_MAX;

	// TODO 3 - flex base size and hypothetical main size of each item
	struct ItemInfo
	{
		// initial (step 3)
		UIObject* ptr;
		float flexBaseSize;
		float hypMainSize;
		float outerMainPadding;
		// step 6
		bool frozen = false;
		float targetMainSize;
	};
	std::vector<ItemInfo> items;

	// TODO 4 - main size of the flex container
	float mainSize = 0;

	// TODO 5 - Collect flex items into flex lines
	std::vector<int> flexLines;
	flexLines.push_back(0);
#if TODO
	bool isSingleLine = obj->styleProps && obj->styleProps->IsEnum<style::FlexWrap>("flex-wrap", style::FlexWrap::NoWrap);
	if (!isSingleLine)
	{
		float accum = 0;
		for (int i = 0; i < int(items.size()); i++)
		{
			// TODO fragmenting
			if (accum + items[i].hypMainSize + items[i].outerMainPadding > maxMain && flexLines.back() != i - 1)
			{
				flexLines.push_back(i);
				accum = 0;
			}
			else
				accum += items[i].hypMainSize + items[i].outerMainPadding;
		}
	}
#endif
	flexLines.push_back(int(items.size()));

	// TODO 6 - Resolve the flexible lengths of all the flex items to find their used main size.
	for (int i = 0; i + 1 < int(flexLines.size()); i++)
	{
		// 9.7-1
		float outerHypotLineSum = 0;
		for (int j = flexLines[i]; j < flexLines[i + 1]; j++)
		{
			outerHypotLineSum += items[j].hypMainSize + items[j].outerMainPadding;
		}

		// 9.7-2
		int numFrozen = 0;
		bool growLine = outerHypotLineSum < mainSize;
		for (int j = flexLines[i]; j < flexLines[i + 1]; j++)
		{
			auto& I = items[j];
			float factor = growLine ? 0.0f : 1.0f;
#if TODO
			if (I.ptr->styleProps)
				I.ptr->styleProps->GetFloat(growLine ? "flex-grow" : "flex-shrink", factor);
#endif
			if (factor == 0 || (growLine ? I.flexBaseSize > I.hypMainSize : I.flexBaseSize < I.hypMainSize))
			{
				I.frozen = true;
				numFrozen++;
				I.targetMainSize = I.hypMainSize;
			}
		}

		// 9.7-3
		float initialFreeSpace = mainSize;
		for (int j = flexLines[i]; j < flexLines[i + 1]; j++)
		{
			initialFreeSpace -= items[j].frozen ? items[j].targetMainSize : items[j].flexBaseSize;
			initialFreeSpace -= items[j].outerMainPadding;
		}

		// 9.7-4
		while (numFrozen < flexLines[i + 1] - flexLines[i]) // a
		{
		}
	}

	// TODO 7 - Determine the hypothetical cross size of each item
}


} // ui
