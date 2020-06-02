
#include <vector>

#include "Layout.h"
#include "Objects.h"


namespace style {


namespace layouts {

struct InlineBlockLayout : Layout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		float size = 0;
		for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Exact).min;
		return size;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		float size = GetFontHeight();
		for (auto* ch = curObj->firstChild; ch; ch = ch->next)
			size = std::max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min);
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
			float w = ch->GetFullEstimatedWidth(contSize, EstSizeType::Exact).min;
			float h = ch->GetFullEstimatedHeight(contSize, EstSizeType::Exact).min;
			ch->PerformLayout({ p, y0, p + w, y0 + h }, contSize);
			p += w;
			maxH = std::max(maxH, h);
		}
		state.finalContentRect = { inrect.x0, inrect.y0, p, y0 + maxH };
	}
}
g_inlineBlockLayout;
Layout* InlineBlock() { return &g_inlineBlockLayout; }

struct StackLayout : Layout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		auto style = curObj->GetStyle();
		float size = 0;
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min);
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedWidth(containerSize, EstSizeType::Expanding).min;
			break;
		}
		return size;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		auto style = curObj->GetStyle();
		float size = 0;
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min;
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min);
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
			if (ha != style::HAlign::Undefined && ha != style::HAlign::Left)
			{
				float tw = 0;
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
					tw += ch->GetFullEstimatedWidth(inrect.GetSize(), EstSizeType::Expanding).min;
				float diff = inrect.GetWidth() - tw;
				switch (ha)
				{
				case style::HAlign::Center:
					p += diff / 2;
					break;
				case style::HAlign::Right:
					p += diff;
					break;
				case style::HAlign::Justify: {
					auto cc = curObj->CountChildrenImmediate();
					if (cc > 1)
						xw += diff / (cc - 1);
					else
						p += diff / 2;
					break; }
				}
			}
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
Layout* Stack() { return &g_stackLayout; }

struct StackExpandLayout : Layout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		auto style = curObj->GetStyle();
		float size = 0;
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedWidth(containerSize, EstSizeType::Exact).min);
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
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
	float CalcEstimatedHeight(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		auto style = curObj->GetStyle();
		float size = 0;
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			if (type == EstSizeType::Expanding)
			{
				for (auto* ch = curObj->firstChild; ch; ch = ch->next)
					size += ch->GetFullEstimatedHeight(containerSize, EstSizeType::Expanding).min;
			}
			else
				size = containerSize.y;
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = curObj->firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedHeight(containerSize, EstSizeType::Exact).min);
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
				float fr = sw.unit == style::CoordTypeUnit::Fraction ? sw.value : 1;
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
			float leftover = std::max(inrect.GetWidth() - sum, 0.0f);
			for (auto idx : sorted)
			{
				auto& item = items[idx];
				float mylo = leftover * item.fr / frsum;
				float w = item.minw + mylo;
				if (w > item.maxw)
					w = item.maxw;
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
			state.finalContentRect = { inrect.x0, inrect.y0, std::max(inrect.x1, p), inrect.y1 };
			break; }
		}
	}
}
g_stackExpandLayout;
Layout* StackExpand() { return &g_stackExpandLayout; }


