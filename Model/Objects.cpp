
#include <unordered_map>
#include "Objects.h"
#include "System.h"
#include "Theme.h"


UIObject::UIObject()
{
	styleProps = ui::Theme::current->object;
}

UIObject::~UIObject()
{
	system->eventSystem.OnDestroy(this);
}

void UIObject::OnPaint()
{
	styleProps->paint_func(this);
	PaintChildren();
}

void UIObject::Paint()
{
	if (_CanPaint())
		OnPaint();
}

static Point<float> CalcEdgeSliceSize(UIObject* o, const Size<float>& containerSize)
{
	Point<float> ret = {};
	for (auto* ch = o->lastChild; ch; ch = ch->prev)
	{
		auto e = ch->GetStyle().GetEdge();
		if (e == style::Edge::Left || e == style::Edge::Right)
		{
			ret.x += ch->GetFullEstimatedWidth(containerSize).min;
			ret.y = std::max(ret.y, ch->GetFullEstimatedHeight(containerSize).min);
		}
		else
		{
			ret.x = std::max(ret.x, ch->GetFullEstimatedWidth(containerSize).min);
			ret.y += ch->GetFullEstimatedHeight(containerSize).min;
		}
	}
	return ret;
}

float UIObject::CalcEstimatedWidth(const Size<float>& containerSize)
{
	float size = 0;
	auto style = GetStyle();
	auto layout = style.GetLayout();
	if (layout == style::Layout::Undefined)
		layout = style::Layout::Stack;
	switch (layout)
	{
	case style::Layout::InlineBlock:
		for (auto* ch = firstChild; ch; ch = ch->next)
			size = std::max(size, ch->GetFullEstimatedWidth(containerSize).min);
		break;
	case style::Layout::Stack: {
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedWidth(containerSize).min);
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedWidth(containerSize).min;
			break;
		}
		break; }
	case style::Layout::StackExpand: {
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedWidth(containerSize).min);
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			size = containerSize.x;
			break;
		}
		break; }
	case style::Layout::EdgeSlice:
		size = containerSize.x;
		break;
	}
	return size;
}

float UIObject::CalcEstimatedHeight(const Size<float>& containerSize)
{
	float size = 0;
	auto style = GetStyle();
	auto layout = style.GetLayout();
	if (layout == style::Layout::Undefined)
		layout = style::Layout::Stack;
	switch (layout)
	{
	case style::Layout::InlineBlock:
		size = GetFontHeight();
		for (auto* ch = firstChild; ch; ch = ch->next)
			size = std::max(size, ch->GetFullEstimatedHeight(containerSize).min);
		break;
	case style::Layout::Stack: {
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedHeight(containerSize).min;
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedHeight(containerSize).min);
			break;
		}
		break; }
	case style::Layout::StackExpand: {
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown:
		case style::StackingDirection::BottomUp:
			size = containerSize.y;
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedHeight(containerSize).min);
			break;
		}
		break; }
	case style::Layout::EdgeSlice:
		size = containerSize.y;
		break;
	}
	return size;
}

Range<float> UIObject::GetEstimatedWidth(const Size<float>& containerSize)
{
	auto style = GetStyle();
	float size = 0;

	auto width = style.GetWidth();
	if (!width.IsDefined())
	{
		auto height = style::Coord::Undefined();
		GetSize(width, height);
	}

	float maxsize = FLT_MAX;
	if (width.IsDefined())
	{
		size = maxsize = ResolveUnits(width, containerSize.x);
	}
	else
	{
		size = CalcEstimatedWidth(containerSize);
	}

	auto min_width = style.GetMinWidth();
	if (min_width.IsDefined())
	{
		float w = ResolveUnits(min_width, containerSize.x);
		size = std::max(size, w);
		maxsize = std::max(maxsize, w);
	}

	auto max_width = style.GetMaxWidth();
	if (max_width.IsDefined())
	{
		maxsize = ResolveUnits(max_width, containerSize.x);
		size = std::min(size, maxsize);
	}

	return { size, maxsize };
}

