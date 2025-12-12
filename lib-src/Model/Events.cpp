
#include "Events.h"
#include "EventSystem.h" // TODO refactor?
#include "Objects.h"
#include "Menu.h"
#include "Native.h"
#include "System.h"


namespace ui {

uint32_t g_curLayoutFrame = 0;
DataCategoryTag DCT_UIObject[1];
MulticastDelegate<NativeWindowBase*, int, int> OnMouseMoved;
MulticastDelegate<> OnTooltipChanged;

static bool g_tooltipInChangeContext;
static OwnerID g_curTooltipOwner;
static NativeWindowBase* g_curTooltipWindow = nullptr;
static Tooltip::BuildFunc g_curTooltipBuildFn;
static OwnerID g_newTooltipOwner;
static Tooltip::BuildFunc g_newTooltipBuildFn;


const char* EventTypeToBaseString(EventType type)
{
	switch (type)
	{
#define X(x) case EventType::x: return #x
		X(Any);

		X(Click);
		X(Activate);
		X(ContextMenu);
		X(Change);
		X(Commit);
		X(IMChange);
		X(BackgroundClick);
		X(Paint);
		X(Timer);
		X(Tooltip);
		X(Scroll);

		X(MouseEnter);
		X(MouseLeave);
		X(MouseMove);
		X(MouseScroll);
		X(MouseCaptureChanged);
		X(SetCursor);
		X(ButtonDown);
		X(ButtonUp);
		X(DragStart);
		X(DragEnd);
		X(DragEnter);
		X(DragLeave);
		X(DragMove);
		X(DragDrop);

		X(GotFocus);
		X(LostFocus);
		X(KeyDown);
		X(KeyUp);
		X(KeyAction);
		X(TextInput);
		X(SelectionChange);
#undef X

	default:
		if (type >= EventType::User)
			return "User";
		return "unknown";
	}
}

bool ModKeyCheck(u8 pressed, u8 expected)
{
	u8 diff = pressed ^ expected;
	for (int i = 0; i < 4; i++)
	{
		u8 psh = (pressed >> i) & 0x11;
		u8 esh = (expected >> i) & 0x11;
		if (psh != esh)
		{
			if (esh == 0)
				return false; // any pressed, none expected
			else if ((psh & esh) == 0)
				return false; // none of the expected keys were pressed
		}
	}
	return true;
}


Buildable* Event::GetTargetBuildable() const
{
	for (auto* t = target; t; t = t->parent)
		if (auto* n = dynamic_cast<Buildable*>(t))
			return n;
	return nullptr;
}

MouseButton Event::GetButton() const
{
	return static_cast<MouseButton>(shortCode);
}

KeyAction Event::GetKeyAction() const
{
	return static_cast<KeyAction>(shortCode);
}

bool Event::GetKeyActionModifier() const
{
	return arg0 != 0;
}

uint32_t Event::GetUTF32Char() const
{
	return longCode;
}

bool Event::GetUTF8Text(char out[5]) const
{
	auto utf32char = GetUTF32Char();
	if (utf32char <= 0x7f)
	{
		out[0] = utf32char;
		out[1] = 0;
	}
	else if (utf32char <= 0x7ff)
	{
		out[0] = 0xc0 | ((utf32char >> 6) & 0x1f);
		out[1] = 0x80 | (utf32char & 0x3f);
		out[2] = 0;
	}
	else if (utf32char <= 0xffff)
	{
		out[0] = 0xe0 | ((utf32char >> 12) & 0xf);
		out[1] = 0x80 | ((utf32char >> 6) & 0x3f);
		out[2] = 0x80 | (utf32char & 0x3f);
		out[3] = 0;
	}
	else if (utf32char <= 0x10ffff)
	{
		out[0] = 0xf0 | ((utf32char >> 18) & 0x7);
		out[1] = 0x80 | ((utf32char >> 12) & 0x3f);
		out[2] = 0x80 | ((utf32char >> 6) & 0x3f);
		out[3] = 0x80 | (utf32char & 0x3f);
		out[4] = 0;
	}
	else
		return false;
	return true;
}


EventSystem::EventSystem()
{
	for (auto& val : clickLastTimes)
		val = platform::GetTimeMs();
}

static Point2f ResolvePos(Point2f p, UIObject* o)
{
	if (!o->parent)
		return p;
	return o->LocalToChildPoint(ResolvePos(p, o->parent));
}

static void UpdateEventPosition(Event& e, UIObject* cur)
{
	e.position = ResolvePos(e.topLevelPosition, cur);
}

void EventSystem::BubblingEvent(Event& e, UIObject* tgt, bool stopOnDisabled)
{
	UIObject* obj = e.target;
	while (obj != tgt && !e.IsPropagationStopped())
	{
		if (stopOnDisabled && obj->IsInputDisabled())
			break;
		UpdateEventPosition(e, obj);
		obj->_DoEvent(e);
		obj = obj->parent;
	}
}

void EventSystem::RecomputeLayout()
{
	g_curLayoutFrame++;
	if (container->rootBuildable)
		container->rootBuildable->PerformLayout({ 0, 0, width, height }, { LayoutInfo::FillH | LayoutInfo::FillV });
}

static void RemoveTimer(Array<TimerData>& timers, size_t& i, size_t& count)
{
	timers.UnorderedRemoveAt(i);
	i--;
	count--;
}

float EventSystem::ProcessTimers(float dt)
{
	float minTime = FLT_MAX;
	size_t endOfInitialTimers = pendingTimers.size();
	for (size_t i = 0; i < endOfInitialTimers; i++)
	{
		auto& T = pendingTimers[i];
		T.timeLeft -= dt;

		if (!T.target)
		{
			RemoveTimer(pendingTimers, i, endOfInitialTimers);
			continue;
		}

		if (T.timeLeft <= 0)
		{
			auto Tcopy = T;
			RemoveTimer(pendingTimers, i, endOfInitialTimers);

			Event ev(this, Tcopy.target, EventType::Timer);
			Tcopy.target->_DoEvent(ev);
		}
		else if (minTime > T.timeLeft)
			minTime = T.timeLeft;
	}
	for (size_t i = endOfInitialTimers; i < pendingTimers.size(); i++)
		minTime = min(minTime, pendingTimers[i].timeLeft);
	return minTime;
}

void EventSystem::Repaint(UIObject* o)
{
	// TODO
}

void EventSystem::OnActivate(UIObject* o)
{
	Event ev(this, o, EventType::Activate);
	BubblingEvent(ev);
}

void EventSystem::OnCommit(UIObject* o)
{
	Event ev(this, o, EventType::Commit);
	BubblingEvent(ev);
}

void EventSystem::OnChange(UIObject* o)
{
	Event ev(this, o, EventType::Change);
	BubblingEvent(ev);
}

void EventSystem::OnIMChange(UIObject* o)
{
	Event ev(this, o, EventType::IMChange);
	BubblingEvent(ev);
}

void EventSystem::OnUserEvent(UIObject* o, int id, uintptr_t arg0, uintptr_t arg1)
{
	Event ev(this, o, UserEvent(id));
	ev.arg0 = arg0;
	ev.arg1 = arg1;
	BubblingEvent(ev);
}

void EventSystem::SetKeyboardFocus(UIObject* o, bool nav)
{
	wasFocusSet = true;
	if (focusObj == o)
		return;

	if (focusObj)
	{
		Event ev(this, focusObj, EventType::LostFocus);
		ev.arg0 = nav;
		focusObj->_DoEvent(ev);
	}

	focusObj = o;
	if (o)
		lastFocusObj = o;

	if (focusObj)
	{
		Event ev(this, focusObj, EventType::GotFocus);
		ev.arg0 = nav;
		focusObj->_DoEvent(ev);
	}
}

bool EventSystem::DragCheck(Event& e, MouseButton btn)
{
	int at = (int)btn;
	if (DragDrop::GetData())
		return false;
	return e.type == EventType::MouseMove
		&& clickObj[at]
		&& (fabsf(e.position.x - clickStartPositions[at].x) >= 3.0f
			|| fabsf(e.position.y - clickStartPositions[at].y) >= 3.0f);
}

void EventSystem::CaptureMouse(UIObject* o)
{
	ReleaseMouse();
	mouseCaptureObj = o;
}

bool EventSystem::ReleaseMouse()
{
	if (mouseCaptureObj)
	{
		Event ev(this, mouseCaptureObj, EventType::MouseCaptureChanged);
		BubblingEvent(ev);

		mouseCaptureObj = nullptr;
		return true;
	}
	return false;
}

UIObject* EventSystem::GetMouseCapture()
{
	return mouseCaptureObj;
}

void EventSystem::SetTimer(UIObject* tgt, float t, int id)
{
	pendingTimers.Append({ tgt, t, id });
}

void EventSystem::SetDefaultCursor(DefaultCursor cur)
{
	GetNativeWindow()->SetDefaultCursor(cur);
}

UIObject* EventSystem::FindObjectAtPosition(Point2f pos)
{
	overlays->UpdateSorted();
	for (size_t i = overlays->sorted.size(); i > 0; )
	{
		i--;
		if (auto* ovr = static_cast<OverlayElement*>(overlays->sorted[i])->_child)
			if (auto* o = _FindObjectAtPosition(ovr, ResolvePos(pos, ovr)))
				return o;
	}
	return _FindObjectAtPosition(container->rootBuildable, pos);
}

UIObject* EventSystem::_FindObjectAtPosition(UIObject* root, Point2f pos)
{
	if (root)
		return root->FindObjectAtPoint(pos);
	return nullptr;
}

void EventSystem::MoveClickTo(UIObject* obj, MouseButton btn)
{
	int at = (int)btn;
	if (auto* old = clickObj[at].Get())
	{
		if (hoverObj == old)
		{
			hoverObj->flags &= ~UIObject_IsHovered;
			hoverObj = obj;
			hoverObj->flags |= UIObject_IsHovered;
		}
		old->flags &= ~(_UIObject_IsClicked_First << at);
		clickObj[at] = obj;
		obj->flags |= _UIObject_IsClicked_First << at;
	}
}

static void _HoverEnterEvent(UIObject* o, UIObject* end, Event& e, uint32_t fl)
{
	if (o == end)
		return;
	_HoverEnterEvent(o->parent, end, e, fl);

	o->flags |= fl;
	e.target = o;
	UpdateEventPosition(e, o);
	e._stopPropagation = false;
	o->_DoEvent(e);
}

void EventSystem::_UpdateHoverObj(WeakPtr<UIObject>& curHoverObj, Point2f cursorPos, uint8_t mod, bool dragEvents)
{
	auto* tgt = dragEvents ?
		DragDrop::GetData() == nullptr ? nullptr : FindObjectAtPosition(cursorPos) :
		mouseCaptureObj ? mouseCaptureObj : FindObjectAtPosition(cursorPos);

	Event ev(this, curHoverObj, dragEvents ? EventType::DragLeave : EventType::MouseLeave);
	ev.topLevelPosition = cursorPos;
	ev.modifiers = mod;

	uint32_t hoverFlag = dragEvents ? UIObject_DragHovered : UIObject_IsHovered;

	auto* o = curHoverObj.Get();
	while (o && (!tgt || !tgt->IsChildOrSame(o)))
	{
		UpdateEventPosition(ev, o);
		o->flags &= ~hoverFlag;
		ev._stopPropagation = false;
		o->_DoEvent(ev);
		o = o->parent;
	}

	ev.type = dragEvents ? EventType::DragMove : EventType::MouseMove;
	ev.delta = cursorPos - prevMousePos;
	ev.target = tgt;
	ev._stopPropagation = false;
	BubblingEvent(ev);
#if 0
	for (auto* p = tgt; p; p = p->parent)
	{
		printf("UHO type=%d ptr=%p\n", ev.type, p);
		p->_DoEvent(ev);
	}
#endif

	ev.type = dragEvents ? EventType::DragEnter : EventType::MouseEnter;
	ev.delta = { 0, 0 };

	_HoverEnterEvent(tgt, o, ev, hoverFlag);

	curHoverObj = tgt;
}

void EventSystem::_UpdateCursor(UIObject* hoverObj)
{
	Event ev(this, hoverObj, EventType::SetCursor);
	BubblingEvent(ev);
	if (!ev.IsPropagationStopped())
	{
		SetDefaultCursor(DefaultCursor::Default);
	}
}

void EventSystem::_UpdateTooltip(Point2f cursorPos)
{
	g_newTooltipBuildFn = {};
	g_newTooltipOwner = {};
	if (DragDrop::GetData())
	{
		if (g_curTooltipOwner != OwnerID::Empty() || g_curTooltipBuildFn)
		{
			g_curTooltipBuildFn = {};
			g_curTooltipOwner = {};
			g_curTooltipWindow = nullptr;
			OnTooltipChanged.Call();
		}
		return;
	}
	g_tooltipInChangeContext = true;

	Event ev(this, hoverObj, EventType::Tooltip);
	ev.topLevelPosition = cursorPos;

	UIObject* obj = ev.target;
	while (obj && !ev.IsPropagationStopped())
	{
		UpdateEventPosition(ev, obj);
		obj->_DoEvent(ev);
		if (g_newTooltipBuildFn || g_newTooltipOwner != OwnerID::Empty())
			break;
		obj = obj->parent;
	}

	if (g_curTooltipOwner != g_newTooltipOwner || !g_curTooltipBuildFn != !g_newTooltipBuildFn)
	{
		g_curTooltipBuildFn = g_newTooltipBuildFn;
		g_curTooltipOwner = g_newTooltipOwner;
		g_curTooltipWindow = g_newTooltipBuildFn ? GetNativeWindow() : nullptr;
		OnTooltipChanged.Call();
	}

	g_tooltipInChangeContext = false;
}

void EventSystem::OnMouseMove(Point2f cursorPos, uint8_t mod)
{
	bool moved = cursorPos != prevMousePos;
	if (moved)
	{
		for (int i = 0; i < 5; i++)
		{
			clickCounts[i] = 0;
			mouseBtnPressCounts[i] = 0;
			mouseBtnReleaseCounts[i] = 0;
		}
	}

#if 0
	if (clickObj[0] && !dragEventAttempted && (x != prevMouseX || y != prevMouseY))
	{
		dragEventAttempted = true;
		if (clickObj[0])
		{
			Event ev(this, clickObj[0], EventType::DragStart);
			ev.x = prevMouseX;
			ev.y = prevMouseY;
			dragEventInProgress = false;
			for (auto* p = clickObj[0]; p; p = p->parent)
			{
				p->_DoEvent(ev);
				if (ev.IsPropagationStopped() || DragDrop::GetData())
				{
					dragEventInProgress = DragDrop::GetData() != nullptr;
					if (dragEventInProgress)
						dragHoverObj = nullptr; // allow it to be entered from root
					break;
				}
			}
		}
	}
#endif

	_UpdateHoverObj(hoverObj, cursorPos, mod, false);
	_UpdateCursor(hoverObj);
	_UpdateHoverObj(dragHoverObj, cursorPos, mod, true);

	if (moved)
	{
		_UpdateTooltip(cursorPos);
	}

	prevMousePos = cursorPos;

	if (moved)
		OnMouseMoved.Call(GetNativeWindow(), cursorPos.x, cursorPos.y);
}

Array<UIObject*> g_tmpHoverA;
Array<UIObject*> g_tmpHoverB;
static UIObject* FindSharedUIObject(UIObject* a, UIObject* b)
{
	g_tmpHoverA.Clear();
	g_tmpHoverB.Clear();

	while (a)
	{
		g_tmpHoverA.Append(a);
		a = a->parent;
	}
	while (b)
	{
		g_tmpHoverB.Append(b);
		b = b->parent;
	}

	if (g_tmpHoverA.IsEmpty() || g_tmpHoverB.IsEmpty() || g_tmpHoverA.Last() != g_tmpHoverB.Last())
		return nullptr;

	size_t ia = g_tmpHoverA.Size() - 1;
	size_t ib = g_tmpHoverB.Size() - 1;
	while (ia > 0 && ib > 0)
	{
		if (g_tmpHoverA[ia - 1] != g_tmpHoverB[ib - 1])
			break;
		ia--;
		ib--;
	}
	return g_tmpHoverA[ia];
}

void EventSystem::OnMouseButton(bool down, MouseButton which, Point2f cursorPos, uint8_t mod)
{
	int id = int(which);
	uint32_t t = platform::GetTimeMs();

	wasFocusSet = false;

	Event ev(this, hoverObj, EventType::ButtonDown);
	ev.shortCode = id;
	ev.topLevelPosition = cursorPos;
	ev.modifiers = mod;

	if (down)
	{
		unsigned mouseBtnPressCount = (t - mouseBtnPressLastTimes[id] < platform::GetDoubleClickTime() ? mouseBtnPressCounts[id] : 0) + 1;
		mouseBtnPressLastTimes[id] = t;
		mouseBtnPressCounts[id] = mouseBtnPressCount;

		ev.numRepeats = mouseBtnPressCount;

		for (auto* p = hoverObj.Get(); p; p = p->parent)
			p->flags |= _UIObject_IsClicked_First << id;

		clickObj[id] = hoverObj;
		clickStartPositions[id] = cursorPos;
		BubblingEvent(ev);

		if (!wasFocusSet)
			SetKeyboardFocus(nullptr);
	}
	else
	{
		if (which == MouseButton::Left)
		{
			if (DragDrop::GetData())
			{
				ev.type = EventType::DragDrop;
				ev.target = dragHoverObj;
				ev._stopPropagation = false;
				BubblingEvent(ev);

#if 0
				ev.type = EventType::DragLeave;
				ev.target = dragHoverObj;
				ev._stopPropagation = false;
				for (auto* p = dragHoverObj; p; p = p->parent)
				{
					p->_DoEvent(ev);
				}
#endif

				ev.type = EventType::DragEnd;
				ev.target = clickObj[id];
				ev._stopPropagation = false;
				BubblingEvent(ev);

				DragDrop::SetData(nullptr);
				_UpdateHoverObj(dragHoverObj, cursorPos, mod, true);
				//dragHoverObj = nullptr;
				dragEventInProgress = false;
			}
			dragEventAttempted = false;
		}

		unsigned mouseBtnReleaseCount = (t - mouseBtnReleaseLastTimes[id] < platform::GetDoubleClickTime() ? mouseBtnReleaseCounts[id] : 0) + 1;
		mouseBtnReleaseLastTimes[id] = t;
		mouseBtnReleaseCounts[id] = mouseBtnReleaseCount;

		ev.numRepeats = mouseBtnReleaseCount;

		ev.type = EventType::ButtonUp;
		ev._stopPropagation = false;
		ev.target = hoverObj;
		BubblingEvent(ev);

		for (auto* p = ev.target; p; p = p->parent)
			p->flags &= ~(_UIObject_IsClicked_First << id);

		if (UIObject* shel = FindSharedUIObject(clickObj[id], hoverObj))
		{
			unsigned clickCount = (t - clickLastTimes[id] < platform::GetDoubleClickTime() ? clickCounts[id] : 0) + 1;
			clickLastTimes[id] = t;
			clickCounts[id] = clickCount;

			ev.type = EventType::Click;
			ev.numRepeats = clickCount;
			ev._stopPropagation = false;
			ev.target = shel;
			BubblingEvent(ev);

			if (which == MouseButton::Right && !ev._stopPropagation)
			{
				auto& CM = ContextMenu::Get();
				CM.StartNew();

				ev.type = EventType::ContextMenu;
				{
					UIObject* obj = ev.target;
					while (obj != nullptr && !ev.IsPropagationStopped())
					{
						UpdateEventPosition(ev, obj);
						obj->_DoEvent(ev);
						obj = obj->parent;
						CM.basePriority += MenuItemCollection::BASE_ADVANCE;
					}
				}

				if (CM.HasAny())
				{
					Menu menu(CM.Finalize());
					menu.Show(container->rootBuildable);
					CM.Clear();
				}
			}
		}

		clickObj[id] = nullptr;
	}
}

void EventSystem::OnMouseScroll(Vec2f delta, u8 mod)
{
	if (hoverObj)
	{
		Event e(this, hoverObj, EventType::MouseScroll);
		e.delta = delta;
		e.modifiers = mod;
		e.topLevelPosition = prevMousePos;
		BubblingEvent(e);
	}
}

bool EventSystem::OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint8_t mod, bool isRepeated, uint16_t numRepeats)
{
	if (focusObj)
	{
		Event ev(this, focusObj, down ? EventType::KeyDown : EventType::KeyUp);
		ev.longCode = vk;
		ev.shortCode = pk;
		ev.modifiers = mod;
		ev.arg0 = isRepeated;
		ev.numRepeats = isRepeated;// numRepeats; -- TODO
		BubblingEvent(ev);
		return ev.IsFallbackPrevented();
	}
	return false;
}

