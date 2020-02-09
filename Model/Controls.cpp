
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
	if (e.type == UIEventType::Activate)
	{
		if (IsInputDisabled())
		{
			e.handled = true;
			return;
		}
		if (onClick)
			onClick();
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
		if (IsInputDisabled())
		{
			e.handled = true;
			return;
		}
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
	tb->text = buf;
	tb->HandleEvent(UIEventType::Commit) = [v, tb](UIEvent& e)
	{
		*v = atof(tb->text.c_str());
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
			tb.text = buf;
			tb.HandleEvent(UIEventType::Commit) = [vc, &tb](UIEvent& e)
			{
				*vc = atof(tb.text.c_str());
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
		float w = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth());
		return w / std::max(w, sp->finalRectC.GetWidth() - w);
	}
	else
	{
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
				SetSplit(i, SplitXToQ(this, e.x + _dragOff));
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
				SetSplit(i, SplitYToQ(this, e.y + _dragOff));
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

Range<float> SplitPane::GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type)
{
	return { containerSize.x, containerSize.x };
}

Range<float> SplitPane::GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type)
{
	return { containerSize.y, containerSize.y };
}

float SplitPane::GetSplit(unsigned which)
{
	CheckSplits(this);
	if (which >= _splits.size())
		return 1;
	return _splits[which];
}

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

SplitPane* SplitPane::SetDirection(bool vertical)
{
	_verticalSplit = vertical;
	return this;
}


ScrollArea::ScrollArea()
{
	trackVStyle = Theme::current->scrollVTrack;
	thumbVStyle = Theme::current->scrollVThumb;
}

void ScrollArea::OnPaint()
{
	styleProps->paint_func(this);

	PaintChildren();

	float w = GetContentRect().GetWidth();
	style::PaintInfo vsinfo(this);
	vsinfo.rect.x0 = vsinfo.rect.x1 - ResolveUnits(trackVStyle->width, w);
	vsinfo.state &= ~style::PS_Down;
	vsinfo.state &= ~style::PS_Hover;
	trackVStyle->paint_func(vsinfo);
	vsinfo.rect = vsinfo.rect.ShrinkBy(GetPaddingRect(trackVStyle, w));
	vsinfo.rect.y0 += 10;
	vsinfo.rect.y1 -= 10;
	thumbVStyle->paint_func(vsinfo);
}

void ScrollArea::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::MouseScroll)
	{
		yoff -= e.dy / 4;
	}
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
		DrawTextLine(r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text.c_str(), 1, 1, 1);

		if (IsFocused())
		{
			if (startCursor != endCursor)
			{
				int minpos = startCursor < endCursor ? startCursor : endCursor;
				int maxpos = startCursor > endCursor ? startCursor : endCursor;
				float x0 = GetTextWidth(text.c_str(), minpos);
				float x1 = GetTextWidth(text.c_str(), maxpos);

				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(0.5f, 0.7f, 0.9f, 0.4f);
				br.Quad(r.x0 + x0, r.y0, r.x0 + x1, r.y1, 0, 0, 1, 1);
				br.End();
			}

			if (showCaretState)
			{
				float x = GetTextWidth(text.c_str(), endCursor);
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
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor > 0)
				text.erase(startCursor = --endCursor, 1); // TODO unicode
			e.context->OnChange(this);
			break;
		case UIKeyAction::Delete:
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor + 1 < text.size())
				text.erase(endCursor, 1); // TODO unicode
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
			else if (endCursor < text.size())
				startCursor = ++endCursor;
			break;
		case UIKeyAction::Home:
			startCursor = endCursor = 0;
			break;
		case UIKeyAction::End:
			startCursor = endCursor = text.size();
			break;

		case UIKeyAction::SelectAll:
			startCursor = 0;
			endCursor = text.size();
			break;
		}
	}
	else if (e.type == UIEventType::TextInput)
	{
		char ch[5];
		if (e.GetUTF32Char() >= 32 && e.GetUTF8Text(ch))
			EnterText(ch);
	}
}

