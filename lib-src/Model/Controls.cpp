
#include "Controls.h"

#include "Layout.h"
#include "Native.h"
#include "Painting.h"
#include "System.h"
#include "Theme.h"


namespace ui {

void FrameStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnFieldBorderBox(oi, "padding", padding);
	OnFieldPainter(oi, td, "backgroundPainter", backgroundPainter);
	OnFieldFontSettings(oi, td, "font", font);
}


void FrameElement::OnReset()
{
	UIObjectSingleChild::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	frameStyle = {};
}

ContentPaintAdvice FrameElement::PaintFrame(const PaintInfo& info)
{
	ContentPaintAdvice cpa;
	if (frameStyle.backgroundPainter)
		cpa = frameStyle.backgroundPainter->Paint(info);
	if (frameStyle.textColor.HasValue())
		cpa.SetTextColor(frameStyle.textColor.GetValue());
	return cpa;
}

void FrameElement::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();
	PaintChildren(ctx, cpa);
}

struct VertexShifter
{
	draw::VertexTransformCallback prev;
	Vec2f offset;
};

void FrameElement::PaintChildren(const UIPaintContext& ctx, const ContentPaintAdvice& cpa)
{
	if (!_child)
		return;

	UIPaintContext subctx = ctx.WithAdvice(cpa);
	if (!cpa.HasTextColor() && frameStyle.textColor.HasValue())
		subctx.textColor = frameStyle.textColor.GetValue();

	if (cpa.offset != Vec2f())
	{
		VertexShifter vsh;
		vsh.offset = cpa.offset;

		draw::VertexTransformFunction* vtf = [](void* userdata, Vertex* vertices, size_t count)
		{
			auto* data = static_cast<VertexShifter*>(userdata);
			for (auto* v = vertices, *vend = vertices + count; v != vend; v++)
			{
				v->x += data->offset.x;
				v->y += data->offset.y;
			}

			data->prev.Call(vertices, count);
		};
		vsh.prev = draw::SetVertexTransformCallback({ &vsh, vtf });

		if (_child)
			_child->Paint(subctx);

		draw::SetVertexTransformCallback(vsh.prev);
	}
	else
	{
		if (_child)
			_child->Paint(subctx);
	}
}

const FontSettings* FrameElement::_GetFontSettings() const
{
	return &frameStyle.font;
}

Size2f FrameElement::GetReducedContainerSize(Size2f size)
{
	size.x -= frameStyle.padding.x0 + frameStyle.padding.x1;
	size.y -= frameStyle.padding.y0 + frameStyle.padding.y1;
	return size;
}

Rangef FrameElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = frameStyle.padding.x0 + frameStyle.padding.x1;
	return (_child ? Rangef::AtLeast(_child->CalcEstimatedWidth(GetReducedContainerSize(containerSize), type).min) : Rangef::AtLeast(0)).Add(pad);
}

Rangef FrameElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad = frameStyle.padding.y0 + frameStyle.padding.y1;
	return (_child ? Rangef::AtLeast(_child->CalcEstimatedHeight(GetReducedContainerSize(containerSize), type).min) : Rangef::AtLeast(0)).Add(pad);
}

void FrameElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto padsub = rect.ShrinkBy(frameStyle.padding);
	if (_child)
	{
		_child->PerformLayout(padsub, info);
		_finalRect = _child->GetFinalRect().ExtendBy(frameStyle.padding);
	}
	else
	{
		_finalRect = rect;
	}
}

FrameElement& FrameElement::RemoveFrameStyle()
{
	frameStyle = {};
	return *this;
}

FrameElement& FrameElement::SetFrameStyle(const StaticID<FrameStyle>& id)
{
	frameStyle = *GetCurrentTheme()->GetStruct<FrameStyle>(id);
	return *this;
}

