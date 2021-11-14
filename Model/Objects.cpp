
#include "../Core/HashTable.h"
#include "Objects.h"
#include "Native.h"
#include "System.h"
#include "Theme.h"


namespace ui {

extern uint32_t g_curLayoutFrame;
extern FrameContents* g_curSystem;


static HashMap<UIObject*, Overlays::Info> g_overlays;


struct EventHandlerEntry
{
	EventHandlerEntry* next;
	UIObject* target;
	EventType type;
	bool isLocal;
	EventFunc func;
};

UIObject::UIObject()
{
	SetStyle(Theme::current->object);
}

UIObject::~UIObject()
{
	ClearEventHandlers();
}

void UIObject::_AttachToFrameContents(FrameContents* owner)
{
	if (!system)
	{
		system = owner;

		OnEnable();
	}

	system->container.pendingDeactivationSet.RemoveIfFound(this);

	if (flags & UIObject_IsOverlay)
	{
		system->overlays.Register(this, g_overlays.find(this)->value.depth);
		g_overlays.erase(this);
	}

	// TODO verify possibly inconsistent duplicate flags of states (if flag set)
#if 0
	for (UIObject* p = system->eventSystem.hoverObj; p; p = p->parent)
		if (p == this)
			flags |= UIObject_IsHovered;
	for (int i = 0; i < 5; i++)
		for (UIObject* p = system->eventSystem.clickObj[i]; p; p = p->parent)
			if (p == this)
				flags |= _UIObject_IsClicked_First << i;
#endif

	if (flags & UIObject_NeedsTreeUpdates)
		OnEnterTree();

	flags |= UIObject_IsInTree;

	_OnChangeStyle();

	for (auto* ch = firstChild; ch; ch = ch->next)
		ch->_AttachToFrameContents(owner);
}

void UIObject::_DetachFromFrameContents()
{
	for (auto* ch = firstChild; ch; ch = ch->next)
		ch->_DetachFromFrameContents();

	OnDisable();

	system->container.layoutStack.OnDestroy(this);
	system->container.pendingDeactivationSet.RemoveIfFound(this);

	if (flags & UIObject_IsOverlay)
	{
		g_overlays.insert(this, system->overlays.mapped.find(this)->value);
		system->overlays.Unregister(this);
	}

	system = nullptr;
}

void UIObject::_DetachFromTree()
{
	if (!(flags & UIObject_IsInTree))
		return;

	for (auto* ch = firstChild; ch; ch = ch->next)
		ch->_DetachFromTree();

	if (flags & UIObject_NeedsTreeUpdates)
		OnExitTree();

	// remove from build/layout sets
	system->container.buildStack.OnDestroy(this);
	system->container.nextFrameBuildStack.OnDestroy(this);
	system->container.layoutStack.OnDestroy(this);
	flags &= ~(UIObject_IsInBuildStack | UIObject_IsInLayoutStack | UIObject_IsInTree);

	// add to deactivation set
	system->container.pendingDeactivationSet.InsertIfMissing(this);
}

void UIObject::PO_ResetConfiguration()
{
	ClearEventHandlers();

	// before detaching from frame to avoid a transfer
	UnregisterAsOverlay();

	DetachAll();

	SetStyle(Theme::current->object);

	auto origFlags = flags;
	const uint32_t KEEP_MASK =
		UIObject_IsHovered |
		UIObject_IsClickedAnyMask |
		UIObject_IsEdited |
		UIObject_IsChecked |
		UIObject_IsPressedAny |
		UIObject_NeedsTreeUpdates;
	flags = UIObject_DB__Defaults | (origFlags & KEEP_MASK);

	_InitReset();
}

void UIObject::PO_BeforeDelete()
{
	UnregisterAsOverlay();
	DetachAll();

	if (system)
		_DetachFromFrameContents();

	_livenessToken.SetAlive(false);
}

void UIObject::_InitReset()
{
	OnReset();
}

void UIObject::_DoEvent(Event& e)
{
	e.current = this;
	for (auto* n = _firstEH; n && !e.IsPropagationStopped(); n = n->next)
	{
		if (n->type != EventType::Any && n->type != e.type)
			continue;
		if (n->target && !e.target->IsChildOrSame(n->target))
			continue;
		n->func(e);
	}

	if (!e.IsPropagationStopped())
		OnEvent(e);

	if (!e.IsPropagationStopped())
	{
		// default behaviors
		_PerformDefaultBehaviors(e, flags);
	}
}

void UIObject::_PerformDefaultBehaviors(Event& e, uint32_t f)
{
	if (!IsInputDisabled())
	{
		if (HasFlags(f, UIObject_DB_IMEdit) && e.target == this && e.type == EventType::Activate)
		{
			flags |= UIObject_IsEdited;
			Rebuild();
		}

		if (HasFlags(f, UIObject_DB_RebuildOnChange) &&
			(e.type == EventType::Change || e.type == EventType::Commit))
		{
			Rebuild();
		}

		if (HasFlags(f, UIObject_DB_Button))
		{
			if (e.type == EventType::ButtonUp &&
				e.GetButton() == MouseButton::Left &&
				e.context->GetMouseCapture() == this &&
				HasFlags(UIObject_IsPressedMouse))
			{
				e.context->OnActivate(this);
			}
			if (e.type == EventType::MouseMove)
			{
				if (e.context->GetMouseCapture() == this) // TODO separate flag for this!
					SetFlag(UIObject_IsPressedMouse, finalRectCPB.Contains(e.position));
				//e.StopPropagation();
			}
			if (e.type == EventType::KeyAction && e.GetKeyAction() == KeyAction::ActivateDown)
			{
				e.context->CaptureMouse(this);
				flags |= UIObject_IsPressedOther;
				e.StopPropagation();
			}
			if (e.type == EventType::KeyAction && e.GetKeyAction() == KeyAction::ActivateUp)
			{
				if (flags & UIObject_IsPressedOther)
				{
					e.context->OnActivate(this);
					flags &= ~UIObject_IsPressedOther;
				}
				if (!(flags & UIObject_IsPressedMouse))
					e.context->ReleaseMouse();
				e.StopPropagation();
			}
		}

		if (HasFlags(f, UIObject_DB_Draggable))
		{
			if (e.context->DragCheck(e, MouseButton::Left))
			{
				e.context->SetKeyboardFocus(nullptr);
				Event e(&system->eventSystem, this, EventType::DragStart);
				e.context->BubblingEvent(e);
			}
		}

		if (HasFlags(f, UIObject_DB_Selectable))
		{
			if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
			{
				e.context->OnActivate(this);
				e.StopPropagation();
			}
		}

		if (HasFlags(f, UIObject_DB_CaptureMouseOnLeftClick))
		{
			if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
			{
				e.context->CaptureMouse(e.current);
				e.current->flags |= UIObject_IsPressedMouse;
				e.StopPropagation();
			}
			if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
			{
				if (e.context->GetMouseCapture() == this)
					e.context->ReleaseMouse();
				e.StopPropagation();
			}
			if (e.type == EventType::MouseCaptureChanged)
			{
				flags &= ~UIObject_IsPressedMouse;
				e.StopPropagation();
			}
		}
	}

	if (HasFlags(f, UIObject_DB_FocusOnLeftClick))
	{
		if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
		{
			e.context->SetKeyboardFocus(this);
			e.StopPropagation();
		}
	}
}

void UIObject::SendUserEvent(int id, uintptr_t arg0, uintptr_t arg1)
{
	system->eventSystem.OnUserEvent(this, id, arg0, arg1);
}

EventFunc& UIObject::HandleEvent(UIObject* target, EventType type)
{
	auto eh = new EventHandlerEntry;
	if (_lastEH)
		_lastEH->next = eh;
	else
		_firstEH = _lastEH = eh;
	eh->next = nullptr;
	eh->target = target;
	eh->type = type;
	eh->isLocal = system->container._curBuildable == this;
	_lastEH = eh;
	return eh->func;
}

void UIObject::ClearLocalEventHandlers()
{
	auto** eh = &_firstEH;
	EventHandlerEntry* lastEH = nullptr;
	while (*eh)
	{
		auto* n = (*eh)->next;
		if ((*eh)->isLocal)
		{
			delete *eh;
			*eh = n;
		}
		else
		{
			lastEH = *eh;
			eh = &(*eh)->next;
		}
	}
	_lastEH = lastEH;
}

void UIObject::ClearEventHandlers()
{
	while (_firstEH)
	{
		auto* n = _firstEH->next;
		delete _firstEH;
		_firstEH = n;
	}
	_lastEH = nullptr;
}

void UIObject::RegisterAsOverlay(float depth)
{
	if (system)
		system->overlays.Register(this, depth);
	else
		g_overlays.insert(this, { depth });
	flags |= UIObject_IsOverlay;
}

void UIObject::UnregisterAsOverlay()
{
	if (flags & UIObject_IsOverlay)
	{
		if (system)
			system->overlays.Unregister(this);
		else
			g_overlays.erase(this);
		flags &= ~UIObject_IsOverlay;
	}
}

struct VertexShifter
{
	draw::VertexTransformCallback prev;
	Vec2f offset;
};

void UIObject::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = styleProps->background_painter->Paint(this);
	PaintChildren(ctx, cpa);
}

