
#include "Events.h"
#include "Objects.h"
#include "Menu.h"
#include "Native.h"
#include "System.h"


namespace ui {
uint32_t g_curLayoutFrame = 0;
DataCategoryTag DCT_MouseMoved[1];
} // ui


ui::Node* UIEvent::GetTargetNode() const
{
	for (auto* t = target; t; t = t->parent)
		if (auto* n = dynamic_cast<ui::Node*>(t))
			return n;
	return nullptr;
}

UIMouseButton UIEvent::GetButton() const
{
	return static_cast<UIMouseButton>(shortCode);
}

UIKeyAction UIEvent::GetKeyAction() const
{
	return static_cast<UIKeyAction>(shortCode);
}

bool UIEvent::GetKeyActionModifier() const
{
	return arg0 != 0;
}

uint32_t UIEvent::GetUTF32Char() const
{
	return longCode;
}

bool UIEvent::GetUTF8Text(char out[5]) const
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


UIEventSystem::UIEventSystem()
{
	for (auto& val : clickLastTimes)
		val = ui::platform::GetTimeMs();
}

void UIEventSystem::BubblingEvent(UIEvent& e, UIObject* tgt, bool stopOnDisabled)
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

void UIEventSystem::RecomputeLayout()
{
	ui::g_curLayoutFrame++;
	if (container->rootNode)
		container->rootNode->OnLayout({ 0, 0, width, height }, { width, height });
}

float UIEventSystem::ProcessTimers(float dt)
{
	float minTime = FLT_MAX;
	size_t endOfInitialTimers = pendingTimers.size();
	for (size_t i = 0; i < endOfInitialTimers; i++)
	{
		auto& T = pendingTimers[i];
		T.timeLeft -= dt;
		if (T.timeLeft <= 0)
		{
			UIEvent ev(this, T.target, UIEventType::Timer);
			size_t sizeBefore = pendingTimers.size();
			T.target->_DoEvent(ev);
			bool added = pendingTimers.size() > sizeBefore;

			if (i + 1 < pendingTimers.size())
				std::swap(T, pendingTimers.back());
			pendingTimers.pop_back();
			if (!added)
			{
				i--;
				endOfInitialTimers--;
			}
		}
		else if (minTime > T.timeLeft)
			minTime = T.timeLeft;
	}
	return minTime;
}

void UIEventSystem::Repaint(UIObject* o)
{
	// TODO
}

void UIEventSystem::OnDestroy(UIObject* o)
{
	if (hoverObj == o)
		hoverObj = nullptr;
	if (dragHoverObj == o)
		dragHoverObj = nullptr;
	if (mouseCaptureObj == o)
		mouseCaptureObj = nullptr;
	if (tooltipObj == o)
	{
		tooltipObj = nullptr;
		ui::Tooltip::Unset();
		ui::Notify(ui::DCT_TooltipChanged);
	}
	for (size_t i = 0; i < sizeof(clickObj) / sizeof(clickObj[0]); i++)
		if (clickObj[i] == o)
			clickObj[i] = nullptr;
	if (focusObj == o)
		focusObj = nullptr;
	if (lastFocusObj == o)
		lastFocusObj = nullptr;
	for (size_t i = 0; i < pendingTimers.size(); i++)
	{
		if (pendingTimers[i].target == o)
		{
			if (i + 1 < pendingTimers.size())
				std::swap(pendingTimers[i], pendingTimers.back());
			pendingTimers.pop_back();
			i--;
		}
	}
}

void UIEventSystem::OnActivate(UIObject* o)
{
	UIEvent ev(this, o, UIEventType::Activate);
	BubblingEvent(ev);
}

void UIEventSystem::OnCommit(UIObject* o)
{
	UIEvent ev(this, o, UIEventType::Commit);
	BubblingEvent(ev);
}

void UIEventSystem::OnChange(UIObject* o)
{
	UIEvent ev(this, o, UIEventType::Change);
	BubblingEvent(ev);
}

void UIEventSystem::OnIMChange(UIObject* o)
{
	UIEvent ev(this, o, UIEventType::IMChange);
	BubblingEvent(ev);
}