static StaticID<FrameStyle> sid_framestyle_label("label");
static StaticID<FrameStyle> sid_framestyle_header("header");
static StaticID<FrameStyle> sid_framestyle_group_box("group_box");
static StaticID<FrameStyle> sid_framestyle_button("button");
static StaticID<FrameStyle> sid_framestyle_selectable("selectable");
static StaticID<FrameStyle> sid_framestyle_dropdown_button("dropdown_button");
static StaticID<FrameStyle> sid_framestyle_listbox("listbox");
static StaticID<FrameStyle> sid_framestyle_textbox("textbox");
static StaticID<FrameStyle> sid_framestyle_proc_graph_node("proc_graph_node");
static StaticID<FrameStyle> sid_framestyle_checkerboard("checkerboard");
FrameElement& FrameElement::SetDefaultFrameStyle(DefaultFrameStyle style)
{
	switch (style)
	{
	case DefaultFrameStyle::Label: return SetFrameStyle(sid_framestyle_label);
	case DefaultFrameStyle::Header: return SetFrameStyle(sid_framestyle_header);
	case DefaultFrameStyle::GroupBox: return SetFrameStyle(sid_framestyle_group_box);
	case DefaultFrameStyle::Button: return SetFrameStyle(sid_framestyle_button);
	case DefaultFrameStyle::Selectable: return SetFrameStyle(sid_framestyle_selectable);
	case DefaultFrameStyle::DropdownButton: return SetFrameStyle(sid_framestyle_dropdown_button);
	case DefaultFrameStyle::ListBox: return SetFrameStyle(sid_framestyle_listbox);
	case DefaultFrameStyle::TextBox: return SetFrameStyle(sid_framestyle_textbox);
	case DefaultFrameStyle::ProcGraphNode: return SetFrameStyle(sid_framestyle_proc_graph_node);
	case DefaultFrameStyle::Checkerboard: return SetFrameStyle(sid_framestyle_checkerboard);
	}
	return *this;
}


void IconStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnField(oi, "width", size.x);
	OnField(oi, "height", size.y);
	OnFieldPainter(oi, td, "painter", painter);
}

void IconElement::OnReset()
{
	UIObjectNoChildren::OnReset();

	style = {};
}

void IconElement::OnPaint(const UIPaintContext& ctx)
{
	if (style.painter)
		style.painter->Paint(this);
}

Rangef IconElement::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::Exact(style.size.x);
}

Rangef IconElement::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::Exact(style.size.y);
}

void IconElement::OnLayout(const UIRect& rect, LayoutInfo info)
{
	_finalRect = rect;
}

IconElement& IconElement::SetStyle(const StaticID<IconStyle>& id)
{
	style = *GetCurrentTheme()->GetStruct<IconStyle>(id);
	return *this;
}

static StaticID<IconStyle> sid_iconstyle_checkbox("checkbox");
static StaticID<IconStyle> sid_iconstyle_radio_button("radio_button");
static StaticID<IconStyle> sid_iconstyle_tree_expand("tree_expand");

static StaticID<IconStyle> sid_iconstyle_add("add");
static StaticID<IconStyle> sid_iconstyle_close("close");
static StaticID<IconStyle> sid_iconstyle_delete("delete");
static StaticID<IconStyle> sid_iconstyle_play("play");
static StaticID<IconStyle> sid_iconstyle_pause("pause");
static StaticID<IconStyle> sid_iconstyle_stop("stop");

IconElement& IconElement::SetDefaultStyle(DefaultIconStyle style)
{
	switch (style)
	{
	case DefaultIconStyle::Checkbox: return SetStyle(sid_iconstyle_checkbox);
	case DefaultIconStyle::RadioButton: return SetStyle(sid_iconstyle_radio_button);
	case DefaultIconStyle::TreeExpand: return SetStyle(sid_iconstyle_tree_expand);

	case DefaultIconStyle::Add: return SetStyle(sid_iconstyle_add);
	case DefaultIconStyle::Close: return SetStyle(sid_iconstyle_close);
	case DefaultIconStyle::Delete: return SetStyle(sid_iconstyle_delete);
	case DefaultIconStyle::Play: return SetStyle(sid_iconstyle_play);
	case DefaultIconStyle::Pause: return SetStyle(sid_iconstyle_pause);
	case DefaultIconStyle::Stop: return SetStyle(sid_iconstyle_stop);
	}
	return *this;
}


void LabelFrame::OnReset()
{
	FrameElement::OnReset();

	SetDefaultFrameStyle(DefaultFrameStyle::Label);
}

