
#include "Controls.h"
#include "Layout.h"
#include "System.h"
#include "Native.h"
#include "Theme.h"

#include "WIP.h"


namespace ui {

void FrameStyle::Serialize(/*IThemeLoader* tl, */IObjectIterator& oi) // TODO
{
	//tl->OnFieldAABB(oi, "padding", padding);
	//tl->OnFieldPainter(oi, "backgroundPainter", backgroundPainter);

	float shared = 0;
	OnField(oi, "padding", shared);
	padding = AABB2f::UniformBorder(shared);
	OnField(oi, "paddingLeft", padding.x0);
	OnField(oi, "paddingRight", padding.x1);
	OnField(oi, "paddingTop", padding.y0);
	OnField(oi, "paddingBottom", padding.y1);

	std::string bpname;
	OnField(oi, "backgroundPainter", bpname);
	backgroundPainter = GetCurrentTheme()->FindPainterByName(bpname);
}


void FrameElement::OnReset()
{
	UIObjectSingleChild::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	style = {};
}

void FrameElement::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	if (style.backgroundPainter)
		ph.cpa = style.backgroundPainter->Paint(this);
	ph.PaintChildren(this, ctx);
}

Size2f FrameElement::GetReducedContainerSize(Size2f size)
{
	size.x -= style.padding.x0 + style.padding.x1;
	size.y -= style.padding.y0 + style.padding.y1;
	return size;
}

Rangef FrameElement::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	float pad = style.padding.x0 + style.padding.x1;
	return (_child ? _child->GetFullEstimatedWidth(GetReducedContainerSize(containerSize), type, forParentLayout) : Rangef::AtLeast(0)).Add(pad);
}

Rangef FrameElement::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	float pad = style.padding.y0 + style.padding.y1;
	return (_child ? _child->GetFullEstimatedHeight(GetReducedContainerSize(containerSize), type, forParentLayout) : Rangef::AtLeast(0)).Add(pad);
}

void FrameElement::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	auto padsub = rect.ShrinkBy(style.padding);
	if (_child)
	{
		_child->PerformLayout(padsub, GetReducedContainerSize(containerSize));
		finalRectC = _child->finalRectCP;
		finalRectCP = finalRectC.ExtendBy(style.padding);
	}
	else
	{
		finalRectCP = rect;
		finalRectC = padsub;
	}
}

FrameElement& FrameElement::SetStyle(const StaticID<FrameStyle>& id)
{
	style = *GetCurrentTheme()->GetStruct<FrameStyle>(id);
	return *this;
}

static StaticID<FrameStyle> sid_framestyle_panel("group_box");
FrameElement& FrameElement::SetDefaultStyle(DefaultFrameStyle style)
{
	switch (style)
	{
	case DefaultFrameStyle::GroupBox: return SetStyle(sid_framestyle_panel);
	}
	return *this;
}


static StaticID_Style sid_prop_label("prop_label");
void LabelFrame::OnReset()
{
	PaddedWrapperElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_prop_label); // TODO: do we need another style?
}


static StaticID_Style sid_panel("panel");
void Panel::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_panel);
}


static StaticID_Style sid_header("header");
void Header::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_header);
}


static StaticID_Style sid_button("button");
void Button::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_DB_Button | UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_button);
}


void StateButtonBase::OnReset()
{
	WrapperElement::OnReset();

	flags |= UIObject_DB_Button | UIObject_SetsChildTextStyle;
}


void StateToggleBase::OnEvent(Event& e)
{
	StateButtonBase::OnEvent(e);
	if (e.type == EventType::Activate && OnActivate())
	{
		e.StopPropagation();
		e.context->OnChange(this);
		e.context->OnCommit(this);
	}
}


StateToggle& StateToggle::InitReadOnly(uint8_t state)
{
	_state = state;
	_maxNumStates = 0;
	return *this;
}

StateToggle& StateToggle::InitEditable(uint8_t state, uint8_t maxNumStates)
{
	_state = state;
	_maxNumStates = maxNumStates;
	return *this;
}

StateToggle& StateToggle::SetState(uint8_t state)
{
	_state = state;
	return *this;
}

bool StateToggle::OnActivate()
{
	if (!_maxNumStates)
		return false;
	_state = (_state + 1) % _maxNumStates;
	return true;
}


void StateToggleVisualBase::OnPaint(const UIPaintContext& ctx)
{
	StateButtonBase* st = FindParentOfType<StateButtonBase>();
	PaintInfo info(st ? static_cast<UIObject*>(st) : this);
	if (st)
		info.SetCheckState(st->GetState());

	info.obj = this;
	info.rect = GetPaddingRect();

	UIPaintHelper::Paint(info, ctx);
}


static StaticID_Style sid_checkbox("checkbox");
void CheckboxIcon::OnReset()
{
	StateToggleVisualBase::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_checkbox);
}


static StaticID_Style sid_radio_button("radio_button");
void RadioButtonIcon::OnReset()
{
	StateToggleVisualBase::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_radio_button);
}


static StaticID_Style sid_tree_expand("tree_expand");
void TreeExpandIcon::OnReset()
{
	StateToggleVisualBase::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_tree_expand);
}


void StateButtonSkin::OnReset()
{
	StateToggleVisualBase::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_button);
}


static StaticID_Style sid_listbox("listbox");
void ListBox::OnReset()
{
	UIElement::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_listbox);
	SetFlag(UIObject_ClipChildren, true);
}


void ListBoxFrame::OnReset()
{
	PaddedWrapperElement::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_listbox);
}


static StaticID_Style sid_selectable("selectable");
void Selectable::OnReset()
{
	PaddedWrapperElement::OnReset();

	flags |= UIObject_DB_Selectable | UIObject_SetsChildTextStyle;
	SetStyle(GetCurrentTheme()->GetStyle(sid_selectable));
}


static StaticID_Style sid_progress_bar_base("progress_bar_base");
static StaticID_Style sid_progress_bar_completion("progress_bar_completion");
void ProgressBar::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_progress_bar_base);
	completionBarStyle = GetCurrentTheme()->GetStyle(sid_progress_bar_completion);
	progress = 0.5f;
}

void ProgressBar::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	PaintInfo cinfo(this);
	cinfo.rect = cinfo.rect.ShrinkBy(completionBarStyle->GetMarginRect());
	cinfo.rect.x1 = lerp(cinfo.rect.x0, cinfo.rect.x1, progress);
	completionBarStyle->background_painter->Paint(cinfo);

	ph.PaintChildren(this, ctx);
}


