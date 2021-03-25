
#pragma once
#include "Objects.h"
#include "Controls.h"
#include "../Core/Image.h"


namespace ui {
namespace imm {

constexpr float UNITS_PER_PX = 0.1f;
constexpr float DEFAULT_SPEED = 1.0f;

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
bool EditInt(UIObject* dragObj, int& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool EditInt(UIObject* dragObj, unsigned& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool EditInt(UIObject* dragObj, int64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool EditInt(UIObject* dragObj, uint64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool EditFloat(UIObject* dragObj, float& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool EditString(const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});
bool EditColor(Color4f& val, ModInitList mods = {});
bool EditColor(Color4b& val, ModInitList mods = {});

// length of `val` = length of `axes`
bool EditFloatVec(float* val, const char* axes = "XYZ", ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");

void PropText(const char* label, const char* text, ModInitList mods = {});
bool PropButton(const char* label, const char* text, ModInitList mods = {});
bool PropEditBool(const char* label, bool& val, ModInitList mods = {});
bool PropEditInt(const char* label, int& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool PropEditInt(const char* label, unsigned& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool PropEditInt(const char* label, int64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool PropEditInt(const char* label, uint64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool PropEditFloat(const char* label, float& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool PropEditString(const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});
bool PropEditColor(const char* label, Color4f& val, ModInitList mods = {});
bool PropEditColor(const char* label, Color4b& val, ModInitList mods = {});

// length of `val` = length of `axes`
bool PropEditFloatVec(const char* label, float* val, const char* axes = "XYZ", ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");

} // imm
} // ui
