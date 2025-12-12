
#include "ImmediateMode.h"

#include "Controls.h"
#include "Graphics.h"
#include "Layout.h"
#include "System.h"

#include "../Elements/Textbox.h"
#include "../Editors/NumberEditor.h"


namespace ui {
namespace imm {


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


LabelFrame& StdText(StringView text)
{
	return MakeWithText<LabelFrame>(text);
}


imCtrlInfoT<Button> imButton(UIObject& obj)
{
	auto& btn = Push<ui::Button>();
	Add(obj);
	Pop();

	btn.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		btn.flags |= UIObject_IsDisabled;

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

imCtrlInfoT<Button> imButton(StringView text)
{
	return imButton(NewText(text));
}

imCtrlInfoT<Button> imButton(DefaultIconStyle icon)
{
	return imButton(New<IconElement>().SetDefaultStyle(icon));
}

imCtrlInfoT<Selectable> imSelectable(UIObject& obj)
{
	auto& btn = Push<ui::Selectable>();
	Add(obj);
	Pop();

	btn.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		btn.flags |= UIObject_IsDisabled;

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

imCtrlInfoT<Selectable> imSelectable(StringView text)
{
	return imSelectable(NewText(text));
}

imCtrlInfoT<StateToggle> imCheckboxExtRaw(u8 val, StringView text, const IStateToggleSkin& skin)
{
	auto& cb = Push<StateToggle>();
	skin.BuildContents(cb, text, val);
	Pop();

	cb.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		cb.flags |= UIObject_IsDisabled;
	cb.InitReadOnly(val);

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

imCtrlInfoT<StateToggle> imEditBool(bool& val, StringView text, const IStateToggleSkin& skin)
{
	imCtrlInfoT<StateToggle> ci = imCheckboxRaw(val, text, skin);
	if (ci)
		val = !val;
	return ci;
}

imCtrlInfoT<StateToggle> imRadioButtonRaw(bool val, StringView text, const IStateToggleSkin& skin)
{
	auto& rb = Push<StateToggle>();
	skin.BuildContents(rb, text, val);
	Pop();

	rb.flags |= UIObject_DB_IMEdit;
	if (!imGetEnabled())
		rb.flags |= UIObject_IsDisabled;
	rb.InitReadOnly(val);

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

static bool HasMoreThanOneChild(UIObject* obj)
{
	UIObjectIteratorData oid;
	obj->SlotIterator_Init(oid);
	if (!obj->SlotIterator_GetNext(oid))
		return false;
	if (obj->SlotIterator_GetNext(oid))
		return true;
	return false;
}

template <class TNum> imCtrlInfoNumberEditor EditNumber(TNum& val, const DragConfig& cfg, Range<TNum> range, NumberFormatSettings fmt)
{
#if 0
	auto& tb = Make<NumberTextbox<TNum>>();
	if (!imGetEnabled())
		tb.flags |= UIObject_IsDisabled;

	NumFmtBox fb(fmt);

	if (tb.flags & UIObject_AfterIMEdit)
	{
		tb._OnIMChange();
		tb.flags &= ~UIObject_AfterIMEdit;
	}
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
		if (val != tmp)
		{
			val = tmp;
			edited = true;
			tb.flags |= UIObject_AfterIMEdit;
			tb.RebuildContainer();
		}
		tb.edited = 0;
	}

	char buf[1024];
	snprintf(buf, 1024, fb.printFmt, val);
	tb.numberValue = val;
	tb.SetText(RemoveNegZero(buf));

	UIObject* dragObj = &tb;
	bool foundDragObj = false;
	for (int i = 0; i < 5; i++)
	{
		UIObject* p = dragObj->parent;
		if (!p)
			break;
		if (HasMoreThanOneChild(p))
			break;
		dragObj = p;
		if (dynamic_cast<LabeledProperty*>(dragObj))
		{
			foundDragObj = true;
			break;
		}
	}

	if (foundDragObj)
	{
		dragObj->SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
		dragObj->HandleEvent() = [val, cfg = cfg, range, &tb, fb](Event& e)
		{
			if (tb.IsInputDisabled() || e.target->IsChildOrSame(&tb))
				return;
			if (e.type == EventType::MouseMove && e.target->IsPressed() && e.delta.x != 0)
			{
				if (auto* veg = e.target->FindParentOfType<imVectorEditGroupBase>())
					veg->_ongoingEdit = true;

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

				tb.RebuildContainer();
			}
			if (e.type == EventType::ButtonUp && e.context->GetMouseCapture() == e.target)
			{
				if (auto* veg = e.target->FindParentOfType<imVectorEditGroupBase>())
				{
					veg->_ongoingEdit = false;
					tb.RebuildContainer();
				}
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

	return { edited, &tb };
#else
	auto& ne = Make<NumberEditorT<TNum>>();
	ne.dragConfig = cfg;
	ne.range = range;
	ne.format = fmt;
	if (!imGetEnabled())
		ne.flags |= UIObject_IsDisabled;
	bool changed = false;
	if (ne.flags & UIObject_IsEdited)
	{
		val = ne.value;
		ne.flags &= ~UIObject_IsEdited;
		ne._OnIMChange();
		ne.RebuildContainer();
		changed = true;
	}
	else
		ne.SetValue(val);
	ne.HandleEvent() = [&ne](Event& ev)
	{
		if (ev.type == EventType::Change)
		{
			ev.StopPropagation();
			ne.flags |= UIObject_IsEdited;
			ne.RebuildContainer();
		}
		if (ev.type == EventType::Commit)
			ev.StopPropagation();
	};

	return { changed, &ne };
#endif
}

imCtrlInfoNumberEditor& imCtrlInfoNumberEditor::SetLabel(StringView label)
{
	static_cast<NumberEditorBase*>(root)->label <<= label;
	return *this;
}

imCtrlInfoNumberEditor imEditInt(int& val, const DragConfig& cfg, Range<int> range, NumberFormatSettings fmt)
{
	return EditNumber(val, cfg, range, fmt);
}

imCtrlInfoNumberEditor imEditInt(unsigned& val, const DragConfig& cfg, Range<unsigned> range, NumberFormatSettings fmt)
{
	return EditNumber(val, cfg, range, fmt);
}

imCtrlInfoNumberEditor imEditInt(int64_t& val, const DragConfig& cfg, Range<int64_t> range, NumberFormatSettings fmt)
{
	return EditNumber(val, cfg, range, fmt);
}

imCtrlInfoNumberEditor imEditInt(uint64_t& val, const DragConfig& cfg, Range<uint64_t> range, NumberFormatSettings fmt)
{
	return EditNumber(val, cfg, range, fmt);
}

imCtrlInfoNumberEditor imEditFloat(float& val, const DragConfig& cfg, Range<float> range, NumberFormatSettings fmt)
{
	return EditNumber(val, cfg, range, fmt);
}


imCtrlInfoTextbox& imCtrlInfoTextbox::Multiline(bool is)
{
	static_cast<Textbox*>(root)->SetMultiline(is);
	return *this;
}

imCtrlInfoTextbox& imCtrlInfoTextbox::Placeholder(StringView pch)
{
	static_cast<Textbox*>(root)->SetPlaceholder(pch);
	return *this;
}

static imCtrlInfoTextbox imEditStringImpl(const IBufferRW& textRW)
{
	auto& tb = Make<Textbox>();
	if (!imGetEnabled())
		tb.flags |= UIObject_IsDisabled;
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
	tb.HandleEvent() = [&tb](Event& ev)
	{
		if (ev.type == EventType::Change)
		{
			ev.StopPropagation();
			tb.flags |= UIObject_IsEdited;
			tb.RebuildContainer();
		}
		if (ev.type == EventType::Commit)
			ev.StopPropagation();
	};

	return { changed, &tb };
}

imCtrlInfoTextbox imEditString(std::string& text)
{
	return imEditStringImpl(StdStringRW(text));
}

imCtrlInfoTextbox imEditString(const IBufferRW& textRW)
{
	return imEditStringImpl(textRW);
}


imCtrlInfo imEditColor(Color4f& val, bool delayed)
{
	auto& ced = delayed
		? (IColorEdit&)Make<ColorEdit>()
		: (IColorEdit&)Make<ColorEditRT>();
	if (!imGetEnabled())
		ced.flags |= UIObject_IsDisabled;
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
	ced.HandleEvent() = [&ced](Event& ev)
	{
		if (ev.type == EventType::Change)
		{
			ev.StopPropagation();
			ced.flags |= UIObject_IsEdited;
			ced.RebuildContainer();
		}
		if (ev.type == EventType::Commit)
			ev.StopPropagation();
	};

	return { changed, &ced };
}

imCtrlInfo imEditColor(Color4b& val, bool delayed)
{
	Color4f tmp = val;
	imCtrlInfo ci = imEditColor(tmp, delayed);
	if (ci)
		val = tmp;
	return ci;
}


const char* axesXY[] = { "X", "Y", nullptr };
const char* axesXYZ[] = { "X", "Y", "Z", nullptr };
const char* axesXYZW[] = { "X", "Y", "Z", "W", nullptr };
const char* axesRGBA[] = { "R", "G", "B", "A", nullptr };
const char* axesMinMax[] = { "Min", "Max", nullptr };
const char* axesWidthHeight[] = { "Width", "Height", nullptr };

template <class T>
struct VecEditConfig
{
	T* val;
	DragConfig dragcfg;
	Range<T> range;
	NumberFormatSettings fmt;
};

template <class TNum>
bool imEditTNumVec(TNum* val, const imLoop& loop, const DragConfig& dragcfg, Range<TNum> range, NumberFormatSettings fmt)
{
	VecEditConfig<TNum> vecfg = { val, dragcfg, range, fmt };
	imLoopCallback* cb = [](void* ud, const char* label) -> imCtrlInfo
	{
		auto* vecfg = (VecEditConfig<TNum>*)ud;
		return EditNumber(*vecfg->val++, vecfg->dragcfg, vecfg->range, vecfg->fmt).SetLabel(label);
	};
	return loop.Iterate(cb, &vecfg);
}

bool imEditIntVec(int* val, const imLoop& loop, const DragConfig& dragcfg, Range<int> range, NumberFormatSettings fmt)
{
	return imEditTNumVec(val, loop, dragcfg, range, fmt);
}

bool imEditFloatVec(float* val, const imLoop& loop, const DragConfig& dragcfg, Rangef range, NumberFormatSettings fmt)
{
	return imEditTNumVec(val, loop, dragcfg, range, fmt);
}

bool imEditIntVec(int* val, const char** axes, const DragConfig& dragcfg, Range<int> range, NumberFormatSettings fmt)
{
	return imEditIntVec(val, imAxisLoop(axes), dragcfg, range, fmt);
}

bool imEditFloatVec(float* val, const char** axes, const DragConfig& dragcfg, Range<float> range, NumberFormatSettings fmt)
{
	return imEditFloatVec(val, imAxisLoop(axes), dragcfg, range, fmt);
}


} // imm
} // ui