static StaticID_Style sid_slider_h_base("slider_h_base");
static StaticID_Style sid_slider_h_track("slider_h_track");
static StaticID_Style sid_slider_h_track_fill("slider_h_track_fill");
static StaticID_Style sid_slider_h_thumb("slider_h_thumb");
void Slider::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_slider_h_base);
	trackStyle = GetCurrentTheme()->GetStyle(sid_slider_h_track);
	trackFillStyle = GetCurrentTheme()->GetStyle(sid_slider_h_track_fill);
	thumbStyle = GetCurrentTheme()->GetStyle(sid_slider_h_thumb);

	_value = 0;
	_limits = { 0, 1, 0 };
}

void Slider::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	// track
	PaintInfo trkinfo(this);
	trkinfo.rect = trkinfo.rect.ShrinkBy(trackStyle->GetMarginRect());
	trackStyle->background_painter->Paint(trkinfo);

	// track fill
	trkinfo.rect = trkinfo.rect.ShrinkBy(trackStyle->GetPaddingRect());
	trkinfo.rect.x1 = round(lerp(trkinfo.rect.x0, trkinfo.rect.x1, ValueToQ(_value)));
	auto prethumb = trkinfo.rect;

	trkinfo.rect = trkinfo.rect.ExtendBy(trackFillStyle->GetPaddingRect());
	trackFillStyle->background_painter->Paint(trkinfo);

	// thumb
	prethumb.x0 = prethumb.x1;
	trkinfo.rect = prethumb.ExtendBy(thumbStyle->GetPaddingRect());
	thumbStyle->background_painter->Paint(trkinfo);

	ph.PaintChildren(this, ctx);
}

void Slider::OnEvent(Event& e)
{
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
	{
		e.context->CaptureMouse(this);
		_mxoff = 0;
		// TODO if inside, calc offset
	}
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		e.context->ReleaseMouse();
	}
	if (e.type == EventType::MouseMove && IsClicked())
	{
		_value = PosToValue(e.position.x + _mxoff);
		e.context->OnChange(this);
	}
}

double Slider::PosToQ(double x)
{
	auto rect = GetPaddingRect();
	rect = rect.ShrinkBy(trackStyle->GetMarginRect());
	rect = rect.ShrinkBy(trackStyle->GetPaddingRect());
	float fw = rect.GetWidth();
	return clamp(fw ? float(x - rect.x0) / fw : 0, 0.0f, 1.0f);
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
	return clamp(diff ? ((v - _limits.min) / diff) : 0.0, 0.0, 1.0);
}


void PropertyList::OnReset()
{
	UIElement::OnReset();

	splitPos = Coord::Percent(40);
	minSplitPos = 0;

	_defaultLabelStyle = GetCurrentTheme()->GetStyle(sid_prop_label);
}

UIRect PropertyList::CalcPaddingRect(const UIRect& expTgtRect)
{
	auto pad = UIElement::CalcPaddingRect(expTgtRect);
	auto estContent = expTgtRect.ShrinkBy(pad);
	float splitPosR = ResolveUnits(splitPos, estContent.GetWidth());
	float minSplitPosR = ResolveUnits(minSplitPos, estContent.GetWidth());
	float finalSplitPosR = max(splitPosR, minSplitPosR);
	_calcSplitX = roundf(finalSplitPosR + estContent.x0);
	return pad;
}


LabeledProperty& LabeledProperty::Begin(const char* label, ContentLayoutType layout)
{
	auto& lp = Push<LabeledProperty>();
	if (label)
	{
		if (*label == '\b')
		{
			lp.SetText(label + 1);
			lp.SetBrief(true);
		}
		else
			lp.SetText(label);
	}
	if (layout == StackExpandLTR)
		Push<StackExpandLTRLayoutElement>();
	return lp;
}

void LabeledProperty::End(ContentLayoutType layout)
{
	if (layout == StackExpandLTR)
		Pop();
	Pop();
}

void LabeledProperty::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	_labelStyle = {};

	_labelText = {};
	_isBrief = false;
}

void LabeledProperty::OnEnterTree()
{
	UIElement::OnEnterTree();

	_propList = FindParentOfType<PropertyList>();
}

void LabeledProperty::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	if (!_labelText.empty())
	{
		StyleBlock* labelStyle = FindCurrentLabelStyle();

		auto* font = labelStyle->GetFont();

		auto contPadRect = GetPaddingRect();
		UIRect labelContRect = { contPadRect.x0, contPadRect.y0, GetContentRect().x0, contPadRect.y1 };
		auto labelPadRect = labelStyle->GetPaddingRect();
		auto cr = labelContRect;
		auto r = labelContRect.ShrinkBy(labelPadRect);

		// TODO optimize scissor (shared across labels)
		if (draw::PushScissorRectIfNotEmpty(cr))
		{
			draw::TextLine(
				font,
				labelStyle->font_size,
				r.x0, r.y1 - (r.y1 - r.y0 - labelStyle->font_size) / 2,
				_labelText,
				labelStyle->text_color);
			draw::PopScissorRect();
		}
	}

	ph.PaintChildren(this, ctx);
}

UIRect LabeledProperty::CalcPaddingRect(const UIRect& expTgtRect)
{
	auto r = UIElement::CalcPaddingRect(expTgtRect);
	if (!_labelText.empty())
	{
		if (_isBrief)
		{
			StyleBlock* labelStyle = FindCurrentLabelStyle();

			auto* font = labelStyle->GetFont();

			r.x0 += GetTextWidth(font, labelStyle->font_size, _labelText);
			auto labelPadRect = labelStyle->GetPaddingRect();
			r.x0 += labelPadRect.x0 + labelPadRect.x1;
		}
		else
		{
			r.x0 += _propList
				? _propList->_calcSplitX - expTgtRect.x0
				: roundf(expTgtRect.ShrinkBy(r).GetWidth() * 0.4f);
		}
	}
	return r;
}

LabeledProperty& LabeledProperty::SetText(StringView text)
{
	_labelText.assign(text.data(), text.size());
	return *this;
}

StyleBlock* LabeledProperty::FindCurrentLabelStyle() const
{
	if (_labelStyle)
		return _labelStyle;
	if (_propList && _propList->_defaultLabelStyle)
		return _propList->_defaultLabelStyle;
	return GetCurrentTheme()->GetStyle(sid_prop_label);
}

