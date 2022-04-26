
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

void UIObject::SlotIterator_Init(UIObjectIteratorData& data)
{
	data.data0 = uintptr_t(firstChild);
}

UIObject* UIObject::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	UIObject* ret = reinterpret_cast<UIObject*>(data.data0);
	if (ret)
		data.data0 = uintptr_t(ret->next);
	return ret;
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
					SetFlag(UIObject_IsPressedMouse, GetFinalRect().Contains(e.position));
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
	eh->isLocal = system && system->container._curBuildable == this;
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

void UIObject::OnPaint(const UIPaintContext& ctx)
{
	for (auto* ch = firstChild; ch; ch = ch->next)
		ch->Paint(ctx);
}

void UIObject::Paint(const UIPaintContext& ctx)
{
	if (!_CanPaint())
		return;

	if (!((flags & UIObject_DisableCulling) || draw::GetCurrentScissorRectF().Overlaps(GetFinalRect())))
		return;

	OnPaint(ctx);
}

static void PaintBacktracker(UIObject* cur, SingleChildPaintPtr* next, const UIPaintContext& ctx)
{
	if (cur->parent)
	{
		SingleChildPaintPtr entry = { next, cur };
		PaintBacktracker(cur->parent, &entry, ctx);
	}
	else
	{
		cur->OnPaintSingleChild(next, ctx);
	}
}

void UIObject::RootPaint()
{
	UIPaintContext pc;
	if (!parent)
		OnPaint(pc);
	else
		PaintBacktracker(this, nullptr, pc);
}

void UIObject::OnPaintSingleChild(SingleChildPaintPtr* next, const UIPaintContext& ctx)
{
	if (!next)
		OnPaint(ctx);
	else
		next->child->OnPaintSingleChild(next->next, ctx);
}

Rangef UIObject::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(layouts::Stack()->CalcEstimatedWidth(this, containerSize, type));
}

Rangef UIObject::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(layouts::Stack()->CalcEstimatedHeight(this, containerSize, type));
}

Rangef UIObject::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (!_NeedsLayout())
		return Rangef::AtLeast(0);
	if (g_curLayoutFrame == _cacheFrameWidth)
		return _cacheValueWidth;

	Rangef r = CalcEstimatedWidth(containerSize, type);

	_cacheFrameWidth = g_curLayoutFrame;
	_cacheValueWidth = r;
	return r;
}

Rangef UIObject::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (!_NeedsLayout())
		return Rangef::AtLeast(0);
	if (g_curLayoutFrame == _cacheFrameHeight)
		return _cacheValueHeight;

	Rangef r = CalcEstimatedHeight(containerSize, type);

	_cacheFrameHeight = g_curLayoutFrame;
	_cacheValueHeight = r;
	return r;
}

void UIObject::PerformLayout(const UIRect& rect)
{
	if (_NeedsLayout())
	{
		OnLayout(rect);
		OnLayoutChanged();
	}
}

void UIObject::OnLayout(const UIRect& inRect)
{
	UIRect rect = inRect;
	layouts::Stack()->OnLayout(this, rect);
	_finalRect = rect;
}

void UIObject::RedoLayout()
{
	UIRect r = _finalRect;
	PerformLayout(r);
}

UIObject* UIObject::FindLastChildContainingPos(Point2f pos) const
{
	for (auto* ch = lastChild; ch; ch = ch->prev)
	{
		if (ch->Contains(pos))
			return ch;
	}
	return nullptr;
}

void UIObject::SetFlag(UIObjectFlags flag, bool set)
{
	if (set)
		flags |= flag;
	else
		flags &= ~flag;
}


