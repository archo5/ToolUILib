
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
	SetFlag(UIObject_IsFocusable, true);
}

void Button::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		e.context->SetKeyboardFocus(this);
		e.handled = true;
	}
	if (e.type == UIEventType::Activate)
	{
		e.handled = true;
	}
}


CheckableBase::CheckableBase()
{
	SetFlag(UIObject_IsFocusable, true);
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
		e.context->OnChange(this);
		e.context->OnCommit(this);
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


TabButtonList::TabButtonList()
{
	styleProps = Theme::current->tabList;
}

void TabButtonList::OnPaint()
{
	styleProps->paint_func(this);

	if (auto* g = FindParentOfType<TabGroup>())
	{
		for (UIObject* ch = firstChild; ch; ch = ch->next)
		{
			if (auto* p = dynamic_cast<TabButtonBase*>(ch))
				if (p->IsSelected())
					continue;
			ch->OnPaint();
		}
	}
	else
		PaintChildren();
}


TabButtonBase::TabButtonBase()
{
	styleProps = Theme::current->tabButton;
}

void TabButtonBase::OnDestroy()
{
	if (auto* g = FindParentOfType<TabGroup>())
		if (g->_activeBtn == this)
			g->_activeBtn = nullptr;
}

void TabButtonBase::OnPaint()
{
	style::PaintInfo info(this);
	if (IsSelected())
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);
	PaintChildren();
}

void TabButtonBase::OnEvent(UIEvent& e)
{
	if ((e.type == UIEventType::Activate || e.type == UIEventType::ButtonDown) && IsChildOrSame(e.GetTargetNode()))
	{
		OnSelect();
		e.context->OnChange(this);
		e.context->OnCommit(this);
		e.handled = true;
	}
}


TabPanel::TabPanel()
{
	styleProps = Theme::current->tabPanel;
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
	SetFlag(UIObject_IsFocusable, true);
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


} // ui
