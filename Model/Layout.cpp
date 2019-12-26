
#include <vector>

#include "Layout.h"
#include "Objects.h"


namespace style {


PaintInfo::PaintInfo(const UIObject* o) : obj(o)
{
	rect = o->GetPaddingRect();
	if (o->IsHovered())
		state |= PS_Hover;
	if (o->IsClicked())
		state |= PS_Down;
	if (o->IsInputDisabled())
		state |= PS_Disabled;
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
	if (layout == Layout::Undefined) layout = o.layout;
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
	if (layout == Layout::Inherit) layout = o.layout;
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

void Block::_Release()
{
	if (--_refCount <= 0)
		delete this;
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


style::Block* Accessor::GetOrCreate()
{
	// TODO optimize?
	if (block->_refCount <= 1 || !blkref)
		return block;
	return block = *blkref = new Block(**blkref);
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

Layout Accessor::GetLayout() const
{
	return block->layout;
}

void Accessor::SetLayout(Layout v)
{
	GetOrCreate()->layout = v;
}

StackingDirection Accessor::GetStackingDirection() const
{
	return block->stacking_direction;
}

void Accessor::SetStackingDirection(StackingDirection v)
{
	GetOrCreate()->stacking_direction = v;
}

Edge Accessor::GetEdge() const
{
	return block->edge;
}

void Accessor::SetEdge(Edge v)
{
	GetOrCreate()->edge = v;
}

BoxSizing Accessor::GetBoxSizing() const
{
	return block->box_sizing;
}

void Accessor::SetBoxSizing(BoxSizing v)
{
	GetOrCreate()->box_sizing = v;
}

PaintFunction Accessor::GetPaintFunc() const
{
	return block->paint_func;
}

PaintFunction& Accessor::MutablePaintFunc()
{
	return GetOrCreate()->paint_func;
}

Coord Accessor::GetWidth() const
{
	return block->width;
}

void Accessor::SetWidth(Coord v)
{
	GetOrCreate()->width = v;
}

Coord Accessor::GetHeight() const
{
	return block->height;
}

void Accessor::SetHeight(Coord v)
{
	GetOrCreate()->height = v;
}

Coord Accessor::GetMinWidth() const
{
	return block->min_width;
}

void Accessor::SetMinWidth(Coord v)
{
	GetOrCreate()->min_width = v;
}

Coord Accessor::GetMinHeight() const
{
	return block->min_height;
}

void Accessor::SetMinHeight(Coord v)
{
	GetOrCreate()->min_height = v;
}

Coord Accessor::GetMaxWidth() const
{
	return block->max_width;
}

void Accessor::SetMaxWidth(Coord v)
{
	GetOrCreate()->max_width = v;
}

Coord Accessor::GetMaxHeight() const
{
	return block->max_height;
}

void Accessor::SetMaxHeight(Coord v)
{
	GetOrCreate()->max_height = v;
}

Coord Accessor::GetLeft() const
{
	return block->left;
}

void Accessor::SetLeft(Coord v)
{
	GetOrCreate()->left = v;
}

Coord Accessor::GetRight() const
{
	return block->right;
}

void Accessor::SetRight(Coord v)
{
	GetOrCreate()->right = v;
}

Coord Accessor::GetTop() const
{
	return block->top;
}

void Accessor::SetTop(Coord v)
{
	GetOrCreate()->top = v;
}

Coord Accessor::GetBottom() const
{
	return block->bottom;
}

void Accessor::SetBottom(Coord v)
{
	GetOrCreate()->bottom = v;
}

Coord Accessor::GetMarginLeft() const
{
	return block->margin_left;
}

void Accessor::SetMarginLeft(Coord v)
{
	GetOrCreate()->margin_left = v;
}

Coord Accessor::GetMarginRight() const
{
	return block->margin_right;
}

void Accessor::SetMarginRight(Coord v)
{
	GetOrCreate()->margin_right = v;
}

Coord Accessor::GetMarginTop() const
{
	return block->margin_top;
}

void Accessor::SetMarginTop(Coord v)
{
	GetOrCreate()->margin_top = v;
}

Coord Accessor::GetMarginBottom() const
{
	return block->margin_bottom;
}

void Accessor::SetMarginBottom(Coord v)
{
	GetOrCreate()->margin_bottom = v;
}

void Accessor::SetMargin(Coord t, Coord r, Coord b, Coord l)
{
	GetOrCreate();
	block->margin_top = t;
	block->margin_right = r;
	block->margin_bottom = b;
	block->margin_left = l;
}

Coord Accessor::GetPaddingLeft() const
{
	return block->padding_left;
}

void Accessor::SetPaddingLeft(Coord v)
{
	GetOrCreate()->padding_left = v;
}

Coord Accessor::GetPaddingRight() const
{
	return block->padding_right;
}

void Accessor::SetPaddingRight(Coord v)
{
	GetOrCreate()->padding_right = v;
}

Coord Accessor::GetPaddingTop() const
{
	return block->padding_top;
}

void Accessor::SetPaddingTop(Coord v)
{
	GetOrCreate()->padding_top = v;
}

Coord Accessor::GetPaddingBottom() const
{
	return block->padding_bottom;
}

void Accessor::SetPaddingBottom(Coord v)
{
	GetOrCreate()->padding_bottom = v;
}

void Accessor::SetPadding(Coord t, Coord r, Coord b, Coord l)
{
	GetOrCreate();
	block->padding_top = t;
	block->padding_right = r;
	block->padding_bottom = b;
	block->padding_left = l;
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