void UIObject::Paint(const UIPaintContext& ctx)
{
	if (!_CanPaint())
		return;

	if (!((flags & UIObject_DisableCulling) || draw::GetCurrentScissorRectF().Overlaps(finalRectCPB)))
		return;

	OnPaint(ctx);
}

void UIObject::PaintChildren(const UIPaintContext& ctx, const ContentPaintAdvice& cpa)
{
	if (!firstChild)
		return;

	bool clipChildren = !!(flags & UIObject_ClipChildren);
	if (clipChildren)
		draw::PushScissorRect(finalRectC.Cast<int>());

	UIPaintContext subctx(DoNotInitialize{});
	auto* pctx = &ctx;
	if (flags & UIObject_SetsChildTextStyle)
	{
		subctx = UIPaintContext(ctx, cpa.textColor);
		pctx = &subctx;
	}

	if (cpa.offset != Vec2f())
	{
		VertexShifter vsh;
		vsh.offset = cpa.offset;

		draw::VertexTransformFunction* vtf = [](void* userdata, Vertex* vertices, size_t count)
		{
			auto* data = static_cast<VertexShifter*>(userdata);
			for (auto* v = vertices, *vend = vertices + count; v != vend; v++)
			{
				v->x += data->offset.x;
				v->y += data->offset.y;
			}

			data->prev.Call(vertices, count);
		};
		vsh.prev = draw::SetVertexTransformCallback({ &vsh, vtf });

		for (auto* ch = firstChild; ch; ch = ch->next)
			ch->Paint(*pctx);

		draw::SetVertexTransformCallback(vsh.prev);
	}
	else
	{
		for (auto* ch = firstChild; ch; ch = ch->next)
			ch->Paint(*pctx);
	}

	if (clipChildren)
		draw::PopScissorRect();
}

