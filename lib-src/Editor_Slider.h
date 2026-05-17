
#pragma once

#include "Model/Objects.h"
#include "Model/ImmediateMode.h"
#include "Editors/NumEditConfig.h"


namespace ui {


struct FloatLimits
{
	double min = -DBL_MAX;
	double max = DBL_MAX;
	double step = 0;
};

struct SliderStyle
{
	static constexpr const char* NAME = "SliderStyle";

	float minSize = 20;
	float trackSize = 0;
	AABB2f trackMargin = AABB2f::UniformBorder(0);
	AABB2f trackFillMargin = AABB2f::UniformBorder(0);
	AABB2f thumbExtend = AABB2f::UniformBorder(0);
	PainterHandle backgroundPainter;
	PainterHandle trackBasePainter;
	PainterHandle trackFillPainter;
	PainterHandle thumbPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct Slider : UIObjectNoChildren
{
	SliderStyle style;

	double _value = 0;
	FloatLimits _limits = { 0, 1, 0 };
	NumberFormatSettings format;
	float _mxoff = 0;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return {}; }
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override { return EstSizeRange::SoftAtLeast(style.minSize); }

	double PosToQ(double x);
	double QToValue(double q);
	double ValueToQ(double v);
	double PosToValue(double x) { return QToValue(PosToQ(x)); }

	double GetValue() const { return _value; }
	Slider& SetValue(double v) { _value = v; return *this; }
	FloatLimits GetLimits() const { return _limits; }
	Slider& SetLimits(FloatLimits limits) { _limits = limits; return *this; }
	Slider& SetFormat(NumberFormatSettings fmt) { format = fmt; return *this; }

	Slider& Init(float& vp, FloatLimits limits = { 0, 1, 0 })
	{
		SetLimits(limits);
		SetValue(vp);
		HandleEvent(EventType::Change) = [this, &vp](Event&) { vp = (float)GetValue(); };
		return *this;
	}
	Slider& Init(double& vp, FloatLimits limits = { 0, 1, 0 })
	{
		SetLimits(limits);
		SetValue(vp);
		HandleEvent(EventType::Change) = [this, &vp](Event&) { vp = GetValue(); };
		return *this;
	}
};

namespace imm {

inline imCtrlInfoT<Slider> imSliderFloat(float& val, Rangef range, float step = 0, NumberFormatSettings fmt = {})
{
	auto& ne = Make<Slider>();
	ne.SetLimits({ range.min, range.max, step });
	ne.format = fmt;
	if (!imGetEnabled())
		ne.flags |= UIObject_IsDisabled;

	bool changed = imProcessEventFlags(ne);
	if (changed)
		val = ne.GetValue();
	else
		ne.SetValue(val);

	ne.HandleEvent() = [&ne](Event& ev)
	{
		if (ev.type == EventType::Change)
		{
			ev.StopPropagation();
			ne.flags |= UIObject_IsEdited;
			ne.RebuildContainer();
		}
		if (ev.type == EventType::Commit)
			ev.StopPropagation();
	};

	return { changed, &ne };
}

} // imm


} // ui
