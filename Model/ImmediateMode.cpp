
#include "ImmediateMode.h"
#include "Controls.h"
#include "System.h"


namespace ui {
namespace imm {

bool Button(UIContainer* ctx, const char* text, ModInitList mods)
{
	auto* btn = ctx->MakeWithText<ui::Button>(text);
	btn->flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(btn);
	bool clicked = false;
	if (btn->flags & UIObject_IsEdited)
	{
		clicked = true;
		btn->flags &= ~UIObject_IsEdited;
		btn->RerenderNode();
	}
	return clicked;
}

bool CheckboxRaw(UIContainer* ctx, bool val, ModInitList mods)
{
	auto* cb = ctx->Make<ui::Checkbox>();
	cb->flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(cb);
	bool edited = false;
	if (cb->flags & UIObject_IsEdited)
	{
		cb->flags &= ~UIObject_IsEdited;
		edited = true;
		cb->RerenderNode();
	}
	cb->Init(val);
	return edited;
}

bool EditBool(UIContainer* ctx, bool& val, ModInitList mods)
{
	if (CheckboxRaw(ctx, val, mods))
	{
		val = !val;
		return true;
	}
	return false;
}

bool RadioButtonRaw(UIContainer* ctx, bool val, const char* text, ModInitList mods)
{
	auto* rb = text ? ctx->MakeWithText<ui::RadioButton>(text) : ctx->Make<ui::RadioButton>();
	rb->flags |= UIObject_DB_IMEdit;
	for (auto& mod : mods)
		mod->Apply(rb);
	bool edited = false;
	if (rb->flags & UIObject_IsEdited)
	{
		rb->flags &= ~UIObject_IsEdited;
		edited = true;
		rb->RerenderNode();
	}
	rb->Init(val);
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

template <class TNum> bool EditNumber(UIContainer* ctx, UIObject* dragObj, TNum& val, ModInitList mods, float speed, TNum vmin, TNum vmax, const char* fmt)
{
	auto* tb = ctx->Make<Textbox>();
	for (auto& mod : mods)
		mod->Apply(tb);

	NumFmtBox fb(fmt);

	bool edited = false;
	if (tb->flags & UIObject_IsEdited)
	{
		decltype(val + 0) tmp = 0;
		sscanf(tb->GetText().c_str(), fb.fmt, &tmp);
		if (tmp == 0)
			tmp = 0;
		if (tmp > vmax)
			tmp = vmax;
		if (tmp < vmin)
			tmp = vmin;
		val = tmp;
		tb->flags &= ~UIObject_IsEdited;
		edited = true;
		tb->RerenderNode();
	}

	char buf[1024];
	snprintf(buf, 1024, fb.fmt + 1, val);
	tb->SetText(RemoveNegZero(buf));

	if (dragObj)
	{
		dragObj->HandleEvent() = [val, speed, vmin, vmax, tb, fb](UIEvent& e)
		{
			if (tb->IsInputDisabled())
				return;
			if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
			{
				e.context->CaptureMouse(e.current);
				e.current->flags |= UIObject_IsPressedMouse;
			}
			if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
			{
				e.current->flags &= ~UIObject_IsPressedMouse;
				e.context->ReleaseMouse();
			}
			if (e.type == UIEventType::MouseMove && e.target->IsPressed() && e.dx != 0)
			{
				if (tb->IsFocused())
					e.context->SetKeyboardFocus(nullptr);

				float diff = e.dx * speed * UNITS_PER_PX;
				tb->accumulator += diff;
				TNum nv = val;
				if (fabsf(tb->accumulator) >= speed)
				{
					nv += trunc(tb->accumulator / speed);
					tb->accumulator = fmodf(tb->accumulator, speed);
				}

				if (nv > vmax || (diff > 0 && nv < val))
					nv = vmax;
				if (nv < vmin || (diff < 0 && nv > val))
					nv = vmin;

				char buf[1024];
				snprintf(buf, 1024, fb.fmt + 1, nv);
				tb->SetText(RemoveNegZero(buf));
				tb->flags |= UIObject_IsEdited;

				e.context->OnCommit(e.target);
				e.target->RerenderNode();
			}
			if (e.type == UIEventType::SetCursor)
			{
				e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
				e.StopPropagation();
			}
		};
	}
	tb->HandleEvent(UIEventType::Commit) = [tb](UIEvent& e)
	{
		tb->flags |= UIObject_IsEdited;
		e.target->RerenderNode();
	};

	return edited;
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, int& val, ModInitList mods, float speed, int vmin, int vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, unsigned& val, ModInitList mods, float speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, int64_t& val, ModInitList mods, float speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, uint64_t& val, ModInitList mods, float speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditFloat(UIContainer* ctx, UIObject* dragObj, float& val, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditString(UIContainer* ctx, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	auto* tb = ctx->Make<ui::Textbox>();
	for (auto& mod : mods)
		mod->Apply(tb);
	bool changed = false;
	if (tb->flags & UIObject_IsEdited)
	{
		retfn(tb->GetText().c_str());
		tb->flags &= ~UIObject_IsEdited;
		tb->RerenderNode();
		changed = true;
	}
	else // text can be invalidated if retfn is called
		tb->SetText(text);
	tb->HandleEvent(UIEventType::Change) = [tb](UIEvent&)
	{
		tb->flags |= UIObject_IsEdited;
		tb->RerenderNode();
	};
	return changed;
}


void PropText(UIContainer* ctx, const char* label, const char* text, ModInitList mods)
{
	Property::Begin(ctx, label);
	auto& ctrl = ctx->Text(text) + ui::Padding(5);
	for (auto& mod : mods)
		mod->Apply(&ctrl);
	Property::End(ctx);
}

bool PropButton(UIContainer* ctx, const char* label, const char* text, ModInitList mods)
{
	Property::Begin(ctx, label);
	bool ret = Button(ctx, text, mods);
	Property::End(ctx);
	return ret;
}

bool PropEditBool(UIContainer* ctx, const char* label, bool& val, ModInitList mods)
{
	Property::Begin(ctx, label);
	bool ret = EditBool(ctx, val, mods);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, int& val, ModInitList mods, float speed, int vmin, int vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, unsigned& val, ModInitList mods, float speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, int64_t& val, ModInitList mods, float speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, uint64_t& val, ModInitList mods, float speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditFloat(UIContainer* ctx, const char* label, float& val, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditFloat(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditString(UIContainer* ctx, const char* label, const char* text, const std::function<void(const char*)>& retfn, ModInitList mods)
{
	Property::Begin(ctx, label);
	bool ret = EditString(ctx, text, retfn, mods);
	Property::End(ctx);
	return ret;
}

bool PropEditFloatVec(UIContainer* ctx, const char* label, float* val, const char* axes, ModInitList mods, float speed, float vmin, float vmax, const char* fmt)
{
	Property::Begin(ctx, label);
	bool any = false;
	char axisLabel[3] = "\b\0";
	while (*axes)
	{
		axisLabel[1] = *axes++;
		any |= PropEditFloat(ctx, axisLabel, *val++, mods, speed, vmin, vmax, fmt);
	}
	Property::End(ctx);
	return any;
}

} // imm
} // ui
