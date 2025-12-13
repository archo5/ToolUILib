
#include "Animated.h"


namespace ui {

void VExpandContainer::OnAnimationFrame()
{
	if (_animTimeSec == 0)
		_openState = _open;
	else
	{
		// TODO delta time?
		_openState = clamp(_openState + (_open ? 1.0f : -1.0f) / 60.0f / _animTimeSec, 0.0f, 1.0f);
	}
	_OnChangeStyle();
	if (auto* w = GetNativeWindow())
		w->InvalidateAll();
	if ((_open ? 1 : 0) == _openState)
	{
		EndAnimation();
	}
}

void VExpandContainer::OnReset()
{
	WrapperElement::OnReset();
	_animTimeSec = 0.1f;
}

void VExpandContainer::OnPaint(const UIPaintContext& ctx)
{
	_firstFrame = false;

	if (_openState < 1)
	{
		if (draw::PushScissorRectIfNotEmpty(GetFinalRect()))
		{
			WrapperElement::OnPaint(ctx);
			draw::PopScissorRect();
		}
	}
	else
	{
		WrapperElement::OnPaint(ctx);
	}
}

EstSizeRange VExpandContainer::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	EstSizeRange ret = WrapperElement::CalcEstimatedHeight(containerSize, type);

	if (_openState < 1)
	{
		ret.hardMin *= _openState;
		ret.softMin *= _openState;
		ret.hardMax *= _openState;
	}

	return ret;
}

VExpandContainer& VExpandContainer::Init(bool open, float animTimeSec)
{
	if (_firstFrame)
	{
		_open = open;
		_openState = open ? 1 : 0;
	}
	else
		SetOpen(open);
	_animTimeSec = animTimeSec;
	return *this;
}

VExpandContainer& VExpandContainer::SetOpen(bool open)
{
	if (_open != open)
	{
		_open = open;
		BeginAnimation();
	}
	return *this;
}

VExpandContainer& VExpandContainer::SetAnimTime(float animTimeSec)
{
	_animTimeSec = animTimeSec;
	return *this;
}

} // ui
