
#pragma once

#include "../Core/Math.h"

#include "Keyboard.h"

#include <vector>
#include <functional>


namespace ui {

struct NativeWindowBase;
struct Buildable;
struct Overlays;
extern struct DataCategoryTag DCT_MouseMoved[1];

struct UIObject;
struct UIElement;
struct UIContainer;
struct EventSystem;


enum class EventType
{
	Any,

	Click,
	Activate, // left click or space with focus (except input)
	ContextMenu,
	Change,
	Commit,
	IMChange,
	BackgroundClick,
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
inline EventType UserEvent(int id)
{
	return EventType(int(EventType::User) + id);
}

enum class MouseButton
{
	Left = 0,
	Right = 1,
	Middle = 2,
	X1 = 3,
	X2 = 4,
};

enum class KeyAction
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

using MouseCoord = float;

struct Event
{
	EventSystem* context;
	UIObject* target;
	UIObject* current;

	EventType type;

	Point2f position;
	Point2f topLevelPosition;
	Vec2f delta;

	uint32_t longCode = 0;
	uint8_t shortCode = 0;
	uint8_t modifiers = 0;
	bool down = false;
	bool _stopPropagation = false;
	uint16_t numRepeats = 0;
	uintptr_t arg0 = 0, arg1 = 0;

	Event(EventSystem* ctx, UIObject* tgt, EventType ty) : context(ctx), target(tgt), current(tgt), type(ty) {}
	Buildable* GetTargetBuildable() const;
	MouseButton GetButton() const;
	uint8_t GetModifierKeys() const { return modifiers; }
	KeyAction GetKeyAction() const;
	bool GetKeyActionModifier() const;
	uint32_t GetUTF32Char() const;
	bool GetUTF8Text(char out[5]) const;

	bool IsPropagationStopped() const { return _stopPropagation; }
	void StopPropagation() { _stopPropagation = true; }

	bool IsCtrlPressed() const { return (GetModifierKeys() & MK_Ctrl) != 0; }
	bool IsShiftPressed() const { return (GetModifierKeys() & MK_Shift) != 0; }
	bool IsAltPressed() const { return (GetModifierKeys() & MK_Alt) != 0; }
	bool IsWinPressed() const { return (GetModifierKeys() & MK_Win) != 0; }
};

enum class SubUIDragState
{
	None,
	Start,
	Move,
	Stop,
};

template <class T> struct SubUIValueHelper
{
	static constexpr T GetNullValue()
	{
		return ~(T)0;
	}
};

template <class T> struct SubUI
{
	static constexpr T NoValue = SubUIValueHelper<T>::GetNullValue();

	bool IsAnyHovered() const { return _hovered != NoValue; }
	bool IsAnyPressed() const { return _pressed != NoValue; }
	bool IsHovered(T id) const { return _hovered == id; }
	bool IsPressed(T id) const { return _pressed == id; }

	void InitOnEvent(Event& e)
	{
		if (e.type == EventType::MouseMove)
			_hovered = NoValue;
		if (e.type == EventType::MouseLeave)
			_hovered = NoValue;
	}
	void FinalizeOnEvent(Event& e)
	{
		if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
			_pressed = NoValue;
	}

	bool ButtonOnEvent(T id, const AABB2f& r, Event& e)
	{
		if (e.type == EventType::MouseMove)
		{
			if (r.Contains(e.position))
				_hovered = id;
		}
		else if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left && _hovered == id)
			_pressed = id;
		else if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
			if (_pressed == id && _hovered == id)
				return true;
		return false;
	}

	SubUIDragState DragOnEvent(T id, const AABB2f& r, Event& e)
	{
		if (e.type == EventType::MouseMove)
		{
			if (r.Contains(e.position))
				_hovered = id;
			if (_pressed == id)
				return SubUIDragState::Move;
		}
		else if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left && _hovered == id)
		{
			_pressed = id;
			return SubUIDragState::Start;
		}
		else if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
		{
			_pressed = NoValue;
			return SubUIDragState::Stop;
		}
		else if (e.type == EventType::Click && e.GetButton() == MouseButton::Left)
		{
			if (_pressed != NoValue)
				e.StopPropagation(); // eat invalid event
		}
		return SubUIDragState::None;
	}
	SubUIDragState GlobalDragOnEvent(T id, Event& e)
	{
		if (e.type == EventType::MouseMove)
		{
			if (id != NoValue)
				_hovered = id;
			if (_pressed != NoValue)
				return SubUIDragState::Move;
		}
		else if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left && _hovered != NoValue)
		{
			_pressed = _hovered;
			return SubUIDragState::Start;
		}
		else if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
		{
			_pressed = NoValue;
			return SubUIDragState::Stop;
		}
		else if (e.type == EventType::Click && e.GetButton() == MouseButton::Left)
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
	virtual bool ShouldBuild() { return true; }
	virtual void Build();

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
	using BuildFunc = std::function<void()>;

	static void Set(const BuildFunc& f);
	static void Unset();
	static bool IsSet();
	static bool WasChanged();
	static void ClearChangedFlag();
	static void Build();
};

struct ContextMenu
{
	static struct MenuItemCollection& Get();
};

} // ui