Rangef LabelFrame::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = frameStyle.padding.x0 + frameStyle.padding.x1;
	return (_child ? _child->CalcEstimatedWidth(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

Rangef LabelFrame::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad = frameStyle.padding.y0 + frameStyle.padding.y1;
	return (_child ? _child->CalcEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}


void Header::OnReset()
{
	FrameElement::OnReset();

	SetDefaultFrameStyle(DefaultFrameStyle::Header);
}


void Button::OnReset()
{
	FrameElement::OnReset();

	flags |= UIObject_DB_Button;
	SetDefaultFrameStyle(DefaultFrameStyle::Button);
}

Rangef Button::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(FrameElement::CalcEstimatedWidth(containerSize, type).min);
}

Rangef Button::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::AtLeast(FrameElement::CalcEstimatedHeight(containerSize, type).min);
}

void Button::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child)
	{
		UIRect r = rect.ShrinkBy(frameStyle.padding);
		auto rSize = r.GetSize();
		float minw = _child->CalcEstimatedWidth(rSize, EstSizeType::Expanding).min;
		float minh = _child->CalcEstimatedHeight(rSize, EstSizeType::Expanding).min;
		float ox = roundf((rSize.x - minw) * 0.5f);
		float oy = roundf((rSize.y - minh) * 0.5f);
		UIRect fr = { r.x0 + ox, r.y0 + oy, r.x0 + ox + minw, r.y0 + oy + minh };
		fr = fr.Intersect(r);
		_child->PerformLayout(fr, info);
	}
	_finalRect = rect;
}


void StateButtonBase::OnReset()
{
	WrapperElement::OnReset();

	flags |= UIObject_DB_Button;
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


void StateToggleIconBase::OnPaint(const UIPaintContext& ctx)
{
	if (!style.painter)
		return;

	StateButtonBase* st = FindParentOfType<StateButtonBase>();
	PaintInfo info(st ? static_cast<UIObject*>(st) : this);
	if (st)
		info.SetCheckState(st->GetState());

	info.obj = this;
	info.rect = GetFinalRect();

	style.painter->Paint(info);
}


void StateToggleFrameBase::OnPaint(const UIPaintContext& ctx)
{
	StateButtonBase* st = FindParentOfType<StateButtonBase>();
	PaintInfo info(st ? static_cast<UIObject*>(st) : this);
	if (st)
		info.SetCheckState(st->GetState());

	info.obj = this;
	info.rect = GetFinalRect();

	auto cpa = PaintFrame(info);
	PaintChildren(ctx, cpa);
}


void CheckboxIcon::OnReset()
{
	StateToggleIconBase::OnReset();

	SetDefaultStyle(DefaultIconStyle::Checkbox);
}


void RadioButtonIcon::OnReset()
{
	StateToggleIconBase::OnReset();

	SetDefaultStyle(DefaultIconStyle::RadioButton);
}


void TreeExpandIcon::OnReset()
{
	StateToggleIconBase::OnReset();

	SetDefaultStyle(DefaultIconStyle::TreeExpand);
}


void StateButtonSkin::OnReset()
{
	StateToggleFrameBase::OnReset();

	SetDefaultFrameStyle(DefaultFrameStyle::Button);
}

void StateButtonSkin::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (_child)
	{
		UIRect r = rect.ShrinkBy(frameStyle.padding);
		auto rSize = r.GetSize();
		float minw = _child->CalcEstimatedWidth(rSize, EstSizeType::Expanding).min;
		float minh = _child->CalcEstimatedHeight(rSize, EstSizeType::Expanding).min;
		float ox = roundf((rSize.x - minw) * 0.5f);
		float oy = roundf((rSize.y - minh) * 0.5f);
		UIRect fr = { r.x0 + ox, r.y0 + oy, r.x0 + ox + minw, r.y0 + oy + minh };
		fr = fr.Intersect(r);
		_child->PerformLayout(fr, info);
	}
	_finalRect = rect;
}


void ListBoxFrame::OnReset()
{
	FrameElement::OnReset();

	SetDefaultFrameStyle(DefaultFrameStyle::ListBox);
}


