
#include "Events.h"
#include "EventSystem.h" // TODO refactor?
#include "Objects.h"
#include "Menu.h"
#include "Native.h"
#include "System.h"


namespace ui {

uint32_t g_curLayoutFrame = 0;
DataCategoryTag DCT_MouseMoved[1];


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
		out[0] = 0xc0 | (utf32char & 0x1f);
		out[1] = 0x80 | ((utf32char >> 5) & 0x3f);
		out[2] = 0;
	}
	else if (utf32char <= 0xffff)
	{
		out[0] = 0xe0 | (utf32char & 0xf);
		out[1] = 0x80 | ((utf32char >> 4) & 0x3f);
		out[2] = 0x80 | ((utf32char >> 10) & 0x3f);
		out[3] = 0;
	}
	else if (utf32char <= 0x10ffff)
	{
		out[0] = 0xf0 | (utf32char & 0x7);
		out[1] = 0x80 | ((utf32char >> 3) & 0x3f);
		out[2] = 0x80 | ((utf32char >> 9) & 0x3f);
		out[3] = 0x80 | ((utf32char >> 15) & 0x3f);
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

void EventSystem::BubblingEvent(Event& e, UIObject* tgt, bool stopOnDisabled)
{
	UIObject* obj = e.target;
	while (obj != tgt && !e.IsPropagationStopped())
	{
		if (stopOnDisabled && obj->IsInputDisabled())
			break;
		obj->_DoEvent(e);
		obj = obj->parent;
	}
}

void EventSystem::RecomputeLayout()
{
	g_curLayoutFrame++;
	if (container->rootBuildable)
		container->rootBuildable->OnLayout({ 0, 0, width, height }, { width, height });
}

static void RemoveTimer(std::vector<TimerData>& timers, size_t& i, size_t& count)
{
	if (i + 1 < timers.size())
		std::swap(timers[i], timers.back());
	timers.pop_back();
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

void EventSystem::SetKeyboardFocus(UIObject* o)
{
	wasFocusSet = true;
	if (focusObj == o)
		return;

	if (focusObj)
	{
		Event ev(this, focusObj, EventType::LostFocus);
		focusObj->_DoEvent(ev);
	}

	focusObj = o;
	if (o)
		lastFocusObj = o;

	if (focusObj)
	{
		Event ev(this, focusObj, EventType::GotFocus);
		focusObj->_DoEvent(ev);
	}
}

bool EventSystem::DragCheck(Event& e, MouseButton btn)
{
	int at = (int)btn;
	if (DragDrop::GetData())
		return false;
	return e.type == EventType::MouseMove
		&& (e.current->flags & UIObject_IsPressedMouse)
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
	pendingTimers.push_back({ tgt, t, id });
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
		if (auto* o = _FindObjectAtPosition(overlays->sorted[i].obj, pos))
			return o;
	}
	return _FindObjectAtPosition(container->rootBuildable, pos);
}

UIObject* EventSystem::_FindObjectAtPosition(UIObject* root, Point2f pos)
{
	UIObject* o = root;
	if (!o || !o->Contains(pos))
		return nullptr;

	bool found = true;
	while (found)
	{
		found = false;
		for (auto* ch = o->lastChild; ch; ch = ch->prev)
		{
			if (ch->Contains(pos))
			{
				o = ch;
				found = true;
				break;
			}
		}
	}
	return o;
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
	e._stopPropagation = false;
	o->_DoEvent(e);
}

void EventSystem::_UpdateHoverObj(UIWeakPtr<UIObject>& curHoverObj, Point2f cursorPos, uint8_t mod, bool dragEvents)
{
	auto* tgt = dragEvents ?
		DragDrop::GetData() == nullptr ? nullptr : FindObjectAtPosition(cursorPos) :
		mouseCaptureObj ? mouseCaptureObj : FindObjectAtPosition(cursorPos);

	Event ev(this, curHoverObj, dragEvents ? EventType::DragLeave : EventType::MouseLeave);
	ev.position = cursorPos;
	ev.modifiers = mod;

	uint32_t hoverFlag = dragEvents ? UIObject_DragHovered : UIObject_IsHovered;

	auto* o = curHoverObj.Get();
	while (o && (!tgt || !tgt->IsChildOrSame(o)))
	{
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
#if 0
	if (Tooltip::IsSet())
		return;
#endif
	Tooltip::ClearChangedFlag();
	Event ev(this, hoverObj, EventType::Tooltip);
	ev.position = cursorPos;
	UIObject* obj = ev.target;
	while (obj && !ev.IsPropagationStopped())
	{
		obj->_DoEvent(ev);
		if (Tooltip::WasChanged())
		{
			tooltipObj = obj;
			Tooltip::ClearChangedFlag();
			Notify(DCT_TooltipChanged);
			break;
		}
		obj = obj->parent;
	}
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
		if (Tooltip::IsSet() && tooltipObj && (!hoverObj || !hoverObj->IsChildOrSame(tooltipObj)))
		{
			Tooltip::Unset();
			Notify(DCT_TooltipChanged);
		}
		_UpdateTooltip(cursorPos);
	}

	prevMousePos = cursorPos;

	if (moved)
		Notify(DCT_MouseMoved, GetNativeWindow());
}

void EventSystem::OnMouseButton(bool down, MouseButton which, Point2f cursorPos, uint8_t mod)
{
	int id = int(which);
	uint32_t t = platform::GetTimeMs();

	wasFocusSet = false;

	Event ev(this, hoverObj, EventType::ButtonDown);
	ev.shortCode = id;
	ev.position = cursorPos;
	ev.modifiers = mod;

	if (down)
	{
		unsigned mouseBtnPressCount = (t - mouseBtnPressLastTimes[id] < platform::GetDoubleClickTime() ? mouseBtnPressCounts[id] : 0) + 1;
		mouseBtnPressLastTimes[id] = t;
		mouseBtnPressCounts[id] = mouseBtnPressCount;

		ev.numRepeats = mouseBtnPressCount - 1;

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
		for (auto* p = ev.target; p; p = p->parent)
			p->flags &= ~(_UIObject_IsClicked_First << id);

		if (clickObj[id] == hoverObj)
		{
			unsigned clickCount = (t - clickLastTimes[id] < platform::GetDoubleClickTime() ? clickCounts[id] : 0) + 1;
			clickLastTimes[id] = t;
			clickCounts[id] = clickCount;

			ev.type = EventType::Click;
			ev.numRepeats = clickCount - 1;
			ev._stopPropagation = false;
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

		ev.numRepeats = mouseBtnReleaseCount - 1;

		ev.type = EventType::ButtonUp;
		ev._stopPropagation = false;
		ev.target = hoverObj;
		clickObj[id] = nullptr;
		BubblingEvent(ev);
	}
}

void EventSystem::OnMouseScroll(Vec2f delta)
{
	if (hoverObj)
	{
		Event e(this, hoverObj, EventType::MouseScroll);
		e.delta = delta;
		BubblingEvent(e);
	}
}

void EventSystem::OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint8_t mod, bool isRepeated, uint16_t numRepeats)
{
	if (focusObj)
	{
		Event ev(this, focusObj, down ? EventType::KeyDown : EventType::KeyUp);
		ev.longCode = vk;
		ev.shortCode = pk;
		ev.modifiers = mod;
		ev.arg0 = isRepeated;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

void EventSystem::OnKeyAction(KeyAction act, uint8_t mod, uint16_t numRepeats, bool modifier)
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

	if (!ev.IsPropagationStopped())
	{
		if (act == KeyAction::Inspect)
			Application::OpenInspector(GetNativeWindow(), hoverObj);
		else if (act == KeyAction::FocusNext && lastFocusObj)
		{
			bool found = false;
			for (UIObject* it = lastFocusObj->GetNextInOrder(); it; it = it->GetNextInOrder())
			{
				if (it->flags & UIObject_IsFocusable)
				{
					SetKeyboardFocus(it);
					found = true;
					break;
				}
			}
			if (!found)
			{
				for (UIObject* it = container->rootBuildable->GetFirstInOrder(); it && it != lastFocusObj; it = it->GetNextInOrder())
				{
					if (it->flags & UIObject_IsFocusable)
					{
						SetKeyboardFocus(it);
						break;
					}
				}
			}
		}
		else if (act == KeyAction::FocusPrev && lastFocusObj)
		{
			bool found = false;
			for (UIObject* it = lastFocusObj->GetPrevInOrder(); it; it = it->GetPrevInOrder())
			{
				if (it->flags & UIObject_IsFocusable)
				{
					SetKeyboardFocus(it);
					found = true;
					break;
				}
			}
			if (!found)
			{
				for (UIObject* it = container->rootBuildable->GetLastInOrder(); it && it != lastFocusObj; it = it->GetPrevInOrder())
				{
					if (it->flags & UIObject_IsFocusable)
					{
						SetKeyboardFocus(it);
						break;
					}
				}
			}
		}
	}
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

DataCategoryTag DCT_DragDropDataChanged[1];
static DragDropData* g_curDragDropData = nullptr;

void DragDrop::SetData(DragDropData* data)
{
	auto* pd = g_curDragDropData;
	delete g_curDragDropData;
	g_curDragDropData = data;
	if (pd != data)
	{
		Notify(DCT_DragDropDataChanged);
	}
}

DragDropData* DragDrop::GetData(const char* type)
{
	if (type && g_curDragDropData && g_curDragDropData->type != type)
		return nullptr;
	return g_curDragDropData;
}


static Tooltip::BuildFunc g_curTooltipBuildFn;
static bool g_tooltipChanged;

DataCategoryTag DCT_TooltipChanged[1];

void Tooltip::Set(const BuildFunc& f)
{
	g_curTooltipBuildFn = f;
	g_tooltipChanged = true;
}

void Tooltip::Unset()
{
	g_curTooltipBuildFn = {};
	g_tooltipChanged = true;
}

bool Tooltip::IsSet()
{
	return !!g_curTooltipBuildFn;
}

bool Tooltip::WasChanged()
{
	return g_tooltipChanged;
}

void Tooltip::ClearChangedFlag()
{
	g_tooltipChanged = false;
}

void Tooltip::Build()
{
	g_curTooltipBuildFn();
}


static MenuItemCollection g_menuItemCollection;

MenuItemCollection& ContextMenu::Get()
{
	return g_menuItemCollection;
}

} // ui
