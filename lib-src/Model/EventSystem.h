
#pragma once

#include "Events.h"
#include "Objects.h"


namespace ui {

struct TimerData
{
	WeakPtr<UIObject> target;
	float timeLeft;
	int id;
};

struct EventSystem
{
	EventSystem();
	void BubblingEvent(Event& e, UIObject* tgt = nullptr, bool stopOnDisabled = false);

	void RecomputeLayout();
	float ProcessTimers(float dt);

	void Repaint(UIObject* o);
	void OnActivate(UIObject* o);
	void OnCommit(UIObject* o);
	void OnChange(UIObject* o);
	void OnIMChange(UIObject* o);
	void OnUserEvent(UIObject* o, int id, uintptr_t arg0, uintptr_t arg1);
	void SetKeyboardFocus(UIObject* o);

	bool DragCheck(Event& e, MouseButton btn);

	void CaptureMouse(UIObject* o);
	bool ReleaseMouse();
	UIObject* GetMouseCapture();

	void SetTimer(UIObject* tgt, float t, int id = 0);
	void SetDefaultCursor(DefaultCursor cur);

	UIObject* FindObjectAtPosition(Point2f pos);
	static UIObject* _FindObjectAtPosition(UIObject* root, Point2f pos);
	void MoveClickTo(UIObject* obj, MouseButton btn = MouseButton::Left);
	void _UpdateHoverObj(WeakPtr<UIObject>& curHoverObj, Point2f cursorPos, uint8_t mod, bool dragEvents);
	void _UpdateCursor(UIObject* hoverObj);
	void _UpdateTooltip(Point2f cursorPos);
	void OnMouseMove(Point2f cursorPos, uint8_t mod);
	void OnMouseButton(bool down, MouseButton which, Point2f cursorPos, uint8_t mod);
	void OnMouseScroll(Vec2f delta, u8 mod);
	bool OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint8_t mod, bool isRepeated, uint16_t numRepeats);
	bool OnKeyAction(KeyAction act, uint8_t mod, uint16_t numRepeats, bool modifier);
	void OnTextInput(uint32_t ch, uint8_t mod, uint16_t numRepeats);

	NativeWindowBase* GetNativeWindow() const;

	UIContainer* container;
	Overlays* overlays;

	WeakPtr<UIObject> hoverObj;
	WeakPtr<UIObject> dragHoverObj;
	WeakPtr<UIObject> mouseCaptureObj = nullptr;
	WeakPtr<UIObject> clickObj[5] = {};
	unsigned clickCounts[5] = {};
	uint32_t clickLastTimes[5] = {};
	Point2f clickStartPositions[5] = {};
	unsigned mouseBtnPressCounts[5] = {};
	uint32_t mouseBtnPressLastTimes[5] = {};
	unsigned mouseBtnReleaseCounts[5] = {};
	uint32_t mouseBtnReleaseLastTimes[5] = {};
	WeakPtr<UIObject> focusObj;
	WeakPtr<UIObject> lastFocusObj;
	Array<TimerData> pendingTimers;
	float width = 100;
	float height = 100;
	Point2f prevMousePos;

	bool dragEventAttempted = false;
	bool dragEventInProgress = false;
	bool wasFocusSet = false;
};

} // ui
