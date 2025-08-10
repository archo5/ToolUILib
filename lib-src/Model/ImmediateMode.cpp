
#include "ImmediateMode.h"

#include "Controls.h"
#include "Graphics.h"
#include "Layout.h"
#include "System.h"

#include "../Elements/Textbox.h"


namespace ui {


struct ModInitListReverseIter
{
	const CRef<Modifier>* ptr;

	UI_FORCEINLINE ModInitListReverseIter(const CRef<Modifier>* p) : ptr(p) {}
	UI_FORCEINLINE const CRef<Modifier>& operator * () const { return *ptr; }
	UI_FORCEINLINE bool operator != (const ModInitListReverseIter& o) const { return ptr != o.ptr; }
	UI_FORCEINLINE void operator ++ () { ptr--; }
};
struct ModInitListReverseRange
{
	ModInitListReverseIter _begin, _end;

	UI_FORCEINLINE ModInitListReverseRange(const ModInitList& mil) : _begin(mil.end() - 1), _end(mil.begin() - 1) {}
	UI_FORCEINLINE ModInitListReverseIter begin() const { return _begin; }
	UI_FORCEINLINE ModInitListReverseIter end() const { return _end; }
};
inline ModInitListReverseRange ReverseIterate(const ModInitList& iterable) { return { iterable }; }


static bool g_imEnabled = true;

bool imGetEnabled()
{
	return g_imEnabled;
}

bool imSetEnabled(bool newValue)
{
	bool old = g_imEnabled;
	g_imEnabled = newValue;
	return old;
}


void CheckboxStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	if (text.NotEmpty())
	{
		Push<StackExpandLTRLayoutElement>();
		Make<CheckboxIcon>();
		auto& lbl = MakeWithText<LabelFrame>(text);
		if (!imGetEnabled())
			lbl.flags |= UIObject_IsDisabled;
		Pop();
	}
	else
		Make<CheckboxIcon>();
}

void RadioButtonStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	if (text.NotEmpty())
	{
		Push<StackExpandLTRLayoutElement>();
		Make<RadioButtonIcon>();
		auto& lbl = MakeWithText<LabelFrame>(text);
		if (!imGetEnabled())
			lbl.flags |= UIObject_IsDisabled;
		Pop();
	}
	else
		Make<RadioButtonIcon>();
}

void ButtonStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	MakeWithText<StateButtonSkin>(text);
}

void TreeStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	if (text.NotEmpty())
	{
		Push<StackExpandLTRLayoutElement>();
		Make<TreeExpandIcon>();
		auto& lbl = MakeWithText<LabelFrame>(text);
		if (!imGetEnabled())
			lbl.flags |= UIObject_IsDisabled;
		Pop();
	}
	else
		Make<TreeExpandIcon>();
}


void StdText(StringView text, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& ctrl = Push<LabelFrame>();
	ui::Text(text);
	Pop();

	for (auto& mod : mods)
		mod->Apply(&ctrl);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();
}


imCtrlInfo imButton(UIObject& obj, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& btn = Push<ui::Button>();
	Add(obj);
	Pop();

	btn.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		btn.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&btn);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	if (btn.flags & UIObject_AfterIMEdit)
	{
		btn._OnIMChange();
		btn.flags &= ~UIObject_AfterIMEdit;
	}
	bool clicked = false;
	if (btn.flags & UIObject_IsEdited)
	{
		clicked = true;
		btn.flags &= ~UIObject_IsEdited;
		btn.flags |= UIObject_AfterIMEdit;
		btn.RebuildContainer();
	}

	return { clicked, &btn };
}

imCtrlInfo imButton(StringView text, ModInitList mods)
{
	return imButton(NewText(text), mods);
}

imCtrlInfo imButton(DefaultIconStyle icon, ModInitList mods)
{
	return imButton(New<IconElement>().SetDefaultStyle(icon), mods);
}

imCtrlInfo imSelectable(UIObject& obj, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& btn = Push<ui::Selectable>();
	Add(obj);
	Pop();

	btn.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		btn.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&btn);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	if (btn.flags & UIObject_AfterIMEdit)
	{
		btn._OnIMChange();
		btn.flags &= ~UIObject_AfterIMEdit;
	}
	bool clicked = false;
	if (btn.flags & UIObject_IsEdited)
	{
		clicked = true;
		btn.flags &= ~UIObject_IsEdited;
		btn.flags |= UIObject_AfterIMEdit;
		btn.RebuildContainer();
	}

	return { clicked, &btn };
}

