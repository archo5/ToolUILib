
#include "../Core/HashMap.h"
#include "Objects.h"
#include "Native.h"
#include "System.h"
#include "Theme.h"

#include "../Render/RenderText.h"


namespace ui {

extern FrameContents* g_curSystem;


AABB2f UIObject_GetFinalRect(UIObject* obj)
{
	return obj->GetFinalRect();
}


PersistentObjectList::~PersistentObjectList()
{
	DeleteAll();
}

void PersistentObjectList::DeleteAll()
{
	_curPO = &_firstPO;
	DeleteRemaining();
	_firstPO = nullptr;
}

void PersistentObjectList::BeginAllocations()
{
	_curPO = &_firstPO;
	// reset all
	for (auto* cur = _firstPO; cur; cur = cur->_next)
		cur->PO_ResetConfiguration();
}

void PersistentObjectList::EndAllocations()
{
	DeleteRemaining();
	_curPO = nullptr;
}

void PersistentObjectList::DeleteRemaining()
{
	IPersistentObject* next;
	for (auto* cur = *_curPO; cur; cur = next)
	{
		next = cur->_next;
		DeletePersistentObject(cur);
	}
	*_curPO = nullptr;
}


struct EventHandlerEntry
{
	EventHandlerEntry* next;
	UIObject* target;
	EventType type;
	bool isLocal;
	bool early;
	EventFunc func;
};

UIObject::UIObject()
{
}

UIObject::~UIObject()
{
	ClearEventHandlers();
	// ensure PO_BeforeDelete was called
	assert(!parent);
	assert(!system);
	assert(!_livenessToken.IsAlive());
}

void UIObject::_DetachFromTree()
{
	if (!(flags & UIObject_IsInTree))
		return;

	if (flags & UIObject_NeedsTreeUpdates)
		OnExitTree();

	// remove from build/layout sets
	system->container.pendingBuildSet.Remove(static_cast<Buildable*>(this));
	system->container.pendingNextBuildSet.Remove(static_cast<Buildable*>(this));
	system->container.layoutStack.OnDestroy(this);
	flags &= ~(UIObject_IsInLayoutStack | UIObject_IsInTree);

	// add to deactivation set
	system->container.pendingDeactivationSet.InsertIfMissing(this);
}

void UIObject::PO_ResetConfiguration()
{
	ClearEventHandlers();

	DetachAll();

	auto origFlags = flags;
	const uint32_t KEEP_MASK =
		UIObject_IsHovered |
		UIObject_IsClickedAnyMask |
		UIObject_IsEdited |
		UIObject_IsChecked |
		UIObject_IsPressedAny |
		UIObject_NeedsTreeUpdates |
		UIObject_AfterIMEdit;
	flags = UIObject_DB__Defaults | (origFlags & KEEP_MASK);

	_InitReset();
}

void UIObject::PO_BeforeDelete()
{
	DetachAll();

	if (system)
		_DetachFromFrameContents();

	_livenessToken.SetAlive(false);
}

void UIObject::_InitReset()
{
	OnReset();
}

void UIObject::_AttachToFrameContents(FrameContents* owner)
{
	if (!system)
	{
		system = owner;

		OnEnable();
	}

	system->container.pendingDeactivationSet.RemoveIfFound(this);

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
}

void UIObject::_DetachFromFrameContents()
{
	if (!system)
		return;

	OnDisable();

	system->container.layoutStack.OnDestroy(this);
	system->container.pendingDeactivationSet.RemoveIfFound(this);

	system = nullptr;
}

void UIObject::_DoEvent(Event& e)
{
	e.current = this;

	if (e.IsPropagationStopped())
		return;

	for (auto* n = _firstEH; n; n = n->next)
	{
		if (!n->early)
			continue;
		if (n->type != EventType::Any && n->type != e.type)
			continue;
		if (n->target && e.target != n->target)
			continue;
		n->func(e);
		if (e.IsPropagationStopped())
			return;
	}

	OnEvent(e);

	if (e.IsPropagationStopped())
		return;

	// default behaviors
	_PerformDefaultBehaviors(e, flags);

	if (e.IsPropagationStopped())
		return;

	for (auto* n = _firstEH; n; n = n->next)
	{
		if (n->early)
			continue;
		if (n->type != EventType::Any && n->type != e.type)
			continue;
		if (n->target && e.target != n->target)
			continue;
		n->func(e);
		if (e.IsPropagationStopped())
			return;
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
			if (e.type == EventType::Click)
			{
				e.StopPropagation();
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

EventFunc& UIObject::HandleEvent(UIObject* target, EventType type, bool early)
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
	eh->early = early;
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

void UIObject::PerformLayout(const UIRect& rect, LayoutInfo info)
{
	_rcvdLayoutInfo = info;
	if (_NeedsLayout())
	{
		OnLayout(rect, info);
		OnLayoutChanged();
	}
}

void UIObject::ApplyLayoutInfo(const UIRect& rectFromChild, const UIRect& layoutRect, LayoutInfo info)
{
	if (info.flags & LayoutInfo::FillH)
	{
		_finalRect.x0 = layoutRect.x0;
		_finalRect.x1 = layoutRect.x1;
	}
	else
	{
		_finalRect.x0 = rectFromChild.x0;
		_finalRect.x1 = rectFromChild.x1;
	}

	if (info.flags & LayoutInfo::FillV)
	{
		_finalRect.y0 = layoutRect.y0;
		_finalRect.y1 = layoutRect.y1;
	}
	else
	{
		_finalRect.y0 = rectFromChild.y0;
		_finalRect.y1 = rectFromChild.y1;
	}
}

void UIObject::RedoLayout()
{
	UIRect r = _finalRect;
	PerformLayout(r, _rcvdLayoutInfo);
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

	parent->RemoveChildImpl(this);

	parent = nullptr;
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

// ORDER:
// 1      [first in forward order]
// - 2
// - - 3
// - - - 4
// - 5
// - - 6  [last in reverse order]

UIObject* UIObject::LPN_GetNextInForwardOrder()
{
	if (auto* firstChild = LPN_GetFirstChild())
		return firstChild;

	for (UIObject* it = this; it && it->parent; it = it->parent)
	{
		if (auto* next = it->parent->LPN_GetChildAfter(it))
			return next;
	}
	return nullptr;
}

UIObject* UIObject::LPN_GetPrevInReverseOrder()
{
	if (!parent)
		return nullptr;
	if (UIObject* ret = parent->LPN_GetChildBefore(this))
		return ret->LPN_GetLastInReverseOrder();
	return parent;
}

UIObject* UIObject::LPN_GetLastInReverseOrder()
{
	UIObject* ret = this;
	while (UIObject* lastChild = ret->LPN_GetLastChild())
		ret = lastChild;
	return ret;
}

UIObject* UIObject::LPN_GetFirstChild()
{
	UIObjectIterator it(this);
	if (UIObject* firstChild = it.GetNext())
		return firstChild;
	return nullptr;
}

UIObject* UIObject::LPN_GetChildAfter(UIObject* cur)
{
	UIObjectIterator it(this);
	while (UIObject* o = it.GetNext())
	{
		if (o == cur)
			return it.GetNext();
	}
	return nullptr;
}

UIObject* UIObject::LPN_GetChildBefore(UIObject* cur)
{
	UIObjectIterator it(this);
	UIObject* lastChild = nullptr;
	while (UIObject* o = it.GetNext())
	{
		if (o == cur)
			return lastChild;
		lastChild = o;
	}
	return nullptr;
}

UIObject* UIObject::LPN_GetLastChild()
{
	UIObjectIterator it(this);
	UIObject* lastChild = nullptr;
	while (UIObject* o = it.GetNext())
		lastChild = o;
	return lastChild;
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


void UIObjectNoChildren::AppendChild(UIObject* obj)
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

void UIObjectSingleChild::AppendChild(UIObject* obj)
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

UIObject* UIObjectSingleChild::FindObjectAtPoint(Point2f pos)
{
	if (Contains(pos))
	{
		if (_child)
			if (auto* o = _child->FindObjectAtPoint(pos))
				return o;
		if (!(flags & UIObject_HitTestPassthrough))
			return this;
	}
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

void UIObjectSingleChild::_DetachFromTree()
{
	if (!(flags & UIObject_IsInTree))
		return;

	if (_child)
		_child->_DetachFromTree();

	UIObject::_DetachFromTree();
}


EstSizeRange WrapperElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (!_child || !_child->_NeedsLayout())
		return {};
	return _child->CalcEstimatedWidth(containerSize, type);
}

EstSizeRange WrapperElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (!_child || !_child->_NeedsLayout())
		return {};
	return _child->CalcEstimatedHeight(containerSize, type);
}

void WrapperElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child && _child->_NeedsLayout())
	{
		_child->PerformLayout(rect, info);

		ApplyLayoutInfo(_child->GetFinalRect(), rect, info);
	}
	else _finalRect = rect;
}


EstSizeRange FillerElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return EstSizeRange::Exact(containerSize.x);
}

EstSizeRange FillerElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return EstSizeRange::Exact(containerSize.y);
}

void FillerElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child && _child->_NeedsLayout())
		_child->PerformLayout(rect, info);
	_finalRect = rect;
}