void UIObject::RemoveChildImpl(UIObject* ch)
{
	if (ch->prev)
		ch->prev->next = ch->next;
	else
		firstChild = ch->next;

	if (ch->next)
		ch->next->prev = ch->prev;
	else
		lastChild = ch->prev;

	ch->prev = nullptr;
	ch->next = nullptr;
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

	parent->RemoveChildImpl(this);

	parent = nullptr;
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

void UIObject::AppendChild(UIObject* obj)
{
	if (lastChild == obj)
		return;

	if (lastChild)
		lastChild->InsertNext(obj);
	else
		_AddFirstChild(obj);
}

void UIObject::CustomAppendChild(UIObject* obj)
{
	AppendChild(obj);
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
	return system && this == system->eventSystem.focusObj;
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
	_OnChangeStyle();
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

const FontSettings* UIObject::_FindClosestParentFontSettings() const
{
	for (auto* p = parent; p; p = p->parent)
	{
		if (p->flags & UIObject_SetsChildTextStyle)
			if (auto* fs = p->_GetFontSettings())
				return fs;
	}
	return GetCurrentTheme()->FindStructByName<FontSettings>("");
}

const FontSettings* UIObject::_GetFontSettings() const
{
	return GetCurrentTheme()->FindStructByName<FontSettings>("");
}

NativeWindowBase* UIObject::GetNativeWindow() const
{
	return system->nativeWindow;
}


void UIObjectNoChildren::CustomAppendChild(UIObject* obj)
{
	puts("WARNING: trying to add child to a no-children UI object");
}


void UIObjectSingleChild::OnReset()
{
	UIObject::OnReset();

	_child = nullptr;
}

void UIObjectSingleChild::SlotIterator_Init(UIObjectIteratorData& data)
{
	data.data0 = 0;
}

UIObject* UIObjectSingleChild::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	if (data.data0 > 0)
		return nullptr;
	data.data0 = 1;
	return _child;
}

void UIObjectSingleChild::RemoveChildImpl(UIObject* ch)
{
	if (_child == ch)
		_child = nullptr;
}

void UIObjectSingleChild::DetachChildren(bool recursive)
{
	if (_child)
	{
		if (recursive)
			_child->DetachChildren(true);

		if (system)
			_child->_DetachFromTree();

		_child->parent = nullptr;
		_child = nullptr;
	}
}

void UIObjectSingleChild::CustomAppendChild(UIObject* obj)
{
	if (_child)
	{
		puts("WARNING: child already added to a single-child element!");
		return;
	}

	obj->DetachParent();

	obj->parent = this;
	_child = obj;

	if (system)
		obj->_AttachToFrameContents(system);
}

void UIObjectSingleChild::OnPaint(const UIPaintContext& ctx)
{
	if (_child)
		_child->Paint(ctx);
}

UIObject* UIObjectSingleChild::FindLastChildContainingPos(Point2f pos) const
{
	if (_child && _child->Contains(pos))
		return _child;
	return nullptr;
}

void UIObjectSingleChild::_AttachToFrameContents(FrameContents* owner)
{
	UIObject::_AttachToFrameContents(owner);

	if (_child)
		_child->_AttachToFrameContents(owner);
}

void UIObjectSingleChild::_DetachFromFrameContents()
{
	if (_child)
		_child->_DetachFromFrameContents();

	UIObject::_DetachFromFrameContents();
}


Rangef WrapperElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (!_child)
		return Rangef::AtLeast(0);
	return _child->GetFullEstimatedWidth(containerSize, type);
}

Rangef WrapperElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (!_child)
		return Rangef::AtLeast(0);
	return _child->GetFullEstimatedHeight(containerSize, type);
}

void WrapperElement::OnLayout(const UIRect& rect)
{
	if (_child)
	{
		_child->PerformLayout(rect);
		_finalRect = _child->GetFinalRect();
	}
	else _finalRect = rect;
}


void SizeConstraintElement::OnReset()
{
	WrapperElement::OnReset();
	widthRange = Rangef::AtLeast(0);
	heightRange = Rangef::AtLeast(0);
}

Rangef SizeConstraintElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (widthRange.min >= widthRange.max)
		return widthRange;
	return WrapperElement::GetFullEstimatedWidth(containerSize, type).Intersect(widthRange);
}

Rangef SizeConstraintElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (heightRange.min >= heightRange.max)
		return heightRange;
	return WrapperElement::GetFullEstimatedHeight(containerSize, type).Intersect(heightRange);
}


void TextElement::OnReset()
{
	UIObjectNoChildren::OnReset();

	text = {};
}

void TextElement::OnPaint(const UIPaintContext& ctx)
{
	auto* fs = _FindClosestParentFontSettings();
	auto* font = fs->GetFont();

	auto r = GetFinalRect();
	float w = r.x1 - r.x0;
	draw::TextLine(font, fs->size, r.x0, r.y1 - (r.y1 - r.y0 - fs->size) / 2, text, ctx.textColor);
}


void Placeholder::OnPaint(const UIPaintContext& ctx)
{
	auto r = GetFinalRect();
	draw::RectCol(r.x0, r.y0, r.x1, r.y1, Color4b(0, 127));
	draw::RectCutoutCol(r, r.ShrinkBy(UIRect::UniformBorder(1)), Color4b(255, 127));

	char text[32];
	snprintf(text, sizeof(text), "%gx%g", r.GetWidth(), r.GetHeight());

	auto* fs = _FindClosestParentFontSettings();
	auto* font = fs->GetFont();

	float w = r.x1 - r.x0;
	float tw = GetTextWidth(font, fs->size, text);
	draw::TextLine(font, fs->size, r.x0 + w * 0.5f - tw * 0.5f, r.y1 - (r.y1 - r.y0 - fs->size) / 2, text, ctx.textColor);
}