bool EventSystem::OnKeyAction(KeyAction act, uint8_t mod, uint16_t numRepeats, bool modifier)
{
	Event ev(this, focusObj, EventType::KeyAction);
	if (focusObj)
	{
		ev.shortCode = uint8_t(act);
		ev.modifiers = mod;
		ev.numRepeats = numRepeats;
		ev.arg0 = modifier;
		BubblingEvent(ev);
	}

	if (!ev.IsFallbackPrevented())
	{
		if (act == KeyAction::Inspect)
			Application::OpenInspector(GetNativeWindow(), hoverObj);
		else if (act == KeyAction::FocusNext && lastFocusObj)
		{
			bool found = false;
			for (UIObject* it = lastFocusObj->LPN_GetNextInForwardOrder(); it; it = it->LPN_GetNextInForwardOrder())
			{
				if (it->flags & UIObject_IsFocusable)
				{
					SetKeyboardFocus(it, true);
					found = true;
					break;
				}
			}
			if (!found)
			{
				for (UIObject* it = container->rootBuildable->LPN_GetFirstInForwardOrder(); it && it != lastFocusObj; it = it->LPN_GetNextInForwardOrder())
				{
					if (it->flags & UIObject_IsFocusable)
					{
						SetKeyboardFocus(it, true);
						break;
					}
				}
			}
		}
		else if (act == KeyAction::FocusPrev && lastFocusObj)
		{
			bool found = false;
			for (UIObject* it = lastFocusObj->LPN_GetPrevInReverseOrder(); it; it = it->LPN_GetPrevInReverseOrder())
			{
				if (it->flags & UIObject_IsFocusable)
				{
					SetKeyboardFocus(it, true);
					found = true;
					break;
				}
			}
			if (!found)
			{
				for (UIObject* it = container->rootBuildable->LPN_GetLastInReverseOrder(); it && it != lastFocusObj; it = it->LPN_GetPrevInReverseOrder())
				{
					if (it->flags & UIObject_IsFocusable)
					{
						SetKeyboardFocus(it, true);
						break;
					}
				}
			}
		}
	}
	return ev.IsFallbackPrevented();
}