void UIEventSystem::OnUserEvent(UIObject* o, int id, uintptr_t arg0, uintptr_t arg1)
{
	UIEvent ev(this, o, UserEvent(id));
	ev.arg0 = arg0;
	ev.arg1 = arg1;
	BubblingEvent(ev);
}

void UIEventSystem::SetKeyboardFocus(UIObject* o)
{
	if (focusObj == o)
		return;

	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::LostFocus);
		focusObj->_DoEvent(ev);
	}

	focusObj = o;
	if (o)
		lastFocusObj = o;

	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::GotFocus);
		focusObj->_DoEvent(ev);
	}
}

bool UIEventSystem::DragCheck(UIEvent& e, UIMouseButton btn)
{
	int at = (int)btn;
	if (ui::DragDrop::GetData())
		return false;
	return e.type == UIEventType::MouseMove
		&& (e.current->flags & UIObject_IsPressedMouse)
		&& (fabsf(e.x - clickStartPositions[at].x) >= 3.0f
			|| fabsf(e.y - clickStartPositions[at].y) >= 3.0f);
}

void UIEventSystem::CaptureMouse(UIObject* o)
{
	ReleaseMouse();
	mouseCaptureObj = o;
}

bool UIEventSystem::ReleaseMouse()
{
	if (mouseCaptureObj)
	{
		UIEvent ev(this, mouseCaptureObj, UIEventType::MouseCaptureChanged);
		BubblingEvent(ev);

		mouseCaptureObj = nullptr;
		return true;
	}
	return false;
}

UIObject* UIEventSystem::GetMouseCapture()
{
	return mouseCaptureObj;
}

void UIEventSystem::SetTimer(UIObject* tgt, float t, int id)
{
	pendingTimers.push_back({ tgt, t, id });
}

void UIEventSystem::SetDefaultCursor(ui::DefaultCursor cur)
{
	GetNativeWindow()->SetDefaultCursor(cur);
}

UIObject* UIEventSystem::FindObjectAtPosition(float x, float y)
{
	overlays->UpdateSorted();
	for (size_t i = overlays->sorted.size(); i > 0; )
	{
		i--;
		if (auto* o = _FindObjectAtPosition(overlays->sorted[i].obj, x, y))
			return o;
	}
	return _FindObjectAtPosition(container->rootNode, x, y);
}

UIObject* UIEventSystem::_FindObjectAtPosition(UIObject* root, float x, float y)
{
	UIObject* o = root;
	if (!o || !o->Contains(x, y))
		return nullptr;

	bool found = true;
	while (found)
	{
		found = false;
		for (auto* ch = o->lastChild; ch; ch = ch->prev)
		{
			if (ch->Contains(x, y))
			{
				o = ch;
				found = true;
				break;
			}
		}
	}
	return o;
}