void ChildScaleOffsetElement::OnReset()
{
	WrapperElement::OnReset();

	transform = {};
}

void ChildScaleOffsetElement::TransformVerts(void* userdata, Vertex* vertices, size_t count)
{
	auto* me = (ChildScaleOffsetElement*)userdata;

	auto cr = me->GetFinalRect();
	for (size_t i = 0; i < count; i++)
	{
		vertices[i].x = me->transform.TransformX(vertices[i].x) + cr.x0;
		vertices[i].y = me->transform.TransformY(vertices[i].y) + cr.y0;
	}

	me->_prevVTCB.Call(vertices, count);
}

void ChildScaleOffsetElement::OnPaint(const UIPaintContext& ctx)
{
	draw::VertexTransformCallback cb = { this, TransformVerts };
	_prevVTCB = draw::SetVertexTransformCallback(cb);
	float prevTRS = MultiplyTextResolutionScale(transform.scale);

	auto cr = GetFinalRect();
	float sroX = cr.x0;
	float sroY = cr.y0;

	auto prevSRRT = draw::AddScissorRectResolutionTransform(transform.GetScaleOnly() * ScaleOffset2D(1, cr.x0, cr.y0));

	if (draw::PushScissorRectIfNotEmpty({ 0, 0, _childSize.x, _childSize.y }))
	{
		WrapperElement::OnPaint(ctx);

		draw::PopScissorRect();
	}

	draw::SetScissorRectResolutionTransform(prevSRRT);

	SetTextResolutionScale(prevTRS);
	draw::SetVertexTransformCallback(_prevVTCB);
}

void ChildScaleOffsetElement::OnPaintSingleChild(SingleChildPaintPtr* next, const UIPaintContext& ctx)
{
	draw::VertexTransformCallback cb = { this, TransformVerts };
	_prevVTCB = draw::SetVertexTransformCallback(cb);
	float prevTRS = MultiplyTextResolutionScale(transform.scale);

	auto cr = GetFinalRect();
	float sroX = cr.x0;
	float sroY = cr.y0;

	auto prevSRRT = draw::AddScissorRectResolutionTransform(transform.GetScaleOnly() * ScaleOffset2D(1, cr.x0, cr.y0));

	WrapperElement::OnPaintSingleChild(next, ctx);

	draw::SetScissorRectResolutionTransform(prevSRRT);

	SetTextResolutionScale(prevTRS);
	draw::SetVertexTransformCallback(_prevVTCB);
}

Point2f ChildScaleOffsetElement::LocalToChildPoint(Point2f pos) const
{
	auto cr = GetFinalRect();
	pos -= { cr.x0, cr.y0 };
	pos = transform.InverseTransformPoint(pos);
	return pos;
}

Rangef ChildScaleOffsetElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	Rangef r = WrapperElement::GetFullEstimatedWidth(containerSize / transform.scale, type);
	r.min *= transform.scale;
	if (r.max < FLT_MAX)
		r.max *= transform.scale;
	return r;
}

Rangef ChildScaleOffsetElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	Rangef r = WrapperElement::GetFullEstimatedHeight(containerSize / transform.scale, type);
	r.min *= transform.scale;
	if (r.max < FLT_MAX)
		r.max *= transform.scale;
	return r;
}

void ChildScaleOffsetElement::OnLayout(const UIRect& rect)
{
	auto srect = rect;
	srect.x1 = (srect.x1 - srect.x0) / transform.scale;
	srect.y1 = (srect.y1 - srect.y0) / transform.scale;
	srect.x0 = 0;
	srect.y0 = 0;
	float prevTRS = MultiplyTextResolutionScale(transform.scale);

	WrapperElement::OnLayout(srect);
	_childSize = GetFinalRect().GetSize();
	_finalRect = rect;

	SetTextResolutionScale(prevTRS);
}


EdgeSliceLayoutElement::Slot EdgeSliceLayoutElement::_slotTemplate;

void EdgeSliceLayoutElement::OnReset()
{
	UIElement::OnReset();

	_slots.clear();
}

void EdgeSliceLayoutElement::SlotIterator_Init(UIObjectIteratorData& data)
{
	data.data0 = 0;
}

