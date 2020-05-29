
#include <unordered_set>
#include "Controls.h"
#include "Layout.h"
#include "System.h"
#include "Theme.h"


namespace ui {

Panel::Panel()
{
	styleProps = Theme::current->panel;
}


Button::Button()
{
	styleProps = Theme::current->button;
}

void Button::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
}


void CheckableBase::OnPaint()
{
	style::PaintInfo info(this);
	if (IsSelected())
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);

	PaintChildren();
}

void CheckableBase::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::Activate)
	{
		OnSelect();
		if (onChange)
			onChange();
		e.context->OnChange(this);
	}
}


CheckboxBase::CheckboxBase()
{
	styleProps = Theme::current->checkbox;
}


RadioButtonBase::RadioButtonBase()
{
	styleProps = Theme::current->radioButton;
}


ListBox::ListBox()
{
	styleProps = Theme::current->listBox;
}


void SelectableBase::OnPaint()
{
	UIElement::OnPaint();
}

void SelectableBase::OnEvent(UIEvent& e)
{
	UIElement::OnEvent(e);
}


Selectable::Selectable()
{
	styleProps = Theme::current->button;
}


ProgressBar::ProgressBar()
{
	styleProps = Theme::current->progressBarBase;
	completionBarStyle = Theme::current->progressBarCompletion;
}

void ProgressBar::OnPaint()
{
	styleProps->paint_func(this);

	style::PaintInfo cinfo(this);
	cinfo.rect = cinfo.rect.ShrinkBy(GetMarginRect(completionBarStyle, cinfo.rect.GetWidth()));
	cinfo.rect.x1 = lerp(cinfo.rect.x0, cinfo.rect.x1, progress);
	completionBarStyle->paint_func(cinfo);

	PaintChildren();
}


void Slider::OnInit()
{
	styleProps = Theme::current->sliderHBase;
	trackStyle = Theme::current->sliderHTrack;
	trackFillStyle = Theme::current->sliderHTrackFill;
	thumbStyle = Theme::current->sliderHThumb;
}

void Slider::OnPaint()
{
	styleProps->paint_func(this);

	// track
	style::PaintInfo trkinfo(this);
	float w = trkinfo.rect.GetWidth();
	trkinfo.rect = trkinfo.rect.ShrinkBy(GetMarginRect(trackStyle, w));
	trackStyle->paint_func(trkinfo);

	// track fill
	trkinfo.rect = trkinfo.rect.ShrinkBy(GetPaddingRect(trackStyle, w));
	trkinfo.rect.x1 = round(lerp(trkinfo.rect.x0, trkinfo.rect.x1, ValueToQ(_value)));
	auto prethumb = trkinfo.rect;

	trkinfo.rect = trkinfo.rect.ExtendBy(GetPaddingRect(trackFillStyle, w));
	trackFillStyle->paint_func(trkinfo);

	// thumb
	prethumb.x0 = prethumb.x1;
	trkinfo.rect = prethumb.ExtendBy(GetPaddingRect(thumbStyle, w));
	thumbStyle->paint_func(trkinfo);

	PaintChildren();
}

void Slider::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
	{
		_mxoff = 0;
		// TODO if inside, calc offset
	}
	if (e.type == UIEventType::MouseMove && IsClicked())
	{
		_value = PosToValue(e.x + _mxoff);
		e.context->OnChange(this);
	}
}

double Slider::PosToQ(double x)
{
	auto rect = GetPaddingRect();
	float w = rect.GetWidth();
	rect = rect.ShrinkBy(GetMarginRect(trackStyle, w));
	rect = rect.ShrinkBy(GetPaddingRect(trackStyle, w));
	float fw = rect.GetWidth();
	return std::max(0.0, std::min(1.0, fw ? (x - rect.x0) / fw : 0));
}

double Slider::QToValue(double q)
{
	if (q < 0)
		q = 0;
	else if (q > 1)
		q = 1;

	double v = _limits.min * (1 - q) + _limits.max * q;
	if (_limits.step)
	{
		v = round((v - _limits.min) / _limits.step) * _limits.step + _limits.min;

		if (v > _limits.max + 0.001 * (_limits.max - _limits.min))
			v -= _limits.step;
		if (v < _limits.min)
			v = _limits.min;
	}

	return v;
}

double Slider::ValueToQ(double v)
{
	double diff = _limits.max - _limits.min;
	return std::max(0.0, std::min(1.0, diff ? ((v - _limits.min) / diff) : 0.0));
}


Property::Property()
{
	GetStyle().SetLayout(style::layouts::StackExpand());
	GetStyle().SetStackingDirection(style::StackingDirection::LeftToRight);
}

void Property::Begin(UIContainer* ctx, const char* label)
{
	ctx->Push<Property>();
	if (label)
	{
		Label(ctx, label);
	}
}

void Property::End(UIContainer* ctx)
{
	ctx->Pop();
}

UIObject& Property::Label(UIContainer* ctx, const char* label)
{
	if (*label == '\b')
		return MinLabel(ctx, label + 1);
	return ctx->Text(label)
		+ Padding(5)
		+ MinWidth(100)
		+ Width(style::Coord::Percent(30));
}

UIObject& Property::MinLabel(UIContainer* ctx, const char* label)
{
	return ctx->Text(label)
		+ Padding(5)
		+ Width(style::Coord::Fraction(0));
}

void Property::EditFloat(UIContainer* ctx, const char* label, float* v)
{
	Property::Begin(ctx);
	auto& lbl = Property::Label(ctx, label);
	auto* tb = ctx->Make<Textbox>();
	lbl.HandleEvent() = [v](UIEvent& e)
	{
		if (e.type == UIEventType::MouseMove && e.target->IsClicked() && e.dx != 0)
		{
			*v += 0.1f * e.dx;
			e.context->OnCommit(e.target);
			e.target->RerenderNode();
		}
		if (e.type == UIEventType::SetCursor)
		{
			e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
			e.handled = true;
		}
	};
	char buf[64];
	snprintf(buf, 64, "%g", *v);
	tb->SetText(buf);
	tb->HandleEvent(UIEventType::Commit) = [v, tb](UIEvent& e)
	{
		*v = atof(tb->GetText().c_str());
		e.target->RerenderNode();
	};
	Property::End(ctx);
}