EstSizeRange HFillVWrapElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return EstSizeRange::Exact(containerSize.x);
}

EstSizeRange HFillVWrapElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (!_child || !_child->_NeedsLayout())
		return {};
	return _child->CalcEstimatedHeight(containerSize, type);
}

void HFillVWrapElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child && _child->_NeedsLayout())
	{
		_child->PerformLayout(rect, info);

		info.flags |= LayoutInfo::FillH;
		//info.flags &= ~LayoutInfo::FillV;
		ApplyLayoutInfo(_child->GetFinalRect(), rect, info);
	}
	else
	{
		_finalRect = rect;
		_finalRect.y1 = rect.y0;
	}
}


void SizeConstraintElement::OnReset()
{
	WrapperElement::OnReset();
	widthRange = {};
	heightRange = {};
}

EstSizeRange SizeConstraintElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (widthRange.hardMin >= widthRange.hardMax)
		return widthRange;
	return WrapperElement::CalcEstimatedWidth(containerSize, type).LimitTo(widthRange);
}

EstSizeRange SizeConstraintElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (heightRange.hardMin >= heightRange.hardMax)
		return heightRange;
	return WrapperElement::CalcEstimatedHeight(containerSize, type).LimitTo(heightRange);
}


void CenteringElement::OnReset()
{
	WrapperElement::OnReset();

	flags |= UIObject_HitTestPassthrough;
}

void CenteringElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child)
	{
		auto wr = CalcEstimatedWidth(rect.GetSize(), ui::EstSizeType::Exact);
		auto hr = CalcEstimatedHeight(rect.GetSize(), ui::EstSizeType::Exact);
		float w = roundf(wr.ExpandToFill(rect.GetWidth()));
		float h = roundf(hr.ExpandToFill(rect.GetHeight()));
		float x = roundf(rect.x0 + (rect.GetWidth() - w) * align.x);
		float y = roundf(rect.y0 + (rect.GetHeight() - h) * align.y);
		UIRect r = { x, y, x + w, y + h };
		_child->PerformLayout(r, info);
	}
	_finalRect = rect;
}


OverlayElement& OverlayElement::SetDepth(float depth)
{
	_depth = depth;
	if (_isRegistered)
		system->overlays.sortedOutdated = true;
	return *this;
}

void OverlayElement::OnReset()
{
	WrapperElement::OnReset();

	_depth = 0;
}

void OverlayElement::_AttachToFrameContents(FrameContents* owner)
{
	WrapperElement::_AttachToFrameContents(owner);

	system->overlays.Register(this);
	_isRegistered = true;
}

void OverlayElement::_DetachFromFrameContents()
{
	_isRegistered = false;
	if (system)
		system->overlays.Unregister(this);

	WrapperElement::_DetachFromFrameContents();
}


