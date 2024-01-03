
#pragma once


namespace ui {

// https://docs.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-6.0/aa299374(v=vs.60)
// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
enum KeyScanCode
{
	// custom fold
	KSC_EXTENDED_MASK = 0x80,

	KSC_Escape = 1,

	KSC_1 = 2,
	KSC_2 = 3,
	KSC_3 = 4,
	KSC_4 = 5,
	KSC_5 = 6,
	KSC_6 = 7,
	KSC_7 = 8,
	KSC_8 = 9,
	KSC_9 = 10,
	KSC_0 = 11,

	KSC_Minus = 12,
	KSC_Equal = 13,
	KSC_Backspace = 14,

	KSC_Tab = 15,

	KSC_Q = 16,
	KSC_W = 17,
	KSC_E = 18,
	KSC_R = 19,
	KSC_T = 20,
	KSC_Y = 21,
	KSC_U = 22,
	KSC_I = 23,
	KSC_O = 24,
	KSC_P = 25,

	KSC_LeftBracket = 26,
	KSC_RightBracket = 27,

	KSC_Enter = 28,

	KSC_LeftCtrl = 29,
	KSC_RightCtrl = 29 | KSC_EXTENDED_MASK,

	KSC_A = 30,
	KSC_S = 31,
	KSC_D = 32,
	KSC_F = 33,
	KSC_G = 34,
	KSC_H = 35,
	KSC_J = 36,
	KSC_K = 37,
	KSC_L = 38,

	KSC_Semicolon = 39,
	KSC_Quote = 40,

	KSC_Backtick = 41,

	KSC_LeftShift = 42,

	KSC_Backslash = 43,

	KSC_Z = 44,
	KSC_X = 45,
	KSC_C = 46,
	KSC_V = 47,
	KSC_B = 48,
	KSC_N = 49,
	KSC_M = 50,

	KSC_Comma = 51,
	KSC_Dot = 52,
	KSC_Slash = 53,

	KSC_RightShift = 54,
	
	KSC_PrintScreen = 55 | KSC_EXTENDED_MASK,

	KSC_Alt = 56,

	KSC_Space = 57,

	KSC_CapsLock = 58,

	KSC_F1 = 59,
	KSC_F2 = 60,
	KSC_F3 = 61,
	KSC_F4 = 62,
	KSC_F5 = 63,
	KSC_F6 = 64,
	KSC_F7 = 65,
	KSC_F8 = 66,
	KSC_F9 = 67,
	KSC_F10 = 68,
	KSC_F11 = 87,
	KSC_F12 = 88,

	KSC_Pause = 69,
	KSC_NumLock = 69 | KSC_EXTENDED_MASK,
	KSC_ScrollLock = 70,

	KSC_Home = 71 | KSC_EXTENDED_MASK,
	KSC_Up = 72 | KSC_EXTENDED_MASK,
	KSC_PageUp = 73 | KSC_EXTENDED_MASK,

	// TODO 74?

	KSC_Left = 75 | KSC_EXTENDED_MASK,

	// TODO 76?

	KSC_Right = 77 | KSC_EXTENDED_MASK,

	// TODO 78?

	KSC_End = 79 | KSC_EXTENDED_MASK,
	KSC_Down = 80 | KSC_EXTENDED_MASK,
	KSC_PageDown = 81 | KSC_EXTENDED_MASK,
	KSC_Insert = 82 | KSC_EXTENDED_MASK,
	KSC_Delete = 83 | KSC_EXTENDED_MASK,

	KSC_NumpadDivide = 53 | KSC_EXTENDED_MASK,
	KSC_NumpadMultiply = 55,
	KSC_Numpad7 = 71,
	KSC_Numpad8 = 72,
	KSC_Numpad9 = 73,
	KSC_NumpadMinus = 74,
	KSC_Numpad4 = 75,
	KSC_Numpad5 = 76,
	KSC_Numpad6 = 77,
	KSC_NumpadPlus = 78,
	KSC_Numpad1 = 79,
	KSC_Numpad2 = 80,
	KSC_Numpad3 = 81,
	KSC_NumpadEnter = 28 | KSC_EXTENDED_MASK,
	KSC_Numpad0 = 82,
	KSC_NumpadDot = 83,

	KSC_AltSysRq = 84,

	KSC_LeftWindow = 91 | KSC_EXTENDED_MASK,
	KSC_RightWindow = 92 | KSC_EXTENDED_MASK,
	KSC_Menu = 93 | KSC_EXTENDED_MASK,
};

enum ModifierKeyFlags
{
	MK_LeftCtrl = 1 << 0,
	MK_LeftShift = 1 << 1,
	MK_LeftAlt = 1 << 2,
	MK_LeftWin = 1 << 3,

	MK_RightCtrl = 1 << 4,
	MK_RightShift = 1 << 5,
	MK_RightAlt = 1 << 6,
	MK_RightWin = 1 << 7,

	MK_Ctrl = 0x11 << 0,
	MK_Shift = 0x11 << 1,
	MK_Alt = 0x11 << 2,
	MK_Win = 0x11 << 3,
};

enum class MouseButton
{
	Left = 0,
	Right = 1,
	Middle = 2,
	X1 = 3,
	X2 = 4,
};

} // ui