UIObject* EdgeSliceLayoutElement::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	if (data.data0 >= _slots.size())
		return nullptr;
	return _slots[data.data0++]._element;
}

void EdgeSliceLayoutElement::OnLayout(const UIRect& rect)
{
	auto subr = rect;
	for (const auto& slot : _slots)
	{
		auto* ch = slot._element;
		auto e = slot.edge;
		Rangef r(DoNotInitialize{});
		float d;
		switch (e)
		{
		case Edge::Top:
			r = ch->GetFullEstimatedHeight(subr.GetSize(), EstSizeType::Expanding);
			if (&slot == &_slots.back())
				d = r.Clamp(subr.GetHeight());
			else
				d = r.min;
			ch->PerformLayout({ subr.x0, subr.y0, subr.x1, subr.y0 + d });
			subr.y0 += d;
			break;
		case Edge::Bottom:
			d = ch->GetFullEstimatedHeight(subr.GetSize(), EstSizeType::Expanding).min;
			ch->PerformLayout({ subr.x0, subr.y1 - d, subr.x1, subr.y1 });
			subr.y1 -= d;
			break;
		case Edge::Left:
			d = ch->GetFullEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).min;
			ch->PerformLayout({ subr.x0, subr.y0, subr.x0 + d, subr.y1 });
			subr.x0 += d;
			break;
		case Edge::Right:
			d = ch->GetFullEstimatedWidth(subr.GetSize(), EstSizeType::Expanding).min;
			ch->PerformLayout({ subr.x1 - d, subr.y0, subr.x1, subr.y1 });
			subr.x1 -= d;
			break;
		}
	}
	_finalRect = rect;
}

void EdgeSliceLayoutElement::RemoveChildImpl(UIObject* ch)
{
	for (size_t i = 0; i < _slots.size(); i++)
	{
		if (_slots[i]._element == ch)
		{
			_slots.erase(_slots.begin() + i);
			break;
		}
	}
}

void EdgeSliceLayoutElement::DetachChildren(bool recursive)
{
	for (size_t i = 0; i < _slots.size(); i++)
	{
		auto* ch = _slots[i]._element;

		if (recursive)
			ch->DetachChildren(true);

		// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
		if (system)
			ch->_DetachFromTree();

		ch->parent = nullptr;
	}
	_slots.clear();
}

void EdgeSliceLayoutElement::CustomAppendChild(UIObject* obj)
{
	obj->DetachParent();

	obj->parent = this;
	Slot slot = _slotTemplate;
	slot._element = obj;
	_slots.push_back(slot);

	if (system)
		obj->_AttachToFrameContents(system);
}

void EdgeSliceLayoutElement::OnPaint(const UIPaintContext& ctx)
{
	for (auto& slot : _slots)
		slot._element->Paint(ctx);
}

UIObject* EdgeSliceLayoutElement::FindLastChildContainingPos(Point2f pos) const
{
	for (size_t i = _slots.size(); i > 0; )
	{
		i--;
		if (_slots[i]._element->Contains(pos))
			return _slots[i]._element;
	}
	return nullptr;
}

void EdgeSliceLayoutElement::_AttachToFrameContents(FrameContents* owner)
{
	UIElement::_AttachToFrameContents(owner);

	for (auto& slot : _slots)
		slot._element->_AttachToFrameContents(owner);
}

void EdgeSliceLayoutElement::_DetachFromFrameContents()
{
	for (auto& slot : _slots)
		slot._element->_DetachFromFrameContents();

	UIElement::_DetachFromFrameContents();
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

Rangef Buildable::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (TEMP_LAYOUT_MODE)
		return firstChild ? firstChild->GetFullEstimatedWidth(containerSize, type) : Rangef::AtLeast(0);

	return UIObject::GetFullEstimatedWidth(containerSize, type);
}

Rangef Buildable::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (TEMP_LAYOUT_MODE)
		return firstChild ? firstChild->GetFullEstimatedHeight(containerSize, type) : Rangef::AtLeast(0);

	return UIObject::GetFullEstimatedHeight(containerSize, type);
}

void Buildable::OnLayout(const UIRect& inRect)
{
	if (TEMP_LAYOUT_MODE)
	{
		_finalRect = inRect;
		if (firstChild)
		{
			firstChild->PerformLayout(inRect);
			if (TEMP_LAYOUT_MODE == WRAPPER)
				_finalRect = firstChild->GetFinalRect();
		}
		return;
	}

	UIObject::OnLayout(inRect);
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
