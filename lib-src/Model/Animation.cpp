
#include "Animation.h"


namespace ui {

void AnimPlayer::PlayAnim(const AnimPtr& anim)
{
	StopAnim(anim);
	anim->Reset(this);
	_activeAnims.Append(anim);

	if (!IsAnimating())
		_prevTime = platform::GetTimeMs();
	BeginAnimation();
}

void AnimPlayer::StopAnim(const AnimPtr& anim)
{
	_activeAnims.RemoveFirstOf(anim);

	if (_activeAnims.IsEmpty())
		EndAnimation();
}

void AnimPlayer::StopAllAnims()
{
	_activeAnims.Clear();
	EndAnimation();
}

float AnimPlayer::GetVariable(const std::string& name, float def) const
{
	return _variables.GetValueOrDefault(name, def);
}

void AnimPlayer::SetVariable(const std::string& name, float value)
{
	_variables.Insert(name, value);
}

void AnimPlayer::OnAnimationFrame()
{
	uint32_t t = platform::GetTimeMs();
	for (size_t i = 0; i < _activeAnims.size(); i++)
	{
		if (_activeAnims[i]->Advance((t - _prevTime) * 0.001f, this) > 0)
			_activeAnims.RemoveAt(i--);
	}
	_prevTime = t;

	if (onAnimUpdate)
		onAnimUpdate();

	if (_activeAnims.IsEmpty())
		EndAnimation();
}


void SequenceAnimation::Reset(IAnimState* asrw)
{
	_curAnim = SIZE_MAX;
}

float SequenceAnimation::Advance(float dt, IAnimState* asrw)
{
	if (_curAnim == SIZE_MAX)
	{
		_curAnim = 0;
		if (animations.NotEmpty())
			animations[0]->Reset(asrw);
	}
	while (dt > 0 && _curAnim < animations.size())
	{
		float res = animations[_curAnim]->Advance(dt, asrw);
		if (res > 0)
		{
			dt = res;
			_curAnim++;
			if (_curAnim < animations.size())
				animations[_curAnim]->Reset(asrw);
		}
		else
			return 0;
	}
	return dt;
}


void ParallelAnimation::Reset(IAnimState* asrw)
{
	_states.Clear();
	_states.ResizeWith(animations.Size(), 0);
}

float ParallelAnimation::Advance(float dt, IAnimState* asrw)
{
	float minUnused = dt;
	for (size_t i = 0; i < animations.size(); i++)
	{
		if (_states[i] == 0)
		{
			animations[i]->Reset(asrw);
			_states[i] = 1;
		}
		if (_states[i] == 1)
		{
			float res = animations[i]->Advance(dt, asrw);
			if (minUnused > res)
				minUnused = res;
			if (res > 0)
				_states[i] = 2;
		}
	}
	return minUnused;
}


void RepeatAnimation::Reset(IAnimState* asrw)
{
	_curTime = 0;
	animation->Reset(asrw);
}

float RepeatAnimation::Advance(float dt, IAnimState* asrw)
{
	while (dt > 0 && _curTime < times)
	{
		float res = animation->Advance(dt, asrw);
		if (res > 0)
		{
			dt = res;
			_curTime++;
			animation->Reset(asrw);
		}
		else
			return 0;
	}
	return dt;
}


void EasingAnimation::Reset(IAnimState* asrw)
{
	_t = 0;
	_startVal = asrw->GetVariable(param, target);
}

float EasingAnimation::Advance(float dt, IAnimState* asrw)
{
	if (_t + dt > length)
	{
		_t = length;
		_Apply(asrw);
		return _t + dt - length;
	}
	else
	{
		_t += dt;
		_Apply(asrw);
		return 0;
	}
}

void EasingAnimation::_Apply(IAnimState* asrw)
{
	float q = Evaluate(_t / length);
	float v = _startVal + (target - _startVal) * q;
	asrw->SetVariable(param, v);
}

} // ui
