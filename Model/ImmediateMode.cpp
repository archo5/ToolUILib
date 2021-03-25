
#include "ImmediateMode.h"
#include "Controls.h"
#include "Graphics.h"
#include "System.h"


namespace ui {
namespace imm {

void CheckboxStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	Make<CheckboxIcon>();
	if (!text.empty())
		Text(text) + SetPadding(4);
}

void RadioButtonStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	Make<RadioButtonIcon>();
	if (!text.empty())
		Text(text) + SetPadding(4);
}

void ButtonStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	MakeWithText<StateButtonSkin>(text);
}

void TreeStateToggleSkin::BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const
{
	Make<TreeExpandIcon>();
	if (!text.empty())
		Text(text) + SetPadding(4);
}

bool Button(const char* text, ModInitList mods)
{
	auto& btn = MakeWithText<ui::Button>(text);
	btn.flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(&btn);
	bool clicked = false;
	if (btn.flags & UIObject_IsEdited)
	{
		clicked = true;
		btn.flags &= ~UIObject_IsEdited;
		btn._OnIMChange();
	}
	return clicked;
}

bool CheckboxRaw(bool val, const char* text, ModInitList mods, const IStateToggleSkin& skin)
{
	auto& cb = Push<StateToggle>();
	skin.BuildContents(cb, text ? text : StringView(), val);
	Pop();

	cb.flags |= UIObject_DB_IMEdit;
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
	auto& rb = Push<StateToggle>();
	skin.BuildContents(rb, text ? text : StringView(), val);
	Pop();

	rb.flags |= UIObject_DB_IMEdit;
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
	return edited;
}

struct NumFmtBox
{
	char fmt[8];

	NumFmtBox(const char* f)
	{
		fmt[0] = ' ';
		strncpy(fmt + 1, f, 6);
		fmt[7] = '\0';
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

template <class TNum> bool EditNumber(UIObject* dragObj, TNum& val, ModInitList mods, float speed, TNum vmin, TNum vmax, const char* fmt)
{
	auto& tb = Make<Textbox>();
	for (auto& mod : mods)
		mod->Apply(&tb);

	NumFmtBox fb(fmt);

	bool edited = false;
	if (tb.flags & UIObject_IsEdited)
	{
		decltype(val + 0) tmp = 0;
		sscanf(tb.GetText().c_str(), fb.fmt, &tmp);
		if (tmp == 0)
			tmp = 0;
		if (tmp > vmax)
			tmp = vmax;
		if (tmp < vmin)
			tmp = vmin;
		val = tmp;
		tb.flags &= ~UIObject_IsEdited;
		edited = true;
		tb._OnIMChange();
	}

	char buf[1024];
	snprintf(buf, 1024, fb.fmt + 1, val);
	tb.SetText(RemoveNegZero(buf));

	if (dragObj)
	{
		dragObj->SetFlag(UIObject_DB_CaptureMouseOnLeftClick, true);
		dragObj->HandleEvent() = [val, speed, vmin, vmax, &tb, fb](Event& e)
		{
			if (tb.IsInputDisabled())
				return;
			if (e.type == EventType::MouseMove && e.target->IsPressed() && e.delta.x != 0)
			{
				if (tb.IsFocused())
					e.context->SetKeyboardFocus(nullptr);

				float diff = e.delta.x * speed * UNITS_PER_PX;
				tb.accumulator += diff;
				TNum nv = val;
				if (fabsf(tb.accumulator) >= speed)
				{
					nv += trunc(tb.accumulator / speed) * speed;
					tb.accumulator = fmodf(tb.accumulator, speed);
				}

				if (nv > vmax || (diff > 0 && nv < val))
					nv = vmax;
				if (nv < vmin || (diff < 0 && nv > val))
					nv = vmin;

				char buf[1024];
				snprintf(buf, 1024, fb.fmt + 1, nv);
				tb.SetText(RemoveNegZero(buf));
				tb.flags |= UIObject_IsEdited;

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
		tb.flags |= UIObject_IsEdited;
		tb.RebuildContainer();
	};

	return edited;
}

bool EditInt(UIObject* dragObj, int& val, ModInitList mods, float speed, int vmin, int vmax, const char* fmt)
{
	return EditNumber(dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIObject* dragObj, unsigned& val, ModInitList mods, float speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	return EditNumber(dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIObject* dragObj, int64_t& val, ModInitList mods, float speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	return EditNumber(dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIObject* dragObj, uint64_t& val, ModInitList mods, float speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	return EditNumber(dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditFloat(UIObject* dragObj, float& val, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	return EditNumber(dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditString(const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	auto& tb = Make<Textbox>();
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
	return changed;
}

bool EditColor(Color4f& val, ModInitList mods)
{
	auto& ced = Make<ColorEdit>();
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
		ced.SetColor(val);
	ced.HandleEvent(EventType::Change) = [&ced](Event&)
	{
		ced.flags |= UIObject_IsEdited;
		ced.RebuildContainer();
	};
	return changed;
}

bool EditColor(Color4b& val, ModInitList mods)
{
	Color4f tmp = val;
	if (EditColor(tmp, mods))
	{
		val = tmp;
		return true;
	}
	return false;
}

bool EditFloatVec(float* val, const char* axes, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	bool any = false;
	char axisLabel[3] = "\b\0";
	while (*axes)
	{
		axisLabel[1] = *axes++;
		any |= PropEditFloat(axisLabel, *val++, mods, speed, vmin, vmax, fmt);
	}
	return any;
}


void PropText(const char* label, const char* text, ModInitList mods)
{
	LabeledProperty::Scope ps(label);
	auto& ctrl = Text(text) + SetPadding(5);
	for (auto& mod : mods)
		mod->Apply(&ctrl);
}

bool PropButton(const char* label, const char* text, ModInitList mods)
{
	LabeledProperty::Scope ps(label);
	return Button(text, mods);
}

bool PropEditBool(const char* label, bool& val, ModInitList mods)
{
	LabeledProperty::Scope ps(label);
	return EditBool(val, nullptr, mods);
}

bool PropEditInt(const char* label, int& val, ModInitList mods, float speed, int vmin, int vmax, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	return EditInt(ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditInt(const char* label, unsigned& val, ModInitList mods, float speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	return EditInt(ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditInt(const char* label, int64_t& val, ModInitList mods, float speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	return EditInt(ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditInt(const char* label, uint64_t& val, ModInitList mods, float speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	return EditInt(ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditFloat(const char* label, float& val, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	return EditFloat(ps.label, val, mods, speed, vmin, vmax, fmt);
}

bool PropEditString(const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	LabeledProperty::Scope ps(label);
	return EditString(text, retfn, mods);
}

bool PropEditColor(const char* label, Color4f& val, ModInitList mods)
{
	LabeledProperty::Scope ps(label);
	return EditColor(val, mods);
}

bool PropEditColor(const char* label, Color4b& val, ModInitList mods)
{
	LabeledProperty::Scope ps(label);
	return EditColor(val, mods);
}

bool PropEditFloatVec(const char* label, float* val, const char* axes, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	return EditFloatVec(val, axes, mods, speed, vmin, vmax, fmt);
}

} // imm
} // ui
