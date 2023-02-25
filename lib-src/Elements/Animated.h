
#pragma once
#include "../Model/Objects.h"
#include "../Model/Animation.h"


namespace ui {

struct VExpandContainer : WrapperElement, private AnimationRequester
{
	bool _firstFrame = true;
	bool _open = true;
	float _animTimeSec = 0.1f;
	float _openState = 1;
	
	// AnimationRequester
	void OnAnimationFrame() override;
	// WrapperElement
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	VExpandContainer& Init(bool open = true, float animTimeSec = 0.1f);
	VExpandContainer& SetOpen(bool open);
	VExpandContainer& SetAnimTime(float animTimeSec);
};

} // ui