float UIObject::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	auto layout = GetStyle().GetLayout();
	if (layout == nullptr)
		layout = layouts::Stack();
	return layout->CalcEstimatedWidth(this, containerSize, type);
}

float UIObject::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float size = 0;
	auto layout = GetStyle().GetLayout();
	if (layout == nullptr)
		layout = layouts::Stack();
	return layout->CalcEstimatedHeight(this, containerSize, type);
}

Rangef UIObject::GetEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	auto style = GetStyle();
	float size = 0;

	auto width = style.GetWidth();
	if (!width.IsDefined())
	{
		auto height = Coord::Undefined();
		GetSize(width, height);
	}

	float maxsize = FLT_MAX;
	if (width.IsDefined())
	{
		size = maxsize = ResolveUnits(width, containerSize.x);
	}
	else
	{
		size = CalcEstimatedWidth(containerSize, type);
	}

	auto min_width = style.GetMinWidth();
	if (min_width.IsDefined())
	{
		float w = ResolveUnits(min_width, containerSize.x);
		size = max(size, w);
		maxsize = max(maxsize, w);
	}

	auto max_width = style.GetMaxWidth();
	if (max_width.IsDefined())
	{
		maxsize = ResolveUnits(max_width, containerSize.x);
		size = min(size, maxsize);
	}

	return { size, maxsize };
}