StyleAccessor LabeledProperty::GetLabelStyle()
{
	if (!_labelStyle)
		_labelStyle = FindCurrentLabelStyle();
	return StyleAccessor(_labelStyle, this);
}


SplitPane::SplitPane()
{
	// TODO
	vertSepStyle = GetCurrentTheme()->GetStyle(sid_button);
	StyleAccessor(vertSepStyle, this).SetWidth(8);
	horSepStyle = GetCurrentTheme()->GetStyle(sid_button);
	StyleAccessor(horSepStyle, this).SetHeight(8);
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
	float w = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth());
	float hw = w / 2;
	float x = roundf(lerp(sp->finalRectC.x0 + hw, sp->finalRectC.x1 - hw, split) - hw);
	return { x, sp->finalRectC.y0, x + roundf(w), sp->finalRectC.y1 };
}

static UIRect GetSplitRectV(SplitPane* sp, int which)
{
	float split = sp->_splits[which];
	float h = sp->ResolveUnits(sp->horSepStyle->height, sp->finalRectC.GetWidth());
	float hh = h / 2;
	float y = roundf(lerp(sp->finalRectC.y0 + hh, sp->finalRectC.y1 - hh, split) - hh);
	return { sp->finalRectC.x0, y, sp->finalRectC.x1, y + roundf(h) };
}

static float SplitWidthAsQ(SplitPane* sp)
{
	if (!sp->_verticalSplit)
	{
		if (sp->finalRectC.GetWidth() == 0)
			return 0;
		float w = sp->ResolveUnits(sp->vertSepStyle->width, sp->finalRectC.GetWidth());
		return w / max(w, sp->finalRectC.GetWidth() - w);
	}
	else
	{
		if (sp->finalRectC.GetHeight() == 0)
			return 0;
		float w = sp->ResolveUnits(sp->vertSepStyle->height, sp->finalRectC.GetWidth());
		return w / max(w, sp->finalRectC.GetHeight() - w);
	}
}

static void CheckSplits(SplitPane* sp)
{
	size_t cc = sp->_children.size() - 1;
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

void SplitPane::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	if (!_children.empty())
	{
		size_t split = 0;
		if (!_verticalSplit)
		{
			float splitWidth = ResolveUnits(vertSepStyle->width, finalRectC.GetWidth());
			float prevEdge = finalRectC.x0;
			for (auto* ch : _children)
			{
				UIRect r = finalRectC;
				auto sr = split < _splits.size() ? GetSplitRectH(this, split) : UIRect{ r.x1, 0, r.x1, 0 };
				split++;
				r.x0 = prevEdge;
				r.x1 = sr.x0;

				if (draw::PushScissorRectIfNotEmpty(r))
				{
					ch->Paint(ctx);
					draw::PopScissorRect();
				}

				prevEdge = sr.x1;
			}
		}
		else
		{
			float splitHeight = ResolveUnits(horSepStyle->height, finalRectC.GetWidth());
			float prevEdge = finalRectC.y0;
			for (auto* ch : _children)
			{
				UIRect r = finalRectC;
				auto sr = split < _splits.size() ? GetSplitRectV(this, split) : UIRect{ 0, r.y1, 0, r.y1 };
				split++;
				r.y0 = prevEdge;
				r.y1 = sr.y0;

				if (draw::PushScissorRectIfNotEmpty(r))
				{
					ch->Paint(ctx);
					draw::PopScissorRect();
				}

				prevEdge = sr.y1;
			}
		}
	}

	PaintInfo info(this);
	if (!_verticalSplit)
	{
		for (size_t i = 0; i < _splits.size(); i++)
		{
			auto ii = (uint16_t)i;
			info.rect = GetSplitRectH(this, ii);
			info.state &= ~(PS_Hover | PS_Down);
			if (_splitUI.IsHovered(ii))
				info.state |= PS_Hover;
			if (_splitUI.IsPressed(ii))
				info.state |= PS_Down;
			vertSepStyle->background_painter->Paint(info);
		}
	}
	else
	{
		for (size_t i = 0; i < _splits.size(); i++)
		{
			auto ii = (uint16_t)i;
			info.rect = GetSplitRectV(this, ii);
			info.state &= ~(PS_Hover | PS_Down);
			if (_splitUI.IsHovered(ii))
				info.state |= PS_Hover;
			if (_splitUI.IsPressed(ii))
				info.state |= PS_Down;
			horSepStyle->background_painter->Paint(info);
		}
	}
}

void SplitPane::OnEvent(Event& e)
{
	// event while building, understanding of children is incomplete
	if (UIContainer::GetCurrent() &&
		GetCurrentBuildable())
		return;

	CheckSplits(this);

	_splitUI.InitOnEvent(e);
	for (size_t i = 0; i < _splits.size(); i++)
	{
		if (!_verticalSplit)
		{
			switch (_splitUI.DragOnEvent(uint16_t(i), GetSplitRectH(this, i), e))
			{
			case SubUIDragState::Start:
				e.context->CaptureMouse(this);
				_dragOff = SplitQToX(this, _splits[i]) - e.position.x;
				break;
			case SubUIDragState::Move:
				_splits[i] = SplitXToQ(this, e.position.x + _dragOff);
				_OnChangeStyle();
				break;
			case SubUIDragState::Stop:
				if (e.context->GetMouseCapture() == this)
					e.context->ReleaseMouse();
				break;
			}
		}
		else
		{
			switch (_splitUI.DragOnEvent(uint16_t(i), GetSplitRectV(this, i), e))
			{
			case SubUIDragState::Start:
				e.context->CaptureMouse(this);
				_dragOff = SplitQToY(this, _splits[i]) - e.position.y;
				break;
			case SubUIDragState::Move:
				_splits[i] = SplitYToQ(this, e.position.y + _dragOff);
				_OnChangeStyle();
				break;
			case SubUIDragState::Stop:
				if (e.context->GetMouseCapture() == this)
					e.context->ReleaseMouse();
				break;
			}
		}
	}
	if (e.type == EventType::SetCursor && _splitUI.IsAnyHovered())
	{
		e.context->SetDefaultCursor(_verticalSplit ? DefaultCursor::ResizeRow : DefaultCursor::ResizeCol);
		e.StopPropagation();
	}
	_splitUI.FinalizeOnEvent(e);
}