static const char* subLabelNames[] = { "X", "Y", "Z", "W" };
static void EditFloatVec(UIContainer* ctx, const char* label, float* v, int size)
{
	Property::Begin(ctx, label);

	ctx->PushBox()
		+ Layout(style::layouts::StackExpand())
		+ StackingDirection(style::StackingDirection::LeftToRight);
	{
		for (int i = 0; i < size; i++)
		{
			float* vc = v + i;
			ctx->PushBox()
				+ Layout(style::layouts::StackExpand())
				+ StackingDirection(style::StackingDirection::LeftToRight)
				+ Width(style::Coord::Fraction(1));

			Property::MinLabel(ctx, subLabelNames[i]).HandleEvent() = [vc](UIEvent& e)
			{
				if (e.type == UIEventType::MouseMove && e.target->IsClicked() && e.dx != 0)
				{
					*vc += 0.1f * e.dx;
					e.context->OnCommit(e.target);
					e.target->RerenderNode();
				}
				if (e.type == UIEventType::SetCursor)
				{
					e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
					e.handled = true;
				}
			};

			auto& tb = *ctx->Make<Textbox>();
			tb + Width(style::Coord::Fraction(0.2f));
			char buf[64];
			snprintf(buf, 64, "%g", v[i]);
			tb.SetText(buf);
			tb.HandleEvent(UIEventType::Commit) = [vc, &tb](UIEvent& e)
			{
				*vc = atof(tb.GetText().c_str());
				e.target->RerenderNode();
			};

			ctx->Pop();
		}
	}
	ctx->Pop();

	Property::End(ctx);
}

void Property::EditFloat2(UIContainer* ctx, const char* label, float* v)
{
	EditFloatVec(ctx, label, v, 2);
}

void Property::EditFloat3(UIContainer* ctx, const char* label, float* v)
{
	EditFloatVec(ctx, label, v, 3);
}

void Property::EditFloat4(UIContainer* ctx, const char* label, float* v)
{
	EditFloatVec(ctx, label, v, 4);
}