void UIEventSystem::MoveClickTo(UIObject* obj, UIMouseButton btn)
{
	int at = (int)btn;
	if (auto* old = clickObj[at])
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

static void _HoverEnterEvent(UIObject* o, UIObject* end, UIEvent& e, uint32_t fl)
{
	if (o == end)
		return;
	_HoverEnterEvent(o->parent, end, e, fl);

	o->flags |= fl;
	e.target = o;
	e._stopPropagation = false;
	o->_DoEvent(e);
}

void UIEventSystem::_UpdateHoverObj(UIObject*& curHoverObj, UIMouseCoord x, UIMouseCoord y, uint8_t mod, bool dragEvents)
{
	auto* tgt = dragEvents ?
		ui::DragDrop::GetData() == nullptr ? nullptr : FindObjectAtPosition(x, y) :
		mouseCaptureObj ? mouseCaptureObj : FindObjectAtPosition(x, y);

	UIEvent ev(this, curHoverObj, dragEvents ? UIEventType::DragLeave : UIEventType::MouseLeave);
	ev.x = x;
	ev.y = y;
	ev.modifiers = mod;

	uint32_t hoverFlag = dragEvents ? UIObject_DragHovered : UIObject_IsHovered;

	auto* o = curHoverObj;
	while (o && (!tgt || !tgt->IsChildOrSame(o)))
	{
		o->flags &= ~hoverFlag;
		ev._stopPropagation = false;
		o->_DoEvent(ev);
		o = o->parent;
	}

	ev.type = dragEvents ? UIEventType::DragMove : UIEventType::MouseMove;
	ev.dx = x - prevMouseX;
	ev.dy = y - prevMouseY;
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

	ev.type = dragEvents ? UIEventType::DragEnter : UIEventType::MouseEnter;
	ev.dx = 0;
	ev.dy = 0;

	_HoverEnterEvent(tgt, o, ev, hoverFlag);

	curHoverObj = tgt;
}

void UIEventSystem::_UpdateCursor(UIObject* hoverObj)
{
	UIEvent ev(this, hoverObj, UIEventType::SetCursor);
	BubblingEvent(ev);
	if (!ev.IsPropagationStopped())
	{
		SetDefaultCursor(ui::DefaultCursor::Default);
	}
}

void UIEventSystem::_UpdateTooltip()
{
	if (ui::Tooltip::IsSet())
		return;

	UIEvent ev(this, hoverObj, UIEventType::Tooltip);
	UIObject* obj = ev.target;
	while (obj && !ev.IsPropagationStopped())
	{
		obj->_DoEvent(ev);
		if (ui::Tooltip::IsSet())
		{
			tooltipObj = obj;
			ui::Notify(ui::DCT_TooltipChanged);
			break;
		}
		obj = obj->parent;
	}
}

void UIEventSystem::OnMouseMove(UIMouseCoord x, UIMouseCoord y, uint8_t mod)
{
	bool moved = x != prevMouseX || y != prevMouseY;
	if (moved)
	{
		for (int i = 0; i < 5; i++)
			clickCounts[i] = 0;
	}

#if 0
	if (clickObj[0] && !dragEventAttempted && (x != prevMouseX || y != prevMouseY))
	{
		dragEventAttempted = true;
		if (clickObj[0])
		{
			UIEvent ev(this, clickObj[0], UIEventType::DragStart);
			ev.x = prevMouseX;
			ev.y = prevMouseY;
			dragEventInProgress = false;
			for (auto* p = clickObj[0]; p; p = p->parent)
			{
				p->_DoEvent(ev);
				if (ev.IsPropagationStopped() || ui::DragDrop::GetData())
				{
					dragEventInProgress = ui::DragDrop::GetData() != nullptr;
					if (dragEventInProgress)
						dragHoverObj = nullptr; // allow it to be entered from root
					break;
				}
			}
		}
	}
#endif

	_UpdateHoverObj(hoverObj, x, y, mod, false);
	_UpdateCursor(hoverObj);
	_UpdateHoverObj(dragHoverObj, x, y, mod, true);

	if (moved)
	{
		if (ui::Tooltip::IsSet() && tooltipObj && (!hoverObj || !hoverObj->IsChildOrSame(tooltipObj)))
		{
			ui::Tooltip::Unset();
			ui::Notify(ui::DCT_TooltipChanged);
		}
	}
	_UpdateTooltip();

	prevMouseX = x;
	prevMouseY = y;

	if (moved)
		ui::Notify(ui::DCT_MouseMoved, GetNativeWindow());
}

void UIEventSystem::OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y, uint8_t mod)
{
	int id = int(which);

	UIEvent ev(this, hoverObj, UIEventType::ButtonDown);
	ev.shortCode = id;
	ev.x = x;
	ev.y = y;
	ev.modifiers = mod;

	if (down)
	{
		for (auto* p = hoverObj; p; p = p->parent)
			p->flags |= _UIObject_IsClicked_First << id;

		clickObj[id] = hoverObj;
		clickStartPositions[id] = { x, y };
		BubblingEvent(ev);
	}
	else
	{
		for (auto* p = ev.target; p; p = p->parent)
			p->flags &= ~(_UIObject_IsClicked_First << id);

		if (clickObj[id] == hoverObj)
		{
			uint32_t t = ui::platform::GetTimeMs();
			unsigned clickCount = (t - clickLastTimes[id] < ui::platform::GetDoubleClickTime() ? clickCounts[id] : 0) + 1;
			clickLastTimes[id] = t;
			clickCounts[id] = clickCount;

			ev.type = UIEventType::Click;
			ev.numRepeats = clickCount;
			ev._stopPropagation = false;
			BubblingEvent(ev);

			if (which == UIMouseButton::Right && !ev._stopPropagation)
			{
				auto& CM = ui::ContextMenu::Get();
				CM.StartNew();

				ev.type = UIEventType::ContextMenu;
				{
					UIObject* obj = ev.target;
					while (obj != nullptr && !ev.IsPropagationStopped())
					{
						obj->_DoEvent(ev);
						obj = obj->parent;
						CM.basePriority += ui::MenuItemCollection::BASE_ADVANCE;
					}
				}

				if (CM.HasAny())
				{
					ui::Menu menu(CM.Finalize());
					menu.Show(container->rootNode);
					CM.Clear();
				}
			}
		}

		if (which == UIMouseButton::Left)
		{
			if (ui::DragDrop::GetData())
			{
				ev.type = UIEventType::DragDrop;
				ev.target = dragHoverObj;
				ev._stopPropagation = false;
				BubblingEvent(ev);

#if 0
				ev.type = UIEventType::DragLeave;
				ev.target = dragHoverObj;
				ev._stopPropagation = false;
				for (auto* p = dragHoverObj; p; p = p->parent)
				{
					p->_DoEvent(ev);
				}
#endif

				ev.type = UIEventType::DragEnd;
				ev.target = clickObj[id];
				ev._stopPropagation = false;
				BubblingEvent(ev);

				ui::DragDrop::SetData(nullptr);
				_UpdateHoverObj(dragHoverObj, x, y, mod, true);
				//dragHoverObj = nullptr;
				dragEventInProgress = false;
			}
			dragEventAttempted = false;
		}

		ev.type = UIEventType::ButtonUp;
		ev._stopPropagation = false;
		ev.target = hoverObj;
		clickObj[id] = nullptr;
		BubblingEvent(ev);
	}
}

