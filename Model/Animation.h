
#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "../Core/HashTable.h"
#include "Native.h" // TODO


namespace ui {

struct IAnimState
{
	virtual float GetVariable(const std::string& name, float def = 0) = 0;
	virtual void SetVariable(const std::string& name, float value) = 0;
};

struct Animation
{
	virtual void Reset(IAnimState* asrw) = 0;
	// returns how much time was NOT used (0 if used all)
	virtual float Advance(float dt, IAnimState* asrw) = 0;
};
using AnimPtr = std::shared_ptr<Animation>;

struct AnimPlayer : IAnimState, protected AnimationRequester
{
	void PlayAnim(const AnimPtr& anim);
	void StopAnim(const AnimPtr& anim);
	void StopAllAnims();
	float GetVariable(const std::string& name, float def = 0);
	void SetVariable(const std::string& name, float value);

	void OnAnimationFrame() override;

	std::function<void()> onAnimUpdate;

	HashMap<std::string, float> _variables;
	std::vector<std::shared_ptr<Animation>> _activeAnims;
	uint32_t _prevTime;
};


struct SequenceAnimation : Animation
{
	void Reset(IAnimState* asrw) override;
	float Advance(float dt, IAnimState* asrw) override;

	std::vector<std::shared_ptr<Animation>> animations;

	int _curAnim = -1;
};

struct ParallelAnimation : Animation
{
	void Reset(IAnimState* asrw) override;
	float Advance(float dt, IAnimState* asrw) override;

	std::vector<std::shared_ptr<Animation>> animations;

	std::vector<uint8_t> _states;
};

struct RepeatAnimation : Animation
{
	RepeatAnimation() {}
	RepeatAnimation(int n) : times(n) {}

	void Reset(IAnimState* asrw) override;
	float Advance(float dt, IAnimState* asrw) override;

	std::shared_ptr<Animation> animation;
	int times = 1;

	int _curTime = 0;
};

struct EasingAnimation : Animation
{
	EasingAnimation() {}
	EasingAnimation(const std::string& prm, float tgt, float len) : param(prm), target(tgt), length(len) {}

	void Reset(IAnimState* asrw) override;
	float Advance(float dt, IAnimState* asrw) override;
	virtual float Evaluate(float q) = 0;

	void _Apply(IAnimState* asrw);

	std::string param;
	float target = 0;
	float length = 0;

	float _startVal;
	float _t = 0;
};

struct AnimSetValue : EasingAnimation
{
	AnimSetValue() {}
	AnimSetValue(const std::string& prm, float tgt) : EasingAnimation(prm, tgt, 0) {}

	float Evaluate(float) override { return 1; }
};

struct EasingAnimLinear : EasingAnimation
{
	using EasingAnimation::EasingAnimation;
	float Evaluate(float q) override { return q; }
};

} // ui
