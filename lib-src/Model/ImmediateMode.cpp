
#include "ImmediateMode.h"

#include "Controls.h"
#include "Graphics.h"
#include "Layout.h"
#include "System.h"

#include "../Elements/Textbox.h"


namespace ui {
namespace imm {

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


static bool g_enabled = true;

bool GetEnabled()
{
	return g_enabled;
}

bool SetEnabled(bool newValue)
{
	bool old = g_enabled;
	g_enabled = newValue;
	return old;
}


void CheckboxStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	if (text.NotEmpty())
	{
		Push<StackExpandLTRLayoutElement>();
		Make<CheckboxIcon>();
		auto& lbl = MakeWithText<LabelFrame>(text);
		if (!GetEnabled())
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
		if (!GetEnabled())
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
		if (!GetEnabled())
			lbl.flags |= UIObject_IsDisabled;
		Pop();
	}
	else
		Make<TreeExpandIcon>();
}

bool Button(UIObject& obj, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& btn = Push<ui::Button>();
	for (auto& mod : mods)
		mod->OnBeforeContent();
	Add(obj);
	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterContent();
	Pop();

	btn.flags |= UIObject_DB_IMEdit;
	if (!GetEnabled())
		btn.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&btn);
	bool clicked = false;
	if (btn.flags & UIObject_IsEdited)
	{
		clicked = true;
		btn.flags &= ~UIObject_IsEdited;
		btn._OnIMChange();
	}

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return clicked;
}

bool Button(StringView text, ModInitList mods)
{
	return Button(NewText(text), mods);
}

bool Button(DefaultIconStyle icon, ModInitList mods)
{
	return Button(New<IconElement>().SetDefaultStyle(icon), mods);
}

bool CheckboxExtRaw(u8 val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& cb = Push<StateToggle>();
	skin.BuildContents(cb, text ? text : StringView(), val);
	Pop();

	cb.flags |= UIObject_DB_IMEdit;
	if (!GetEnabled())
		cb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&cb);
	bool edited = false;
	if (cb.flags & UIObject_IsEdited)
	{
		cb.flags &= ~UIObject_IsEdited;
		edited = true;
		cb._OnIMChange();
	}
	cb.InitReadOnly(val);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return edited;
}

bool EditBool(bool& val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	if (CheckboxRaw(val, text, mods, skin))
	{
		val = !val;
		return true;
	}
	return false;
}

bool RadioButtonRaw(bool val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& rb = Push<StateToggle>();
	skin.BuildContents(rb, text ? text : StringView(), val);
	Pop();

	rb.flags |= UIObject_DB_IMEdit;
	if (!GetEnabled())
		rb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&rb);
	bool edited = false;
	if (rb.flags & UIObject_IsEdited)
	{
		rb.flags &= ~UIObject_IsEdited;
		edited = true;
		rb._OnIMChange();
	}
	rb.InitReadOnly(val);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return edited;
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
	if (!GetEnabled())
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

