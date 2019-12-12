
#include "Objects.h"
#include "System.h"


UIObject::~UIObject()
{
	system->eventSystem.OnDestroy(this);
	if (styleProps)
		delete styleProps;
}

float UIObject::CalcEstimatedWidth(float containerWidth, float containerHeight)
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
			size = std::max(size, ch->GetFullEstimatedWidth(containerWidth, containerHeight));
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
				size = std::max(size, ch->GetFullEstimatedWidth(containerWidth, containerHeight));
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size += ch->GetFullEstimatedWidth(containerWidth, containerHeight);
			break;
		}
		break; }
	}
	return size;
}

float UIObject::CalcEstimatedHeight(float containerWidth, float containerHeight)
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
			size = std::max(size, ch->GetFullEstimatedHeight(containerWidth, containerHeight));
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
				size += ch->GetFullEstimatedHeight(containerWidth, containerHeight);
			break;
		case style::StackingDirection::LeftToRight:
		case style::StackingDirection::RightToLeft:
			for (auto* ch = firstChild; ch; ch = ch->next)
				size = std::max(size, ch->GetFullEstimatedHeight(containerWidth, containerHeight));
			break;
		}
		break; }
	}
	return size;
}

float UIObject::GetEstimatedWidth(float containerWidth, float containerHeight)
{
	auto style = GetStyle();
	float size = 0;
	auto width = style.GetWidth();
	if (width.IsDefined())
	{
		size = ResolveUnits(width, containerWidth);
	}
	else
	{
		size = CalcEstimatedWidth(containerWidth, containerHeight);
	}
	auto min_width = style.GetMinWidth();
	if (min_width.IsDefined())
		size = std::max(size, ResolveUnits(min_width, containerWidth));
	auto max_width = style.GetMaxWidth();
	if (max_width.IsDefined())
		size = std::min(size, ResolveUnits(max_width, containerWidth));
	return size;
}

float UIObject::GetEstimatedHeight(float containerWidth, float containerHeight)
{
	auto style = GetStyle();
	float size = 0;
	auto height = style.GetHeight();
	if (height.IsDefined())
	{
		size = ResolveUnits(height, containerHeight);
	}
	else
	{
		size = CalcEstimatedHeight(containerWidth, containerHeight);
	}
	auto min_height = style.GetMinHeight();
	if (min_height.IsDefined())
		size = std::max(size, ResolveUnits(min_height, containerHeight));
	auto max_height = style.GetMaxHeight();
	if (max_height.IsDefined())
		size = std::min(size, ResolveUnits(max_height, containerHeight));
	return size;
}

float UIObject::GetFullEstimatedWidth(float containerWidth, float containerHeight)
{
	auto style = GetStyle();
	float size = GetEstimatedWidth(containerWidth, containerHeight);
	auto box_sizing = style.GetBoxSizing();
	if (box_sizing != style::BoxSizing::BorderBox || !style.GetWidth().IsDefined())
	{
		size += ResolveUnits(style.GetPaddingLeft(), containerWidth);
		size += ResolveUnits(style.GetPaddingRight(), containerWidth);
	}
	size += ResolveUnits(style.GetMarginLeft(), containerWidth);
	size += ResolveUnits(style.GetMarginRight(), containerWidth);
	return size;
}

float UIObject::GetFullEstimatedHeight(float containerWidth, float containerHeight)
{
	auto style = GetStyle();
	float size = GetEstimatedHeight(containerWidth, containerHeight);
	auto box_sizing = style.GetBoxSizing();
	if (box_sizing != style::BoxSizing::BorderBox || !style.GetHeight().IsDefined())
	{
		size += ResolveUnits(style.GetPaddingTop(), containerHeight);
		size += ResolveUnits(style.GetPaddingBottom(), containerHeight);
	}
	size += ResolveUnits(style.GetMarginTop(), containerHeight);
	size += ResolveUnits(style.GetMarginBottom(), containerHeight);
	return size;
}