void SplitPane::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	CheckSplits(this);

	finalRectCP = rect;
	finalRectC = finalRectCP.ShrinkBy(styleProps->GetPaddingRect());

	size_t split = 0;
	if (!_verticalSplit)
	{
		float splitWidth = ResolveUnits(vertSepStyle->width, finalRectC.GetWidth());
		float prevEdge = finalRectC.x0;
		for (auto* ch : _children)
		{
			UIRect r = finalRectC;
			auto sr = split < _splits.size() ? GetSplitRectH(this, split) : UIRect{ r.x1, 0, r.x1, 0 };
			split++;
			r.x0 = prevEdge;
			r.x1 = sr.x0;
			ch->PerformLayout(r, finalRectC.GetSize());
			prevEdge = sr.x1;
		}
	}
	else
	{
		float splitHeight = ResolveUnits(horSepStyle->height, finalRectC.GetWidth());
		float prevEdge = finalRectC.y0;
		for (auto* ch : _children)
		{
			UIRect r = finalRectC;
			auto sr = split < _splits.size() ? GetSplitRectV(this, split) : UIRect{ 0, r.y1, 0, r.y1 };
			split++;
			r.y0 = prevEdge;
			r.y1 = sr.y0;
			ch->PerformLayout(r, finalRectC.GetSize());
			prevEdge = sr.y1;
		}
	}
}

void SplitPane::OnReset()
{
	UIElement::OnReset();

	_verticalSplit = false;
	_children.clear();
}

void SplitPane::SlotIterator_Init(UIObjectIteratorData& data)
{
	data.data0 = 0;
}

UIObject* SplitPane::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	if (data.data0 >= _children.size())
		return nullptr;
	return _children[data.data0++];
}

void SplitPane::RemoveChildImpl(UIObject* ch)
{
	for (size_t i = 0; i < _children.size(); i++)
	{
		if (_children[i] == ch)
		{
			_children.erase(_children.begin() + i);
			break;
		}
	}
}

void SplitPane::DetachChildren(bool recursive)
{
	for (size_t i = 0; i < _children.size(); i++)
	{
		auto* ch = _children[i];

		if (recursive)
			ch->DetachChildren(true);

		// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
		if (system)
			ch->_DetachFromTree();

		ch->parent = nullptr;
	}
	_children.clear();
}

void SplitPane::CustomAppendChild(UIObject* obj)
{
	obj->DetachParent();

	obj->parent = this;
	_children.push_back(obj);

	if (system)
		obj->_AttachToFrameContents(system);
}

UIObject* SplitPane::FindLastChildContainingPos(Point2f pos) const
{
	for (size_t i = _children.size(); i > 0; )
	{
		i--;
		if (_children[i]->Contains(pos))
			return _children[i];
	}
	return nullptr;
}

void SplitPane::_AttachToFrameContents(FrameContents* owner)
{
	UIElement::_AttachToFrameContents(owner);

	for (auto* ch : _children)
		ch->_AttachToFrameContents(owner);
}

void SplitPane::_DetachFromFrameContents()
{
	for (auto* ch : _children)
		ch->_DetachFromFrameContents();

	UIElement::_DetachFromFrameContents();
}

Rangef SplitPane::GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
{
	return { containerSize.x, containerSize.x };
}

Rangef SplitPane::GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout)
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
	OnReset();
}

static StaticID_Style sid_scroll_v_track("scroll_v_track");
static StaticID_Style sid_scroll_v_thumb("scroll_v_thumb");
void ScrollbarV::OnReset()
{
	trackVStyle = GetCurrentTheme()->GetStyle(sid_scroll_v_track);
	thumbVStyle = GetCurrentTheme()->GetStyle(sid_scroll_v_thumb);
}

Coord ScrollbarV::GetWidth()
{
	return trackVStyle->width;
}

static UIRect sbv_GetTrackRect(const ScrollbarData& info, StyleBlock* trackVStyle)
{
	return info.rect.ShrinkBy(trackVStyle->GetPaddingRect());
}

UIRect ScrollbarV::GetThumbRect(const ScrollbarData& info)
{
	float viewportSize = max(info.viewportSize, 1.0f);
	float contentSize = max(info.contentSize, viewportSize);

	float scrollFactor = contentSize == viewportSize ? 0 : info.contentOff / (contentSize - viewportSize);
	float thumbSizeFactor = viewportSize / contentSize;

	UIRect trackRect = sbv_GetTrackRect(info, trackVStyle);

	float thumbSize = thumbSizeFactor * trackRect.GetHeight();
	float minH = info.owner->ResolveUnits(thumbVStyle->min_height, info.rect.GetWidth());
	thumbSize = max(thumbSize, minH);

	float pos = (trackRect.GetHeight() - thumbSize) * scrollFactor;

	return { trackRect.x0, trackRect.y0 + pos, trackRect.x1, trackRect.y0 + pos + thumbSize };
}

void ScrollbarV::OnPaint(const ScrollbarData& info)
{
	PaintInfo vsinfo;
	vsinfo.obj = info.owner;
	vsinfo.rect = info.rect;
	trackVStyle->background_painter->Paint(vsinfo);

	vsinfo.rect = GetThumbRect(info);
	vsinfo.state = 0;
	if (info.contentSize <= info.viewportSize)
		vsinfo.state |= PS_Disabled;
	else
	{
		if (uiState.IsHovered(0))
			vsinfo.state |= PS_Hover;
		if (uiState.IsPressed(0))
			vsinfo.state |= PS_Down;
	}
	thumbVStyle->background_painter->Paint(vsinfo);
}

bool ScrollbarV::OnEvent(const ScrollbarData& info, Event& e)
{
	uiState.InitOnEvent(e);

	float viewportSize = max(info.viewportSize, 1.0f);
	float contentSize = max(info.contentSize, viewportSize);

	float maxOff = max(contentSize - viewportSize, 0.0f);

	bool contentOffsetChanged = false;
	switch (uiState.DragOnEvent(0, GetThumbRect(info), e))
	{
	case SubUIDragState::Start:
		dragStartContentOff = info.contentOff;
		dragStartCursorPos = e.position.y;
		e.context->CaptureMouse(info.owner);
		e.StopPropagation();
		break;
	case SubUIDragState::Move: {
		float thumbSizeFactor = viewportSize / contentSize;

		UIRect trackRect = sbv_GetTrackRect(info, trackVStyle);

		float thumbSize = thumbSizeFactor * trackRect.GetHeight();
		float minH = info.owner->ResolveUnits(thumbVStyle->min_height, info.rect.GetWidth());
		thumbSize = max(thumbSize, minH);

		float trackRange = trackRect.GetHeight() - thumbSize;
		if (trackRange > 0)
		{
			float dragSpeed = (contentSize - viewportSize) / trackRange;
			info.contentOff = dragStartContentOff + (e.position.y - dragStartCursorPos) * dragSpeed;
			contentOffsetChanged = true;
		}
		e.StopPropagation();
		break; }
	case SubUIDragState::Stop:
		if (e.context->GetMouseCapture() == info.owner)
			e.context->ReleaseMouse();
		e.StopPropagation();
		break;
	}

	if (e.type == EventType::MouseScroll)
	{
		info.contentOff -= e.delta.y;
		contentOffsetChanged = true;
	}
	if (contentOffsetChanged)
	{
		info.contentOff = max(0.0f, min(maxOff, info.contentOff));
	}
	return contentOffsetChanged;
}