namespace imm {

bool Button(UIContainer* ctx, const char* text, std::initializer_list<ui::Modifier*> mods)
{
	auto* btn = ctx->MakeWithText<ui::Button>(text);
	for (auto* mod : mods)
		mod->Apply(btn);
	btn->HandleEvent(UIEventType::Activate) = [btn](UIEvent&)
	{
		btn->flags |= UIObject_IsEdited;
		btn->RerenderNode();
	};
	bool clicked = false;
	if (btn->flags & UIObject_IsEdited)
	{
		clicked = true;
		btn->flags &= ~UIObject_IsEdited;
		btn->RerenderNode();
	}
	return clicked;
}

bool EditBool(UIContainer* ctx, bool& val, std::initializer_list<ui::Modifier*> mods)
{
	auto* cb = ctx->Make<Checkbox>();
	for (auto* mod : mods)
		mod->Apply(cb);
	bool edited = false;
	if (cb->flags & UIObject_IsEdited)
	{
		val = cb->GetChecked();
		cb->flags &= ~UIObject_IsEdited;
		edited = true;
		cb->RerenderNode();
	}
	cb->HandleEvent(UIEventType::Change) = [cb](UIEvent&)
	{
		cb->flags |= UIObject_IsEdited;
		cb->RerenderNode();
	};
	cb->SetChecked(val);
	return edited;
}

bool CheckboxRaw(UIContainer* ctx, bool val, std::initializer_list<ui::Modifier*> mods)
{
	auto* cb = ctx->Make<ui::Checkbox>();
	for (auto* mod : mods)
		mod->Apply(cb);
	bool edited = false;
	if (cb->flags & UIObject_IsEdited)
	{
		cb->flags &= ~UIObject_IsEdited;
		edited = true;
		cb->RerenderNode();
	}
	cb->HandleEvent(UIEventType::Activate) = [cb](UIEvent&)
	{
		cb->flags |= UIObject_IsEdited;
		cb->RerenderNode();
	};
	cb->SetChecked(val);
	return edited;
}

bool RadioButtonRaw(UIContainer* ctx, bool val, const char* text, std::initializer_list<ui::Modifier*> mods)
{
	auto* cb = text ? ctx->MakeWithText<ui::Checkbox>(text) : ctx->Make<ui::Checkbox>();
	cb->SetStyle(Theme::current->radioButton);
	for (auto* mod : mods)
		mod->Apply(cb);
	bool edited = false;
	if (cb->flags & UIObject_IsEdited)
	{
		cb->flags &= ~UIObject_IsEdited;
		edited = true;
		cb->RerenderNode();
	}
	cb->HandleEvent(UIEventType::Activate) = [cb](UIEvent&)
	{
		cb->flags |= UIObject_IsEdited;
		cb->RerenderNode();
	};
	cb->SetChecked(val);
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

template <class TNum> bool EditNumber(UIContainer* ctx, UIObject* dragObj, TNum& val, std::initializer_list<ui::Modifier*> mods, TNum speed, TNum vmin, TNum vmax, const char* fmt)
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
			if (e.type == UIEventType::MouseMove && e.target->IsClicked() && e.dx != 0)
			{
				if (tb->IsFocused())
					e.context->SetKeyboardFocus(nullptr);
				typename MakeSigned<TNum>::type diff = e.dx * speed;
				TNum nv = val + diff;
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
				e.handled = true;
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

bool EditInt(UIContainer* ctx, UIObject* dragObj, int& val, std::initializer_list<ui::Modifier*> mods, int speed, int vmin, int vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, unsigned& val, std::initializer_list<ui::Modifier*> mods, unsigned speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, int64_t& val, std::initializer_list<ui::Modifier*> mods, int64_t speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditInt(UIContainer* ctx, UIObject* dragObj, uint64_t& val, std::initializer_list<ui::Modifier*> mods, uint64_t speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditFloat(UIContainer* ctx, UIObject* dragObj, float& val, std::initializer_list<ui::Modifier*> mods, float speed, float vmin, float vmax, const char* fmt)
{
	return EditNumber(ctx, dragObj, val, mods, speed, vmin, vmax, fmt);
}

bool EditString(UIContainer* ctx, const char* text, const std::function<void(const char*)>& retfn, std::initializer_list<ui::Modifier*> mods)
{
	auto* tb = ctx->Make<ui::Textbox>();
	for (auto* mod : mods)
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


bool PropButton(UIContainer* ctx, const char* label, const char* text, std::initializer_list<ui::Modifier*> mods)
{
	Property::Begin(ctx, label);
	bool ret = Button(ctx, text, mods);
	Property::End(ctx);
	return ret;
}

bool PropEditBool(UIContainer* ctx, const char* label, bool& val, std::initializer_list<ui::Modifier*> mods)
{
	Property::Begin(ctx, label);
	bool ret = EditBool(ctx, val, mods);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, int& val, std::initializer_list<ui::Modifier*> mods, int speed, int vmin, int vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, unsigned& val, std::initializer_list<ui::Modifier*> mods, unsigned speed, unsigned vmin, unsigned vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, int64_t& val, std::initializer_list<ui::Modifier*> mods, int64_t speed, int64_t vmin, int64_t vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditInt(UIContainer* ctx, const char* label, uint64_t& val, std::initializer_list<ui::Modifier*> mods, uint64_t speed, uint64_t vmin, uint64_t vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditInt(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditFloat(UIContainer* ctx, const char* label, float& val, std::initializer_list<ui::Modifier*> mods, float speed, float vmin, float vmax, const char* fmt)
{
	Property::Begin(ctx);
	auto* lbl = label ? &Property::Label(ctx, label) : nullptr;
	bool ret = EditFloat(ctx, lbl, val, mods, speed, vmin, vmax, fmt);
	Property::End(ctx);
	return ret;
}

bool PropEditString(UIContainer* ctx, const char* label, const char* text, const std::function<void(const char*)>& retfn, std::initializer_list<ui::Modifier*> mods)
{
	Property::Begin(ctx, label);
	bool ret = EditString(ctx, text, retfn, mods);
	Property::End(ctx);
	return ret;
}

} // imm


SplitPane::SplitPane()
{
	// TODO
	vertSepStyle = Theme::current->button;
	style::Accessor(vertSepStyle).SetWidth(8);
	horSepStyle = Theme::current->button;
	style::Accessor(horSepStyle).SetHeight(8);
}

static float SplitQToX(SplitPane* sp, float split)
{
	float hw = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth()) / 2;
	return lerp(sp->finalRectC.x0 + hw, sp->finalRectC.x1 - hw, split);
}

static float SplitQToY(SplitPane* sp, float split)
{
	float hh = sp->ResolveUnits(sp->horSepStyle->height, sp->finalRectC.GetWidth()) / 2;
	return lerp(sp->finalRectC.y0 + hh, sp->finalRectC.y1 - hh, split);
}

static float SplitXToQ(SplitPane* sp, float c)
{
	float hw = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth()) / 2;
	return sp->finalRectC.GetWidth() > hw * 2 ? (c - sp->finalRectC.x0 - hw) / (sp->finalRectC.GetWidth() - hw * 2) : 0;
}

static float SplitYToQ(SplitPane* sp, float c)
{
	float hh = sp->ResolveUnits(sp->horSepStyle->height, sp->finalRectC.GetWidth()) / 2;
	return sp->finalRectC.GetHeight() > hh * 2 ? (c - sp->finalRectC.y0 - hh) / (sp->finalRectC.GetHeight() - hh * 2) : 0;
}

static UIRect GetSplitRectH(SplitPane* sp, int which)
{
	float split = sp->_splits[which];
	float hw = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth()) / 2;
	float x = roundf(lerp(sp->finalRectC.x0 + hw, sp->finalRectC.x1 - hw, split));
	return { x - hw, sp->finalRectC.y0, x + hw, sp->finalRectC.y1 };
}

static UIRect GetSplitRectV(SplitPane* sp, int which)
{
	float split = sp->_splits[which];
	float hh = sp->ResolveUnits(sp->horSepStyle->height, sp->finalRectC.GetWidth()) / 2;
	float y = roundf(lerp(sp->finalRectC.y0 + hh, sp->finalRectC.y1 - hh, split));
	return { sp->finalRectC.x0, y - hh, sp->finalRectC.x1, y + hh };
}

static float SplitWidthAsQ(SplitPane* sp)
{
	if (!sp->_verticalSplit)
	{
		if (sp->finalRectC.GetWidth() == 0)
			return 0;
		float w = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth());
		return w / std::max(w, sp->finalRectC.GetWidth() - w);
	}
	else
	{
		if (sp->finalRectC.GetHeight() == 0)
			return 0;
		float w = sp->ResolveUnits(sp->vertSepStyle->height, sp->finalRectC.GetWidth());
		return w / std::max(w, sp->finalRectC.GetHeight() - w);
	}
}

static void CheckSplits(SplitPane* sp)
{
	// TODO optimize
	size_t cc = sp->CountChildrenImmediate() - 1;
	size_t oldsize = sp->_splits.size();
	if (oldsize < cc)
	{
		// rescale to fit
		float scale = cc ? cc / (float)oldsize : 1.0f;
		for (auto& s : sp->_splits)
			s *= scale;

		sp->_splits.resize(cc);
		for (size_t i = oldsize; i < cc; i++)
			sp->_splits[i] = (i + 1.0f) / (cc + 1.0f);
	}
	else if (oldsize > cc)
	{
		sp->_splits.resize(cc);
		// rescale to fit
		float scale = cc ? oldsize / (float)cc : 1.0f;
		for (auto& s : sp->_splits)
			s *= scale;
	}
}

void SplitPane::OnPaint()
{
	styleProps->paint_func(this);
	PaintChildren();

	style::PaintInfo info(this);
	if (!_verticalSplit)
	{
		for (size_t i = 0; i < _splits.size(); i++)
		{
			auto ii = (uint16_t)i;
			info.rect = GetSplitRectH(this, ii);
			info.state &= ~(style::PS_Hover | style::PS_Down);
			if (_splitUI.IsHovered(ii))
				info.state |= style::PS_Hover;
			if (_splitUI.IsPressed(ii))
				info.state |= style::PS_Down;
			vertSepStyle->paint_func(info);
		}
	}
	else
	{
		for (size_t i = 0; i < _splits.size(); i++)
		{
			auto ii = (uint16_t)i;
			info.rect = GetSplitRectV(this, ii);
			info.state &= ~(style::PS_Hover | style::PS_Down);
			if (_splitUI.IsHovered(ii))
				info.state |= style::PS_Hover;
			if (_splitUI.IsPressed(ii))
				info.state |= style::PS_Down;
			horSepStyle->paint_func(info);
		}
	}
}

void SplitPane::OnEvent(UIEvent& e)
{
	CheckSplits(this);

	_splitUI.InitOnEvent(e);
	for (size_t i = 0; i < _splits.size(); i++)
	{
		if (!_verticalSplit)
		{
			switch (_splitUI.DragOnEvent(uint16_t(i), GetSplitRectH(this, i), e))
			{
			case SubUIDragState::Start:
				_dragOff = SplitQToX(this, _splits[i]) - e.x;
				break;
			case SubUIDragState::Move:
				_splits[i] = SplitXToQ(this, e.x + _dragOff);
				break;
			}
		}
		else
		{
			switch (_splitUI.DragOnEvent(uint16_t(i), GetSplitRectV(this, i), e))
			{
			case SubUIDragState::Start:
				_dragOff = SplitQToY(this, _splits[i]) - e.y;
				break;
			case SubUIDragState::Move:
				_splits[i] = SplitYToQ(this, e.y + _dragOff);
				break;
			}
		}
	}
	if (e.type == UIEventType::SetCursor && _splitUI.IsAnyHovered())
	{
		e.context->SetDefaultCursor(_verticalSplit ? ui::DefaultCursor::ResizeRow : ui::DefaultCursor::ResizeCol);
		e.handled = true;
	}
}

void SplitPane::OnLayout(const UIRect& rect, const Size<float>& containerSize)
{
	CheckSplits(this);

	finalRectCPB = rect;
	finalRectCP = finalRectCPB; // TODO
	finalRectC = finalRectCP.ShrinkBy(GetPaddingRect(styleProps, rect.GetWidth()));

	size_t split = 0;
	if (!_verticalSplit)
	{
		float splitWidth = ResolveUnits(vertSepStyle->width, finalRectC.GetWidth());
		float prevEdge = finalRectC.x0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			UIRect r = finalRectC;
			auto sr = split < _splits.size() ? GetSplitRectH(this, split) : UIRect{ r.x1, 0, r.x1, 0 };
			split++;
			r.x0 = prevEdge;
			r.x1 = sr.x0;
			ch->OnLayout(r, finalRectC.GetSize());
			prevEdge = sr.x1;
		}
	}
	else
	{
		float splitHeight = ResolveUnits(horSepStyle->height, finalRectC.GetWidth());
		float prevEdge = finalRectC.y0;
		for (auto* ch = firstChild; ch; ch = ch->next)
		{
			UIRect r = finalRectC;
			auto sr = split < _splits.size() ? GetSplitRectV(this, split) : UIRect{ 0, r.y1, 0, r.y1 };
			split++;
			r.y0 = prevEdge;
			r.y1 = sr.y0;
			ch->OnLayout(r, finalRectC.GetSize());
			prevEdge = sr.y1;
		}
	}
}

void SplitPane::OnSerialize(IDataSerializer& s)
{
	s << _splitsSet;
	auto size = _splits.size();
	s << size;
	_splits.resize(size);
	for (auto& v : _splits)
		s << v;
}

Range<float> SplitPane::GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type)
{
	return { containerSize.x, containerSize.x };
}

Range<float> SplitPane::GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type)
{
	return { containerSize.y, containerSize.y };
}

#if 0
SplitPane* SplitPane::SetSplit(unsigned which, float f, bool clamp)
{
	CheckSplits(this);
	if (clamp)
	{
		float swq = SplitWidthAsQ(this);
		if (which > 0)
			f = std::max(f, _splits[which - 1] + swq);
		else
			f = std::max(f, 0.0f);
		if (which + 1 < _splits.size())
			f = std::min(f, _splits[which + 1] - swq);
		else
			f = std::min(f, 1.0f);
	}
	_splits[which] = f;
	return this;
}
#endif

SplitPane* SplitPane::SetSplits(std::initializer_list<float> splits, bool firstTimeOnly)
{
	if (!firstTimeOnly || !_splitsSet)
	{
		_splits = splits;
		_splitsSet = true;
	}
	return this;
}

SplitPane* SplitPane::SetDirection(bool vertical)
{
	_verticalSplit = vertical;
	return this;
}


ScrollbarV::ScrollbarV()
{
	trackVStyle = Theme::current->scrollVTrack;
	thumbVStyle = Theme::current->scrollVThumb;
}

style::Coord ScrollbarV::GetWidth()
{
	return trackVStyle->width;
}

UIRect ScrollbarV::GetThumbRect(const ScrollbarData& info)
{
	UIRect rect = info.rect.ShrinkBy(info.owner->GetPaddingRect(trackVStyle, info.rect.GetWidth()));
	float viewportSize = std::max(info.viewportSize, 1.0f);
	float contentSize = std::max(info.contentSize, viewportSize);
	float localMin = std::min(1.0f, std::max(0.0f, info.contentOff / contentSize));
	float localMax = std::min(1.0f, std::max(0.0f, (info.contentOff + viewportSize) / contentSize));
	return { rect.x0, round(lerp(rect.y0, rect.y1, localMin)), rect.x1, round(lerp(rect.y0, rect.y1, localMax)) };
}

void ScrollbarV::OnPaint(const ScrollbarData& info)
{
	style::PaintInfo vsinfo;
	vsinfo.obj = info.owner;
	vsinfo.rect = info.rect;
	trackVStyle->paint_func(vsinfo);

	vsinfo.rect = GetThumbRect(info);
	vsinfo.state = 0;
	if (uiState.IsHovered(0))
		vsinfo.state |= style::PS_Hover;
	if (uiState.IsPressed(0))
		vsinfo.state |= style::PS_Down;
	thumbVStyle->paint_func(vsinfo);
}

void ScrollbarV::OnEvent(const ScrollbarData& info, UIEvent& e)
{
	uiState.InitOnEvent(e);
	float dragSpeed = std::max(1.0f, info.contentSize / std::max(info.viewportSize, 1.0f));
	float maxOff = std::max(info.contentSize - info.viewportSize, 0.0f);
	switch (uiState.DragOnEvent(0, GetThumbRect(info), e))
	{
	case ui::SubUIDragState::Start:
		dragStartContentOff = info.contentOff;
		dragStartCursorPos = e.y;
		break;
	case ui::SubUIDragState::Move:
		info.contentOff = std::min(maxOff, std::max(0.0f, dragStartContentOff + (e.y - dragStartCursorPos) * dragSpeed));
		break;
	}
}


ScrollArea::ScrollArea()
{
}

void ScrollArea::OnPaint()
{
	styleProps->paint_func(this);

	PaintChildren();

	auto cr = GetContentRect();
	float w = cr.GetWidth();
	auto sbvr = cr;
	sbvr.x0 = sbvr.x1 - ResolveUnits(sbv.GetWidth(), w);
	sbv.OnPaint({ this, sbvr, cr.GetHeight(), 300, yoff });
}

void ScrollArea::OnEvent(UIEvent& e)
{
	auto cr = GetContentRect();
	float w = cr.GetWidth();
	auto sbvr = cr;
	sbvr.x0 = sbvr.x1 - ResolveUnits(sbv.GetWidth(), w);
	ScrollbarData info = { this, sbvr, cr.GetHeight(), 300, yoff };

	if (e.type == UIEventType::MouseScroll)
	{
		yoff -= e.dy / 4;
		yoff = std::min(std::max(info.contentSize - info.viewportSize, 0.0f), std::max(0.0f, yoff));
	}
	sbv.OnEvent(info, e);
}

void ScrollArea::OnLayout(const UIRect& rect, const Size<float>& containerSize)
{
	UIRect r = rect;
	r.y0 -= yoff;
	r.y1 -= yoff;

	UIElement::OnLayout(r, containerSize);

	finalRectC.y0 += yoff;
	finalRectC.y1 += yoff;
	finalRectCP.y0 += yoff;
	finalRectCP.y1 += yoff;
	finalRectCPB.y0 += yoff;
	finalRectCPB.y1 += yoff;
}

void ScrollArea::OnSerialize(IDataSerializer& s)
{
	s << yoff;
}


TabGroup::TabGroup()
{
	styleProps = Theme::current->tabGroup;
}

void TabGroup::OnInit()
{
	_curButton = 0;
	_curPanel = 0;
}

void TabGroup::OnPaint()
{
	styleProps->paint_func(this);

	for (UIObject* ch = firstChild; ch; ch = ch->next)
	{
		if (auto* p = dynamic_cast<TabPanel*>(ch))
			if (p->id != active)
				continue;
		ch->OnPaint();
	}
}

void TabGroup::OnSerialize(IDataSerializer& s)
{
	s << active;
}


TabList::TabList()
{
	styleProps = Theme::current->tabList;
}

void TabList::OnPaint()
{
	styleProps->paint_func(this);

	if (auto* g = FindParentOfType<TabGroup>())
	{
		for (UIObject* ch = firstChild; ch; ch = ch->next)
		{
			if (auto* p = dynamic_cast<TabButton*>(ch))
				if (p->id == g->active)
					continue;
			ch->OnPaint();
		}
	}
	else
		PaintChildren();
}


TabButton::TabButton()
{
	styleProps = Theme::current->tabButton;
}

void TabButton::OnInit()
{
	if (auto* g = FindParentOfType<TabGroup>())
	{
		id = g->_curButton++;
		if (id == g->active)
			g->_activeBtn = this;
	}
}

void TabButton::OnDestroy()
{
	if (auto* g = FindParentOfType<TabGroup>())
		if (g->_activeBtn == this)
			g->_activeBtn = nullptr;
}

void TabButton::OnPaint()
{
	style::PaintInfo info(this);
	if (FindParentOfType<TabGroup>()->active == id)
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);
	PaintChildren();
}