bool EditInt(UIObject* dragObj, int& val, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool EditInt(UIObject* dragObj, unsigned& val, ModInitList mods, const DragConfig& cfg, Range<unsigned> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool EditInt(UIObject* dragObj, int64_t& val, ModInitList mods, const DragConfig& cfg, Range<int64_t> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool EditInt(UIObject* dragObj, uint64_t& val, ModInitList mods, const DragConfig& cfg, Range<uint64_t> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool EditFloat(UIObject* dragObj, float& val, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	return EditNumber(dragObj, val, mods, cfg, range, fmt);
}

bool EditStringImpl(bool multiline, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& tb = Make<Textbox>();
	if (multiline)
		tb.SetMultiline(true);
	if (!GetEnabled())
		tb.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&tb);
	bool changed = false;
	if (tb.flags & UIObject_IsEdited)
	{
		retfn(tb.GetText().c_str());
		tb.flags &= ~UIObject_IsEdited;
		tb._OnIMChange();
		changed = true;
	}
	else // text can be invalidated if retfn is called
		tb.SetText(text);
	tb.HandleEvent(EventType::Change) = [&tb](Event&)
	{
		tb.flags |= UIObject_IsEdited;
		tb.RebuildContainer();
	};

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();

	return changed;
}

bool EditString(const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	return EditStringImpl(false, text, retfn, mods);
}

bool EditStringMultiline(const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	return EditStringImpl(true, text, retfn, mods);
}

bool EditColor(Color4f& val, bool delayed, ModInitList mods)
{
	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& ced = delayed
		? (IColorEdit&)Make<ColorEdit>()
		: (IColorEdit&)Make<ColorEditRT>();
	if (!GetEnabled())
		ced.flags |= UIObject_IsDisabled;
	for (auto& mod : mods)
		mod->Apply(&ced);
	bool changed = false;
	if (ced.flags & UIObject_IsEdited)
	{
		val = ced.GetColor().GetRGBA();
		ced.flags &= ~UIObject_IsEdited;
		ced._OnIMChange();
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

	return changed;
}

bool EditColor(Color4b& val, bool delayed, ModInitList mods)
{
	Color4f tmp = val;
	if (EditColor(tmp, delayed, mods))
	{
		val = tmp;
		return true;
	}
	return false;
}

const char* XY[] = { "\bX", "\bY", nullptr };
const char* XYZ[] = { "\bX", "\bY", "\bZ", nullptr };
const char* XYZW[] = { "\bX", "\bY", "\bZ", "\bW", nullptr };
const char* RGBA[] = { "\bR", "\bG", "\bB", "\bA", nullptr };
const char* MinMax[] = { "\bMin", "\bMax", nullptr };
const char* WidthHeight[] = { "\bWidth", "\bHeight", nullptr };

bool EditIntVec(int* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	for (auto& mod : mods)
		mod->OnBeforeControlGroup();

	bool any = false;
	for (const char** plabel = axes; *plabel; plabel++)
	{
		any |= PropEditInt(*plabel, *val++, mods, cfg, range, fmt);
	}

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControlGroup();

	return any;
}

bool EditFloatVec(float* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	for (auto& mod : mods)
		mod->OnBeforeControlGroup();

	bool any = false;
	for (const char** plabel = axes; *plabel; plabel++)
	{
		any |= PropEditFloat(*plabel, *val++, mods, cfg, range, fmt);
	}

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControlGroup();

	return any;
}


struct LabelPropScope : LabeledProperty::Scope
{
	LabelPropScope(const char* lblstr, ModInitList mods) : Scope(lblstr)
	{
		if (!GetEnabled())
			label->flags |= UIObject_IsDisabled;
		for (auto& mod : mods)
			mod->ApplyToLabel(label);
	}
};

void PropText(const char* label, const char* text, ModInitList mods)
{
	LabelPropScope ps(label, mods);

	for (auto& mod : mods)
		mod->OnBeforeControl();

	auto& ctrl = Push<LabelFrame>();
	for (auto& mod : mods)
		mod->OnBeforeContent();
	Text(text);
	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterContent();
	Pop();

	for (auto& mod : mods)
		mod->Apply(&ctrl);

	for (auto& mod : ReverseIterate(mods))
		mod->OnAfterControl();
}

bool PropButton(const char* label, const char* text, ModInitList mods)
{
	LabelPropScope ps(label, mods);
	return Button(text, mods);
}

bool PropEditBool(const char* label, bool& val, ModInitList mods)
{
	LabelPropScope ps(label, mods);
	return EditBool(val, nullptr, mods);
}

bool PropEditInt(const char* label, int& val, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditInt(ps.label, val, mods, cfg, range, fmt);
}

bool PropEditInt(const char* label, unsigned& val, ModInitList mods, const DragConfig& cfg, Range<unsigned> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditInt(ps.label, val, mods, cfg, range, fmt);
}

bool PropEditInt(const char* label, int64_t& val, ModInitList mods, const DragConfig& cfg, Range<int64_t> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditInt(ps.label, val, mods, cfg, range, fmt);
}

bool PropEditInt(const char* label, uint64_t& val, ModInitList mods, const DragConfig& cfg, Range<uint64_t> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditInt(ps.label, val, mods, cfg, range, fmt);
}

bool PropEditFloat(const char* label, float& val, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditFloat(ps.label, val, mods, cfg, range, fmt);
}

bool PropEditString(const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	LabelPropScope ps(label, mods);
	return EditString(text, retfn, mods);
}

bool PropEditStringMultiline(const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	LabelPropScope ps(label, mods);
	return EditStringMultiline(text, retfn, mods);
}

bool PropEditColor(const char* label, Color4f& val, bool delayed, ModInitList mods)
{
	LabelPropScope ps(label, mods);
	return EditColor(val, delayed, mods);
}

bool PropEditColor(const char* label, Color4b& val, bool delayed, ModInitList mods)
{
	LabelPropScope ps(label, mods);
	return EditColor(val, delayed, mods);
}

bool PropEditIntVec(const char* label, int* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<int> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditIntVec(val, axes, mods, cfg, range, fmt);
}

bool PropEditFloatVec(const char* label, float* val, const char** axes, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	LabelPropScope ps(label, mods);
	return EditFloatVec(val, axes, mods, cfg, range, fmt);
}

} // imm
} // ui
