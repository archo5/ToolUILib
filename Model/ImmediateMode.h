
#pragma once
#include "Objects.h"


namespace ui {
namespace imm {

bool Button(UIContainer* ctx, const char* text, ModInitList mods = {});
bool CheckboxRaw(UIContainer* ctx, bool val, ModInitList mods = {});
bool EditBool(UIContainer* ctx, bool& val, ModInitList mods = {});
template <class T> bool EditFlag(UIContainer* ctx, T& val, T cur, ModInitList mods = {})
{
	if (CheckboxRaw(ctx, (val & cur) == cur, mods))
	{
		if ((val & cur) != cur)
			val |= cur;
		else
			val &= ~cur;
		return true;
	}
	return false;
}
bool RadioButtonRaw(UIContainer* ctx, bool val, const char* text, ModInitList mods = {});
template <class T> bool RadioButton(UIContainer* ctx, T& val, T cur, const char* text, ModInitList mods = {})
{
	if (RadioButtonRaw(ctx, val == cur, text, mods))
	{
		val = cur;
		return true;
	}
	return false;
}
bool EditInt(UIContainer* ctx, UIObject* dragObj, int& val, ModInitList mods = {}, int speed = 1, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool EditInt(UIContainer* ctx, UIObject* dragObj, unsigned& val, ModInitList mods = {}, unsigned speed = 1, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool EditInt(UIContainer* ctx, UIObject* dragObj, int64_t& val, ModInitList mods = {}, int64_t speed = 1, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool EditInt(UIContainer* ctx, UIObject* dragObj, uint64_t& val, ModInitList mods = {}, uint64_t speed = 1, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool EditFloat(UIContainer* ctx, UIObject* dragObj, float& val, ModInitList mods = {}, float speed = 1, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool EditString(UIContainer* ctx, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});

void PropText(UIContainer* ctx, const char* label, const char* text, ModInitList mods = {});
bool PropButton(UIContainer* ctx, const char* label, const char* text, ModInitList mods = {});
bool PropEditBool(UIContainer* ctx, const char* label, bool& val, ModInitList mods = {});
bool PropEditInt(UIContainer* ctx, const char* label, int& val, ModInitList mods = {}, int speed = 1, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool PropEditInt(UIContainer* ctx, const char* label, unsigned& val, ModInitList mods = {}, unsigned speed = 1, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool PropEditInt(UIContainer* ctx, const char* label, int64_t& val, ModInitList mods = {}, int64_t speed = 1, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool PropEditInt(UIContainer* ctx, const char* label, uint64_t& val, ModInitList mods = {}, uint64_t speed = 1, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool PropEditFloat(UIContainer* ctx, const char* label, float& val, ModInitList mods = {}, float speed = 1, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool PropEditString(UIContainer* ctx, const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods = {});

} // imm
} // ui