void ScrollArea::OnPaint(const UIPaintContext& ctx)
{
	FillerElement::OnPaint(ctx);

	auto cr = GetContentRect();
	float w = cr.GetWidth();
	auto sbvr = cr;
	sbvr.x0 = cr.x1;
	sbvr.x1 = sbvr.x0 + ResolveUnits(sbv.GetWidth(), w);
	sbv.OnPaint({ this, sbvr, cr.GetHeight(), estContentSize.y, yoff });
}

void ScrollArea::OnEvent(Event& e)
{
	auto cr = GetContentRect();
	float w = cr.GetWidth();
	auto sbvr = cr;
	sbvr.x0 = cr.x1;
	sbvr.x1 = sbvr.x0 + ResolveUnits(sbv.GetWidth(), w);
	ScrollbarData info = { this, sbvr, cr.GetHeight(), estContentSize.y, yoff };

	if (sbv.OnEvent(info, e))
	{
		yoff = info.contentOff;
		_OnChangeStyle();
	}
}

void ScrollArea::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	auto realContSize = containerSize;

	bool hasVScroll = estContentSize.y > rect.GetHeight();
	float vScrollWidth = ResolveUnits(sbv.GetWidth(), rect.GetWidth());
	//if (hasVScroll)
		realContSize.x -= vScrollWidth;
	estContentSize.y = _child ? _child->GetFullEstimatedHeight(realContSize, EstSizeType::Exact).min : 0;
	float maxYOff = max(0.0f, estContentSize.y - rect.GetHeight());
	if (yoff > maxYOff)
		yoff = maxYOff;

	UIRect r = rect;
	r.y0 -= yoff;
	r.y1 -= yoff;
	//if (hasVScroll)
		r.x1 -= vScrollWidth;

	auto crect = r;
	crect.y1 = crect.y0 + estContentSize.y;
	if (_child)
		_child->PerformLayout(crect, realContSize);
	finalRectCP = finalRectC = rect;
	finalRectC.x1 -= vScrollWidth;
}

void ScrollArea::OnReset()
{
	FillerElement::OnReset();

	SetFlag(UIObject_ClipChildren, true);
	sbv.OnReset();
}


static StaticID_Style sid_tab_group("tab_group");
void TabGroup::OnReset()
{
	UIElement::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_tab_group);
	_activeBtn = nullptr;
}


static StaticID_Style sid_tab_list("tab_list");
void TabButtonList::OnReset()
{
	UIElement::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_tab_list);
}

void TabButtonList::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	if (auto* g = FindParentOfType<TabGroup>())
	{
		for (UIObject* ch = firstChild; ch; ch = ch->next)
		{
			if (auto* p = dynamic_cast<TabButtonBase*>(ch))
				if (p == g->_activeBtn)
					continue;
			ch->OnPaint(ctx);
		}
	}
	else
		ph.PaintChildren(this, ctx);
}


static StaticID_Style sid_tab_button("tab_button");
void TabButtonBase::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_DB_Button | UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_tab_button);
}

void TabButtonBase::OnExitTree()
{
	UIElement::OnExitTree();

	if (auto* g = FindParentOfType<TabGroup>())
		if (g->_activeBtn == this)
			g->_activeBtn = nullptr;
}

void TabButtonBase::OnPaint(const UIPaintContext& ctx)
{
	PaintInfo info(this);
	if (IsSelected())
		info.SetChecked(true);

	UIPaintHelper::Paint(info, ctx);
}

void TabButtonBase::OnEvent(Event& e)
{
	if ((e.type == EventType::Activate || e.type == EventType::ButtonDown) && e.target == e.current)
	{
		OnSelect();
		if (e.type != EventType::Activate)
			e.context->OnActivate(this);
		e.context->OnChange(this);
		e.context->OnCommit(this);
		e.StopPropagation();
	}
}


static StaticID_Style sid_tab_panel("tab_panel");
void TabPanel::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_tab_panel);
}

void TabPanel::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	if (auto* g = FindParentOfType<TabGroup>())
	{
		if (g->_activeBtn)
		{
			auto panelRect = GetPaddingRect();
			auto btnRect = g->_activeBtn->FindParentOfType<TabButtonList>()->GetContentRect();

			PaintInfo pi(this);

			// currently only supports overlap at the top
			if (panelRect.Overlaps(btnRect) && btnRect.y1 > panelRect.y0 && btnRect.y0 < panelRect.y0)
			{
				draw::PushScissorRect(UIRect{ panelRect.x0, panelRect.y0, btnRect.x0, panelRect.y1 });
				ph.PaintBackground(pi);
				draw::PopScissorRect();

				draw::PushScissorRect(UIRect{ btnRect.x0, btnRect.y1, btnRect.x1, panelRect.y1 });
				ph.PaintBackground(pi);
				draw::PopScissorRect();

				draw::PushScissorRect(UIRect{ btnRect.x1, panelRect.y0, panelRect.x1, panelRect.y1 });
				ph.PaintBackground(pi);
				draw::PopScissorRect();
			}
			else
			{
				ph.PaintBackground(pi);
			}

			g->_activeBtn->Paint(ctx);

			ph.PaintChildren(this, ctx);

			return;
		}
	}

	UIElement::OnPaint(ctx);
}


static StaticID_Style sid_textbox("textbox");
void Textbox::OnReset()
{
	UIElement::OnReset();

	flags |= UIObject_DB_FocusOnLeftClick | UIObject_DB_CaptureMouseOnLeftClick | UIObject_SetsChildTextStyle;
	styleProps = GetCurrentTheme()->GetStyle(sid_textbox);
	_placeholder = {};
}

