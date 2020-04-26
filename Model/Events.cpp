
#include "Events.h"
#include "Objects.h"
#include "Native.h"
#include "System.h"


namespace ui {
uint32_t g_curLayoutFrame = 0;
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
	else if (utf32char <= 0xffff)
	{
		out[0] = 0xf0 | (utf32char & 0xe);
		out[1] = 0x80 | ((utf32char >> 3) & 0x3f);
		out[2] = 0x80 | ((utf32char >> 9) & 0x3f);
		out[3] = 0x80 | ((utf32char >> 15) & 0x3f);
		out[4] = 0;
	}
	else
		return false;
	return true;
}


void UIEventSystem::BubblingEvent(UIEvent& e, UIObject* tgt)
{
	UIObject* obj = e.target;
	while (obj != tgt && !e.handled)
	{
		e.current = obj;
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

void UIEventSystem::ProcessTimers(float dt)
{
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
	}
}

void UIEventSystem::Repaint(UIElement* e)
{
	// TODO
}

void UIEventSystem::OnDestroy(UIObject* o)
{
	if (hoverObj == o)
		hoverObj = nullptr;
	if (dragHoverObj == o)
		dragHoverObj = nullptr;
	for (size_t i = 0; i < sizeof(clickObj) / sizeof(clickObj[0]); i++)
		if (clickObj[i] == o)
			clickObj[i] = nullptr;
	if (focusObj == o)
		focusObj = nullptr;
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

void UIEventSystem::OnCommit(UIObject* e)
{
	UIEvent ev(this, e, UIEventType::Commit);
	BubblingEvent(ev);
}

void UIEventSystem::OnChange(UIObject* e)
{
	UIEvent ev(this, e, UIEventType::Change);
	BubblingEvent(ev);
}

void UIEventSystem::OnChange(ui::Node* n)
{
	n->Rerender();
	UIEvent ev(this, n, UIEventType::Change);
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

	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::GotFocus);
		focusObj->_DoEvent(ev);
	}
}

void UIEventSystem::SetTimer(UIElement* tgt, float t, int id)
{
	pendingTimers.push_back({ tgt, t, id });
}

void UIEventSystem::SetDefaultCursor(ui::DefaultCursor cur)
{
	GetNativeWindow()->SetDefaultCursor(cur);
}