void UIObject::OnLayout(const UIRect& rect)
{
	auto style = GetStyle();

	auto width = style.GetWidth();
	auto height = style.GetHeight();
	auto min_width = style.GetMinWidth();
	auto min_height = style.GetMinHeight();
	auto max_width = style.GetMaxWidth();
	auto max_height = style.GetMaxHeight();
	auto box_sizing = style.GetBoxSizing();

	UIRect Mrect =
	{
		ResolveUnits(style.GetMarginLeft(), rect.GetWidth()),
		ResolveUnits(style.GetMarginTop(), rect.GetWidth()),
		ResolveUnits(style.GetMarginRight(), rect.GetWidth()),
		ResolveUnits(style.GetMarginBottom(), rect.GetWidth()),
	};
	UIRect Prect =
	{
		ResolveUnits(style.GetPaddingLeft(), rect.GetWidth()),
		ResolveUnits(style.GetPaddingTop(), rect.GetWidth()),
		ResolveUnits(style.GetPaddingRight(), rect.GetWidth()),
		ResolveUnits(style.GetPaddingBottom(), rect.GetWidth()),
	};
	UIRect Arect =
	{
		Mrect.x0 + Prect.x0,
		Mrect.x1 + Prect.x1,
		Mrect.y0 + Prect.y0,
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
	if (layout == style::Layout::Undefined)
		layout = style::Layout::Stack;
	UIRect fcr = inrect;
	switch (layout)
	{
	case style::Layout::InlineBlock: {
		float p = inrect.x0;
		float y0 = inrect.y0;
		float maxH = 0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			float w = ch->GetFullEstimatedWidth(rect.GetWidth(), rect.GetHeight());
			float h = ch->GetFullEstimatedHeight(rect.GetWidth(), rect.GetHeight());
			ch->OnLayout({ p, y0, p + w, y0 + h });
			p += w;
			maxH = std::max(maxH, h);
		}
		fcr = { inrect.x0, inrect.y0, p, y0 + maxH };
		break; }
	case style::Layout::Stack: {
		// put items one after another in the indicated direction
		// container size adapts to child elements in stacking direction, and to parent in the other
		// margins are collapsed
		auto dir = style.GetStackingDirection();
		if (dir == style::StackingDirection::Undefined)
			dir = style::StackingDirection::TopDown;
		switch (dir)
		{
		case style::StackingDirection::TopDown: {
			float p = inrect.y0;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float h = ch->GetFullEstimatedHeight(inrect.GetWidth(), inrect.GetHeight());
				ch->OnLayout({ inrect.x0, p, inrect.x1, p + h });
				p += h;
			}
			fcr = { inrect.x0, inrect.y0, inrect.x1, p };
			break; }
		case style::StackingDirection::RightToLeft: {
			float p = inrect.x1;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float w = ch->GetFullEstimatedWidth(inrect.GetWidth(), inrect.GetHeight());
				ch->OnLayout({ p - w, inrect.y0, p, inrect.y1 });
				p -= w;
			}
			fcr = { p, inrect.y0, inrect.x1, inrect.y1 };
			scaleOriginX = inrect.x1;
			break; }
		case style::StackingDirection::BottomUp: {
			float p = inrect.y1;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float h = ch->GetFullEstimatedHeight(inrect.GetWidth(), inrect.GetHeight());
				ch->OnLayout({ inrect.x0, p - h, inrect.x1, p });
				p -= h;
			}
			fcr = { inrect.x0, p, inrect.x1, inrect.y1 };
			scaleOriginY = inrect.y1;
			break; }
		case style::StackingDirection::LeftToRight: {
			float p = inrect.x0;
			for (auto* ch = firstChild; ch; ch = ch->next)
			{
				float w = ch->GetFullEstimatedWidth(inrect.GetWidth(), inrect.GetHeight());
				ch->OnLayout({ p, inrect.y0, p + w, inrect.y1 });
				p += w;
			}
			fcr = { inrect.x0, inrect.y0, p, inrect.y1 };
			break; }
		}
		break; }
	}

	if (width.IsDefined() || min_width.IsDefined() || max_width.IsDefined())
	{
		float orig = fcr.GetWidth();
		float tgt = width.IsDefined() ? ResolveUnits(width, rect.GetWidth()) : orig;
		if (min_width.IsDefined())
			tgt = std::max(tgt, ResolveUnits(min_width, rect.GetWidth()));
		if (max_width.IsDefined())
			tgt = std::min(tgt, ResolveUnits(max_width, rect.GetWidth()));
		if (box_sizing == style::BoxSizing::BorderBox)
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
		float tgt = height.IsDefined() ? ResolveUnits(height, rect.GetHeight()) : orig;
		if (min_height.IsDefined())
			tgt = std::max(tgt, ResolveUnits(min_height, rect.GetHeight()));
		if (max_height.IsDefined())
			tgt = std::min(tgt, ResolveUnits(max_height, rect.GetHeight()));
		if (box_sizing == style::BoxSizing::BorderBox)
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

int UIObject::CountChildElements() const
{
	int o = 1;
	for (auto* ch = firstChild; ch; ch = ch->next)
		o += ch->CountChildElements();
	return o;
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
	return { this };
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
	default:
		return 0;
	}
}


void UINode::Rerender()
{
	system->container.AddToRenderStack(this);
}