void Selectable::OnReset()
{
	FrameElement::OnReset();

	flags |= UIObject_DB_Selectable;
	SetDefaultFrameStyle(DefaultFrameStyle::Selectable);
}


void ProgressBarStyle::Serialize(ThemeData& td, IObjectIterator& oi) // TODO
{
	OnFieldBorderBox(oi, "padding", padding);
	OnFieldBorderBox(oi, "fillMargin", fillMargin);
	OnFieldPainter(oi, td, "backgroundPainter", backgroundPainter);
	OnFieldPainter(oi, td, "fillPainter", fillPainter);
}

static StaticID<ProgressBarStyle> sid_progress_bar_style("progress_bar");
void ProgressBar::OnReset()
{
	UIObjectSingleChild::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	style = *GetCurrentTheme()->GetStruct(sid_progress_bar_style);
	progress = 0.5f;
}

void ProgressBar::OnPaint(const UIPaintContext& ctx)
{
	ContentPaintAdvice cpa;

	if (style.backgroundPainter)
	{
		cpa = style.backgroundPainter->Paint(this);
	}

	if (style.fillPainter)
	{
		PaintInfo cinfo(this);
		cinfo.rect = cinfo.rect.ShrinkBy(style.fillMargin);
		cinfo.rect.x1 = lerp(cinfo.rect.x0, cinfo.rect.x1, progress);
		style.fillPainter->Paint(cinfo);
	}

	if (_child)
		_child->Paint(ctx.WithAdvice(cpa));
}

Size2f ProgressBar::GetReducedContainerSize(Size2f size)
{
	size.x -= style.padding.x0 + style.padding.x1;
	size.y -= style.padding.y0 + style.padding.y1;
	return size;
}

Rangef ProgressBar::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float pad = style.padding.x0 + style.padding.x1;
	return Rangef::AtLeast(_child ? _child->CalcEstimatedWidth(GetReducedContainerSize(containerSize), type).min : 0).Add(pad);
}

Rangef ProgressBar::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float pad = style.padding.y0 + style.padding.y1;
	return (_child ? _child->CalcEstimatedHeight(GetReducedContainerSize(containerSize), type) : Rangef::AtLeast(0)).Add(pad);
}

void ProgressBar::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto padsub = rect.ShrinkBy(style.padding);
	if (_child)
	{
		_child->PerformLayout(padsub, info);
		ApplyLayoutInfo(_child->GetFinalRect().ExtendBy(style.padding), rect, info);
	}
	else
	{
		_finalRect = rect;
	}
}


void SliderStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnField(oi, "minSize", minSize);
	OnField(oi, "trackSize", trackSize);
	OnFieldBorderBox(oi, "trackMargin", trackMargin);
	OnFieldBorderBox(oi, "trackFillMargin", trackFillMargin);
	OnFieldBorderBox(oi, "thumbExtend", thumbExtend);
	OnFieldPainter(oi, td, "backgroundPainter", backgroundPainter);
	OnFieldPainter(oi, td, "trackBasePainter", trackBasePainter);
	OnFieldPainter(oi, td, "trackFillPainter", trackFillPainter);
	OnFieldPainter(oi, td, "thumbPainter", thumbPainter);
}

static StaticID<SliderStyle> sid_slider_style_hor("slider_style_horizontal");
void Slider::OnReset()
{
	UIObjectNoChildren::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	style = *GetCurrentTheme()->GetStruct(sid_slider_style_hor);

	_value = 0;
	_limits = { 0, 1, 0 };
}

void Slider::OnPaint(const UIPaintContext& ctx)
{
	ContentPaintAdvice cpa;
	if (style.backgroundPainter)
		cpa = style.backgroundPainter->Paint(this);

	// track
	PaintInfo trkinfo(this);
	trkinfo.rect = trkinfo.rect.ShrinkBy(style.trackMargin);
	if (style.trackBasePainter)
		style.trackBasePainter->Paint(trkinfo);

	// track fill
	trkinfo.rect = trkinfo.rect.ShrinkBy(style.trackFillMargin);
	trkinfo.rect.x1 = round(lerp(trkinfo.rect.x0, trkinfo.rect.x1, ValueToQ(_value)));
	auto prethumb = trkinfo.rect;

	if (style.trackFillPainter)
		style.trackFillPainter->Paint(trkinfo);

	// thumb
	prethumb.x0 = prethumb.x1;
	trkinfo.rect = prethumb.ExtendBy(style.thumbExtend);
	if (style.thumbPainter)
		style.thumbPainter->Paint(trkinfo);
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
	auto rect = GetFinalRect().ShrinkBy(style.trackMargin);
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
	WrapperElement::OnReset();

	splitPos = Coord::Percent(40);
	minSplitPos = 0;

	defaultLabelStyle = *GetCurrentTheme()->GetStruct(sid_framestyle_label);
}