Range<float> UIObject::GetEstimatedHeight(const Size<float>& containerSize)
{
	auto style = GetStyle();
	float size = 0;

	auto height = style.GetHeight();
	if (!height.IsDefined())
	{
		auto width = style::Coord::Undefined();
		GetSize(width, height);
	}

	float maxsize = FLT_MAX;
	if (height.IsDefined())
	{
		size = maxsize = ResolveUnits(height, containerSize.y);
	}
	else
	{
		size = CalcEstimatedHeight(containerSize);
	}

	auto min_height = style.GetMinHeight();
	if (min_height.IsDefined())
	{
		float h = ResolveUnits(min_height, containerSize.y);
		size = std::max(size, h);
		maxsize = std::max(maxsize, h);
	}

	auto max_height = style.GetMaxHeight();
	if (max_height.IsDefined())
	{
		maxsize = ResolveUnits(max_height, containerSize.y);
		size = std::min(size, maxsize);
	}

	return { size, maxsize };
}

Range<float> UIObject::GetFullEstimatedWidth(const Size<float>& containerSize)
{
	if (!_NeedsLayout())
		return { 0, FLT_MAX };
	auto style = GetStyle();
	auto s = GetEstimatedWidth(containerSize);
	auto box_sizing = style.GetBoxSizing();
	if (box_sizing == style::BoxSizing::ContentBox || !style.GetWidth().IsDefined())
	{
		s.min += ResolveUnits(style.GetPaddingLeft(), containerSize.x);
		s.min += ResolveUnits(style.GetPaddingRight(), containerSize.x);
	}
	s.min += ResolveUnits(style.GetMarginLeft(), containerSize.x);
	s.min += ResolveUnits(style.GetMarginRight(), containerSize.x);
	return s;
}

Range<float> UIObject::GetFullEstimatedHeight(const Size<float>& containerSize)
{
	if (!_NeedsLayout())
		return { 0, FLT_MAX };
	auto style = GetStyle();
	auto s = GetEstimatedHeight(containerSize);
	auto box_sizing = style.GetBoxSizing();
	if (box_sizing == style::BoxSizing::ContentBox || !style.GetHeight().IsDefined())
	{
		s.min += ResolveUnits(style.GetPaddingTop(), containerSize.y);
		s.min += ResolveUnits(style.GetPaddingBottom(), containerSize.y);
	}
	s.min += ResolveUnits(style.GetMarginTop(), containerSize.y);
	s.min += ResolveUnits(style.GetMarginBottom(), containerSize.y);
	return s;
}

void UIObject::PerformLayout(const UIRect& rect, const Size<float>& containerSize)
{
	if (_NeedsLayout())
		OnLayout(rect, containerSize);
}