void TabButton::OnEvent(UIEvent& e)
{
	if ((e.type == UIEventType::Activate || e.type == UIEventType::ButtonDown) && IsChildOrSame(e.GetTargetNode()))
	{
		if (auto* g = FindParentOfType<TabGroup>())
		{
			g->active = id;
			g->_activeBtn = this;
			g->RerenderNode();
		}
	}
}


TabPanel::TabPanel()
{
	styleProps = Theme::current->tabPanel;
}

void TabPanel::OnInit()
{
	if (auto* g = FindParentOfType<TabGroup>())
		id = g->_curPanel++;
}

void TabPanel::OnPaint()
{
	styleProps->paint_func(this);

	if (auto* g = FindParentOfType<TabGroup>())
		if (g->_activeBtn)
			g->_activeBtn->Paint();

	PaintChildren();
}


Textbox::Textbox()
{
	styleProps = Theme::current->textBoxBase;
}

void Textbox::OnPaint()
{
	// background
	styleProps->paint_func(this);

	{
		auto r = GetContentRect();
		DrawTextLine(r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, _text.c_str(), 1, 1, 1);

		if (IsFocused())
		{
			if (startCursor != endCursor)
			{
				int minpos = startCursor < endCursor ? startCursor : endCursor;
				int maxpos = startCursor > endCursor ? startCursor : endCursor;
				float x0 = GetTextWidth(_text.c_str(), minpos);
				float x1 = GetTextWidth(_text.c_str(), maxpos);

				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(0.5f, 0.7f, 0.9f, 0.4f);
				br.Quad(r.x0 + x0, r.y0, r.x0 + x1, r.y1, 0, 0, 1, 1);
				br.End();
			}

			if (showCaretState)
			{
				float x = GetTextWidth(_text.c_str(), endCursor);
				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(1, 1, 1);
				br.Quad(r.x0 + x, r.y0, r.x0 + x + 1, r.y1, 0, 0, 1, 1);
				br.End();
			}
		}
	}

	PaintChildren();
}

