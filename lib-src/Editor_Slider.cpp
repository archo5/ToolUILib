
#include "Editor_Slider.h"

#include "Model/Theme.h"


namespace ui {


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

	_limits = { 0, 1, 0 };
	format = {};
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
		if (!IsInputDisabled())
			e.context->OnCommit(this);
	}
	if (e.type == EventType::MouseMove && IsClicked() && !IsInputDisabled())
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


} // ui
