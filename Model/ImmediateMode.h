
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
	virtual void BuildContents(UIContainer* ctx, StateToggleBase* parent, StringView text, uint8_t state) const = 0;
};
struct CheckboxStateToggleSkin : IStateToggleSkin
{
	void BuildContents(UIContainer* ctx, StateToggleBase* parent, StringView text, uint8_t state) const override;
};
struct RadioButtonStateToggleSkin : IStateToggleSkin
{
	void BuildContents(UIContainer* ctx, StateToggleBase* parent, StringView text, uint8_t state) const override;
};
struct ButtonStateToggleSkin : IStateToggleSkin
{
	void BuildContents(UIContainer* ctx, StateToggleBase* parent, StringView text, uint8_t state) const override;
};
struct TreeStateToggleSkin : IStateToggleSkin
{
	void BuildContents(UIContainer* ctx, StateToggleBase* parent, StringView text, uint8_t state) const override;
};

bool Button(UIContainer* ctx, const char* text, ModInitList mods = {});
bool CheckboxRaw(UIContainer* ctx, bool val, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
bool EditBool(UIContainer* ctx, bool& val, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin());
template <class T> bool EditFlag(UIContainer* ctx, T& val, T cur, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = CheckboxStateToggleSkin())
{
	if (CheckboxRaw(ctx, (val & cur) == cur, text, mods, skin))
	{
		if ((val & cur) != cur)
			val |= cur;
		else
			val &= ~cur;
		return true;
	}
	return false;
}
bool RadioButtonRaw(UIContainer* ctx, bool val, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin());
template <class T> bool RadioButton(UIContainer* ctx, T& val, T cur, const char* text, ModInitList mods = {}, const IStateToggleSkin& skin = RadioButtonStateToggleSkin())
{
	if (RadioButtonRaw(ctx, val == cur, text, mods, skin))
	{
		val = cur;
		return true;
	}
	return false;
}
bool EditInt(UIContainer* ctx, UIObject* dragObj, int& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool EditInt(UIContainer* ctx, UIObject* dragObj, unsigned& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool EditInt(UIContainer* ctx, UIObject* dragObj, int64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool EditInt(UIContainer* ctx, UIObject* dragObj, uint64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool EditFloat(UIContainer* ctx, UIObject* dragObj, float& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool EditString(UIContainer* ctx, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});
bool EditColor(UIContainer* ctx, Color4f& val, ModInitList mods = {});
bool EditColor(UIContainer* ctx, Color4b& val, ModInitList mods = {});

// length of `val` = length of `axes`
bool EditFloatVec(UIContainer* ctx, float* val, const char* axes = "XYZ", ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");

void PropText(UIContainer* ctx, const char* label, const char* text, ModInitList mods = {});
bool PropButton(UIContainer* ctx, const char* label, const char* text, ModInitList mods = {});
bool PropEditBool(UIContainer* ctx, const char* label, bool& val, ModInitList mods = {});
bool PropEditInt(UIContainer* ctx, const char* label, int& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool PropEditInt(UIContainer* ctx, const char* label, unsigned& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool PropEditInt(UIContainer* ctx, const char* label, int64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool PropEditInt(UIContainer* ctx, const char* label, uint64_t& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool PropEditFloat(UIContainer* ctx, const char* label, float& val, ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool PropEditString(UIContainer* ctx, const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});
bool PropEditColor(UIContainer* ctx, const char* label, Color4f& val, ModInitList mods = {});
bool PropEditColor(UIContainer* ctx, const char* label, Color4b& val, ModInitList mods = {});

// length of `val` = length of `axes`
bool PropEditFloatVec(UIContainer* ctx, const char* label, float* val, const char* axes = "XYZ", ModInitList mods = {}, float speed = DEFAULT_SPEED, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");

} // imm
} // ui