void Textbox::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		if (e.GetButton() == UIMouseButton::Left)
			startCursor = endCursor = _FindCursorPos(e.x);
		if (!IsInputDisabled())
			e.context->SetKeyboardFocus(this);
	}
	else if (e.type == UIEventType::ButtonUp)
	{
		if (e.GetButton() == UIMouseButton::Left)
			endCursor = _FindCursorPos(e.x);
	}
	else if (e.type == UIEventType::MouseMove)
	{
		if (IsClicked(0))
			endCursor = _FindCursorPos(e.x);
	}
	else if (e.type == UIEventType::SetCursor)
	{
		e.context->SetDefaultCursor(DefaultCursor::Text);
		e.handled = true;
	}
	else if (e.type == UIEventType::Timer)
	{
		showCaretState = !showCaretState;
		if (IsFocused())
			e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == UIEventType::GotFocus)
	{
		showCaretState = true;
		e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == UIEventType::LostFocus)
	{
		e.context->OnCommit(this);
	}
	else if (e.type == UIEventType::KeyAction)
	{
		switch (e.GetKeyAction())
		{
		case UIKeyAction::Enter:
			e.context->SetKeyboardFocus(nullptr);
			break;
		case UIKeyAction::Backspace:
			if (IsInputDisabled())
				break;
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor > 0)
				_text.erase(startCursor = --endCursor, 1); // TODO unicode
			e.context->OnChange(this);
			break;
		case UIKeyAction::Delete:
			if (IsInputDisabled())
				break;
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor + 1 < _text.size())
				_text.erase(endCursor, 1); // TODO unicode
			e.context->OnChange(this);
			break;

		case UIKeyAction::Left:
			if (IsLongSelection())
				startCursor = endCursor = startCursor < endCursor ? startCursor : endCursor;
			else if (endCursor > 0)
				startCursor = --endCursor;
			break;
		case UIKeyAction::Right:
			if (IsLongSelection())
				startCursor = endCursor = startCursor > endCursor ? startCursor : endCursor;
			else if (endCursor < _text.size())
				startCursor = ++endCursor;
			break;
		case UIKeyAction::Home:
			startCursor = endCursor = 0;
			break;
		case UIKeyAction::End:
			startCursor = endCursor = _text.size();
			break;

		case UIKeyAction::SelectAll:
			startCursor = 0;
			endCursor = _text.size();
			break;
		}
	}
	else if (e.type == UIEventType::TextInput)
	{
		if (!IsInputDisabled())
		{
			char ch[5];
			if (e.GetUTF32Char() >= 32 && e.GetUTF8Text(ch))
				EnterText(ch);
		}
	}
}

