
#pragma once
#include <vector>
#include "../Core/Math.h"


namespace ui {
struct NativeWindowBase;
struct Node;
} // ui

using UIRect = AABB<float>;

struct UIObject;
struct UIElement;
struct UIContainer;
struct UIEventSystem;


enum class UIEventType
{
	Any,

	Click,
	Activate, // left click or space with focus (except input)
	Change,
	Commit,
	Paint,
	Timer,

	MouseEnter,
	MouseLeave,
	MouseMove,
	MouseScroll,
	SetCursor,
	ButtonDown,
	ButtonUp,
	DragStart, // mouse move with left button down (target: original element)
	DragEnd, // esc / release left button (target: original element)
	DragEnter, // points at element under mouse
	DragLeave,
	DragMove,
	DragDrop, // target: element under mouse

	GotFocus,
	LostFocus,
	KeyDown,
	KeyUp,
	KeyAction,
	TextInput,
	SelectionChange,
};

enum class UIMouseButton
{
	Left = 0,
	Right = 1,
	Middle = 2,
	X1 = 3,
	X2 = 4,
};

enum class UIKeyAction
{
	Activate,
	Enter,
	Backspace,
	Delete,

	Left,
	Right,
	Up,
	Down,
	Home,
	End,
	PageUp,
	PageDown,
	FocusNext,
	FocusPrev,

	Cut,
	Copy,
	Paste,
	SelectAll,

	Inspect,
};

namespace ui {
enum class DefaultCursor
{
	None,
	Default,
	Pointer,
	Crosshair,
	Help,
	Text,
	NotAllowed,
	Progress,
	Wait,
	ResizeAll,
	ResizeHorizontal,
	ResizeVertical,
	ResizeNESW,
	ResizeNWSE,
	ResizeCol,
	ResizeRow,

	_COUNT,
};
} // ui

using UIMouseCoord = float;

struct UIEvent
{
	UIEventSystem* context;
	UIObject* target;
	UIObject* current;

	UIEventType type;

	UIMouseCoord x = 0;
	UIMouseCoord y = 0;
	UIMouseCoord dx = 0;
	UIMouseCoord dy = 0;

	uint32_t longCode = 0;
	uint8_t shortCode = 0;
	bool down = false;
	bool handled = false;
	uint16_t numRepeats = 0;

	UIEvent(UIEventSystem* ctx, UIObject* tgt, UIEventType ty) : context(ctx), target(tgt), current(tgt), type(ty) {}
	ui::Node* GetTargetNode() const;
	UIMouseButton GetButton() const;
	UIKeyAction GetKeyAction() const;
	uint32_t GetUTF32Char() const;
	bool GetUTF8Text(char out[5]) const;
};

struct UITimerData
{
	UIObject* target;
	float timeLeft;
	int id;
};

struct UIEventSystem
{
	UIEventSystem();
	void BubblingEvent(UIEvent& e, UIObject* tgt = nullptr, bool stopOnDisabled = false);

	void RecomputeLayout();
	void ProcessTimers(float dt);

	void Repaint(UIElement* e);
	void OnDestroy(UIObject* o);
	void OnCommit(UIObject* e);
	void OnChange(UIObject* e);
	void OnChange(ui::Node* n);
	void SetKeyboardFocus(UIObject* o);
	void SetTimer(UIElement* tgt, float t, int id = 0);
	void SetDefaultCursor(ui::DefaultCursor cur);

	UIObject* FindObjectAtPosition(float x, float y);
	void _UpdateHoverObj(UIObject*& curHoverObj, UIMouseCoord x, UIMouseCoord y, bool dragEvents);
	void _UpdateCursor(UIObject* hoverObj);
	void OnMouseMove(UIMouseCoord x, UIMouseCoord y);
	void OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y);
	void OnMouseScroll(UIMouseCoord dx, UIMouseCoord dy);
	void OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint16_t numRepeats);
	void OnKeyAction(UIKeyAction act, uint16_t numRepeats);
	void OnTextInput(uint32_t ch, uint16_t numRepeats);

	ui::NativeWindowBase* GetNativeWindow() const;

	UIContainer* container;

	UIObject* hoverObj = nullptr;
	UIObject* dragHoverObj = nullptr;
	UIObject* clickObj[5] = {};
	unsigned clickCounts[5] = {};
	uint32_t clickLastTimes[5] = {};
	UIObject* focusObj = nullptr;
	UIObject* lastFocusObj = nullptr;
	std::vector<UITimerData> pendingTimers;
	float width = 100;
	float height = 100;
	float prevMouseX = 0;
	float prevMouseY = 0;

	bool dragEventAttempted = false;
	bool dragEventInProgress = false;
};

namespace ui {

enum class SubUIDragState
{
	None,
	Start,
	Move,
	Stop,
};

template <class T> struct SubUI
{
	static constexpr T NoValue = ~(T)0;

	bool IsAnyHovered() const { return _hovered != NoValue; }
	bool IsAnyPressed() const { return _pressed != NoValue; }
	bool IsHovered(T id) const { return _hovered == id; }
	bool IsPressed(T id) const { return _pressed == id; }

	void InitOnEvent(UIEvent& e)
	{
		if (e.type == UIEventType::MouseMove)
			_hovered = NoValue;
		if (e.type == UIEventType::MouseLeave)
			_hovered = NoValue;
	}

	bool ButtonOnEvent(T id, const UIRect& r, UIEvent& e)
	{
		if (e.type == UIEventType::MouseMove)
		{
			if (r.Contains(e.x, e.y))
				_hovered = id;
		}
		else if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && _hovered == id)
			_pressed = id;
		else if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
			_pressed = NoValue;
		else if (e.type == UIEventType::Activate)
			if (_pressed == id && _hovered == id)
				return true;
		return false;
	}

	SubUIDragState DragOnEvent(T id, const UIRect& r, UIEvent& e)
	{
		if (e.type == UIEventType::MouseMove)
		{
			if (r.Contains(e.x, e.y))
				_hovered = id;
			if (_pressed == id)
				return SubUIDragState::Move;
		}
		else if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && _hovered == id)
		{
			_pressed = id;
			return SubUIDragState::Start;
		}
		else if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
		{
			_pressed = NoValue;
			return SubUIDragState::Stop;
		}
		return SubUIDragState::None;
	}

	void DragStart(T id)
	{
		_pressed = id;
	}

	T _hovered = NoValue;
	T _pressed = NoValue;
};

class DragDropData
{
public:
	DragDropData() {}
	DragDropData(const std::string& t) : type(t) {}
	virtual ~DragDropData() {}

	std::string type;
};

class DragDropText : public DragDropData
{
public:
	DragDropText();
	DragDropText(const std::string& ty, const std::string& txt) : DragDropData(ty), text(txt) {}

	std::string text;
};

class DragDrop
{
public:
	static void SetData(DragDropData* data);
	static DragDropData* GetData(const char* type = nullptr);
};

} // ui