void Textbox::OnPaint(const UIPaintContext& ctx)
{
	UIPaintHelper ph;
	ph.PaintBackground(this);

	{
		auto* font = styleProps->GetFont();
		int size = styleProps->font_size;

		auto r = GetContentRect();
		if (!_placeholder.empty() && !IsFocused() && _text.empty())
			draw::TextLine(font, size, r.x0, r.y1 - (r.y1 - r.y0 - size) / 2, _placeholder, Color4b(255, 128));
		if (!_text.empty())
			draw::TextLine(font, size, r.x0, r.y1 - (r.y1 - r.y0 - size) / 2, _text, Color4b::White());

		if (IsFocused())
		{
			if (startCursor != endCursor)
			{
				int minpos = startCursor < endCursor ? startCursor : endCursor;
				int maxpos = startCursor > endCursor ? startCursor : endCursor;
				float x0 = GetTextWidth(font, size, StringView(_text).substr(0, minpos));
				float x1 = GetTextWidth(font, size, StringView(_text).substr(0, maxpos));

				draw::RectCol(r.x0 + x0, r.y0, r.x0 + x1, r.y1, Color4f(0.5f, 0.7f, 0.9f, 0.4f));
			}

			if (showCaretState)
			{
				float x = GetTextWidth(font, size, StringView(_text).substr(0, endCursor));
				draw::RectCol(r.x0 + x, r.y0, r.x0 + x + 1, r.y1, Color4b::White());
			}
		}
	}

	ph.PaintChildren(this, ctx);
}

static size_t PrevChar(StringView str, size_t pos)
{
	if (pos == 0)
		return 0;
	pos--;
	while (pos > 0 && (str[pos] & 0xC0) == 0x80)
		pos--;
	return pos;
}

static size_t NextChar(StringView str, size_t pos)
{
	if (pos == str.size())
		return pos;
	pos++;
	while (pos < str.size() && (str[pos] & 0xC0) == 0x80)
		pos++;
	return pos;
}

static uint32_t GetUTF8(StringView str, size_t pos)
{
	if (pos >= str.size())
		return UINT32_MAX;

	char c0 = str[pos];
	if (!(c0 & 0x80))
		return c0;

	if (pos + 1 >= str.size())
		return UINT32_MAX;
	char c1 = str[pos + 1];
	if ((c1 & 0xC0) != 0x80)
		return UINT32_MAX;

	if ((c0 & 0xE0) == 0xC0)
		return (c0 & ~0xE0) | ((c1 & ~0xC0) << 5);

	if (pos + 2 >= str.size())
		return UINT32_MAX;
	char c2 = str[pos + 2];
	if ((c2 & 0xC0) != 0x80)
		return UINT32_MAX;

	if ((c0 & 0xF0) == 0xE0)
		return (c0 & ~0xF0) | ((c1 & ~0xC0) << 4) | ((c2 & ~0xC0) << 10);

	if (pos + 3 >= str.size())
		return UINT32_MAX;
	char c3 = str[pos + 3];
	if ((c3 & 0xC0) != 0x80)
		return UINT32_MAX;

	if ((c0 & 0xF8) == 0xF0)
		return (c0 & ~0xF0) | ((c1 & ~0xC0) << 3) | ((c2 & ~0xC0) << 9) | ((c2 & ~0xC0) << 15);

	return UINT32_MAX;
}

static int GetCharClass(StringView str, size_t pos)
{
	uint32_t c = GetUTF8(str, pos);
	// TODO full unicode
	if (IsSpace(c)) // (ASCII)
		return 0;
	// punctuation (ASCII)
	if ((c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126))
		return 2;
	// anything else is considered to be a word
	return 1;
}

static bool IsWordBreak(int cca, int ccb)
{
	if (cca == ccb) // same class - ignore
		return false;
	if (ccb == 0) // trailing space - ignore
		return false;
	return true;
}

static size_t PrevWord(StringView str, size_t pos)
{
	if (pos == 0)
		return 0;

	pos = PrevChar(str, pos);
	auto pcc = GetCharClass(str, pos);
	while (pos > 0)
	{
		size_t npp = PrevChar(str, pos);
		auto ncc = GetCharClass(str, npp);
		if (IsWordBreak(ncc, pcc))
			break;
		pcc = ncc;
		pos = npp;
	}
	return pos;
}

static size_t NextWord(StringView str, size_t pos)
{
	if (pos == str.size())
		return pos;

	auto pcc = GetCharClass(str, pos);
	pos = NextChar(str, pos);
	while (pos < str.size())
	{
		auto ncc = GetCharClass(str, pos);
		if (IsWordBreak(pcc, ncc))
			break;
		pcc = ncc;
		pos = NextChar(str, pos);
	}
	return pos;
}

static void UpdateSelection(Textbox& tb, Event& e, bool start)
{
	if (start)
	{
		tb._lastPressRepeatCount = e.numRepeats;
		tb._origStartCursor = tb._FindCursorPos(e.position.x);
		if (e.numRepeats == 0)
			tb._hadFocusOnFirstClick = tb.IsFocused();
	}
	unsigned mode = (tb._lastPressRepeatCount + (tb._hadFocusOnFirstClick ? 0 : 2)) % 3;
	if (mode != 2)
	{
		tb.startCursor = tb._origStartCursor;
		tb.endCursor = tb._FindCursorPos(e.position.x);
		if (mode == 1)
		{
			tb.startCursor = PrevWord(tb._text, tb.startCursor);
			tb.endCursor = NextWord(tb._text, tb.endCursor);
		}
	}
	else
	{
		tb.startCursor = 0;
		tb.endCursor = tb._text.size();
	}
}