void UIObject::OnLayout(const UIRect& rect, const Size<float>& containerSize)
{
	using namespace style;

	auto style = GetStyle();

	auto swidth = style::Coord::Undefined();
	auto sheight = style::Coord::Undefined();
	GetSize(swidth, sheight);

	auto width = style.GetWidth();
	if (width.unit == style::CoordTypeUnit::Fraction)
		width = rect.GetWidth();
	if (!width.IsDefined())
		width = swidth;

	auto height = style.GetHeight();
	if (height.unit == style::CoordTypeUnit::Fraction)
		height = rect.GetHeight();
	if (!height.IsDefined())
		height = sheight;

	auto min_width = style.GetMinWidth();
	auto min_height = style.GetMinHeight();
	auto max_width = style.GetMaxWidth();
	auto max_height = style.GetMaxHeight();
	auto box_sizing = style.GetBoxSizing();

	UIRect Mrect = GetMarginRect(style.block, rect.GetWidth());
	UIRect Prect = GetPaddingRect(style.block, rect.GetWidth());
	UIRect Arect =
	{
		Mrect.x0 + Prect.x0,
		Mrect.y0 + Prect.y0,
		Mrect.x1 + Prect.x1,
		Mrect.y1 + Prect.y1,
	};
	UIRect inrect =
	{
		rect.x0 + Arect.x0,
		rect.y0 + Arect.y0,
		rect.x1 - Arect.x1,
		rect.y1 - Arect.y1,
	};

	float scaleOriginX = inrect.x0;
	float scaleOriginY = inrect.y0;

	auto layout = style.GetLayout();
	if (layout == Layout::Undefined)
		layout = Layout::Stack;
	UIRect fcr = inrect;
	switch (layout)
	{
	case Layout::InlineBlock: {
		float p = inrect.x0;
		float y0 = inrect.y0;
		float maxH = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			float w = ch->GetFullEstimatedWidth(rect.GetSize()).min;
			float h = ch->GetFullEstimatedHeight(rect.GetSize()).min;
			ch->PerformLayout({ p, y0, p + w, y0 + h }, inrect.GetSize());
			p += w;
			maxH = std::max(maxH, h);
		}
		fcr = { inrect.x0, inrect.y0, p, y0 + maxH };
		break; }
	case Layout::Stack: {
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		// margins are collapsed
		auto dir = style.GetStackingDirection();
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::TopDown: {
			float p = inrect.y0;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float h = ch->GetFullEstimatedHeight(inrect.GetSize()).min;
				ch->PerformLayout({ inrect.x0, p, inrect.x1, p + h }, inrect.GetSize());
				p += h;
			}
			fcr = { inrect.x0, inrect.y0, inrect.x1, p };
			break; }
		case StackingDirection::RightToLeft: {
			float p = inrect.x1;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float w = ch->GetFullEstimatedWidth(inrect.GetSize()).min;
				ch->PerformLayout({ p - w, inrect.y0, p, inrect.y1 }, inrect.GetSize());
				p -= w;
			}
			fcr = { p, inrect.y0, inrect.x1, inrect.y1 };
			scaleOriginX = inrect.x1;
			break; }
		case StackingDirection::BottomUp: {
			float p = inrect.y1;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float h = ch->GetFullEstimatedHeight(inrect.GetSize()).min;
				ch->PerformLayout({ inrect.x0, p - h, inrect.x1, p }, inrect.GetSize());
				p -= h;
			}
			fcr = { inrect.x0, p, inrect.x1, inrect.y1 };
			scaleOriginY = inrect.y1;
			break; }
		case StackingDirection::LeftToRight: {
			float p = inrect.x0;
			float xw = 0;
			auto ha = style.GetHAlign();
			if (ha != style::HAlign::Undefined && ha != style::HAlign::Left)
			{
				float tw = 0;
				for (auto* ch = firstChild; ch; ch = ch->next)
					tw += ch->GetFullEstimatedWidth(inrect.GetSize()).min;
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
					auto cc = CountChildrenImmediate();
					if (cc > 1)
						xw += diff / (cc - 1);
					else
						p += diff / 2;
					break; }
				}
			}
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float w = ch->GetFullEstimatedWidth(inrect.GetSize()).min;
				ch->PerformLayout({ p, inrect.y0, p + w, inrect.y1 }, inrect.GetSize());
				p += w + xw;
			}
			fcr = { inrect.x0, inrect.y0, p - xw, inrect.y1 };
			break; }
		}
		break; }
	case Layout::StackExpand: {
		auto dir = style.GetStackingDirection();
		if (dir == StackingDirection::Undefined)
			dir = StackingDirection::TopDown;
		switch (dir)
		{
		case StackingDirection::LeftToRight: if (firstChild) {
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
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				auto s = ch->GetFullEstimatedWidth(inrect.GetSize());
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
			fcr = { inrect.x0, inrect.y0, std::max(inrect.x1, p), inrect.y1 };
			break; }
		}
		break; }
	case Layout::EdgeSlice: {
		auto subr = inrect;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			auto e = ch->GetStyle().GetEdge();
			if (e == Edge::Undefined)
				e = Edge::Top;
			float d;
			switch (e)
			{
			case Edge::Top:
				d = ch->GetFullEstimatedHeight(subr.GetSize()).min;
				ch->PerformLayout({ subr.x0, subr.y0, subr.x1, subr.y0 + d }, subr.GetSize());
				subr.y0 += d;
				break;
			case Edge::Bottom:
				d = ch->GetFullEstimatedHeight(subr.GetSize()).min;
				ch->PerformLayout({ subr.x0, subr.y1 - d, subr.x1, subr.y1 }, subr.GetSize());
				subr.y1 -= d;
				break;
			case Edge::Left:
				d = ch->GetFullEstimatedWidth(subr.GetSize()).min;
				ch->PerformLayout({ subr.x0, subr.y0, subr.x0 + d, subr.y1 }, subr.GetSize());
				subr.x0 += d;
				break;
			case Edge::Right:
				d = ch->GetFullEstimatedWidth(subr.GetSize()).min;
				ch->PerformLayout({ subr.x1 - d, subr.y0, subr.x1, subr.y1 }, subr.GetSize());
				subr.x1 -= d;
				break;
			}
		}
		break; }
	}

	if (width.IsDefined() || min_width.IsDefined() || max_width.IsDefined())
	{
		float orig = fcr.GetWidth();
		float tgt = width.IsDefined() ? ResolveUnits(width, containerSize.x) : orig;
		if (min_width.IsDefined())
			tgt = std::max(tgt, ResolveUnits(min_width, containerSize.x));
		if (max_width.IsDefined())
			tgt = std::min(tgt, ResolveUnits(max_width, containerSize.x));
		if (box_sizing != BoxSizing::ContentBox)
			tgt -= Prect.x0 + Prect.x1;
		if (tgt != orig)
		{
			if (orig)
			{
				float scale = tgt / orig;
				fcr.x0 = scaleOriginX + (fcr.x0 - scaleOriginX) * scale;
				fcr.x1 = scaleOriginX + (fcr.x1 - scaleOriginX) * scale;
			}
			else
				fcr.x1 += tgt;
		}
	}
	if (height.IsDefined() || min_height.IsDefined() || max_height.IsDefined())
	{
		float orig = fcr.GetHeight();
		float tgt = height.IsDefined() ? ResolveUnits(height, containerSize.y) : orig;
		if (min_height.IsDefined())
			tgt = std::max(tgt, ResolveUnits(min_height, containerSize.y));
		if (max_height.IsDefined())
			tgt = std::min(tgt, ResolveUnits(max_height, containerSize.y));
		if (box_sizing != BoxSizing::ContentBox)
			tgt -= Prect.y0 + Prect.y1;
		if (tgt != orig)
		{
			if (orig)
			{
				float scale = tgt / orig;
				fcr.y0 = scaleOriginY + (fcr.y0 - scaleOriginY) * scale;
				fcr.y1 = scaleOriginY + (fcr.y1 - scaleOriginY) * scale;
			}
			else
				fcr.y1 += tgt;
		}
	}

	finalRectC = fcr;
	finalRectCP = fcr.ExtendBy(Prect);
	finalRectCPB = finalRectCP; // no border yet
}