UIObject* UIEventSystem::FindObjectAtPosition(float x, float y)
{
	UIObject* o = container->rootNode;
	if (o && !o->Contains(x, y))
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

void UIEventSystem::_UpdateHoverObj(UIObject*& curHoverObj, UIMouseCoord x, UIMouseCoord y, bool dragEvents)
{
	UIEvent ev(this, curHoverObj, dragEvents ? UIEventType::DragLeave : UIEventType::MouseLeave);
	ev.x = x;
	ev.y = y;

	uint32_t hoverFlag = dragEvents ? UIObject_DragHovered : UIObject_IsHovered;

	auto* o = curHoverObj;
	while (o && !o->Contains(x, y))
	{
		o->flags &= ~hoverFlag;
		ev.current = o;
		o->_DoEvent(ev);
		o = o->parent;
	}

	ev.type = dragEvents ? UIEventType::DragMove : UIEventType::MouseMove;
	ev.dx = x - prevMouseX;
	ev.dy = y - prevMouseY;
	for (auto* p = o; p; p = p->parent)
	{
		ev.current = p;
		p->_DoEvent(ev);
	}

	ev.type = dragEvents ? UIEventType::DragEnter : UIEventType::MouseEnter;
	ev.dx = 0;
	ev.dy = 0;
	if (!o && container->rootNode && container->rootNode->Contains(x, y))
	{
		o = container->rootNode;
		o->flags |= hoverFlag;
		ev.current = o;
		ev.target = o;
		o->_DoEvent(ev);
	}

	if (o)
	{
		bool found = true;
		while (found)
		{
			found = false;
			for (auto* ch = o->lastChild; ch; ch = ch->prev)
			{
				if (ch->Contains(x, y))
				{
					o = ch;
					o->flags |= hoverFlag;
					ev.current = o;
					ev.target = o;
					o->_DoEvent(ev);
					found = true;
					break;
				}
			}
		}
	}

	curHoverObj = o;
}

void UIEventSystem::_UpdateCursor(UIObject* hoverObj)
{
	UIEvent ev(this, hoverObj, UIEventType::SetCursor);
	BubblingEvent(ev);
	if (!ev.handled)
	{
		SetDefaultCursor(ui::DefaultCursor::Default);
	}
}

void UIEventSystem::OnMouseMove(UIMouseCoord x, UIMouseCoord y)
{
	if (clickObj[0] && !dragEventAttempted && (x != 0 || y != 0))
	{
		dragEventAttempted = true;
		if (clickObj[0])
		{
			UIEvent ev(this, clickObj[0], UIEventType::DragStart);
			ev.x = x;
			ev.y = y;
			dragEventInProgress = false;
			for (auto* p = clickObj[0]; p; p = p->parent)
			{
				p->_DoEvent(ev);
				if (ev.handled || ui::DragDrop::GetData())
				{
					dragEventInProgress = ui::DragDrop::GetData() != nullptr;
					if (dragEventInProgress)
						dragHoverObj = nullptr; // allow it to be entered from root
					break;
				}
			}
		}
	}

	bool anyClicked = false;
	for (int i = 0; i < 5; i++)
	{
		if (clickObj[i])
		{
			anyClicked = true;
			break;
		}
	}

	if (anyClicked)
	{
		UIEvent ev(this, hoverObj, UIEventType::MouseMove);
		ev.x = x;
		ev.y = y;
		ev.dx = x - prevMouseX;
		ev.dy = y - prevMouseY;
		for (auto* p = hoverObj; p; p = p->parent)
		{
			ev.current = p;
			p->_DoEvent(ev);
		}
	}
	else
	{
		_UpdateHoverObj(hoverObj, x, y, false);
		_UpdateCursor(hoverObj);
	}
	if (dragEventInProgress)
		_UpdateHoverObj(dragHoverObj, x, y, true);

	prevMouseX = x;
	prevMouseY = y;
}

void UIEventSystem::OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y)
{
	int id = int(which);

	UIEvent ev(this, hoverObj, UIEventType::ButtonDown);
	ev.shortCode = id;
	ev.x = x;
	ev.y = y;

	if (down)
	{
		for (auto* p = hoverObj; p; p = p->parent)
			p->flags |= _UIObject_IsClicked_First << id;

		clickObj[id] = hoverObj;
		BubblingEvent(ev);
	}
	else
	{
		for (auto* p = ev.target; p; p = p->parent)
			p->flags &= ~(_UIObject_IsClicked_First << id);

		if (clickObj[id] == hoverObj)
		{
			ev.type = UIEventType::Click;
			ev.handled = false;
			BubblingEvent(ev);
			if (which == UIMouseButton::Left)
			{
				ev.type = UIEventType::Activate;
				ev.handled = false;
				BubblingEvent(ev);
			}
		}

		if (which == UIMouseButton::Left)
		{
			if (dragEventInProgress)
			{
				ev.type = UIEventType::DragDrop;
				ev.target = ev.current = dragHoverObj;
				ev.handled = false;
				BubblingEvent(ev);

				ev.type = UIEventType::DragLeave;
				ev.target = dragHoverObj;
				ev.handled = false;
				for (auto* p = dragHoverObj; p; p = p->parent)
				{
					ev.current = p;
					p->_DoEvent(ev);
				}

				ev.type = UIEventType::DragEnd;
				ev.target = ev.current = clickObj[id];
				ev.handled = false;
				BubblingEvent(ev);

				ui::DragDrop::SetData(nullptr);
				dragHoverObj = nullptr;
				dragEventInProgress = false;
			}
			dragEventAttempted = false;
		}

		ev.type = UIEventType::ButtonUp;
		ev.handled = false;
		ev.target = ev.current = clickObj[id];
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

void UIEventSystem::OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, down ? UIEventType::KeyDown : UIEventType::KeyUp);
		ev.longCode = vk;
		ev.shortCode = pk;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

void UIEventSystem::OnKeyAction(UIKeyAction act, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::KeyAction);
		ev.shortCode = uint8_t(act);
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);

		if (!ev.handled && act == UIKeyAction::Inspect)
			ui::Application::OpenInspector(GetNativeWindow(), hoverObj);
	}
	else if (act == UIKeyAction::Inspect)
		ui::Application::OpenInspector(GetNativeWindow(), hoverObj);
}

void UIEventSystem::OnTextInput(uint32_t ch, uint16_t numRepeats)
{
	if (focusObj)
	{
		UIEvent ev(this, focusObj, UIEventType::TextInput);
		ev.longCode = ch;
		ev.numRepeats = numRepeats;
		BubblingEvent(ev);
	}
}

ui::NativeWindowBase* UIEventSystem::GetNativeWindow() const
{
	return container->GetNativeWindow();
}


namespace ui {

static DragDropData* g_curDragDropData = nullptr;

void DragDrop::SetData(DragDropData* data)
{
	delete g_curDragDropData;
	g_curDragDropData = data;
}

DragDropData* DragDrop::GetData(const char* type)
{
	if (type && g_curDragDropData && g_curDragDropData->type != type)
		return nullptr;
	return g_curDragDropData;
}

} // ui