void TextElement::_UpdateCache()
{
	if (multiline)
	{
		_lines.Clear();
		StringView it = text;
		it = it.trim();
		while (it.NotEmpty())
		{
			StringView line = it.UntilFirst("\n").trim();
			_lines.Append(line);
			it = it.AfterFirst("\n");
		}
		if (_lines.NotEmpty() && _lines.Last().IsEmpty())
			_lines.RemoveLast();
	}
	else
	{
		// do not allocate for single line text
		_lines.Clear();
	}
	_cachedLines = true;
}

void TextElement::OnReset()
{
	UIObjectNoChildren::OnReset();

	text = {};
	multiline = false;
	_cachedLines = false;
	_lines.Clear();
}

void TextElement::OnPaint(const UIPaintContext& ctx)
{
	if (!_cachedLines)
		_UpdateCache();
	auto* fs = _FindClosestParentFontSettings();
	auto* font = fs->GetFont();

	auto r = GetFinalRect();
	if (multiline)
	{
		float y = roundf(r.GetCenterY() - (_lines.Size() - 1) * fs->size * 0.5f);
		for (size_t i = 0; i < _lines.Size(); i++)
		{
			draw::TextLine(font, fs->size, r.x0, y, _lines[i], ctx.textColor, TextBaseline::Middle, &r);
			y += fs->size;
		}
	}
	else
	{
		draw::TextLine(font, fs->size, r.x0, r.GetCenterY(), text, ctx.textColor, TextBaseline::Middle, &r);
	}
}

EstSizeRange TextElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	if (!_cachedLines)
		_UpdateCache();
	auto* fs = _FindClosestParentFontSettings();
	if (multiline)
	{
		float maxw = 0;
		for (StringView line : _lines)
			maxw = max(maxw, GetTextWidth(fs->GetFont(), fs->size, line));
		return EstSizeRange::SoftExact(ceilf(maxw));
	}
	else
	{
		float w = ceilf(GetTextWidth(fs->GetFont(), fs->size, text));
		return EstSizeRange::SoftExact(w);
	}
}

EstSizeRange TextElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	if (!_cachedLines)
		_UpdateCache();
	auto* fs = _FindClosestParentFontSettings();
	if (multiline)
	{
		return EstSizeRange::Exact(fs->size * _lines.Size());
	}
	else
	{
		return EstSizeRange::Exact(fs->size);
	}
}


void Placeholder::OnPaint(const UIPaintContext& ctx)
{
	auto r = GetFinalRect();
	draw::RectCol(r.x0, r.y0, r.x1, r.y1, Color4b(0, 127));
	draw::RectCutoutCol(r, r.ShrinkBy(1), Color4b(255, 127));

	char text[32];
	snprintf(text, sizeof(text), "%gx%g", r.GetWidth(), r.GetHeight());

	auto* fs = _FindClosestParentFontSettings();
	auto* font = fs->GetFont();

	draw::TextLine(font, fs->size, r.GetCenterX(), r.GetCenterY(), text, ctx.textColor, TextHAlign::Center, TextBaseline::Middle);
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

UIObject* ChildScaleOffsetElement::FindObjectAtPoint(Point2f pos)
{
	if (_child)
	{
		auto chpos = ChildScaleOffsetElement::LocalToChildPoint(pos);
		if (auto* o = _child->FindObjectAtPoint(chpos))
			return o;
	}
	if (!(flags & UIObject_HitTestPassthrough) && Contains(pos))
		return this;
	return nullptr;
}

EstSizeRange ChildScaleOffsetElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	EstSizeRange r = WrapperElement::CalcEstimatedWidth(containerSize / transform.scale, type);
	r.softMin *= transform.scale;
	r.hardMin *= transform.scale;
	if (r.hardMax < FLT_MAX)
		r.hardMax *= transform.scale;
	return r;
}