struct EdgeSliceLayout : Layout
{
	float CalcEstimatedWidth(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
	{
		return containerSize.x;
	}
	float CalcEstimatedHeight(UIObject* curObj, const Size<float>& containerSize, EstSizeType type)
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
Layout* EdgeSlice() { return &g_edgeSliceLayout; }
} // layouts


int g_numBlocks;


PaintInfo::PaintInfo(const UIObject* o) : obj(o)
{
	rect = o->GetPaddingRect();
	if (o->IsHovered())
		state |= PS_Hover;
	if (o->IsClicked())
		state |= PS_Down;
	if (o->IsInputDisabled())
		state |= PS_Disabled;
	if (o->IsFocused())
		state |= PS_Focused;
}

void IErrorCallback::OnError(const char* message, const char* at)
{
	hadError = true;
	StringView pre = fullText.substr(0, at - fullText.data());
	OnError(message, pre.count("\n") + 1, pre.size() - pre.find_last_at("\n", SIZE_MAX, 0));
}

static bool CSSIdentChar(char c) { return IsAlphaNum(c) || c == '_' || c == '-'; }
static bool CSSIdentValStartChar(char c) { return IsAlpha(c) || c == '_'; }
static bool CSSNumberStartChar(char c) { return IsDigit(c) || c == '-'; }

template <size_t N, class T>
static bool ParseEnum(StringView value, const StringView(&options)[N], T& out)
{
	for (size_t i = 0; i < N; i++)
	{
		if (!value.empty() && options[i] == value)
		{
			out = T(i);
			return true;
		}
	}
	return false;
}

static StringView CoordTypeValues[] = { "", "inherit" };
static StringView CoordTypeValuesWithAuto[] = { "", "inherit", "auto" };
static void ParseCoord(StringView& text, IErrorCallback* err, Coord& out, bool withAuto)
{
	if (text.first_char_is(CSSIdentValStartChar))
	{
		int which = 0;
		auto value = text.take_while(CSSIdentChar);
		if (withAuto ? ParseEnum(text, CoordTypeValuesWithAuto, which) : ParseEnum(text, CoordTypeValues, which))
			out.unit = CoordTypeUnit(which);
		else
			err->OnError("failed to parse coord value", text.data());
		return;
	}

	if (text.first_char_is(CSSNumberStartChar))
	{
		// TODO number
	}
}


//static StringView DisplayValues[] = { "", "inherit",  "none", "block", "inline", "flex", "inline-flex", "grid", "inline-grid" };
//static StringView PositionValues[] = { "", "inherit",  "static", "relative", "absolute", "fixed", "sticky" };
static StringView LayoutValues[] = { "", "inherit", /*"none", "inline",*/ "inline-block", "stack", "edge-slice" };
static StringView StackingDirectionValues[] = { "", "inherit", "top-down", "right-to-left", "bottom-up", "left-to-right" };
static StringView EdgeValues[] = { "", "inherit", "top", "right", "bottom", "left" };
static StringView BoxSizingValues[] = { "", "inherit", "content-box", "border-box" };

void Block::Compile(StringView& text, IErrorCallback* err)
{
	bool took = text.take_if_equal("{");
	assert(took && "{");

	for (;;)
	{
		text = text.ltrim();
		if (text.take_if_equal("}"))
		{
			break;
		}

		auto name = text.take_while(CSSIdentChar);
		text = text.ltrim();
		if (!text.take_if_equal(":"))
		{
			err->OnError("expected ':'", text.data());
			return;
		}

		text = text.ltrim();

		if (0);
#if 0
		else if (name == "display")
		{
			auto value = text.take_while(CSSIdentChar);
			if (!ParseEnum(value, DisplayValues, display))
			{
				err->OnError("failed to parse 'display' value", text.data());
				text = text.after_first(";");
			}
		}
		else if (name == "position")
		{
			auto value = text.take_while(CSSIdentChar);
			if (!ParseEnum(value, PositionValues, position))
			{
				err->OnError("failed to parse 'position' value", text.data());
				text = text.after_first(";");
			}
		}
#endif
		else if (name == "layout")
		{
			auto value = text.take_while(CSSIdentChar);
			if (!ParseEnum(value, LayoutValues, layout))
			{
				err->OnError("failed to parse 'layout' value", text.data());
				text = text.after_first(";");
			}
		}
		else if (name == "stacking-direction")
		{
			auto value = text.take_while(CSSIdentChar);
			if (!ParseEnum(value, StackingDirectionValues, stacking_direction))
			{
				err->OnError("failed to parse 'stacking-direction' value", text.data());
				text = text.after_first(";");
			}
		}
		else if (name == "edge")
		{
			auto value = text.take_while(CSSIdentChar);
			if (!ParseEnum(value, EdgeValues, edge))
			{
				err->OnError("failed to parse 'edge' value", text.data());
				text = text.after_first(";");
			}
		}
		else if (name == "box-sizing")
		{
			auto value = text.take_while(CSSIdentChar);
			if (!ParseEnum(value, BoxSizingValues, box_sizing))
			{
				err->OnError("failed to parse 'box-sizing' value", text.data());
				text = text.after_first(";");
			}
		}
		else if (name == "width") ParseCoord(text, err, width, true);
		else if (name == "height") ParseCoord(text, err, height, true);

		if (err->hadError)
			break;
		text = text.ltrim();
		if (!text.take_if_equal(";"))
		{
			err->OnError("expected ;", text.data());
			text = text.after_first("}");
			return;
		}
	}
}

void Block::MergeDirect(const Block& o)
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

void Block::MergeParent(const Block& o)
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

Block::~Block()
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

void Block::_Release()
{
	if (--_refCount <= 0)
		delete this;
}

Block* Block::_GetWithChange(int off, FnIsPropEqual feq, FnPropCopy fcopy, const void* ref)
{
	for (auto* ch = _firstChild; ch; ch = ch->_next)
	{
		if (ch->_parentDiffOffset == off && feq(reinterpret_cast<char*>(ch) + off, ref))
		{
			return ch;
		}
	}

	// create new
	Block* copy = new Block(*this);
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


bool Selector::Element::IsDirectMatch(UIObject* obj) const
{
	// TODO element (and *)
	// TODO id
	// TODO class
	if (firstChild && obj->prev)
		return false;
	if (hover && !(obj->flags & UIObject_IsHovered))
		return false;
	if (active && !(obj->flags & UIObject_IsClickedL))
		return false;
	return true;
}

void Selector::Compile(StringView& text, IErrorCallback* err)
{
	for (;;)
	{
		text = text.ltrim();
		if (text.starts_with(",") || text.starts_with("{"))
			break;

		if (!elements.empty())
		{
			if (text.take_if_equal(">"))
			{
				elements.back().immParent = true;
			}
			else if (text.take_if_equal("+"))
			{
				elements.back().prevSibling = true;
			}
			text = text.ltrim();
		}

		Element e;
		if (text.starts_with("*"))
		{
			text = text.substr(1);
			e.element = "*";
		}
		else if (text.first_char_is(CSSIdentChar))
		{
			e.element = text.take_while(CSSIdentChar);
		}

		for (;;)
		{
			if (text.take_if_equal("."))
			{
				auto name = text.take_while(CSSIdentChar);
				e.classes.push_back(name);
				continue;
			}
			if (text.take_if_equal("#"))
			{
				auto name = text.take_while(CSSIdentChar);
				e.ids.push_back(name);
				continue;
			}
			if (text.take_if_equal(":first-child"))
			{
				e.firstChild = true;
				continue;
			}
			if (text.take_if_equal(":hover"))
			{
				e.hover = true;
				continue;
			}
			if (text.take_if_equal(":active"))
			{
				e.active = true;
				continue;
			}
			if (text.starts_with(">") ||
				text.starts_with("+") ||
				text.first_char_is(IsSpace) ||
				text.starts_with(",") ||
				text.starts_with("{"))
				break;

			err->OnError("unexpected character in selector", text.data());
			return;
		}
		elements.push_back(std::move(e));
	}
}

bool Selector::Check(UIObject* obj) const
{
	return _CheckOne(obj, elements.size() - 1);
}

bool Selector::_CheckOne(UIObject* obj, size_t el) const
{
	auto& E = elements[el];
	if (!E.IsDirectMatch(obj))
		return false;
	// last element
	if (el == 0)
		return true;
	auto& E1 = elements[el - 1];
	if (E1.immParent)
		return obj->parent && _CheckOne(obj->parent, el - 1);
	if (E1.prevSibling)
		return obj->prev && _CheckOne(obj->prev, el - 1);
	// check any parent
	for (auto* p = obj->parent; p; p = p->parent)
		if (_CheckOne(p, el - 1))
			return true;
	return false;
}

void Definition::Compile(StringView& text, IErrorCallback* err)
{
	for (;;)
	{
		text = text.ltrim();

		Selector s;
		s.Compile(text, err);
		if (err->hadError)
			return;
		selectors.push_back(std::move(s));

		text = text.ltrim();
		if (text.take_if_equal(","))
			continue;
		if (text.starts_with("{"))
			break;
		err->OnError("expected , or {", text.data());
		return;
	}

	block.Compile(text, err);
}

bool Definition::Check(UIObject* obj) const
{
	for (const auto& s : selectors)
		if (s.Check(obj))
			return true;
	return false;
}

void Sheet::Compile(StringView text, IErrorCallback* err)
{
	fullText.clear();
	definitions.clear();

	err->fullText = text;
	_Preprocess(text, err);
	if (err->hadError)
		return;

	text = fullText; // use saved copy
	err->fullText = text;
	for (;;)
	{
		text = text.ltrim();
		if (text.empty())
			break;

		Definition d;
		d.Compile(text, err);
		if (err->hadError)
			break;
		definitions.emplace_back(std::move(d));
	}
}

void Sheet::_Preprocess(StringView text, IErrorCallback* err)
{
	while (!text.empty())
	{
		if (text.starts_with("/*"))
		{
			auto at = text.find_first_at("*/", 2);
			int numNL = text.count("\n", at);
			for (int i = 0; i < numNL; i++)
				fullText.push_back('\n');
			text = text.substr(at + 2);
			continue;
		}
		fullText.push_back(text.take_char());
	}
}


Accessor::Accessor(Block* b) : block(b), blkref(nullptr)
{
}

Accessor::Accessor(BlockRef& r, bool unique) : block(r), blkref(unique ? &r : nullptr)
{
}

#if 0
Display Accessor::GetDisplay() const
{
	return obj->styleProps ? obj->styleProps->display : Display::Undefined;
}

void Accessor::SetDisplay(Display display)
{
	CreateBlock(obj);
	obj->styleProps->display = display;
}

Position Accessor::GetPosition() const
{
	return obj->styleProps ? obj->styleProps->position : Position::Undefined;
}

void Accessor::SetPosition(Position position)
{
	CreateBlock(obj);
	obj->styleProps->position = position;
}
#endif

template <class T> void AccSet(Accessor& a, int off, T v)
{
	if (a.blkref)
	{
		if (auto* it = a.block->GetWithChange(off, v))
		{
			*a.blkref = a.block = it;
			return;
		}
	}
	PropFuncs<T>::Copy(reinterpret_cast<char*>(a.block) + off, &v);
}

Layout* Accessor::GetLayout() const
{
	return block->layout;
}

void Accessor::SetLayout(Layout* v)
{
	AccSet(*this, offsetof(Block, layout), v);
}

StackingDirection Accessor::GetStackingDirection() const
{
	return block->stacking_direction;
}

void Accessor::SetStackingDirection(StackingDirection v)
{
	AccSet(*this, offsetof(Block, stacking_direction), v);
}

Edge Accessor::GetEdge() const
{
	return block->edge;
}

void Accessor::SetEdge(Edge v)
{
	AccSet(*this, offsetof(Block, edge), v);
}

BoxSizing Accessor::GetBoxSizing() const
{
	return block->box_sizing;
}

void Accessor::SetBoxSizing(BoxSizing v)
{
	AccSet(*this, offsetof(Block, box_sizing), v);
}

HAlign Accessor::GetHAlign() const
{
	return block->h_align;
}

void Accessor::SetHAlign(HAlign a)
{
	AccSet(*this, offsetof(Block, h_align), a);
}

PaintFunction Accessor::GetPaintFunc() const
{
	return block->paint_func;
}

void Accessor::SetPaintFunc(const PaintFunction& f)
{
	AccSet(*this, offsetof(Block, paint_func), f);
}

Coord Accessor::GetWidth() const
{
	return block->width;
}

void Accessor::SetWidth(Coord v)
{
	AccSet(*this, offsetof(Block, width), v);
}

Coord Accessor::GetHeight() const
{
	return block->height;
}

void Accessor::SetHeight(Coord v)
{
	AccSet(*this, offsetof(Block, height), v);
}

Coord Accessor::GetMinWidth() const
{
	return block->min_width;
}

void Accessor::SetMinWidth(Coord v)
{
	AccSet(*this, offsetof(Block, min_width), v);
}

Coord Accessor::GetMinHeight() const
{
	return block->min_height;
}

void Accessor::SetMinHeight(Coord v)
{
	AccSet(*this, offsetof(Block, min_height), v);
}

Coord Accessor::GetMaxWidth() const
{
	return block->max_width;
}

void Accessor::SetMaxWidth(Coord v)
{
	AccSet(*this, offsetof(Block, max_width), v);
}

Coord Accessor::GetMaxHeight() const
{
	return block->max_height;
}

void Accessor::SetMaxHeight(Coord v)
{
	AccSet(*this, offsetof(Block, max_height), v);
}

Coord Accessor::GetLeft() const
{
	return block->left;
}

void Accessor::SetLeft(Coord v)
{
	AccSet(*this, offsetof(Block, left), v);
}

Coord Accessor::GetRight() const
{
	return block->right;
}

void Accessor::SetRight(Coord v)
{
	AccSet(*this, offsetof(Block, right), v);
}

Coord Accessor::GetTop() const
{
	return block->top;
}

void Accessor::SetTop(Coord v)
{
	AccSet(*this, offsetof(Block, top), v);
}

Coord Accessor::GetBottom() const
{
	return block->bottom;
}

void Accessor::SetBottom(Coord v)
{
	AccSet(*this, offsetof(Block, bottom), v);
}

Coord Accessor::GetMarginLeft() const
{
	return block->margin_left;
}

void Accessor::SetMarginLeft(Coord v)
{
	AccSet(*this, offsetof(Block, margin_left), v);
}

Coord Accessor::GetMarginRight() const
{
	return block->margin_right;
}

void Accessor::SetMarginRight(Coord v)
{
	AccSet(*this, offsetof(Block, margin_right), v);
}

Coord Accessor::GetMarginTop() const
{
	return block->margin_top;
}

void Accessor::SetMarginTop(Coord v)
{
	AccSet(*this, offsetof(Block, margin_top), v);
}

Coord Accessor::GetMarginBottom() const
{
	return block->margin_bottom;
}

void Accessor::SetMarginBottom(Coord v)
{
	AccSet(*this, offsetof(Block, margin_bottom), v);
}

void Accessor::SetMargin(Coord t, Coord r, Coord b, Coord l)
{
	// TODO?
#if 0
	GetOrCreate();
	block->margin_top = t;
	block->margin_right = r;
	block->margin_bottom = b;
	block->margin_left = l;
#endif
	AccSet(*this, offsetof(Block, margin_top), t);
	AccSet(*this, offsetof(Block, margin_right), r);
	AccSet(*this, offsetof(Block, margin_bottom), b);
	AccSet(*this, offsetof(Block, margin_left), l);
}

Coord Accessor::GetPaddingLeft() const
{
	return block->padding_left;
}

void Accessor::SetPaddingLeft(Coord v)
{
	AccSet(*this, offsetof(Block, padding_left), v);
}

Coord Accessor::GetPaddingRight() const
{
	return block->padding_right;
}

void Accessor::SetPaddingRight(Coord v)
{
	AccSet(*this, offsetof(Block, padding_right), v);
}

Coord Accessor::GetPaddingTop() const
{
	return block->padding_top;
}

void Accessor::SetPaddingTop(Coord v)
{
	AccSet(*this, offsetof(Block, padding_top), v);
}

Coord Accessor::GetPaddingBottom() const
{
	return block->padding_bottom;
}

void Accessor::SetPaddingBottom(Coord v)
{
	AccSet(*this, offsetof(Block, padding_bottom), v);
}

void Accessor::SetPadding(Coord t, Coord r, Coord b, Coord l)
{
	// TODO?
#if 0
	GetOrCreate();
	block->padding_top = t;
	block->padding_right = r;
	block->padding_bottom = b;
	block->padding_left = l;
#endif
	AccSet(*this, offsetof(Block, padding_top), t);
	AccSet(*this, offsetof(Block, padding_right), r);
	AccSet(*this, offsetof(Block, padding_bottom), b);
	AccSet(*this, offsetof(Block, padding_left), l);
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


}