Rangef UIObject::GetEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	auto style = GetStyle();
	float size = 0;

	auto height = style.GetHeight();
	if (!height.IsDefined())
	{
		auto width = Coord::Undefined();
		GetSize(width, height);
	}

	float maxsize = FLT_MAX;
	if (height.IsDefined())
	{
		size = maxsize = ResolveUnits(height, containerSize.y);
	}
	else
	{
		size = CalcEstimatedHeight(containerSize, type);
	}

	auto min_height = style.GetMinHeight();
	if (min_height.IsDefined())
	{
		float h = ResolveUnits(min_height, containerSize.y);
		size = max(size, h);
		maxsize = max(maxsize, h);
	}

	auto max_height = style.GetMaxHeight();
	if (max_height.IsDefined())
	{
		maxsize = ResolveUnits(max_height, containerSize.y);
		size = min(size, maxsize);
	}

	return { size, maxsize };
}

Rangef UIObject::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	if (!(forParentLayout ? _IsPartOfParentLayout() : _NeedsLayout()))
		return { 0, FLT_MAX };
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;
	auto style = GetStyle();
	auto s = GetEstimatedWidth(containerSize, type);
	auto box_sizing = style.GetBoxSizing();
	auto width = style.GetWidth();
	if (!width.IsDefined())
	{
		auto height = Coord::Undefined();
		GetSize(width, height);
	}

	float w_add = ResolveUnits(style.GetMarginLeft(), containerSize.x) + ResolveUnits(style.GetMarginRight(), containerSize.x);
	if (box_sizing == BoxSizing::ContentBox || !width.IsDefined())
	{
		w_add += ResolveUnits(style.GetPaddingLeft(), containerSize.x) + ResolveUnits(style.GetPaddingRight(), containerSize.x);
	}
	s.min += w_add;
	if (s.max < FLT_MAX)
		s.max += w_add;

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = s;
	return s;
}

Rangef UIObject::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	if (!(forParentLayout ? _IsPartOfParentLayout() : _NeedsLayout()))
		return { 0, FLT_MAX };
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;
	auto style = GetStyle();
	auto s = GetEstimatedHeight(containerSize, type);
	auto box_sizing = style.GetBoxSizing();
	auto height = style.GetHeight();
	if (!height.IsDefined())
	{
		auto width = Coord::Undefined();
		GetSize(width, height);
	}

	float h_add = ResolveUnits(style.GetMarginTop(), containerSize.y) + ResolveUnits(style.GetMarginBottom(), containerSize.y);
	if (box_sizing == BoxSizing::ContentBox || !height.IsDefined())
	{
		h_add += ResolveUnits(style.GetPaddingTop(), containerSize.y) + ResolveUnits(style.GetPaddingBottom(), containerSize.y);
	}
	s.min += h_add;
	if (s.max < FLT_MAX)
		s.max += h_add;

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = s;
	return s;
}

void UIObject::PerformLayout(const UIRect& rect, const Size2f& containerSize)
{
	if (_IsPartOfParentLayout())
	{
		OnLayout(rect, containerSize);
		OnLayoutChanged();
	}
}

void UIObject::_PerformPlacement(const UIRect& rect, const Size2f& containerSize)
{
	if (_NeedsLayout() && GetStyle().GetPlacement() && !GetStyle().GetPlacement()->applyOnLayout)
	{
		OnLayout(rect, containerSize);
		OnLayoutChanged();
	}
}

