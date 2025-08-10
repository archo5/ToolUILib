
#pragma once
#include "Objects.h"
#include "Controls.h"
#include "../Core/Image.h"


namespace ui {

struct SetMinWidth : Modifier
{
	float w;
	SetMinWidth(float _w) : w(_w) {}
	void OnBeforeControl() const override
	{
		Push<SizeConstraintElement>().SetMinWidth(w);
	}
	void OnAfterControl() const override
	{
		Pop();
	}
};


bool imGetEnabled();
bool imSetEnabled(bool newValue);


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


imCtrlInfo imButton(UIObject& obj, ModInitList mods = {});
imCtrlInfo imButton(StringView text, ModInitList mods = {});
imCtrlInfo imButton(DefaultIconStyle icon, ModInitList mods = {});

imCtrlInfo imSelectable(UIObject& obj, ModInitList mods = {});
imCtrlInfo imSelectable(StringView text, ModInitList mods = {});

imCtrlInfo imCheckboxExtRaw(u8 state, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
UI_FORCEINLINE imCtrlInfo imCheckboxRaw(bool val, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	return imCheckboxExtRaw(val ? 1 : 0, text, mods, skin);
}
imCtrlInfo imEditBool(bool& val, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
template <class T> imCtrlInfo imEditFlag(T& val, T cur, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	bool all = (val & cur) == cur;
	bool any = (val & cur) != 0;
	imCtrlInfo ci = imCheckboxExtRaw(any ? all ? 1 : 2 : 0, text, mods, skin);
	if (ci)
	{
		if ((val & cur) != cur)
			val |= cur;
		else
			val &= ~cur;
	}
	return ci;
}

imCtrlInfo imRadioButtonRaw(bool val, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin());
template <class T> imCtrlInfo imRadioButton(T& val, T cur, StringView text = {}, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin())
{
	imCtrlInfo ci = imRadioButtonRaw(val == cur, text, mods, skin);
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

bool imEditInt(UIObject* dragObj, int& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = All{}, const char* fmt = "%d");
bool imEditInt(UIObject* dragObj, unsigned& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<unsigned> range = All{}, const char* fmt = "%u");
bool imEditInt(UIObject* dragObj, int64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int64_t> range = All{}, const char* fmt = "%" PRId64);
bool imEditInt(UIObject* dragObj, uint64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<uint64_t> range = All{}, const char* fmt = "%" PRIu64);
bool imEditFloat(UIObject* dragObj, float& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<float> range = All{}, const char* fmt = "%g");

imCtrlInfo imEditString(std::string& text, ModInitList mods = {});
imCtrlInfo imEditString(const IBufferRW& textRW, ModInitList mods = {});
imCtrlInfo imEditStringMultiline(const IBufferRW& textRW, ModInitList mods = {});

imCtrlInfo imEditColor(Color4f& val, bool delayed = false, ModInitList mods = {});
imCtrlInfo imEditColor(Color4b& val, bool delayed = false, ModInitList mods = {});

extern const char* axesXY[];
extern const char* axesXYZ[];
extern const char* axesXYZW[];
extern const char* axesRGBA[];
extern const char* axesMinMax[];
extern const char* axesWidthHeight[];
// length of `val` = length of `axes` (null-terminated)
bool imEditIntVec(int* val, const char** axes, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = All{}, const char* fmt = "%d");
bool imEditFloatVec(float* val, const char** axes, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g");

namespace imm {

bool PropEditInt(const char* label, int& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = All{}, const char* fmt = "%d");
bool PropEditInt(const char* label, unsigned& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<unsigned> range = All{}, const char* fmt = "%u");
bool PropEditInt(const char* label, int64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int64_t> range = All{}, const char* fmt = "%" PRId64);
bool PropEditInt(const char* label, uint64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<uint64_t> range = All{}, const char* fmt = "%" PRIu64);
bool PropEditFloat(const char* label, float& val, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g");

} // imm

inline bool imEditVec2f(Vec2f& val, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g")
{
	return imEditFloatVec(&val.x, axesXY, mods, cfg, range, fmt);
}

inline bool imEditRangef(Rangef& val, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g")
{
	return imEditFloatVec(&val.min, axesMinMax, mods, cfg, range, fmt);
}

} // ui