void EventSystem::OnTextInput(uint32_t ch, uint8_t mod, uint16_t numRepeats)
{
	if (focusObj)
	{
		Event ev(this, focusObj, EventType::TextInput);
		ev.longCode = ch;
		ev.modifiers = mod;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

NativeWindowBase* EventSystem::GetNativeWindow() const
{
	return container->GetNativeWindow();
}


void DragDropData::Build()
{
	Text(type);
}

MulticastDelegate<> OnDragDropDataChanged;
static DragDropData* g_curDragDropData = nullptr;

void DragDrop::SetData(DragDropData* data)
{
	auto* pd = g_curDragDropData;
	delete g_curDragDropData;
	g_curDragDropData = data;
	if (pd != data)
	{
		OnDragDropDataChanged.Call();
	}
}

DragDropData* DragDrop::GetData(const char* type)
{
	if (type && g_curDragDropData && g_curDragDropData->type != type)
		return nullptr;
	return g_curDragDropData;
}



void Tooltip::Set(const OwnerID& oid, const BuildFunc& f)
{
	if (!g_tooltipInChangeContext)
	{
		assert(!"Tooltip::Set called outside change event");
		return; // TODO error
	}

	g_newTooltipOwner = oid;
	g_newTooltipBuildFn = f;
}

bool Tooltip::IsSet(NativeWindowBase* curWindow)
{
	return !!g_curTooltipBuildFn && g_curTooltipWindow == curWindow;
}

void Tooltip::Build(NativeWindowBase* curWindow)
{
	if (g_curTooltipBuildFn && g_curTooltipWindow == curWindow)
		g_curTooltipBuildFn();
}


static MenuItemCollection g_menuItemCollection;

MenuItemCollection& ContextMenu::Get()
{
	return g_menuItemCollection;
}

} // ui