void UIObject::OnLayout(const UIRect& inRect, const Size2f& containerSize)
{
	lastLayoutInputRect = inRect;
	lastLayoutInputCSize = containerSize;

	auto style = GetStyle();

	auto swidth = Coord::Undefined();
	auto sheight = Coord::Undefined();
	GetSize(swidth, sheight);

	auto placement = style.GetPlacement();
	UIRect rect = inRect;
	if (placement)
	{
		if (!placement->applyOnLayout)
		{
			if (parent && !placement->fullScreenRelative)
				rect = parent->GetContentRect();
			else
				rect = { 0, 0, system->eventSystem.width, system->eventSystem.height };
		}
		placement->OnApplyPlacement(this, rect);
	}

	auto width = style.GetWidth();
	if (width.unit == CoordTypeUnit::Fraction)
		width = rect.GetWidth();
	if (!width.IsDefined())
		width = swidth;

	auto height = style.GetHeight();
	if (height.unit == CoordTypeUnit::Fraction)
		height = rect.GetHeight();
	if (!height.IsDefined())
		height = sheight;

	auto min_width = style.GetMinWidth();
	auto min_height = style.GetMinHeight();
	auto max_width = style.GetMaxWidth();
	auto max_height = style.GetMaxHeight();
	auto box_sizing = style.GetBoxSizing();

	UIRect Mrect = GetMarginRect(style.block, rect.GetWidth());
	UIRect Prect = CalcPaddingRect(rect);
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

	auto layout = style.GetLayout();
	if (layout == nullptr)
		layout = layouts::Stack();
	LayoutState state = { inrect, { inrect.x0, inrect.y0 } };
	layout->OnLayout(this, inrect, state);

	if (placement)
		state.finalContentRect = inrect;

	if (width.IsDefined() || min_width.IsDefined() || max_width.IsDefined())
	{
		float orig = state.finalContentRect.GetWidth();
		float tgt = width.IsDefined() ? ResolveUnits(width, containerSize.x) : orig;
		if (min_width.IsDefined())
			tgt = max(tgt, ResolveUnits(min_width, containerSize.x));
		if (max_width.IsDefined())
			tgt = min(tgt, ResolveUnits(max_width, containerSize.x));
		if (box_sizing != BoxSizing::ContentBox)
			tgt -= Prect.x0 + Prect.x1;
		if (tgt != orig)
		{
			if (orig)
			{
				float scale = tgt / orig;
				state.finalContentRect.x0 = state.scaleOrigin.x + (state.finalContentRect.x0 - state.scaleOrigin.x) * scale;
				state.finalContentRect.x1 = state.scaleOrigin.x + (state.finalContentRect.x1 - state.scaleOrigin.x) * scale;
			}
			else
				state.finalContentRect.x1 += tgt;
		}
	}
	if (height.IsDefined() || min_height.IsDefined() || max_height.IsDefined())
	{
		float orig = state.finalContentRect.GetHeight();
		float tgt = height.IsDefined() ? ResolveUnits(height, containerSize.y) : orig;
		if (min_height.IsDefined())
			tgt = max(tgt, ResolveUnits(min_height, containerSize.y));
		if (max_height.IsDefined())
			tgt = min(tgt, ResolveUnits(max_height, containerSize.y));
		if (box_sizing != BoxSizing::ContentBox)
			tgt -= Prect.y0 + Prect.y1;
		if (tgt != orig)
		{
			if (orig)
			{
				float scale = tgt / orig;
				state.finalContentRect.y0 = state.scaleOrigin.y + (state.finalContentRect.y0 - state.scaleOrigin.y) * scale;
				state.finalContentRect.y1 = state.scaleOrigin.y + (state.finalContentRect.y1 - state.scaleOrigin.y) * scale;
			}
			else
				state.finalContentRect.y1 += tgt;
		}
	}

	finalRectC = state.finalContentRect;
	finalRectCP = state.finalContentRect.ExtendBy(Prect);
	finalRectCPB = finalRectCP; // no border yet

	for (UIObject* ch = firstChild; ch; ch = ch->next)
		ch->_PerformPlacement(finalRectC, finalRectC.GetSize());
}

UIRect UIObject::CalcPaddingRect(const UIRect& expTgtRect)
{
	return GetPaddingRect(styleProps, expTgtRect.GetWidth());
}

void UIObject::SetFlag(UIObjectFlags flag, bool set)
{
	if (set)
		flags |= flag;
	else
		flags &= ~flag;
}


void UIObject::DetachAll()
{
	DetachChildren();
	DetachParent();
}

void UIObject::DetachParent()
{
	if (!parent)
		return;

	if (system)
		_DetachFromTree();

	if (prev)
		prev->next = next;
	else
		parent->firstChild = next;

	if (next)
		next->prev = prev;
	else
		parent->lastChild = prev;

	parent = nullptr;
	prev = nullptr;
	next = nullptr;
}