EstSizeRange ChildScaleOffsetElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	EstSizeRange r = WrapperElement::CalcEstimatedHeight(containerSize / transform.scale, type);
	r.softMin *= transform.scale;
	r.hardMin *= transform.scale;
	if (r.hardMax < FLT_MAX)
		r.hardMax *= transform.scale;
	return r;
}

void ChildScaleOffsetElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto srect = rect;
	srect.x1 = (srect.x1 - srect.x0) / transform.scale;
	srect.y1 = (srect.y1 - srect.y0) / transform.scale;
	srect.x0 = 0;
	srect.y0 = 0;
	float prevTRS = MultiplyTextResolutionScale(transform.scale);

	WrapperElement::OnLayout(srect, info);
	_childSize = GetFinalRect().GetSize();
	_finalRect = rect;

	SetTextResolutionScale(prevTRS);
}


struct IMUIStateSaverEntry
{
	IGlobalIMUIStateSaver* saver;
	void* tmpcopy;
};
Array<IMUIStateSaverEntry> g_imuiStateSavers;

void RegisterGlobalIMUIStateSaver(IGlobalIMUIStateSaver* saver)
{
	g_imuiStateSavers.Append({ saver });
}

void GIMUISS_SaveAll()
{
	for (auto& s : g_imuiStateSavers)
		s.tmpcopy = s.saver->CreateCurrentStateCopy();
}

void GIMUISS_RestoreAll()
{
	for (auto& s : g_imuiStateSavers)
	{
		s.saver->ApplyStateCopy(s.tmpcopy);
		s.saver->DestroyStateCopy(s.tmpcopy);
		s.tmpcopy = nullptr;
	}
}

void GIMUISS_Apply(ArrayView<void*> states)
{
	for (size_t i = 0; i < states.Size(); i++)
		g_imuiStateSavers[i].saver->ApplyStateCopy(states[i]);
}

void GIMUISS_ResaveTo(Array<void*>& states)
{
	for (size_t i = 0; i < states.Size(); i++)
	{
		g_imuiStateSavers[i].saver->DestroyStateCopy(states[i]);
	}
	states.Resize(g_imuiStateSavers.Size());
	for (size_t i = 0; i < states.Size(); i++)
	{
		states[i] = g_imuiStateSavers[i].saver->CreateCurrentStateCopy();
	}
}


void Buildable::PO_ResetConfiguration()
{
	decltype(_deferredDestructors) ddList;
	std::swap(_deferredDestructors, ddList);

	WrapperElement::PO_ResetConfiguration();

	std::swap(_deferredDestructors, ddList);
}

void Buildable::PO_BeforeDelete()
{
	while (_deferredDestructors.size())
	{
		_deferredDestructors.Last()();
		_deferredDestructors.RemoveLast();
	}

	for (size_t i = 0; i < _globalStateCopies.Size(); i++)
		g_imuiStateSavers[i].saver->DestroyStateCopy(_globalStateCopies[i]);
	_globalStateCopies.Clear();

	WrapperElement::PO_BeforeDelete();
}

void Buildable::Rebuild()
{
	if (!(flags & UIObject_IsInTree))
		return;
	system->container.QueueForRebuild(this);
	GetNativeWindow()->InvalidateAll();
}


modAddTooltip::modAddTooltip(StringView sv)
{
	std::string str(sv.Data(), sv.Size());
	_evfn = [str{ Move(str) }]()
	{
		Text(str).SetMultiline(true);
	};
}

void modAddTooltip::Apply(UIObject* obj) const
{
	auto fn = _evfn;
	obj->HandleEvent(EventType::Tooltip) = [fn{ Move(fn) }](Event& e)
	{
		Tooltip::Set(e.current, fn);
		e.StopPropagation();
	};
}

} // ui