imCtrlInfo imSelectable(StringView text, ModInitList mods)
{
	return imSelectable(NewText(text), mods);
}

imCtrlInfo imCheckboxExtRaw(u8 val, StringView text, ModInitList mods, const IStateToggleSkin& skin)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& cb = Push<StateToggle>();
	skin.BuildContents(cb, text, val);
	Pop();

	cb.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		cb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&cb);
	cb.InitReadOnly(val);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	if (cb.flags & UIObject_AfterIMEdit)
	{
		cb._OnIMChange();
		cb.flags &= ~UIObject_AfterIMEdit;
	}
	bool edited = false;
	if (cb.flags & UIObject_IsEdited)
	{
		edited = true;
		cb.flags &= ~UIObject_IsEdited;
		cb.flags |= UIObject_AfterIMEdit;
		cb.RebuildContainer();
	}

	return { edited, &cb };
}

imCtrlInfo imEditBool(bool& val, StringView text, ModInitList mods, const IStateToggleSkin& skin)
{
	imCtrlInfo ci = imCheckboxRaw(val, text, mods, skin);
	if (ci)
		val = !val;
	return ci;
}

imCtrlInfo imRadioButtonRaw(bool val, StringView text, ModInitList mods, const IStateToggleSkin& skin)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& rb = Push<StateToggle>();
	skin.BuildContents(rb, text, val);
	Pop();

	rb.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		rb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&rb);
	rb.InitReadOnly(val);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	if (rb.flags & UIObject_AfterIMEdit)
	{
		rb._OnIMChange();
		rb.flags &= ~UIObject_AfterIMEdit;
	}
	bool edited = false;
	if (rb.flags & UIObject_IsEdited)
	{
		edited = true;
		rb.flags &= ~UIObject_IsEdited;
		rb.flags |= UIObject_AfterIMEdit;
		rb.RebuildContainer();
	}

	return { edited, &rb };
}

float DragConfig::GetSpeed(uint8_t modifierKeys) const
{
	if (modifierKeys & slowdownKey)
		return slowdownSpeed;
	if (modifierKeys & boostKey)
		return boostSpeed;
	return speed;
}

float DragConfig::GetSnap(uint8_t modifierKeys) const
{
	if (modifierKeys & snapOffKey)
		return 0;
	if (modifierKeys & slowdownKey)
		return slowdownSnap;
	if (modifierKeys & boostKey)
		return boostSnap;
	return snap;
}


struct NumFmtBox
{
	char printFmt[8];

	NumFmtBox(const char* f)
	{
		strncpy(printFmt, f, 6);
		printFmt[7] = '\0';
	}
};

const char* RemoveNegZero(const char* str)
{
	return strncmp(str, "-0", 3) == 0 ? "0" : str;
}

template <class T> struct MakeSigned {};
template <> struct MakeSigned<int> { using type = int; };
template <> struct MakeSigned<unsigned> { using type = int; };
template <> struct MakeSigned<int64_t> { using type = int64_t; };
template <> struct MakeSigned<uint64_t> { using type = int64_t; };
template <> struct MakeSigned<float> { using type = float; };

template <class T> float GetMinSnap(T) { return 1; }
float GetMinSnap(float v) { return max(nextafterf(v, INFINITY) - v, FLT_EPSILON); }
float GetMinSnap(double v) { return max(nextafter(v, INFINITY) - v, DBL_EPSILON); }

static inline bool fstrhas(const char* s, char c)
{
	return strchr(s, c) || strchr(s, ToUpper(c));
}
template <class T> const char* GetScanFormat(const char* fmt) { return ""; }
template <> const char* GetScanFormat<signed int>(const char* fmt) { return fstrhas(fmt, 'x') ? "%x" : fstrhas(fmt, 'o') ? "%o" : "%d"; }
template <> const char* GetScanFormat<unsigned int>(const char* fmt) { return fstrhas(fmt, 'x') ? "%x" : fstrhas(fmt, 'o') ? "%o" : "%u"; }
template <> const char* GetScanFormat<signed long>(const char* fmt) { return fstrhas(fmt, 'x') ? "%lx" : fstrhas(fmt, 'o') ? "%lo" : "%ld"; }
template <> const char* GetScanFormat<unsigned long>(const char* fmt) { return fstrhas(fmt, 'x') ? "%lx" : fstrhas(fmt, 'o') ? "%lo" : "%lu"; }
template <> const char* GetScanFormat<signed long long>(const char* fmt) { return fstrhas(fmt, 'x') ? "%llx" : fstrhas(fmt, 'o') ? "%llo" : "%lld"; }
template <> const char* GetScanFormat<unsigned long long>(const char* fmt) { return fstrhas(fmt, 'x') ? "%llx" : fstrhas(fmt, 'o') ? "%llo" : "%llu"; }
template <> const char* GetScanFormat<float>(const char* fmt) { return "%g"; }
template <> const char* GetScanFormat<double>(const char* fmt) { return "%G"; }

