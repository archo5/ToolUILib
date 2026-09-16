
#pragma once
#include "Objects.h"
#include "Controls.h"
#include "../Core/Image.h"
#include "../Editors/NumEditConfig.h"


namespace ui {
namespace imm {


UI_API bool imGetEnabled();
UI_API bool imSetEnabled(bool newValue);
UI_API bool imProcessEventFlags(UIObject& uiobj);

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
			Text(tt).SetMultiline(true);
		};
		return *this;
	}
};

struct imMinWidth
{
	imMinWidth(float w) { Push<SizeConstraintElement>().SetMinWidth(w); }
	~imMinWidth() { Pop(); }
};

UI_API LabelFrame& StdText(StringView text);

struct IStateToggleSkin
{
	virtual void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const = 0;
};
struct UI_API CheckboxStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};
struct UI_API RadioButtonStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};
struct UI_API ButtonStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};
struct UI_API TreeStateToggleSkin : IStateToggleSkin
{
	void BuildContents(StateToggleBase& parent, StringView text, uint8_t state) const override;
};


UI_API imCtrlInfoT<Button> imButton(UIObject& obj);
UI_API imCtrlInfoT<Button> imButton(StringView text);
UI_API imCtrlInfoT<Button> imButton(DefaultIconStyle icon);

UI_API imCtrlInfoT<Selectable> imSelectable(UIObject& obj);
UI_API imCtrlInfoT<Selectable> imSelectable(StringView text);

UI_API imCtrlInfoT<StateToggle> imCheckboxExtRaw(u8 state, StringView text = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
UI_FORCEINLINE imCtrlInfoT<StateToggle> imCheckboxRaw(bool val, StringView text = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	return imCheckboxExtRaw(val ? 1 : 0, text, skin);
}
UI_API imCtrlInfoT<StateToggle> imEditBool(bool& val, StringView text = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
template <class T> imCtrlInfoT<StateToggle> imEditFlag(T& val, T cur, StringView text = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	bool all = (val & cur) == cur;
	bool any = (val & cur) != 0;
	imCtrlInfoT<StateToggle> ci = imCheckboxExtRaw(any ? all ? 1 : 2 : 0, text, skin);
	if (ci)
	{
		if ((val & cur) != cur)
			val |= cur;
		else
			val &= ~cur;
	}
	return ci;
}

UI_API imCtrlInfoT<StateToggle> imRadioButtonRaw(bool val, StringView text = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin());
template <class T> imCtrlInfoT<StateToggle> imRadioButton(T& val, T cur, StringView text = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin())
{
	imCtrlInfoT<StateToggle> ci = imRadioButtonRaw(val == cur, text, skin);
	if (ci)
	{
		val = cur;
	}
	return ci;
}

struct UI_API imCtrlInfoNumberEditor : imCtrlInfo
{
	using imCtrlInfo::imCtrlInfo;

	imCtrlInfoNumberEditor& SetLabel(StringView label);
};
UI_API imCtrlInfoNumberEditor imEditInt(int& val, const DragConfig& cfg = {}, Range<int> range = All{}, NumberFormatSettings fmt = {});
UI_API imCtrlInfoNumberEditor imEditInt(unsigned& val, const DragConfig& cfg = {}, Range<unsigned> range = All{}, NumberFormatSettings fmt = {});
UI_API imCtrlInfoNumberEditor imEditInt(int64_t& val, const DragConfig& cfg = {}, Range<int64_t> range = All{}, NumberFormatSettings fmt = {});
UI_API imCtrlInfoNumberEditor imEditInt(uint64_t& val, const DragConfig& cfg = {}, Range<uint64_t> range = All{}, NumberFormatSettings fmt = {});
UI_API imCtrlInfoNumberEditor imEditFloat(float& val, const DragConfig& cfg = {}, Range<float> range = All{}, NumberFormatSettings fmt = {});

struct UI_API imCtrlInfoTextbox : imCtrlInfo
{
	using imCtrlInfo::imCtrlInfo;

	imCtrlInfoTextbox& Multiline(bool is = true);
	imCtrlInfoTextbox& Placeholder(StringView pch);
};
UI_API imCtrlInfoTextbox imEditString(std::string& text);
UI_API imCtrlInfoTextbox imEditString(const IBufferRW& textRW);

UI_API imCtrlInfo imEditColor(Color4f& val, bool delayed = false);
UI_API imCtrlInfo imEditColor(Color4b& val, bool delayed = false);

UI_API extern const char* axesXY[];
UI_API extern const char* axesXYZ[];
UI_API extern const char* axesXYZW[];
UI_API extern const char* axesRGBA[];
UI_API extern const char* axesMinMax[];
UI_API extern const char* axesWidthHeight[];

typedef imCtrlInfo imLoopCallback(void* data, const char* label);

struct imLoop
{
	virtual bool Iterate(imLoopCallback* cb, void* ud) const = 0;
};

struct imAxisLoop : imLoop
{
	const char** axes;
	float minWidth;
	imAxisLoop(const char** axes_, float minWidth_ = 0) : axes(axes_), minWidth(minWidth_) {}
	bool Iterate(imLoopCallback* cb, void* ud) const override
	{
		bool any = false;
		int n = 0;
		for (const char** plabel = axes; *plabel; plabel++, n++)
		{
			const char* lbl = *plabel;
			if (*lbl == '\b')
				lbl++;
			any |= SingleIteration(n, lbl, cb, ud);
		}
		return any;
	}
	virtual imCtrlInfo SingleIteration(int n, const char* label, imLoopCallback* cb, void* ud) const
	{
		if (minWidth > 0)
			Push<SizeConstraintElement>().SetMinWidth(minWidth);
		UI_DEFER(if (minWidth > 0) Pop());

		return cb(ud, label);
	}
};

UI_API bool imEditIntVec(int* val, const imLoop& loop, const DragConfig& dragcfg = {}, Range<int> range = All{}, NumberFormatSettings fmt = {});
UI_API bool imEditFloatVec(float* val, const imLoop& loop, const DragConfig& dragcfg = {}, Rangef range = All{}, NumberFormatSettings fmt = {});

// length of `val` = length of `axes` (null-terminated)
UI_API bool imEditIntVec(int* val, const char** axes, const DragConfig& cfg = {}, Range<int> range = All{}, NumberFormatSettings fmt = {});
UI_API bool imEditFloatVec(float* val, const char** axes, const DragConfig& cfg = {}, Rangef range = All{}, NumberFormatSettings fmt = {});

inline bool imEditVec2f(Vec2f& val, const DragConfig& cfg = {}, Rangef range = All{}, NumberFormatSettings fmt = {})
{
	bool chg = imEditFloat(val.x, cfg, range, fmt).SetLabel("X");
	return chg | imEditFloat(val.y, cfg, range, fmt).SetLabel("Y");
}

inline bool imEditRangef(Rangef& val, const DragConfig& cfg = {}, Rangef range = All{}, NumberFormatSettings fmt = {})
{
	bool chg = imEditFloat(val.min, cfg, range, fmt).SetLabel("Min");
	return chg | imEditFloat(val.max, cfg, range, fmt).SetLabel("Max");
}

struct imVectorEditGroupBase : WrapperElement
{
	bool _ongoingEdit = false;
};

template <class VecT>
struct imVectorEditGroup : imVectorEditGroupBase
{
	VecT value = {};
	VecT _latest = {};

	void SetValue(VecT v)
	{
		if (!_ongoingEdit)
			value = v;
		_latest = v;
	}
};

} // imm
using namespace imm;

} // ui