bool UIObject::IsChildOf(UIObject* obj) const
{
	for (auto* p = parent; p; p = p->parent)
		if (p == obj)
			return true;
	return false;
}

bool UIObject::IsChildOrSame(UIObject* obj) const
{
	return obj == this || IsChildOf(obj);
}

int UIObject::CountChildrenImmediate() const
{
	int o = 0;
	for (auto* ch = firstChild; ch; ch = ch->next)
		o += 1;
	return o;
}

int UIObject::CountChildrenRecursive() const
{
	int o = 0;
	for (auto* ch = firstChild; ch; ch = ch->next)
		o += 1 + ch->CountChildrenRecursive();
	return o;
}

void UIObject::RerenderNode()
{
	if (auto* n = FindParentOfType<ui::Node>())
		n->Rerender();
}

bool UIObject::IsHovered() const
{
	return !!(flags & UIObject_IsHovered);
}

bool UIObject::IsClicked(int button) const
{
	assert(button >= 0 && button < 5);
	return !!(flags & (_UIObject_IsClicked_First << button));
}

bool UIObject::IsFocused() const
{
	return this == system->eventSystem.focusObj;
}

bool UIObject::IsVisible() const
{
	return !(flags & UIObject_IsHidden);
}

void UIObject::SetVisible(bool v)
{
	if (v)
		flags &= ~UIObject_IsHidden;
	else
		flags |= UIObject_IsHidden;
}