void PropertyList::OnLayout(const UIRect& rect, LayoutInfo info)
{
	float splitPosR = ResolveUnits(splitPos, rect.GetWidth());
	float minSplitPosR = ResolveUnits(minSplitPos, rect.GetWidth());
	float finalSplitPosR = max(splitPosR, minSplitPosR);
	_calcSplitX = roundf(finalSplitPosR + rect.x0);

	WrapperElement::OnLayout(rect, info);
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
	WrapperElement::OnReset();

	flags |= UIObject_SetsChildTextStyle;
	_labelStyle = *GetCurrentTheme()->GetStruct(sid_framestyle_label);

	_labelText = {};
	_isBrief = false;
}

void LabeledProperty::OnEnterTree()
{
	WrapperElement::OnEnterTree();

	_propList = FindParentOfType<PropertyList>();
}

void LabeledProperty::OnPaint(const UIPaintContext& ctx)
{
	if (!_labelText.empty())
	{
		auto& labelStyle = FindCurrentLabelStyle();

		auto* font = labelStyle.font.GetFont();

		auto contPadRect = GetFinalRect();
		UIRect labelContRect = { contPadRect.x0, contPadRect.y0, _lastSepX, contPadRect.y1 };
		auto cr = labelContRect;
		auto r = labelContRect.ShrinkBy(labelStyle.padding);

		// TODO optimize scissor (shared across labels)
		//if (draw::PushScissorRectIfNotEmpty(cr))
		{
			ContentPaintAdvice cpa;
			cpa.SetTextColor(ctx.textColor);
			if (labelStyle.backgroundPainter)
			{
				PaintInfo pi(this);
				pi.rect = cr;
				cpa = labelStyle.backgroundPainter->Paint(pi);
			}
			if (labelStyle.textColor.HasValue())
				cpa.SetTextColor(labelStyle.textColor.GetValue());

			draw::TextLine(
				font,
				labelStyle.font.size,
				r.x0, (r.y0 + r.y1) / 2,
				_labelText,
				cpa.GetTextColor(),
				TextBaseline::Middle,
				&cr);
			//draw::PopScissorRect();
		}
	}

	if (_child)
		_child->Paint(ctx);
}

void LabeledProperty::OnLayout(const UIRect& rect, LayoutInfo info)
{
	if (!_labelText.empty())
	{
		if (_isBrief)
		{
			auto& labelStyle = FindCurrentLabelStyle();

			auto* font = labelStyle.font.GetFont();

			float labelPartWidth = GetTextWidth(font, labelStyle.font.size, _labelText) + labelStyle.padding.x0 + labelStyle.padding.x1;
			float childPartWidth = _child ? _child->CalcEstimatedWidth(rect.GetSize(), ui::EstSizeType::Expanding).min : 0;
			if (labelPartWidth + childPartWidth > rect.GetWidth())
			{
				// reduce size proportionally since cannot fit both
				_lastSepX = rect.x0 + roundf(labelPartWidth * rect.GetWidth() / (labelPartWidth + childPartWidth));
			}
			else
				_lastSepX = rect.x0 + labelPartWidth;
		}
		else
		{
			_lastSepX = _propList
				? _propList->_calcSplitX
				: rect.x0 + roundf(rect.GetWidth() * 0.4f);
		}
	}
	else
		_lastSepX = rect.x0;

	if (_child)
	{
		UIRect r = rect;
		r.x0 = _lastSepX;
		_child->PerformLayout(r, info);
	}
	_finalRect = rect;
}