void UIEventSystem::OnMouseScroll(UIMouseCoord dx, UIMouseCoord dy)
{
	if (hoverObj)
	{
		UIEvent e(this, hoverObj, UIEventType::MouseScroll);
		e.dx = dx;
		e.dy = dy;
		BubblingEvent(e);
	}
}

void UIEventSystem::OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint8_t mod, bool isRepeated, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, down ? UIEventType::KeyDown : UIEventType::KeyUp);
		ev.longCode = vk;
		ev.shortCode = pk;
		ev.modifiers = mod;
		ev.arg0 = isRepeated;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

void UIEventSystem::OnKeyAction(UIKeyAction act, uint8_t mod, uint16_t numRepeats, bool modifier)
{
	UIEvent ev(this, focusObj, UIEventType::KeyAction);
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
		if (act == UIKeyAction::Inspect)
			ui::Application::OpenInspector(GetNativeWindow(), hoverObj);
		else if (act == UIKeyAction::FocusNext && lastFocusObj)
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
				for (UIObject* it = container->rootNode->GetFirstInOrder(); it && it != lastFocusObj; it = it->GetNextInOrder())
				{
					if (it->flags & UIObject_IsFocusable)
					{
						SetKeyboardFocus(it);
						break;
					}
				}
			}
		}
		else if (act == UIKeyAction::FocusPrev && lastFocusObj)
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
				for (UIObject* it = container->rootNode->GetLastInOrder(); it && it != lastFocusObj; it = it->GetPrevInOrder())
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

void UIEventSystem::OnTextInput(uint32_t ch, uint8_t mod, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::TextInput);
		ev.longCode = ch;
		ev.modifiers = mod;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

ui::NativeWindowBase* UIEventSystem::GetNativeWindow() const
{
	return container->GetNativeWindow();
}


namespace ui {

void DragDropData::Render(UIContainer* ctx)
{
	ctx->Text(type);
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


static Tooltip::RenderFunc g_curTooltipRenderFn;

DataCategoryTag DCT_TooltipChanged[1];

void Tooltip::Set(const RenderFunc& f)
{
	g_curTooltipRenderFn = f;
}

void Tooltip::Unset()
{
	g_curTooltipRenderFn = {};
}

bool Tooltip::IsSet()
{
	return !!g_curTooltipRenderFn;
}

void Tooltip::Render(UIContainer* ctx)
{
	g_curTooltipRenderFn(ctx);
}


static MenuItemCollection g_menuItemCollection;

MenuItemCollection& ContextMenu::Get()
{
	return g_menuItemCollection;
}

} // ui