void Textbox::OnSerialize(IDataSerializer& s)
{
	s << startCursor << endCursor << showCaretState;

	uint32_t len = _text.size();
	s << len;
	_text.resize(len);
	if (len)
		s.Process(&_text[0], len);
}

void Textbox::EnterText(const char* str)
{
	EraseSelection();
	size_t num = strlen(str);
	_text.insert(endCursor, str, num);
	startCursor = endCursor += num;
	system->eventSystem.OnChange(this);
}

void Textbox::EraseSelection()
{
	if (IsLongSelection())
	{
		int min = startCursor < endCursor ? startCursor : endCursor;
		int max = startCursor > endCursor ? startCursor : endCursor;
		_text.erase(min, max - min);
		startCursor = endCursor = min;
	}
}

int Textbox::_FindCursorPos(float vpx)
{
	auto r = GetContentRect();
	// TODO kerning
	float x = r.x0;
	for (size_t i = 0; i < _text.size(); i++)
	{
		float lw = GetTextWidth(&_text[i], 1);
		if (vpx < x + lw * 0.5f)
			return i;
		x += lw;
	}
	return _text.size();
}

Textbox& Textbox::SetText(const std::string& s)
{
	if (InUse())
		return *this;
	_text = s;
	if (startCursor > _text.size())
		startCursor = _text.size();
	if (endCursor > _text.size())
		endCursor = _text.size();
	return *this;
}

Textbox& Textbox::Init(float& val)
{
	if (!InUse())
	{
		char bfr[64];
		snprintf(bfr, 64, "%g", val);
		SetText(bfr);
	}
	HandleEvent(UIEventType::Change) = [this, &val](UIEvent&) { val = atof(GetText().c_str()); };
	return *this;
}


CollapsibleTreeNode::CollapsibleTreeNode()
{
	styleProps = Theme::current->collapsibleTreeNode;
}

void CollapsibleTreeNode::OnPaint()
{
	style::PaintInfo info(this);
	info.state &= ~style::PS_Hover;
	if (_hovered)
		info.state |= style::PS_Hover;
	if (open)
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);
	PaintChildren();
}

void CollapsibleTreeNode::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::MouseEnter || e.type == UIEventType::MouseMove)
	{
		auto r = GetPaddingRect();
		float h = GetFontHeight();
		_hovered = e.x >= r.x0 && e.x < r.x0 + h && e.y >= r.y0 && e.y < r.y0 + h;
	}
	if (e.type == UIEventType::MouseLeave)
	{
		_hovered = false;
	}
	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && _hovered)
	{
		open = !open;
		e.GetTargetNode()->Rerender();
	}
}

void CollapsibleTreeNode::OnSerialize(IDataSerializer& s)
{
	s << open;
}


struct Selection1DImpl
{
	std::unordered_set<uintptr_t> sel;
};

Selection1D::Selection1D()
{
	_impl = new Selection1DImpl;
}

Selection1D::~Selection1D()
{
	delete _impl;
}

void Selection1D::OnSerialize(IDataSerializer& s)
{
	auto size = _impl->sel.size();
	s << size;
	if (s.IsWriter())
	{
		for (auto v : _impl->sel)
			s << v;
	}
	else
	{
		for (size_t i = 0; i < size; i++)
		{
			uintptr_t v;
			s << v;
			_impl->sel.insert(v);
		}
	}
}

void Selection1D::Clear()
{
	_impl->sel.clear();
}

bool Selection1D::AnySelected()
{
	return _impl->sel.size() > 0;
}

uintptr_t Selection1D::GetFirstSelection()
{
	for (auto v : _impl->sel)
		return v;
	return UINTPTR_MAX;
}

bool Selection1D::IsSelected(uintptr_t id)
{
	return _impl->sel.find(id) != _impl->sel.end();
}

void Selection1D::SetSelected(uintptr_t id, bool sel)
{
	if (sel)
		_impl->sel.insert(id);
	else
		_impl->sel.erase(id);
}


struct TableViewImpl
{
	TableDataSource* dataSource = nullptr;
	bool firstColWidthCalc = true;
	std::vector<float> colEnds = { 1.0f };
	size_t hoverRow = SIZE_MAX;
};

TableView::TableView()
{
	_impl = new TableViewImpl;
	styleProps = Theme::current->tableBase;
	cellStyle = Theme::current->tableCell;
	rowHeaderStyle = Theme::current->tableRowHeader;
	colHeaderStyle = Theme::current->tableColHeader;
}

TableView::~TableView()
{
	delete _impl;
}