void LabeledProperty::OnEvent(Event& e)
{
	if (e.type == EventType::Tooltip && e.target == this && e.position.x < _lastSepX && _tooltipBuildFunc)
	{
		Tooltip::Set(this, _tooltipBuildFunc);
		e.StopPropagation();
	}
}

LabeledProperty& LabeledProperty::SetText(StringView text)
{
	_labelText.assign(text.data(), text.size());
	return *this;
}

const FrameStyle& LabeledProperty::FindCurrentLabelStyle() const
{
	if (_preferOwnLabelStyle || !_propList)
		return _labelStyle;
	return _propList->defaultLabelStyle;
}

const FrameStyle& LabeledProperty::GetLabelStyle()
{
	return _labelStyle;
}

LabeledProperty& LabeledProperty::SetLabelStyle(const FrameStyle& s)
{
	_labelStyle = s;
	_preferOwnLabelStyle = true;
	return *this;
}


void ScrollbarStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnField(oi, "width", width);
	OnFieldBorderBox(oi, "trackFillMargin", trackFillMargin);
	OnField(oi, "thumbMinLength", thumbMinLength);
	OnFieldPainter(oi, td, "trackPainter", trackPainter);
	OnFieldPainter(oi, td, "thumbPainter", thumbPainter);
}

ScrollbarV::ScrollbarV()
{
	OnReset();
}

static StaticID<ScrollbarStyle> sid_scrollbar_style_vert("scrollbar_vertical");
void ScrollbarV::OnReset()
{
	style = *GetCurrentTheme()->GetStruct(sid_scrollbar_style_vert);
}

Coord ScrollbarV::GetWidth()
{
	return style.width;
}

UIRect ScrollbarV::GetThumbRect(const ScrollbarData& info)
{
	float viewportSize = max(info.viewportSize, 1.0f);
	float contentSize = max(info.contentSize, viewportSize);

	float scrollFactor = contentSize == viewportSize ? 0 : info.contentOff / (contentSize - viewportSize);
	float thumbSizeFactor = viewportSize / contentSize;

	UIRect trackRect = info.rect.ShrinkBy(style.trackFillMargin);

	float thumbSize = thumbSizeFactor * trackRect.GetHeight();
	float minH = style.thumbMinLength;
	thumbSize = max(thumbSize, minH);

	float pos = (trackRect.GetHeight() - thumbSize) * scrollFactor;

	return { trackRect.x0, trackRect.y0 + pos, trackRect.x1, trackRect.y0 + pos + thumbSize };
}

void ScrollbarV::OnPaint(const ScrollbarData& info)
{
	PaintInfo vsinfo;
	vsinfo.obj = info.owner;
	vsinfo.rect = info.rect;
	if (style.trackPainter)
		style.trackPainter->Paint(vsinfo);

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
	if (style.thumbPainter)
		style.thumbPainter->Paint(vsinfo);
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

		UIRect trackRect = info.rect.ShrinkBy(style.trackFillMargin);

		float thumbSize = thumbSizeFactor * trackRect.GetHeight();
		float minH = style.thumbMinLength;
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

	uiState.FinalizeOnEvent(e);

	return contentOffsetChanged;
}


void ScrollArea::OnPaint(const UIPaintContext& ctx)
{
	if (draw::PushScissorRectIfNotEmpty(GetFinalRect()))
	{
		WrapperElement::OnPaint(ctx);

		draw::PopScissorRect();
	}

	auto cr = GetFinalRect();
	float w = cr.GetWidth();
	auto sbvr = cr;
	sbvr.x1 = cr.x1;
	sbvr.x0 = sbvr.x1 - ResolveUnits(sbv.GetWidth(), w);
	sbv.OnPaint({ this, sbvr, cr.GetHeight(), estContentSize.y, yoff });
}

void ScrollArea::OnEvent(Event& e)
{
	auto cr = GetFinalRect();
	float w = cr.GetWidth();
	auto sbvr = cr;
	sbvr.x1 = cr.x1;
	sbvr.x0 = sbvr.x1 - ResolveUnits(sbv.GetWidth(), w);
	ScrollbarData info = { this, sbvr, cr.GetHeight(), estContentSize.y, yoff };

	if (sbv.OnEvent(info, e))
	{
		if (yoff != info.contentOff)
			e.StopPropagation();
		yoff = info.contentOff;
		_OnChangeStyle();
	}
}