void Textbox::EnterText(const char* str)
{
	EraseSelection();
	size_t num = strlen(str);
	text.insert(endCursor, str, num);
	startCursor = endCursor += num;
	system->eventSystem.OnChange(this);
}

void Textbox::EraseSelection()
{
	if (IsLongSelection())
	{
		int min = startCursor < endCursor ? startCursor : endCursor;
		int max = startCursor > endCursor ? startCursor : endCursor;
		text.erase(min, max - min);
		startCursor = endCursor = min;
	}
}

int Textbox::_FindCursorPos(float vpx)
{
	auto r = GetContentRect();
	// TODO kerning
	float x = r.x0;
	for (size_t i = 0; i < text.size(); i++)
	{
		float lw = GetTextWidth(&text[i], 1);
		if (vpx < x + lw * 0.5f)
			return i;
		x += lw;
	}
	return text.size();
}

Textbox& Textbox::SetText(const std::string& s)
{
	text = s;
	if (startCursor > text.size())
		startCursor = text.size();
	if (endCursor > text.size())
		endCursor = text.size();
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


TableView::TableView()
{
	styleProps = Theme::current->tableBase;
	cellStyle = Theme::current->tableCell;
	rowHeaderStyle = Theme::current->tableRowHeader;
	colHeaderStyle = Theme::current->tableColHeader;
}

void TableView::OnPaint()
{
	styleProps->paint_func(this);

	int rhw = 80;
	int chh = 20;
	int w = 100;
	int h = 20;

	int nc = _dataSource->GetNumCols();
	int nr = _dataSource->GetNumRows();

	style::PaintInfo info(this);

	auto RC = GetContentRect();

	// backgrounds
	for (int r = 0; r < nr; r++)
	{
		info.rect =
		{
			RC.x0,
			RC.y0 + chh + h * r,
			RC.x0 + rhw,
			RC.y0 + chh + h * (r + 1),
		};
		rowHeaderStyle->paint_func(info);
	}
	for (int c = 0; c < nc; c++)
	{
		info.rect =
		{
			RC.x0 + rhw + w * c,
			RC.y0,
			RC.x0 + rhw + w * (c + 1),
			RC.y0 + chh,
		};
		colHeaderStyle->paint_func(info);
	}
	for (int r = 0; r < nr; r++)
	{
		for (int c = 0; c < nc; c++)
		{
			info.rect =
			{
				RC.x0 + rhw + w * c,
				RC.y0 + chh + h * r,
				RC.x0 + rhw + w * (c + 1),
				RC.y0 + chh + h * (r + 1),
			};
			cellStyle->paint_func(info);
		}
	}

	// text
	for (int r = 0; r < nr; r++)
	{
		UIRect rect =
		{
			RC.x0,
			RC.y0 + chh + h * r,
			RC.x0 + rhw,
			RC.y0 + chh + h * (r + 1),
		};
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _dataSource->GetRowName(r).c_str(), 1, 1, 1);
	}
	for (int c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + rhw + w * c,
			RC.y0,
			RC.x0 + rhw + w * (c + 1),
			RC.y0 + chh,
		};
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _dataSource->GetColName(c).c_str(), 1, 1, 1);
	}
	for (int r = 0; r < nr; r++)
	{
		for (int c = 0; c < nc; c++)
		{
			UIRect rect =
			{
				RC.x0 + rhw + w * c,
				RC.y0 + chh + h * r,
				RC.x0 + rhw + w * (c + 1),
				RC.y0 + chh + h * (r + 1),
			};
			DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _dataSource->GetText(r, c).c_str(), 1, 1, 1);
		}
	}

	PaintChildren();
}

void TableView::OnEvent(UIEvent& e)
{
	Node::OnEvent(e);
}

void TableView::Render(UIContainer* ctx)
{
}

void TableView::SetDataSource(TableDataSource* src)
{
	_dataSource = src;
	Rerender();
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
	Node::OnEvent(e);
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