void TableView::OnPaint()
{
	styleProps->paint_func(this);

	size_t nc = _impl->dataSource->GetNumCols();
	size_t nr = _impl->dataSource->GetNumRows();

	style::PaintInfo info(this);

	auto RC = GetContentRect();

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhw = 80 + padRH.x0 + padRH.x1;
	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	sbrect.y0 += chh;
	RC.x1 -= sbw;

	size_t minR = floor(yOff / h);
	size_t maxR = size_t(floor((yOff + sbrect.GetHeight()) / h));
	if (maxR > nr)
		maxR = nr;

	// - row header
	GL::PushScissorRect(RC.x0, RC.y0 + chh, RC.x0 + rhw, RC.y1);
	// background:
	for (size_t r = minR; r < maxR; r++)
	{
		info.rect =
		{
			RC.x0,
			RC.y0 + chh - yOff + h * r,
			RC.x0 + rhw,
			RC.y0 + chh - yOff + h * (r + 1),
		};
		rowHeaderStyle->paint_func(info);
	}
	// text:
	for (size_t r = minR; r < maxR; r++)
	{
		UIRect rect =
		{
			RC.x0,
			RC.y0 + chh - yOff + h * r,
			RC.x0 + rhw,
			RC.y0 + chh - yOff + h * (r + 1),
		};
		rect = rect.ShrinkBy(padRH);
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetRowName(r).c_str(), 1, 1, 1);
	}
	GL::PopScissorRect();

	// - column header
	GL::PushScissorRect(RC.x0 + rhw, RC.y0, RC.x1, RC.y0 + chh);
	// background:
	for (size_t c = 0; c < nc; c++)
	{
		info.rect =
		{
			RC.x0 + rhw + _impl->colEnds[c],
			RC.y0,
			RC.x0 + rhw + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		colHeaderStyle->paint_func(info);
	}
	// text:
	for (size_t c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + rhw + _impl->colEnds[c],
			RC.y0,
			RC.x0 + rhw + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		rect = rect.ShrinkBy(padCH);
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetColName(c).c_str(), 1, 1, 1);
	}
	GL::PopScissorRect();

	// - cells
	GL::PushScissorRect(RC.x0 + rhw, RC.y0 + chh, RC.x1, RC.y1);
	// background:
	for (size_t r = minR; r < maxR; r++)
	{
		for (size_t c = 0; c < nc; c++)
		{
			info.rect =
			{
				RC.x0 + rhw + _impl->colEnds[c],
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[c + 1],
				RC.y0 + chh - yOff + h * (r + 1),
			};
			if (selection.IsSelected(r))
				info.state |= style::PS_Checked;
			else
				info.state &= ~style::PS_Checked;
			if (_impl->hoverRow == r)
				info.state |= style::PS_Hover;
			else
				info.state &= ~style::PS_Hover;
			cellStyle->paint_func(info);
		}
	}
	// text:
	for (size_t r = minR; r < maxR; r++)
	{
		for (size_t c = 0; c < nc; c++)
		{
			UIRect rect =
			{
				RC.x0 + rhw + _impl->colEnds[c],
				RC.y0 + chh - yOff + h * r,
				RC.x0 + rhw + _impl->colEnds[c + 1],
				RC.y0 + chh - yOff + h * (r + 1),
			};
			rect = rect.ShrinkBy(padC);
			DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetText(r, c).c_str(), 1, 1, 1);
		}
	}
	GL::PopScissorRect();

	scrollbarV.OnPaint({ this, sbrect, sbrect.GetHeight(), chh + nr * h, yOff });

	PaintChildren();
}

void TableView::OnEvent(UIEvent& e)
{
	size_t nr = _impl->dataSource->GetNumRows();

	auto RC = GetContentRect();

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhw = 80 + padRH.x0 + padRH.x1;
	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

	float sbw = ResolveUnits(scrollbarV.GetWidth(), RC.GetWidth());
	auto sbrect = RC;
	sbrect.x0 = sbrect.x1 - sbw;
	sbrect.y0 += chh;
	RC.x1 -= sbw;
	ScrollbarData sbd = { this, sbrect, RC.GetHeight(), chh + nr * h, yOff };
	scrollbarV.OnEvent(sbd, e);

	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::MouseMove)
	{
		if (e.x < RC.x1 && e.y > RC.y0 + chh)
			_impl->hoverRow = GetRowAt(e.y);
		else
			_impl->hoverRow = SIZE_MAX;
	}
	if (e.type == UIEventType::MouseLeave)
	{
		_impl->hoverRow = SIZE_MAX;
	}
	if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Left)
	{
		if (e.x < RC.x1 && e.y > RC.y0 + chh)
		{
			selection.Clear();
			size_t at = GetRowAt(e.y);
			if (at < SIZE_MAX)
				selection.SetSelected(at, true);
			e.handled = true;

			UIEvent selev(e.context, this, UIEventType::SelectionChange);
			e.context->BubblingEvent(selev);
		}
	}
}

void TableView::OnSerialize(IDataSerializer& s)
{
	s << _impl->firstColWidthCalc;
	auto size = _impl->colEnds.size();
	s << size;
	_impl->colEnds.resize(size);
	for (auto& v : _impl->colEnds)
		s << v;
	s << yOff;
	selection.OnSerialize(s);
}

void TableView::Render(UIContainer* ctx)
{
}

TableDataSource* TableView::GetDataSource() const
{
	return _impl->dataSource;
}

void TableView::SetDataSource(TableDataSource* src)
{
	if (src != _impl->dataSource)
		_impl->firstColWidthCalc = true;
	_impl->dataSource = src;

	size_t nc = src->GetNumCols();
	while (_impl->colEnds.size() < nc + 1)
		_impl->colEnds.push_back(_impl->colEnds.back() + 100);
	while (_impl->colEnds.size() > nc + 1)
		_impl->colEnds.pop_back();

	Rerender();
}

void TableView::CalculateColumnWidths(bool includeHeader, bool firstTimeOnly)
{
	if (firstTimeOnly && !_impl->firstColWidthCalc)
		return;

	_impl->firstColWidthCalc = false;

	auto nc = _impl->dataSource->GetNumCols();
	std::vector<float> colWidths;
	colWidths.resize(nc, 0.0f);

	auto RC = GetContentRect();
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	if (includeHeader)
	{
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetColName(c);
			float w = GetTextWidth(text.c_str()) + padCH.x0 + padCH.x1;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
	}

	for (size_t i = 0, n = _impl->dataSource->GetNumRows(); i < n; i++)
	{
		for (size_t c = 0; c < nc; c++)
		{
			std::string text = _impl->dataSource->GetText(i, c);
			float w = GetTextWidth(text.c_str()) + padC.x0 + padC.x1;
			if (colWidths[c] < w)
				colWidths[c] = w;
		}
	}

	for (size_t c = 0; c < nc; c++)
		_impl->colEnds[c + 1] = _impl->colEnds[c] + colWidths[c];
}