void UIObject::DetachChildren(bool recursive)
{
	UIObject* next;
	for (auto* ch = firstChild; ch; ch = next)
	{
		next = ch->next;

		if (recursive)
			ch->DetachChildren(true);

		// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
		if (system)
			ch->_DetachFromTree();

		ch->parent = nullptr;
		ch->prev = nullptr;
		ch->next = nullptr;
		if (next)
			next->prev = nullptr;
		firstChild = next;
	}
	firstChild = nullptr;
	lastChild = nullptr;
}

void UIObject::InsertPrevious(UIObject* obj)
{
	if (prev == obj)
		return;

	obj->DetachParent();
	obj->prev = prev;
	obj->next = this;
	obj->parent = parent;

	auto* origPrev = prev;
	prev = obj;
	if (origPrev)
		origPrev->next = obj;
	else
		parent->firstChild = obj;

	if (system)
		obj->_AttachToFrameContents(system);
}

void UIObject::InsertNext(UIObject* obj)
{
	if (next == obj)
		return;

	obj->DetachParent();
	obj->prev = this;
	obj->next = next;
	obj->parent = parent;

	auto* origNext = next;
	next = obj;
	if (origNext)
		origNext->prev = obj;
	else
		parent->lastChild = obj;

	if (system)
		obj->_AttachToFrameContents(system);
}

void UIObject::PrependChild(UIObject* obj)
{
	if (firstChild == obj)
		return;

	if (firstChild)
		firstChild->InsertPrevious(obj);
	else
		_AddFirstChild(obj);
}

void UIObject::AppendChild(UIObject* obj)
{
	if (lastChild == obj)
		return;

	if (lastChild)
		lastChild->InsertNext(obj);
	else
		_AddFirstChild(obj);
}