void Textbox::OnEvent(Event& e)
{
	if (e.type == EventType::ButtonDown)
	{
		if (e.GetButton() == MouseButton::Left)
			UpdateSelection(*this, e, true);
	}
	else if (e.type == EventType::ButtonUp)
	{
		if (e.GetButton() == MouseButton::Left)
			UpdateSelection(*this, e, false);
	}
	else if (e.type == EventType::MouseMove)
	{
		if (IsClicked(0))
			UpdateSelection(*this, e, false);
	}
	else if (e.type == EventType::SetCursor)
	{
		e.context->SetDefaultCursor(DefaultCursor::Text);
		e.StopPropagation();
	}
	else if (e.type == EventType::Timer)
	{
		showCaretState = !showCaretState;
		if (system) // TODO better check? needed?
			GetNativeWindow()->InvalidateAll(); // TODO localized
		if (IsFocused())
			e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == EventType::GotFocus)
	{
		showCaretState = true;
		GetNativeWindow()->InvalidateAll(); // TODO localized
		startCursor = 0;
		endCursor = _text.size();
		e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == EventType::LostFocus)
	{
		e.context->OnCommit(this);
	}
	else if (e.type == EventType::KeyAction)
	{
		switch (e.GetKeyAction())
		{
		case KeyAction::Enter:
			e.context->SetKeyboardFocus(nullptr);
			break;

		case KeyAction::PrevLetter:
			if (IsLongSelection() && !e.GetKeyActionModifier())
			{
				startCursor = endCursor = startCursor < endCursor ? startCursor : endCursor;
			}
			else
			{
				endCursor = PrevChar(_text, endCursor);
				if (!e.GetKeyActionModifier())
					startCursor = endCursor;
			}
			break;
		case KeyAction::NextLetter:
			if (IsLongSelection() && !e.GetKeyActionModifier())
			{
				startCursor = endCursor = startCursor > endCursor ? startCursor : endCursor;
			}
			else
			{
				endCursor = NextChar(_text, endCursor);
				if (!e.GetKeyActionModifier())
					startCursor = endCursor;
			}
			break;

		case KeyAction::PrevWord:
			endCursor = PrevWord(_text, endCursor);
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;
		case KeyAction::NextWord:
			endCursor = NextWord(_text, endCursor);
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;

		case KeyAction::GoToLineStart:
		case KeyAction::GoToStart:
			endCursor = 0;
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;
		case KeyAction::GoToLineEnd:
		case KeyAction::GoToEnd:
			endCursor = _text.size();
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;

		case KeyAction::Cut:
			Clipboard::SetText(GetSelectedText());
			if (!IsInputDisabled())
			{
				EraseSelection();
				e.context->OnChange(this);
			}
			break;
		case KeyAction::Copy:
			Clipboard::SetText(GetSelectedText());
			break;
		case KeyAction::SelectAll:
			startCursor = 0;
			endCursor = _text.size();
			break;
		}

		if (!IsInputDisabled())
		{
			switch (e.GetKeyAction())
			{
			case KeyAction::DelPrevLetter:
			case KeyAction::DelNextLetter:
			case KeyAction::DelPrevWord:
			case KeyAction::DelNextWord:
				if (IsLongSelection())
				{
					EraseSelection();
				}
				else
				{
					size_t to;
					switch (e.GetKeyAction())
					{
					case KeyAction::DelPrevLetter:
						to = PrevChar(_text, endCursor);
						_text.erase(to, endCursor - to);
						startCursor = endCursor = to;
						break;
					case KeyAction::DelNextLetter:
						to = NextChar(_text, endCursor);
						_text.erase(endCursor, to - endCursor);
						break;
					case KeyAction::DelPrevWord:
						to = PrevWord(_text, endCursor);
						_text.erase(to, endCursor - to);
						startCursor = endCursor = to;
						break;
					case KeyAction::DelNextWord:
						to = NextWord(_text, endCursor);
						_text.erase(endCursor, to - endCursor);
						break;
					}
				}
				e.context->OnChange(this);
				break;
			case KeyAction::Paste:
				EnterText(Clipboard::GetText().c_str());
				break;
			}
		}
	}
	else if (e.type == EventType::TextInput)
	{
		if (!IsInputDisabled())
		{
			char ch[5];
			if (e.GetUTF32Char() >= 32 && e.GetUTF32Char() != 127 && e.GetUTF8Text(ch))
				EnterText(ch);
		}
	}
}

StringView Textbox::GetSelectedText() const
{
	int min = startCursor < endCursor ? startCursor : endCursor;
	int max = startCursor > endCursor ? startCursor : endCursor;
	return StringView(_text.data() + min, max - min);
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

size_t Textbox::_FindCursorPos(float vpx)
{
	auto r = GetContentRect();
	// TODO kerning
	float x = r.x0;
	for (size_t i = 0; i < _text.size(); i++)
	{
		float lw = GetTextWidth(styleProps->GetFont(), styleProps->font_size, StringView(_text).substr(i, 1));
		if (vpx < x + lw * 0.5f)
			return i;
		x += lw;
	}
	return _text.size();
}

Textbox& Textbox::SetText(StringView s)
{
	if (InUse())
		return *this;
	_text.assign(s.data(), s.size());
	if (startCursor > _text.size())
		startCursor = _text.size();
	if (endCursor > _text.size())
		endCursor = _text.size();
	return *this;
}

Textbox& Textbox::SetPlaceholder(StringView s)
{
	_placeholder.assign(s.data(), s.size());
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
	HandleEvent(EventType::Change) = [this, &val](Event&) { val = atof(GetText().c_str()); };
	return *this;
}


void BackgroundBlocker::OnReset()
{
	UIElement::OnReset();

	RegisterAsOverlay(199.f);
}

void BackgroundBlocker::OnEvent(Event& e)
{
	UIElement::OnEvent(e);
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
		OnButton();
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		if (HasFlags(UIObject_IsChecked))
			OnButton();
		SetFlag(UIObject_IsChecked, true);
	}
}

void BackgroundBlocker::OnLayout(const UIRect& rect, const Size2f& containerSize)
{
	UIRect r = { 0, 0, system->eventSystem.width, system->eventSystem.height };
	finalRectC = finalRectCP = r;
}

void BackgroundBlocker::OnButton()
{
	Event e(&system->eventSystem, this, EventType::BackgroundClick);
	system->eventSystem.BubblingEvent(e);
}


void DropdownMenu::Build()
{
	OnBuildButton();

	if (HasFlags(UIObject_IsChecked))
	{
		Make<BackgroundBlocker>();
		OnBuildMenuWithLayout();
	}
}

void DropdownMenu::OnEvent(Event& e)
{
	Buildable::OnEvent(e);
	if (e.type == EventType::BackgroundClick)
	{
		SetFlag(UIObject_IsChecked, false);
		Rebuild();
		e.StopPropagation();
	}
}