void ScrollArea::OnLayout(const UIRect& rect, LayoutInfo info)
{
	auto realContSize = rect.GetSize();

	bool hasVScroll = estContentSize.y > rect.GetHeight();
	float vScrollWidth = ResolveUnits(sbv.GetWidth(), rect.GetWidth());
	//if (hasVScroll)
		realContSize.x -= vScrollWidth;
	estContentSize.y = _child ? _child->CalcEstimatedHeight(realContSize, EstSizeType::Exact).min : 0;
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
		_child->PerformLayout(crect, info);
	_finalRect = rect;
}

void ScrollArea::OnReset()
{
	WrapperElement::OnReset();

	sbv.OnReset();
}


void BackgroundBlocker::OnReset()
{
	FillerElement::OnReset();

	RegisterAsOverlay(199.f);
}

void BackgroundBlocker::OnEvent(Event& e)
{
	FillerElement::OnEvent(e);
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
		OnButton();
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		if (HasFlags(UIObject_IsChecked))
			OnButton();
		SetFlag(UIObject_IsChecked, true);
	}
}

void BackgroundBlocker::OnLayout(const UIRect& rect, LayoutInfo info)
{
	FillerElement::OnLayout({ 0, 0, system->eventSystem.width, system->eventSystem.height }, { LayoutInfo::FillH | LayoutInfo::FillV });
}

void BackgroundBlocker::OnButton()
{
	Event e(&system->eventSystem, this, EventType::BackgroundClick);
	system->eventSystem.BubblingEvent(e);
}


struct DropdownMenuPlacement : IPlacement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const override
	{
		Rangef rw = curObj->CalcEstimatedWidth(outRect.GetSize(), ui::EstSizeType::Expanding);
		Rangef rh = curObj->CalcEstimatedHeight(outRect.GetSize(), ui::EstSizeType::Expanding);

		rw = rw.Intersect(Rangef::AtLeast(outRect.GetWidth()));

		float x = outRect.x0;
		float y = outRect.y1;
		float w = rw.min;
		float h = rh.min;

		auto windowInnerSize = curObj->GetNativeWindow()->GetInnerRect().GetSize();
		if (y + h > windowInnerSize.y)
		{
			// try putting on top if there's more space
			if (outRect.y0 > windowInnerSize.y - y)
			{
				y = max(outRect.y0 - h, 0.0f);
				h = min(h, outRect.y0);
			}
			else
			{
				// clamp vertical height in the same place
				h = min(h, windowInnerSize.y - y);
			}
		}

		outRect = { x, y, x + w, y + h };
	}
};


void DropdownMenu::Build()
{
	auto tmpl = Push<PlacementLayoutElement>().GetSlotTemplate();

	OnBuildButton();

	if (HasFlags(UIObject_IsChecked))
	{
		Make<BackgroundBlocker>();

		tmpl->measure = false;
		tmpl->placement = UI_BUILD_ALLOC(DropdownMenuPlacement)();
		OnBuildMenuWithLayout();
	}

	Pop();
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

void DropdownMenu::OnBuildButton()
{
	auto& btn = Push<FrameElement>().SetDefaultFrameStyle(DefaultFrameStyle::DropdownButton);
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

	Push<ChildScaleOffsetElement>(); // clip only (TODO optimize?)
	Push<StackLTRLayoutElement>();
	OnBuildButtonContents();
	Pop(); // StackLTRLayoutElement
	Pop(); // ChildScaleOffsetElement

	Pop(); // FrameElement(DropdownButton)
}

void DropdownMenu::OnBuildMenuWithLayout()
{
	auto& list = OnBuildMenu();
	list + MakeOverlay(200.f);
}

struct StopScroll : WrapperElement
{
	void OnEvent(Event& e) override
	{
		if (e.type == EventType::MouseScroll)
			e.StopPropagation();
	}
};

UIObject& DropdownMenu::OnBuildMenu()
{
	auto& ret = Push<StopScroll>();
	Push<ListBoxFrame>();
	Push<SizeConstraintElement>().SetMaxHeight(200);
	Push<ScrollArea>();
	Push<StackTopDownLayoutElement>();

	OnBuildMenuContents();

	Pop();
	Pop();
	Pop();
	Pop();
	Pop();
	return ret;
}


void OptionList::BuildEmptyButtonContents()
{
	Text({});
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
	if (from == 0 && count && emptyStr)
	{
		fn(emptyStr, emptyValue);
		count--;
	}
	for (size_t i = 0; i < from && i < size && *s; i++, s = Next(s));
	for (size_t i = 0; i < count && i + from < size && *s; i++, s = Next(s))
		if (*s != '\b')
			fn(s, i + from);
}

void ZeroSepCStrOptionList::BuildEmptyButtonContents()
{
	Text(emptyStr ? emptyStr : StringView());
}

void CStrArrayOptionList::IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn)
{
	const char* const* a = arr;
	if (from == 0 && count && emptyStr)
	{
		fn(emptyStr, emptyValue);
		count--;
	}
	for (size_t i = 0; i < from && i < size && *a; i++, a++);
	for (size_t i = 0; i < count && i + from < size && *a; i++, a++)
		fn(*a, i + from);
}