void UIObject::_AddFirstChild(UIObject* obj)
{
	obj->DetachParent();

	firstChild = obj;
	lastChild = obj;
	obj->parent = this;

	if (system)
		obj->_AttachToFrameContents(system);
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

UIObject* UIObject::GetPrevInOrder()
{
	if (UIObject* ret = prev)
	{
		while (ret->lastChild)
			ret = ret->lastChild;
		return ret;
	}
	return parent;
}

UIObject* UIObject::GetNextInOrder()
{
	if (firstChild)
		return firstChild;
	for (UIObject* it = this; it; it = it->parent)
	{
		if (it->next)
			return it->next;
	}
	return nullptr;
}

UIObject* UIObject::GetFirstInOrder()
{
	return this;
}

UIObject* UIObject::GetLastInOrder()
{
	UIObject* ret = this;
	while (ret->lastChild)
		ret = ret->lastChild;
	return ret;
}

void UIObject::Rebuild()
{
	if (auto* n = FindParentOfType<Buildable>())
		n->Rebuild();
}

void UIObject::RebuildContainer()
{
	if (auto* n = (parent ? parent : this)->FindParentOfType<Buildable>())
		n->Rebuild();
}

void UIObject::_OnIMChange()
{
	system->eventSystem.OnIMChange(this);
	RebuildContainer();
}

bool UIObject::IsHovered() const
{
	return !!(flags & UIObject_IsHovered);
}

bool UIObject::IsPressed() const
{
	return !!(flags & UIObject_IsPressedAny);
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

StyleAccessor UIObject::GetStyle()
{
	return StyleAccessor(styleProps, this);
}

void UIObject::SetStyle(StyleBlock* style)
{
	styleProps = style;
	_OnChangeStyle();
}

void UIObject::_OnChangeStyle()
{
	if (system && (flags & UIObject_IsInTree))
		system->container.layoutStack.Add(parent ? parent : this);
}

float UIObject::ResolveUnits(Coord coord, float ref)
{
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

UIRect UIObject::GetMarginRect(StyleBlock* style, float ref)
{
	return
	{
		ResolveUnits(style->margin_left, ref),
		ResolveUnits(style->margin_top, ref),
		ResolveUnits(style->margin_right, ref),
		ResolveUnits(style->margin_bottom, ref),
	};
}

UIRect UIObject::GetPaddingRect(StyleBlock* style, float ref)
{
	return
	{
		ResolveUnits(style->padding_left, ref),
		ResolveUnits(style->padding_top, ref),
		ResolveUnits(style->padding_right, ref),
		ResolveUnits(style->padding_bottom, ref),
	};
}

StyleBlock* UIObject::_FindClosestParentTextStyle() const
{
	for (auto* p = parent; p; p = p->parent)
	{
		if ((p->flags & UIObject_SetsChildTextStyle) && p->styleProps)
			return p->styleProps;
	}
	return Theme::current->text;
}

FontInfo UIObject::GetFontInfo(StyleBlock* textStyle)
{
	int size = textStyle->font_size.unit == CoordTypeUnit::Pixels ? textStyle->font_size.value : GetFontHeight();
	short weight = short(textStyle->font_weight > FontWeight::Undefined ? textStyle->font_weight : FontWeight::Normal);
	bool italic = textStyle->font_style == FontStyle::Italic;

	return { FONT_FAMILY_SANS_SERIF, size, weight, italic };
}

NativeWindowBase* UIObject::GetNativeWindow() const
{
	return system->nativeWindow;
}


void TextElement::OnReset()
{
	UIElement::OnReset();

	styleProps = Theme::current->text;

	text = {};
}

void TextElement::GetSize(Coord& outWidth, Coord& outHeight)
{
	// TODO can we nullify styleProps?
	auto* style = styleProps != Theme::current->text ? static_cast<StyleBlock*>(styleProps) : _FindClosestParentTextStyle();
	auto info = GetFontInfo(style);

	auto font = GetFont(info.nameOrFamily, info.weight, info.italic);

	outWidth = ceilf(GetTextWidth(font, info.size, text));
	outHeight = info.size;
}

void TextElement::OnPaint(const UIPaintContext& ctx)
{
	// TODO can we nullify styleProps?
	auto* style = styleProps != Theme::current->text ? static_cast<StyleBlock*>(styleProps) : _FindClosestParentTextStyle();
	auto info = GetFontInfo(style);
	Color4b color = style->text_color.color;

	auto font = GetFont(info.nameOrFamily, info.weight, info.italic);

	UIPaintHelper ph;
	ph.PaintBackground(this);

	auto r = GetContentRect();
	float w = r.x1 - r.x0;
	draw::TextLine(font, info.size, r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text, color);

	ph.PaintChildren(this, ctx);
}

void Placeholder::OnPaint(const UIPaintContext& ctx)
{
	draw::RectCol(finalRectCPB.x0, finalRectCPB.y0, finalRectCPB.x1, finalRectCPB.y1, Color4b(0, 127));
	draw::RectCutoutCol(finalRectCPB, finalRectCPB.ShrinkBy(UIRect::UniformBorder(1)), Color4b(255, 127));

	char text[32];
	snprintf(text, sizeof(text), "%gx%g", finalRectCPB.GetWidth(), finalRectCPB.GetHeight());

	auto info = GetFontInfo(styleProps);
	Color4b color = styleProps->text_color.color;

	auto font = GetFont(info.nameOrFamily, info.weight, info.italic);

	UIPaintHelper ph;
	ph.PaintBackground(this);

	auto r = GetContentRect();
	float w = r.x1 - r.x0;
	float tw = GetTextWidth(font, info.size, text);
	draw::TextLine(font, info.size, r.x0 + w * 0.5f - tw * 0.5f, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text, color);

	ph.PaintChildren(this, ctx);
}


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

using SubscrTable = HashMap<SubscrTableKey, SubscrTableValue*>;
static SubscrTable* g_subscrTable;

void SubscriptionTable_Init()
{
	g_subscrTable = new SubscrTable;
}

void SubscriptionTable_Free()
{
	//assert(g_subscrTable->empty());
	for (auto& p : *g_subscrTable)
		delete p.value;
	delete g_subscrTable;
	g_subscrTable = nullptr;
}

struct Subscription
{
	void Link()
	{
		// append to front because removal starts from that side as well
		prevInBuildable = nullptr;
		nextInBuildable = buildable->_firstSub;
		if (nextInBuildable)
			nextInBuildable->prevInBuildable = this;
		else
			buildable->_lastSub = this;
		buildable->_firstSub = this;

		prevInTable = nullptr;
		nextInTable = tableEntry->_firstSub;
		if (nextInTable)
			nextInTable->prevInTable = this;
		else
			tableEntry->_lastSub = this;
		tableEntry->_firstSub = this;
	}
	void Unlink()
	{
		// buildable
		if (prevInBuildable)
			prevInBuildable->nextInBuildable = nextInBuildable;
		if (nextInBuildable)
			nextInBuildable->prevInBuildable = prevInBuildable;
		if (buildable->_firstSub == this)
			buildable->_firstSub = nextInBuildable;
		if (buildable->_lastSub == this)
			buildable->_lastSub = prevInBuildable;

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

	Buildable* buildable;
	SubscrTableValue* tableEntry;
	DataCategoryTag* tag;
	uintptr_t at;
	Subscription* prevInBuildable;
	Subscription* nextInBuildable;
	Subscription* prevInTable;
	Subscription* nextInTable;
};

static void _Notify(DataCategoryTag* tag, uintptr_t at)
{
	auto it = g_subscrTable->find({ tag, at });
	if (it.is_valid())
	{
		for (auto* s = it->value->_firstSub; s; s = s->nextInTable)
			s->buildable->OnNotify(tag, at);
	}
}

void Notify(DataCategoryTag* tag, uintptr_t at)
{
	if (at != ANY_ITEM)
		_Notify(tag, at);
	_Notify(tag, ANY_ITEM);
}

Buildable::~Buildable()
{
	while (_firstSub)
	{
		auto* s = _firstSub;
		s->Unlink();
		delete s;
	}
	while (_deferredDestructors.size())
	{
		_deferredDestructors.back()();
		_deferredDestructors.pop_back();
	}
}

void Buildable::PO_ResetConfiguration()
{
	decltype(_deferredDestructors) ddList;
	std::swap(_deferredDestructors, ddList);

	UIObject::PO_ResetConfiguration();

	std::swap(_deferredDestructors, ddList);
}

void Buildable::Rebuild()
{
	system->container.AddToBuildStack(this);
	GetNativeWindow()->InvalidateAll();
}

void Buildable::OnNotify(DataCategoryTag* /*tag*/, uintptr_t /*at*/)
{
	Rebuild();
}

bool Buildable::Subscribe(DataCategoryTag* tag, uintptr_t at)
{
	SubscrTableValue* lst;
	auto it = g_subscrTable->find({ tag, at });
	if (it.is_valid())
	{
		lst = it->value;
		// TODO compare list sizes to decide which is the shorter one to iterate
		for (auto* s = _firstSub; s; s = s->nextInBuildable)
			if (s->tag == tag && s->at == at)
				return false;
		//for (auto* s = lst->_firstSub; s; s = s->nextInTable)
		//	if (s->node == this)
		//		return false;
	}
	else
		g_subscrTable->insert(SubscrTableKey{ tag, at }, lst = new SubscrTableValue);

	auto* s = new Subscription;
	s->buildable = this;
	s->tableEntry = lst;
	s->tag = tag;
	s->at = at;
	s->Link();
	return true;
}

bool Buildable::Unsubscribe(DataCategoryTag* tag, uintptr_t at)
{
	auto it = g_subscrTable->find({ tag, at });
	if (it.is_valid())
		return false;

	// TODO compare list sizes to decide which is the shorter one to iterate
	for (auto* s = _firstSub; s; s = s->nextInBuildable)
	{
		if (s->tag == tag && s->at == at)
		{
			s->Unlink();
			delete s;
		}
	}
	return true;
}


AddTooltip::AddTooltip(const std::string& s)
{
	_evfn = [s]()
	{
		Text(s);
	};
}

void AddTooltip::Apply(UIObject* obj) const
{
	auto fn = _evfn;
	obj->HandleEvent(EventType::Tooltip) = [fn{ std::move(fn) }](Event& e)
	{
		Tooltip::Set(fn);
		e.StopPropagation();
	};
}

} // ui