static StaticID_Style sid_dropdown_button("dropdown_button");
static StaticID_Style sid_dropdown_button_icon("dropdown_button_icon");
void DropdownMenu::OnBuildButton()
{
	auto& btn = Push<StackLTRLayoutElement>();
	btn.SetStyle(GetCurrentTheme()->GetStyle(sid_dropdown_button));
	btn.flags |= flags & UIObject_IsDisabled;
	btn.SetFlag(UIObject_IsChecked, !!(flags & UIObject_IsChecked));
	if (!(flags & UIObject_IsDisabled))
	{
		btn.HandleEvent(EventType::ButtonDown) = [this](Event& e)
		{
			if (e.GetButton() != MouseButton::Left)
				return;
			SetFlag(UIObject_IsChecked, true);
			Rebuild();
		};
	}

	auto& icon = Make<BoxElement>();
	icon.SetStyle(GetCurrentTheme()->GetStyle(sid_dropdown_button_icon));
	auto* pap = Allocate<PointAnchoredPlacement>();
	pap->anchor = { 1, 0 };
	pap->pivot = { 0, 0 };
	icon.GetStyle().SetPlacement(pap);

	OnBuildButtonContents();

	Pop();
}

void DropdownMenu::OnBuildMenuWithLayout()
{
	auto& list = OnBuildMenu();
	auto* topLeftPlacement = Allocate<PointAnchoredPlacement>();
	topLeftPlacement->anchor = { 0, 1 };
	list + SetPlacement(topLeftPlacement);
	list + MakeOverlay(200.f);
	list + SetMinWidth(Coord::Percent(100));
}

UIObject& DropdownMenu::OnBuildMenu()
{
	auto& ret = Push<ListBox>();
	Push<StackTopDownLayoutElement>();

	OnBuildMenuContents();

	Pop();
	Pop();
	return ret;
}


void CStrOptionList::BuildElement(const void* ptr, uintptr_t id, bool list)
{
	Text(static_cast<const char*>(ptr));
}

static const char* Next(const char* v)
{
	return v + strlen(v) + 1;
}

void ZeroSepCStrOptionList::IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn)
{
	const char* s = str;
	for (size_t i = 0; i < from && i < size && *s; i++, s = Next(s));
	for (size_t i = 0; i < count && i + from < size && *s; i++, s = Next(s))
		if (*s != '\b')
			fn(s, i + from);
}

void CStrArrayOptionList::IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn)
{
	const char* const* a = arr;
	for (size_t i = 0; i < from && i < size && *a; i++, a++);
	for (size_t i = 0; i < count && i + from < size && *a; i++, a++)
		fn(*a, i + from);
}

void StringArrayOptionList::IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn)
{
	for (size_t i = 0; i < count && i + from < options.size(); i++)
		fn(options[i + from].c_str(), i + from);
}


void DropdownMenuList::OnReset()
{
	DropdownMenu::OnReset();

	_options = nullptr;
}

void DropdownMenuList::OnBuildButtonContents()
{
	bool found = false;
	_options->IterateElements(0, SIZE_MAX, [this, &found](const void* ptr, uintptr_t id)
	{
		if (id == _selected && !found)
		{
			_options->BuildElement(ptr, id, false);
			found = true;
		}
	});
	if (!found)
		OnBuildEmptyButtonContents();
}

void DropdownMenuList::OnBuildMenuContents()
{
	_options->IterateElements(0, SIZE_MAX, [this](const void* ptr, uintptr_t id)
	{
		OnBuildMenuElement(ptr, id);
	});
}

void DropdownMenuList::OnBuildEmptyButtonContents()
{
}

void DropdownMenuList::OnBuildMenuElement(const void* ptr, uintptr_t id)
{
	auto& opt = Push<Selectable>();
	opt.Init(_selected == id);

	_options->BuildElement(ptr, id, true);

	Pop();

	opt + AddEventHandler(EventType::ButtonUp, [this, id](Event& e)
	{
		if (e.GetButton() != MouseButton::Left)
			return;
		_selected = id;
		SetFlag(UIObject_IsChecked, false);
		e.context->OnChange(this);
		e.context->OnCommit(this);
		Rebuild();
	});
}


void OverlayInfoPlacement::OnApplyPlacement(UIObject* curObj, UIRect& outRect) const
{
	auto contSize = curObj->GetNativeWindow()->GetSize();

#if 1
	Size2f minContSize = { float(contSize.x), float(contSize.y) };
#else
	// do not let container size exceed window size
	auto minContSize = containerSize;
	if (minContSize.x > contSize.x)
		minContSize.x = contSize.x;
	if (minContSize.y > contSize.y)
		minContSize.y = contSize.y;
#endif

	float w = curObj->GetFullEstimatedWidth(minContSize, EstSizeType::Expanding, false).min;
	float h = curObj->GetFullEstimatedHeight(minContSize, EstSizeType::Expanding, false).min;

	UIRect avoidRect = UIRect::FromCenterExtents(curObj->system->eventSystem.prevMousePos, 16);

	float freeL = avoidRect.x0;
	float freeR = contSize.x - avoidRect.x1;
	float freeT = avoidRect.y0;
	float freeB = contSize.y - avoidRect.y1;

	int dirX = 1;
	int dirY = 1;
	if (w > freeR && freeL > freeR)
		dirX = -1;
	if (h > freeB && freeT > freeB)
		dirY = -1;

	// anchor
	float px = dirX < 0 ? avoidRect.x0 - w : avoidRect.x1;
	float py = dirY < 0 ? avoidRect.y0 - h : avoidRect.y1;
	px = clamp(px, 0.0f, float(contSize.x));
	py = clamp(py, 0.0f, float(contSize.y));

	UIRect R = { px, py, px + w, py + h };
	outRect = R;
}


void TooltipFrame::OnReset()
{
	UIElement::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_listbox);
}


void DragDropDataFrame::OnReset()
{
	UIElement::OnReset();

	styleProps = GetCurrentTheme()->GetStyle(sid_listbox);
}


void DefaultOverlayBuilder::Build()
{
	if (drawTooltip || drawDragDrop)
		Subscribe(DCT_MouseMoved);

	auto tmpl = ui::Push<ui::PlacementLayoutElement>().GetSlotTemplate();
	tmpl->measure = false;
	tmpl->placement = &placement;

	if (drawTooltip)
	{
		Subscribe(DCT_TooltipChanged);
		if (Tooltip::IsSet())
		{
			Push<TooltipFrame>().RegisterAsOverlay();
			Tooltip::Build();
			Pop();
		}
	}

	if (drawDragDrop)
	{
		Subscribe(DCT_DragDropDataChanged);
		if (auto* ddd = DragDrop::GetData())
		{
			if (ddd->ShouldBuild())
			{
				Push<DragDropDataFrame>().RegisterAsOverlay();
				ddd->Build();
				Pop();
			}
		}
	}

	Pop();
}


} // ui