template <class TNum>
struct NumberTextbox : Textbox
{
	TNum numberValue = 0;
	float accumulator = 0;
	int edited = 0; // 1=text, -1=numberValue
};

template <class TNum> bool EditNumber(UIObject* dragObj, TNum& val, ModInitList mods, const DragConfig& cfg, Range<TNum> range, const char* fmt)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& tb = Make<NumberTextbox<TNum>>();
	if (!imGetEnabled())
		tb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&tb);

	NumFmtBox fb(fmt);

	bool edited = false;
	if (tb.edited)
	{
		decltype(val + 0) tmp;

		if (tb.edited > 0)
			sscanf(tb.GetText().c_str(), GetScanFormat<TNum>(fmt), &tmp);
		else
			tmp = tb.numberValue;

		if (tmp == 0)
			tmp = 0;
		if (tmp > range.max)
			tmp = range.max;
		if (tmp < range.min)
			tmp = range.min;
		val = tmp;
		tb.edited = 0;
		edited = true;
		tb._OnIMChange();
		tb.RebuildContainer();
	}

	char buf[1024];
	snprintf(buf, 1024, fb.printFmt, val);
	tb.numberValue = val;
	tb.SetText(RemoveNegZero(buf));

	if (dragObj)
	{
		dragObj->SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
		dragObj->HandleEvent() = [val, cfg = cfg, range, &tb, fb](Event& e)
		{
			if (tb.IsInputDisabled() || e.target->IsChildOrSame(&tb))
				return;
			if (e.type == EventType::MouseMove && e.target->IsPressed() && e.delta.x != 0)
			{
				if (tb.IsFocused())
					e.context->SetKeyboardFocus(nullptr);

				float speed = cfg.GetSpeed(e.GetModifierKeys());
				float snap = cfg.GetSnap(e.GetModifierKeys());
				if (cfg.relativeSpeed)
				{
					speed *= fabsf(val);
					snap = 0;
				}
				float minSnap = GetMinSnap(val);
				if (snap < minSnap)
					snap = minSnap;

				float diff = e.delta.x * speed;
				TNum nv = val;

				if (fabsf(tb.accumulator) >= snap)
				{
					// previous snap value was bigger
					tb.accumulator = 0;
				}
				tb.accumulator += diff;
				if (fabsf(tb.accumulator) >= snap)
				{
					nv += trunc(tb.accumulator / snap) * snap;
					tb.accumulator = fmodf(tb.accumulator, snap);
				}

				if (nv > range.max || (diff > 0 && nv < val))
					nv = range.max;
				if (nv < range.min || (diff < 0 && nv > val))
					nv = range.min;

				tb.numberValue = nv;
				char buf[1024];
				snprintf(buf, 1024, fb.printFmt, nv);
				tb.SetText(RemoveNegZero(buf));
				tb.edited = -1;

				e.context->OnCommit(e.target);
				tb.RebuildContainer();
			}
			if (e.type == EventType::SetCursor)
			{
				e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
				e.StopPropagation();
			}
		};
	}
	tb.HandleEvent(EventType::Commit) = [&tb](Event& e)
	{
		tb.edited = 1;
		tb.RebuildContainer();
	};

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return edited;
}