void CStrArrayOptionList::BuildEmptyButtonContents()
{
	Text(emptyStr ? emptyStr : StringView());
}

void StringArrayOptionList::IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn)
{
	if (from == 0 && count && emptyStr.HasValue())
	{
		fn(emptyStr.GetValue().c_str(), emptyValue);
		count--;
	}
	for (size_t i = 0; i < count && i + from < options.Size(); i++)
		fn(options[i + from].c_str(), i + from);
}

void StringArrayOptionList::BuildEmptyButtonContents()
{
	Text(emptyStr.HasValue() ? emptyStr.GetValue().c_str() : StringView());
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
	_options->BuildEmptyButtonContents();
}

void DropdownMenuList::OnBuildMenuElement(const void* ptr, uintptr_t id)
{
	auto& opt = Push<Selectable>();
	opt.Init(_selected == id);
	Push<StackExpandLTRLayoutElement>(); // TODO always needed? maybe can manage with just one text element

	_options->BuildElement(ptr, id, true);

	Pop();
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
	auto contSize = curObj->GetNativeWindow()->GetInnerSize();

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

	float w = curObj->CalcEstimatedWidth(minContSize, EstSizeType::Expanding).min;
	float h = curObj->CalcEstimatedHeight(minContSize, EstSizeType::Expanding).min;

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


struct CursorFollowingPLE : PlacementLayoutElement
{
	// hack
	void OnPaint(const UIPaintContext& pc) override
	{
		if (_slots.NotEmpty())
			_OnChangeStyle();
	}
};


void DefaultOverlayBuilder::Build()
{
	auto& cfple = ui::Push<PlacementLayoutElement>();
	BuildMulticastDelegateAdd(OnMouseMoved, [&cfple](NativeWindowBase* win, int, int)
	{
		if (win == cfple.GetNativeWindow())
			cfple._OnChangeStyle();
	});
	auto tmpl = cfple.GetSlotTemplate();

	if (drawTooltip)
	{
		BuildMulticastDelegateAdd(OnTooltipChanged, [this]() { Rebuild(); });
		if (Tooltip::IsSet())
		{
			tmpl->measure = false;
			tmpl->placement = &placement;

			Push<FrameElement>()
				.SetDefaultFrameStyle(DefaultFrameStyle::ListBox)
				.RegisterAsOverlay();
			Tooltip::Build();
			Pop();
		}
	}

	if (drawDragDrop)
	{
		BuildMulticastDelegateAdd(OnDragDropDataChanged, [this]() { Rebuild(); });
		if (auto* ddd = DragDrop::GetData())
		{
			if (ddd->ShouldBuild())
			{
				tmpl->measure = false;
				tmpl->placement = &placement;

				Push<FrameElement>()
					.SetDefaultFrameStyle(DefaultFrameStyle::ListBox)
					.RegisterAsOverlay();
				ddd->Build();
				Pop();
			}
		}
	}

	Pop();
}


} // ui
