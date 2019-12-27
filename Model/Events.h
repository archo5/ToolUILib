
#pragma once
#include <vector>


namespace ui {
class NativeWindowBase;
class Node;
} // ui

class UIObject;
class UIElement;
class UIContainer;
class UIEventSystem;


enum class UIEventType
{
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
};

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

class UIEventSystem
{
public:
	void BubblingEvent(UIEvent& e, UIObject* tgt = nullptr);

	void RecomputeLayout();
	void ProcessTimers(float dt);

	void Repaint(UIElement* e);
	void OnDestroy(UIObject* o);
	void OnCommit(UIElement* e);
	void OnChange(UIElement* e);
	void OnChange(ui::Node* n);
	void SetKeyboardFocus(UIObject* o);
	void SetTimer(UIElement* tgt, float t, int id = 0);

	UIObject* FindObjectAtPosition(float x, float y);
	void OnMouseMove(UIMouseCoord x, UIMouseCoord y);
	void OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y);
	void OnMouseScroll(UIMouseCoord dx, UIMouseCoord dy);
	void OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint16_t numRepeats);
	void OnKeyAction(UIKeyAction act, uint16_t numRepeats);
	void OnTextInput(uint32_t ch, uint16_t numRepeats);

	ui::NativeWindowBase* GetNativeWindow() const;

	UIContainer* container;

	UIObject* hoverObj = nullptr;
	UIObject* clickObj[5] = { nullptr };
	UIObject* focusObj = nullptr;
	std::vector<UITimerData> pendingTimers;
	float width = 100;
	float height = 100;
	float prevMouseX = 0;
	float prevMouseY = 0;
};
