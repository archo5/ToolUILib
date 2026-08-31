
#pragma once

#include "Model/Objects.h"
#include "Gradient.h"


namespace ui {


struct GradientSliderStyle
{
	static constexpr const char* NAME = "GradientSliderStyle";

	float gradientHeight = 50;
	AABB2f gradientMargin = AABB2f::UniformBorder(3);
	float keypointOffset = 8;
	Vec2f keypointSize = 10;
	AABB2f keypointMargin = 3; // also used for hit testing
	PainterHandle backgroundPainter;
	PainterHandle trackFramePainter;
	PainterHandle keypointFramePainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct GradientSlider : UIObjectNoChildren
{
	GradientSliderStyle style;
	Gradient* gradient = nullptr;

	i32 hoveredkp = 0;
	i32 pressedkp = 0;
	float _mxoff = 0;

	GradientSlider& Init(Gradient* g)
	{
		gradient = g;
		return *this;
	}

	i32 FindKP(Vec2f pos);

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return {}; }
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override
	{
		float height = style.gradientHeight + style.gradientMargin.y0 + style.gradientMargin.y1;
		height += style.keypointOffset + style.keypointSize.y;
		if (gradient && gradient->separateAlpha)
			height += style.keypointOffset + style.keypointSize.y;
		return EstSizeRange::Exact(height);
	}

	Rangef TrackUIRange() const;
	float UIXToKPP(float uix) const;
	float KPPToUIX(float kpp) const;
};


struct GradientEditor : Buildable
{
	Gradient* gradient = nullptr;

	GradientEditor& Init(Gradient* g)
	{
		gradient = g;
		return *this;
	}

	void OnReset() override
	{
		Buildable::OnReset();

		gradient = nullptr;
	}

	void Build() override;
};


} // ui
