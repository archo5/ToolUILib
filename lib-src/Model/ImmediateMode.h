
#pragma once
#include "Objects.h"
#include "Controls.h"
#include "../Core/Image.h"


namespace ui {

struct modSetMinWidth : Modifier
{
	float w;
	modSetMinWidth(float _w) : w(_w) {}
	void OnBeforeControl() const override
	{
		Push<SizeConstraintElement>().SetMinWidth(w);
	}
	void OnAfterControl() const override
	{
		Pop();
	}
};


namespace imm {


bool imGetEnabled();
bool imSetEnabled(bool newValue);

struct imEnable
{
	bool prev;
	imEnable(bool e) { prev = imSetEnabled(e); }
	~imEnable() { imSetEnabled(prev); }
};

struct imLabel
{
	LabeledProperty::ContentLayoutType layoutType;
	LabeledProperty* label;

	imLabel(StringView lblstr = {}, LabeledProperty::ContentLayoutType layout = LabeledProperty::StackExpandLTR) : layoutType(layout)
	{
		label = &LabeledProperty::Begin(lblstr, layoutType);
		if (!imGetEnabled())
			label->flags |= UIObject_IsDisabled;
	}
	~imLabel()
	{
		LabeledProperty::End(layoutType);
	}
	imLabel& WithTooltip(Tooltip::BuildFunc&& tbfn)
	{
		label->_tooltipBuildFunc = Move(tbfn);
		return *this;
	}
	imLabel& WithTooltip(StringView tt)
	{
		label->_tooltipBuildFunc = [tt{ to_string(tt) }]()
		{
			Text(tt);
		};
		return *this;
	}
};

struct imMinWidth
{
	imMinWidth(float w) { Push<SizeConstraintElement>().SetMinWidth(w); }
	~imMinWidth() { Pop(); }
};

void StdText(StringView text, ModInitList mods = {});

struct IStateToggleSkin
{
	virtual void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const = 0;
};
struct CheckboxStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};
struct RadioButtonStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};
struct ButtonStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};
struct TreeStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};


imCtrlInfo<Button> imButton(UIObject& obj);
imCtrlInfo<Button> imButton(StringView text);
imCtrlInfo<Button> imButton(DefaultIconStyle icon);

imCtrlInfo<Selectable> imSelectable(UIObject& obj);
imCtrlInfo<Selectable> imSelectable(StringView text);

imCtrlInfo<StateToggle> imCheckboxExtRaw(u8 state, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
UI_FORCEINLINE imCtrlInfo<StateToggle> imCheckboxRaw(bool val, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	return imCheckboxExtRaw(val ? 1 : 0, text, mods, skin);
}
imCtrlInfo<StateToggle> imEditBool(bool& val, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
template <class T> imCtrlInfo<StateToggle> imEditFlag(T& val, T cur, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	bool all = (val & cur) == cur;
	bool any = (val & cur) != 0;
	imCtrlInfo<StateToggle> ci = imCheckboxExtRaw(any ? all ? 1 : 2 : 0, text, mods, skin);
	if (ci)
	{
		if ((val & cur) != cur)
			val |= cur;
		else
			val &= ~cur;
	}
	return ci;
}

imCtrlInfo<StateToggle> imRadioButtonRaw(bool val, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin());
template <class T> imCtrlInfo<StateToggle> imRadioButton(T& val, T cur, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin())
{
	imCtrlInfo<StateToggle> ci = imRadioButtonRaw(val == cur, text, mods, skin);
	if (ci)
	{
		val = cur;
	}
	return ci;
}

struct DragConfig
{
	// all speed values are in units/px
	// snap 0 = snap off

	float snap = 1.0f;
	float speed = 0.1f;
	float slowdownSnap = 0.01f;
	float slowdownSpeed = 0.001f;
	float boostSnap = 1.0f;
	float boostSpeed = 10.0f;

	bool relativeSpeed = false;

	// ModifierKeyFlags
	uint8_t snapOffKey = MK_Alt;
	uint8_t slowdownKey = MK_Ctrl;
	uint8_t boostKey = MK_Shift;

	DragConfig() {}
	DragConfig(float q, bool relSpeed = false) :
		snap(q),
		speed(q * 0.1f),
		slowdownSnap(q * 0.01f),
		slowdownSpeed(q * 0.001f),
		boostSnap(q),
		boostSpeed(q * 10.0f),
		relativeSpeed(relSpeed)
	{}

	float GetSpeed(uint8_t modifierKeys) const;
	float GetSnap(uint8_t modifierKeys) const;
};

bool imEditInt(int& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = All{}, const char* fmt = "%d");
bool imEditInt(unsigned& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<unsigned> range = All{}, const char* fmt = "%u");
bool imEditInt(int64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int64_t> range = All{}, const char* fmt = "%" PRId64);
bool imEditInt(uint64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<uint64_t> range = All{}, const char* fmt = "%" PRIu64);
bool imEditFloat(float& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<float> range = All{}, const char* fmt = "%g");

struct imCtrlInfoTextbox : imCtrlInfo<UIObject>
{
	using imCtrlInfo<UIObject>::imCtrlInfo;

	imCtrlInfoTextbox& Multiline(bool is = true);
	imCtrlInfoTextbox& Placeholder(StringView pch);
};
imCtrlInfoTextbox imEditString(std::string& text);
imCtrlInfoTextbox imEditString(const IBufferRW& textRW);

imCtrlInfo<UIObject> imEditColor(Color4f& val, bool delayed = false, ModInitList mods = {});
imCtrlInfo<UIObject> imEditColor(Color4b& val, bool delayed = false, ModInitList mods = {});

extern const char* axesXY[];
extern const char* axesXYZ[];
extern const char* axesXYZW[];
extern const char* axesRGBA[];
extern const char* axesMinMax[];
extern const char* axesWidthHeight[];
// length of `val` = length of `axes` (null-terminated)
bool imEditIntVec(int* val, const char** axes, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = All{}, const char* fmt = "%d");
bool imEditFloatVec(float* val, const char** axes, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g");

inline bool imEditVec2f(Vec2f& val, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g")
{
	return imEditFloatVec(&val.x, axesXY, mods, cfg, range, fmt);
}

inline bool imEditRangef(Rangef& val, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g")
{
	return imEditFloatVec(&val.min, axesMinMax, mods, cfg, range, fmt);
}

} // imm
using namespace imm;

} // ui