bool imEditInt(UIObject* dragObj, int& val, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool imEditInt(UIObject* dragObj, unsigned& val, ModInitList mods, const DragConfig& cfg, Range<unsigned> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool imEditInt(UIObject* dragObj, int64_t& val, ModInitList mods, const DragConfig& cfg, Range<int64_t> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool imEditInt(UIObject* dragObj, uint64_t& val, ModInitList mods, const DragConfig& cfg, Range<uint64_t> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool imEditFloat(UIObject* dragObj, float& val, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}


static imCtrlInfo imEditStringImpl(bool multiline, const IBufferRW& textRW, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& tb = Make<Textbox>();
	if (multiline)
		tb.SetMultiline(true);
	if (!imGetEnabled())
		tb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&tb);
	bool changed = false;
	if (tb.flags & UIObject_IsEdited)
	{
		textRW.Assign(tb.GetText());
		tb.flags &= ~UIObject_IsEdited;
		tb._OnIMChange();
		tb.RebuildContainer();
		changed = true;
	}
	else // text can be invalidated if retfn is called
		tb.SetText(textRW.Read());
	tb.HandleEvent(EventType::Change) = [&tb](Event&)
	{
		tb.flags |= UIObject_IsEdited;
		tb.RebuildContainer();
	};

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return { changed, &tb };
}

imCtrlInfo imEditString(std::string& text, ModInitList mods)
{
	return imEditStringImpl(false, StdStringRW(text), mods);
}

imCtrlInfo imEditString(const IBufferRW& textRW, ModInitList mods)
{
	return imEditStringImpl(false, textRW, mods);
}

imCtrlInfo imEditStringMultiline(const IBufferRW& textRW, ModInitList mods)
{
	return imEditStringImpl(true, textRW, mods);
}


imCtrlInfo imEditColor(Color4f& val, bool delayed, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& ced = delayed
		? (IColorEdit&)Make<ColorEdit>()
		: (IColorEdit&)Make<ColorEditRT>();
	if (!imGetEnabled())
		ced.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&ced);
	bool changed = false;
	if (ced.flags & UIObject_IsEdited)
	{
		val = ced.GetColor().GetRGBA();
		ced.flags &= ~UIObject_IsEdited;
		ced._OnIMChange();
		ced.RebuildContainer();
		changed = true;
	}
	else
		ced.SetColorAny(val);
	ced.HandleEvent(EventType::Change) = [&ced](Event&)
	{
		ced.flags |= UIObject_IsEdited;
		ced.RebuildContainer();
	};

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return { changed, &ced };
}

imCtrlInfo imEditColor(Color4b& val, bool delayed, ModInitList mods)
{
	Color4f tmp = val;
	imCtrlInfo ci = imEditColor(tmp, delayed, mods);
	if (ci)
		val = tmp;
	return ci;
}

namespace imm {

const char* XY[] = { "\bX", "\bY", nullptr };
const char* XYZ[] = { "\bX", "\bY", "\bZ", nullptr };
const char* XYZW[] = { "\bX", "\bY", "\bZ", "\bW", nullptr };
const char* RGBA[] = { "\bR", "\bG", "\bB", "\bA", nullptr };
const char* MinMax[] = { "\bMin", "\bMax", nullptr };
const char* WidthHeight[] = { "\bWidth", "\bHeight", nullptr };

bool EditIntVec(int* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	bool any = false;
	for (const char** plabel = axes; *plabel; plabel++)
	{
		any |= PropEditInt(*plabel, *val++, mods, cfg, range, fmt);
	}
	return any;
}

bool EditFloatVec(float* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	bool any = false;
	for (const char** plabel = axes; *plabel; plabel++)
	{
		any |= PropEditFloat(*plabel, *val++, mods, cfg, range, fmt);
	}
	return any;
}


bool PropEditInt(const char* label, int& val, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	return imEditInt(imLabel(label).label, val, mods, cfg, range, fmt);
}

bool PropEditInt(const char* label, unsigned& val, ModInitList mods, const DragConfig& cfg, Range<unsigned> range, const char* fmt)
{
	return imEditInt(imLabel(label).label, val, mods, cfg, range, fmt);
}

bool PropEditInt(const char* label, int64_t& val, ModInitList mods, const DragConfig& cfg, Range<int64_t> range, const char* fmt)
{
	return imEditInt(imLabel(label).label, val, mods, cfg, range, fmt);
}

bool PropEditInt(const char* label, uint64_t& val, ModInitList mods, const DragConfig& cfg, Range<uint64_t> range, const char* fmt)
{
	return imEditInt(imLabel(label).label, val, mods, cfg, range, fmt);
}

bool PropEditFloat(const char* label, float& val, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	return imEditFloat(imLabel(label).label, val, mods, cfg, range, fmt);
}

bool PropEditIntVec(const char* label, int* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	return imLabel(label), EditIntVec(val, axes, mods, cfg, range, fmt);
}

bool PropEditFloatVec(const char* label, float* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	return imLabel(label), EditFloatVec(val, axes, mods, cfg, range, fmt);
}

} // imm
} // ui