bool UIObject::IsInputDisabled() const
{
	return !!(flags & UIObject_IsDisabled);
}

void UIObject::SetInputDisabled(bool v)
{
	if (v)
		flags |= UIObject_IsDisabled;
	else
		flags &= ~UIObject_IsDisabled;
}

style::Accessor UIObject::GetStyle()
{
	return style::Accessor(styleProps, true);
}

void UIObject::SetStyle(style::Block* style)
{
	styleProps = style;
}

float UIObject::ResolveUnits(style::Coord coord, float ref)
{
	using namespace style;

	switch (coord.unit)
	{
	case CoordTypeUnit::Pixels:
		return coord.value;
	case CoordTypeUnit::Percent:
		return coord.value * ref * 0.01f;
	case CoordTypeUnit::Fraction:
		// special unit that takes some amount of the remaining space relative to other items in the same container
		return coord.value * ref;
	default:
		return 0;
	}
}

UIRect UIObject::GetMarginRect(style::Block* style, float ref)
{
	return
	{
		ResolveUnits(style->margin_left, ref),
		ResolveUnits(style->margin_top, ref),
		ResolveUnits(style->margin_right, ref),
		ResolveUnits(style->margin_bottom, ref),
	};
}

UIRect UIObject::GetPaddingRect(style::Block* style, float ref)
{
	return
	{
		ResolveUnits(style->padding_left, ref),
		ResolveUnits(style->padding_top, ref),
		ResolveUnits(style->padding_right, ref),
		ResolveUnits(style->padding_bottom, ref),
	};
}

ui::NativeWindowBase* UIObject::GetNativeWindow() const
{
	return system->nativeWindow;
}


namespace ui {

struct SubscrTableKey
{
	bool operator == (const SubscrTableKey& o) const
	{
		return tag == o.tag && at == o.at;
	}

	DataCategoryTag* tag;
	uintptr_t at;
};

} // ui

namespace std {

template <>
struct hash<ui::SubscrTableKey>
{
	size_t operator () (const ui::SubscrTableKey& k) const
	{
		return (uintptr_t(k.tag) * 151) ^ k.at;
	}
};

} // std

namespace ui {

struct SubscrTableValue
{
	Subscription* _firstSub = nullptr;
	Subscription* _lastSub = nullptr;
};

using SubscrTable = std::unordered_map<SubscrTableKey, SubscrTableValue*>;
static SubscrTable* g_subscrTable;

void SubscriptionTable_Init()
{
	g_subscrTable = new SubscrTable;
}

void SubscriptionTable_Free()
{
	//assert(g_subscrTable->empty());
	for (auto& p : *g_subscrTable)
		delete p.second;
	delete g_subscrTable;
	g_subscrTable = nullptr;
}

struct Subscription
{
	void Link()
	{
		// append to front because removal starts from that side as well
		prevInNode = nullptr;
		nextInNode = node->_firstSub;
		if (nextInNode)
			nextInNode->prevInNode = this;
		else
			node->_firstSub = node->_lastSub = this;

		prevInTable = nullptr;
		nextInTable = tableEntry->_firstSub;
		if (nextInTable)
			nextInTable->prevInTable = this;
		else
			tableEntry->_firstSub = tableEntry->_lastSub = this;
	}
	void Unlink()
	{
		// node
		if (prevInNode)
			prevInNode->nextInNode = nextInNode;
		if (nextInNode)
			nextInNode->prevInNode = prevInNode;
		if (node->_firstSub == this)
			node->_firstSub = nextInNode;
		if (node->_lastSub == this)
			node->_lastSub = prevInNode;

		// table
		if (prevInTable)
			prevInTable->nextInTable = nextInTable;
		if (nextInTable)
			nextInTable->prevInTable = prevInTable;
		if (tableEntry->_firstSub == this)
			tableEntry->_firstSub = nextInTable;
		if (tableEntry->_lastSub == this)
			tableEntry->_lastSub = prevInTable;
	}

