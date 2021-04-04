
#pragma once
#include "Objects.h"
#include "Controls.h"
#include "../Core/Image.h"


namespace ui {
namespace imm {


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

bool Button(const char* text, ModInitList mods = {});
bool CheckboxRaw(bool val, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
bool EditBool(bool& val, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
template <class T> bool EditFlag(T& val, T cur, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	if (CheckboxRaw((val & cur) == cur, text, mods, skin))
	{
		if ((val & cur) != cur)
			val |= cur;
		else
			val &= ~cur;
		return true;
	}
	return false;
}
bool RadioButtonRaw(bool val, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin());
template <class T> bool RadioButton(T& val, T cur, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin())
{
	if (RadioButtonRaw(val == cur, text, mods, skin))
	{
		val = cur;
		return true;
	}
	return false;
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

bool EditInt(UIObject* dragObj, int& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = {}, const char* fmt = "%d");
bool EditInt(UIObject* dragObj, unsigned& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<unsigned> range = {}, const char* fmt = "%u");
bool EditInt(UIObject* dragObj, int64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int64_t> range = {}, const char* fmt = "%" PRId64);
bool EditInt(UIObject* dragObj, uint64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<uint64_t> range = {}, const char* fmt = "%" PRIu64);
bool EditFloat(UIObject* dragObj, float& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<float> range = {}, const char* fmt = "%g");
bool EditString(const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});
bool EditColor(Color4f& val, ModInitList mods = {});
bool EditColor(Color4b& val, ModInitList mods = {});

// length of `val` = length of `axes`
bool EditFloatVec(float* val, const char* axes = "XYZ", ModInitList mods = {}, const DragConfig& cfg = {}, Range<float> range = {}, const char* fmt = "%g");

void PropText(const char* label, const char* text, ModInitList mods = {});
bool PropButton(const char* label, const char* text, ModInitList mods = {});
bool PropEditBool(const char* label, bool& val, ModInitList mods = {});
bool PropEditInt(const char* label, int& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int> range = {}, const char* fmt = "%d");
bool PropEditInt(const char* label, unsigned& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<unsigned> range = {}, const char* fmt = "%u");
bool PropEditInt(const char* label, int64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<int64_t> range = {}, const char* fmt = "%" PRId64);
bool PropEditInt(const char* label, uint64_t& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<uint64_t> range = {}, const char* fmt = "%" PRIu64);
bool PropEditFloat(const char* label, float& val, ModInitList mods = {}, const DragConfig& cfg = {}, Range<float> range = {}, const char* fmt = "%g");
bool PropEditString(const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});
bool PropEditColor(const char* label, Color4f& val, ModInitList mods = {});
bool PropEditColor(const char* label, Color4b& val, ModInitList mods = {});

// length of `val` = length of `axes`
bool PropEditFloatVec(const char* label, float* val, const char* axes = "XYZ", ModInitList mods = {}, const DragConfig& cfg = {}, Range<float> range = {}, const char* fmt = "%g");

} // imm
} // ui