bool TableView::IsValidRow(uintptr_t pos)
{
	return pos < _impl->dataSource->GetNumRows();
}

size_t TableView::GetRowAt(float y)
{
	auto RC = GetContentRect();

	auto padRH = GetPaddingRect(rowHeaderStyle, RC.GetWidth());
	auto padCH = GetPaddingRect(colHeaderStyle, RC.GetWidth());
	auto padC = GetPaddingRect(cellStyle, RC.GetWidth());

	float rhh = 20 + padRH.y0 + padRH.y1;
	float chh = 20 + padCH.y0 + padCH.y1;
	float cellh = 20 + padC.y0 + padC.y1;
	float h = std::max(rhh, cellh);

	y += yOff;
	y -= RC.y0;
	y -= chh;
	y = floor(y / h);
	size_t row = y;
	size_t numRows = _impl->dataSource->GetNumRows();
	return row < numRows ? row : SIZE_MAX;
}

size_t TableView::GetHoverRow() const
{
	return _impl->hoverRow;
}


struct TreeView::PaintState
{
	style::PaintInfo info;
	int nc;
	float x;
	float y;
};

struct TreeViewImpl
{
	TreeDataSource* dataSource = nullptr;
	bool firstColWidthCalc = true;
	std::vector<float> colEnds = { 1.0f };
};

TreeView::TreeView()
{
	_impl = new TreeViewImpl;
	styleProps = Theme::current->tableBase;
	cellStyle = Theme::current->tableCell;
	expandButtonStyle = Theme::current->collapsibleTreeNode;
	colHeaderStyle = Theme::current->tableColHeader;
}

TreeView::~TreeView()
{
	delete _impl;
}

void TreeView::OnPaint()
{
	styleProps->paint_func(this);

	int chh = 20;
	int h = 20;

	int nc = _impl->dataSource->GetNumCols();
	int nr = 0;// _dataSource->GetNumRows();

	style::PaintInfo info(this);

	auto RC = GetContentRect();

	// backgrounds
	for (int c = 0; c < nc; c++)
	{
		info.rect =
		{
			RC.x0 + _impl->colEnds[c],
			RC.y0,
			RC.x0 + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		colHeaderStyle->paint_func(info);
	}

	// text
	for (int c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + _impl->colEnds[c],
			RC.y0,
			RC.x0 + _impl->colEnds[c + 1],
			RC.y0 + chh,
		};
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetColName(c).c_str(), 1, 1, 1);
	}

	PaintState ps = { info, nc, RC.x0, RC.y0 + chh };
	for (size_t i = 0, n = _impl->dataSource->GetChildCount(TreeDataSource::ROOT); i < n; i++)
		_PaintOne(_impl->dataSource->GetChild(TreeDataSource::ROOT, i), 0, ps);

	PaintChildren();
}

void TreeView::_PaintOne(uintptr_t id, int lvl, PaintState& ps)
{
	int h = 20;
	int tab = 20;

	// backgrounds
	for (int c = 0; c < ps.nc; c++)
	{
		ps.info.rect =
		{
			ps.x + _impl->colEnds[c],
			ps.y,
			ps.x + _impl->colEnds[c + 1],
			ps.y + h,
		};
		cellStyle->paint_func(ps.info);
	}

	// text
	for (int c = 0; c < ps.nc; c++)
	{
		UIRect rect =
		{
			ps.x + _impl->colEnds[c],
			ps.y,
			ps.x + _impl->colEnds[c + 1],
			ps.y + h,
		};
		if (c == 0)
			rect.x0 += tab * lvl;
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _impl->dataSource->GetText(id, c).c_str(), 1, 1, 1);
	}

	ps.y += h;

	if (true) // open
	{
		for (size_t i = 0, n = _impl->dataSource->GetChildCount(id); i < n; i++)
			_PaintOne(_impl->dataSource->GetChild(id, i), lvl + 1, ps);
	}
}

void TreeView::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
	}
}

void TreeView::Render(UIContainer* ctx)
{
}

TreeDataSource* TreeView::GetDataSource() const
{
	return _impl->dataSource;
}

void TreeView::SetDataSource(TreeDataSource* src)
{
	if (src != _impl->dataSource)
		_impl->firstColWidthCalc = true;
	_impl->dataSource = src;
	
	size_t nc = src->GetNumCols();
	while (_impl->colEnds.size() < nc + 1)
		_impl->colEnds.push_back(_impl->colEnds.back() + 100);
	while (_impl->colEnds.size() > nc + 1)
		_impl->colEnds.pop_back();
	
	Rerender();
}

void TreeView::CalculateColumnWidths(bool firstTimeOnly)
{
	if (firstTimeOnly && !_impl->firstColWidthCalc)
		return;

	_impl->firstColWidthCalc = false;

	int tab = 20;

	auto nc = _impl->dataSource->GetNumCols();
	std::vector<float> colWidths;
	colWidths.resize(nc, 0.0f);

	struct Entry
	{
		uintptr_t id;
		int lev;
	};
	std::vector<Entry> stack = { Entry{ TreeDataSource::ROOT, 0 } };
	while (!stack.empty())
	{
		Entry E = stack.back();
		stack.pop_back();

		for (size_t i = 0, n = _impl->dataSource->GetChildCount(E.id); i < n; i++)
		{
			uintptr_t chid = _impl->dataSource->GetChild(E.id, i);
			stack.push_back({ chid, E.lev + 1 });

			for (size_t c = 0; c < nc; c++)
			{
				std::string text = _impl->dataSource->GetText(chid, c);
				float w = GetTextWidth(text.c_str());
				if (c == 0)
					w += tab * E.lev;
				if (colWidths[c] < w)
					colWidths[c] = w;
			}
		}
	}

	for (size_t c = 0; c < nc; c++)
		_impl->colEnds[c + 1] = _impl->colEnds[c] + colWidths[c];
}

} // ui
