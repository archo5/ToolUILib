
#pragma once
#include <vector>
#include <functional>
#include "../Core/Math.h"


namespace ui {
struct NativeWindowBase;
struct Node;
struct Overlays;
extern struct DataCategoryTag DCT_MouseMoved[1];
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
	ContextMenu,
	Change,
	Commit,
	Paint,
	Timer,
	Tooltip,

	MouseEnter,
	MouseLeave,
	MouseMove,
	MouseScroll,
	MouseCaptureChanged,
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

	User = 256,
};
inline UIEventType UserEvent(int id)
{
	return UIEventType(int(UIEventType::User) + id);
}

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
	ActivateDown,
	ActivateUp,
	Enter,

	Delete,
	DelPrevLetter,
	DelNextLetter,
	DelPrevWord,
	DelNextWord,

	// mod: keep start of selection
	// {
	PrevLetter,
	NextLetter,
	PrevWord,
	NextWord,

	GoToLineStart,
	GoToLineEnd,
	GoToStart,
	GoToEnd,

	PageUp,
	PageDown,

	Up,
	Down,
	// }

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
	bool _stopPropagation = false;
	uint16_t numRepeats = 0;
	uintptr_t arg0 = 0, arg1 = 0;

	UIEvent(UIEventSystem* ctx, UIObject* tgt, UIEventType ty) : context(ctx), target(tgt), current(tgt), type(ty) {}
	ui::Node* GetTargetNode() const;
	UIMouseButton GetButton() const;
	UIKeyAction GetKeyAction() const;
	bool GetKeyActionModifier() const;
	uint32_t GetUTF32Char() const;
	bool GetUTF8Text(char out[5]) const;

	bool IsPropagationStopped() const { return _stopPropagation; }
	void StopPropagation() { _stopPropagation = true; }
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
	float ProcessTimers(float dt);

	void Repaint(UIObject* o);
	void OnDestroy(UIObject* o);
	void OnActivate(UIObject* o);
	void OnCommit(UIObject* o);
	void OnChange(UIObject* o);
	void OnUserEvent(UIObject* o, int id, uintptr_t arg0, uintptr_t arg1);
	void SetKeyboardFocus(UIObject* o);

	bool DragCheck(UIEvent& e, UIMouseButton btn);

	void CaptureMouse(UIObject* o);
	bool ReleaseMouse();
	UIObject* GetMouseCapture();

	void SetTimer(UIObject* tgt, float t, int id = 0);
	void SetDefaultCursor(ui::DefaultCursor cur);

	UIObject* FindObjectAtPosition(float x, float y);
	static UIObject* _FindObjectAtPosition(UIObject* root, float x, float y);
	void MoveClickTo(UIObject* obj, UIMouseButton btn = UIMouseButton::Left);
	void _UpdateHoverObj(UIObject*& curHoverObj, UIMouseCoord x, UIMouseCoord y, bool dragEvents);
	void _UpdateCursor(UIObject* hoverObj);
	void _UpdateTooltip();
	void OnMouseMove(UIMouseCoord x, UIMouseCoord y);
	void OnMouseButton(bool down, UIMouseButton which, UIMouseCoord x, UIMouseCoord y);
	void OnMouseScroll(UIMouseCoord dx, UIMouseCoord dy);
	void OnKeyInput(bool down, uint32_t vk, uint8_t pk, uint16_t numRepeats);
	void OnKeyAction(UIKeyAction act, uint16_t numRepeats, bool modifier);
	void OnTextInput(uint32_t ch, uint16_t numRepeats);

	ui::NativeWindowBase* GetNativeWindow() const;

	UIContainer* container;
	ui::Overlays* overlays;

	UIObject* hoverObj = nullptr;
	UIObject* dragHoverObj = nullptr;
	UIObject* mouseCaptureObj = nullptr;
	UIObject* tooltipObj = nullptr;
	UIObject* clickObj[5] = {};
	unsigned clickCounts[5] = {};
	uint32_t clickLastTimes[5] = {};
	Point<float> clickStartPositions[5] = {};
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
		else if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Left)
		{
			if (_pressed != NoValue)
				e.StopPropagation(); // eat invalid event
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

struct DragDropData
{
	DragDropData() {}
	DragDropData(const std::string& t) : type(t) {}
	virtual ~DragDropData() {}
	virtual bool ShouldRender() { return true; }
	virtual void Render(UIContainer* ctx);

	std::string type;
};

struct DragDropText : DragDropData
{
	DragDropText(const std::string& ty, const std::string& txt) : DragDropData(ty), text(txt) {}

	std::string text;
};

struct DragDropFiles : DragDropData
{
	static constexpr const char* NAME = "files";
	DragDropFiles() : DragDropData(NAME) {}

	std::vector<std::string> paths;
};

extern struct DataCategoryTag DCT_DragDropDataChanged[1];
struct DragDrop
{
	static void SetData(DragDropData* data);
	static DragDropData* GetData(const char* type = nullptr);

	template <class T>
	static T* GetData() { return static_cast<T*>(GetData(T::NAME)); }
};

extern struct DataCategoryTag DCT_TooltipChanged[1];
struct Tooltip
{
	using RenderFunc = std::function<void(UIContainer*)>;

	static void Set(const RenderFunc& f);
	static void Unset();
	static bool IsSet();
	static void Render(UIContainer* ctx);
};

struct ContextMenu
{
	static struct MenuItemCollection& Get();
};

} // ui