	Node* node;
	SubscrTableValue* tableEntry;
	DataCategoryTag* tag;
	uintptr_t at;
	Subscription* prevInNode;
	Subscription* nextInNode;
	Subscription* prevInTable;
	Subscription* nextInTable;
};

struct EventHandlerEntry
{
	EventHandlerEntry* next;
	UIObject* target;
	UIEventType type;
	EventFunc func;
};

static void _Notify(DataCategoryTag* tag, uintptr_t at)
{
	auto it = g_subscrTable->find({ tag, at });
	if (it != g_subscrTable->end())
	{
		for (auto* s = it->second->_firstSub; s; s = s->nextInTable)
			s->node->Rerender();
	}
}

void Notify(DataCategoryTag* tag, uintptr_t at)
{
	if (at != ANY_ITEM)
		_Notify(tag, at);
	_Notify(tag, ANY_ITEM);
}

Node::~Node()
{
	while (_firstSub)
	{
		auto* s = _firstSub;
		s->Unlink();
		delete s;
	}
	ClearEventHandlers();
}

void Node::OnEvent(UIEvent& e)
{
	for (auto* n = _firstEH; n && !e.handled; n = n->next)
	{
		if (n->type != UIEventType::Any && n->type != e.type)
			continue;
		if (n->target && !e.target->IsChildOrSame(n->target))
			continue;
		n->func(e);
	}
}

void Node::Rerender()
{
	system->container.AddToRenderStack(this);
}

bool Node::Subscribe(DataCategoryTag* tag, uintptr_t at)
{
	SubscrTableValue* lst;
	auto it = g_subscrTable->find({ tag, at });
	if (it != g_subscrTable->end())
	{
		lst = it->second;
		// TODO compare list sizes to decide which is the shorter one to iterate
		for (auto* s = _firstSub; s; s = s->nextInNode)
			if (s->tag == tag && s->at == at)
				return false;
		//for (auto* s = lst->_firstSub; s; s = s->nextInTable)
		//	if (s->node == this)
		//		return false;
	}
	else
		g_subscrTable->insert({ SubscrTableKey{ tag, at }, lst = new SubscrTableValue });

	auto* s = new Subscription;
	s->node = this;
	s->tableEntry = lst;
	s->tag = tag;
	s->at = at;
	s->Link();
	return true;
}

bool Node::Unsubscribe(DataCategoryTag* tag, uintptr_t at)
{
	auto it = g_subscrTable->find({ tag, at });
	if (it == g_subscrTable->end())
		return false;

	// TODO compare list sizes to decide which is the shorter one to iterate
	for (auto* s = _firstSub; s; s = s->nextInNode)
	{
		if (s->tag == tag && s->at == at)
		{
			s->Unlink();
			delete s;
		}
	}
	return true;
}

EventFunc& Node::HandleEvent(UIObject* target, UIEventType type)
{
	auto eh = new EventHandlerEntry;
	eh->next = _firstEH;
	eh->target = target;
	eh->type = type;
	_firstEH = eh;
	return eh->func;
}

void Node::ClearEventHandlers()
{
	while (_firstEH)
	{
		auto* n = _firstEH->next;
		delete _firstEH;
		_firstEH = n;
	}
}

} // ui
