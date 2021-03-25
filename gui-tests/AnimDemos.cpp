
#include "pch.h"


struct SlidingHighlightAnimDemo : ui::Buildable
{
	SlidingHighlightAnimDemo()
	{
		animReq.callback = [this]() { GetNativeWindow()->InvalidateAll(); };
	}
	void Build(ui::UIContainer* ctx) override
	{
		ui::imm::RadioButton(ctx, layout, 0, "No button", { ui::ApplyStyle(ui::Theme::current->button) });
		ui::imm::RadioButton(ctx, layout, 1, "Left", { ui::ApplyStyle(ui::Theme::current->button) });
		ui::imm::RadioButton(ctx, layout, 2, "Right", { ui::ApplyStyle(ui::Theme::current->button) });

		tgt = nullptr;
		if (layout != 0)
		{
			ctx->PushBox() + ui::Set(layout == 1 ? ui::StackingDirection::LeftToRight : ui::StackingDirection::RightToLeft);
			tgt = &ctx->MakeWithText<ui::Button>(layout == 1 ? "Left" : "Right button");
			ctx->Pop();
		}
	}
	void OnLayoutChanged() override
	{
		ui::AABB2f tr = GetTargetRect();
		if (memcmp(&targetLayout, &tr, sizeof(tr)) != 0)
		{
			startLayout = GetCurrentRect();
			targetLayout = tr;
			startTime = ui::platform::GetTimeMs();
			endTime = startTime + 1000;

			animReq.BeginAnimation();
		}
	}
	void OnPaint() override
	{
		ui::Buildable::OnPaint();

		auto r = GetCurrentRect();
		r = r.ShrinkBy(ui::AABB2f::UniformBorder(1));

		ui::Color4f colLine(1, 0, 0);
		ui::draw::LineCol(r.x0, r.y0, r.x1, r.y0, 1, colLine);
		ui::draw::LineCol(r.x0, r.y1, r.x1, r.y1, 1, colLine);
		ui::draw::LineCol(r.x0, r.y0, r.x0, r.y1, 1, colLine);
		ui::draw::LineCol(r.x1, r.y0, r.x1, r.y1, 1, colLine);

		if (ui::platform::GetTimeMs() - startTime > endTime - startTime)
			animReq.EndAnimation();
	}

	ui::AABB2f GetCurrentRect()
	{
		uint32_t t = ui::platform::GetTimeMs();
		if (startTime == endTime)
			return targetLayout;
		float q = (t - startTime) / float(endTime - startTime);
		if (q < 0)
			q = 0;
		else if (q > 1)
			q = 1;
		return
		{
			ui::lerp(startLayout.x0, targetLayout.x0, q),
			ui::lerp(startLayout.y0, targetLayout.y0, q),
			ui::lerp(startLayout.x1, targetLayout.x1, q),
			ui::lerp(startLayout.y1, targetLayout.y1, q),
		};
	}
	ui::AABB2f GetTargetRect()
	{
		if (tgt)
			return tgt->GetBorderRect();
		auto r = GetBorderRect();
		r.y1 = r.y0;
		return r;
	}

	UIObject* tgt = nullptr;

	int layout = 0;
	ui::AABB2f startLayout = {};
	ui::AABB2f targetLayout = {};
	uint32_t startTime = 0;
	uint32_t endTime = 0;

	ui::AnimationCallbackRequester animReq;
};
void Demo_SlidingHighlightAnim(ui::UIContainer* ctx)
{
	ctx->Make<SlidingHighlightAnimDemo>();
}


struct ButtonPressHighlightDemo : ui::Buildable
{
	struct ActivationAnimData
	{
		ui::AnimPlayer player;
		ui::AABB2f baseRect;
	};
	void PlayActivationAnim(ui::Button& button)
	{
		std::unique_ptr<ActivationAnimData> aad(new ActivationAnimData);
		aad->baseRect = button.finalRectCPB;
		aad->player.onAnimUpdate = [this]() { GetNativeWindow()->InvalidateAll(); };

		ui::AnimPtr activationAnim = std::make_shared<ui::ParallelAnimation>
			(std::initializer_list<ui::AnimPtr>{
			std::make_shared<ui::SequenceAnimation>
				(std::initializer_list<ui::AnimPtr>{
				std::make_shared<ui::AnimSetValue>("dist", 0),
					std::make_shared<ui::AnimEaseLinear>("dist", 10, 1),
			}),
				std::make_shared<ui::SequenceAnimation>
					(std::initializer_list<ui::AnimPtr>{
					std::make_shared<ui::AnimSetValue>("alpha", 1),
						std::make_shared<ui::AnimEaseOutCubic>("alpha", 0, 1),
				}),
		});
		aad->player.PlayAnim(activationAnim);

		anims.emplace_back(std::move(aad));
	}
	void AddActivationAnim(ui::Button& button)
	{
		button + ui::AddEventHandler(ui::EventType::Activate, [this, &button](ui::Event&) { PlayActivationAnim(button); });
	}

	void Build(ui::UIContainer* ctx) override
	{
		*this + ui::SetPadding(30);
		*this + ui::SetHeight(ui::Coord::Percent(100));

		AddActivationAnim(ctx->MakeWithText<ui::Button>("Press me"));
		AddActivationAnim(ctx->MakeWithText<ui::Button>("...or me"));
	}
	void OnPaint() override
	{
		ui::Buildable::OnPaint();
		for (const auto& anim : anims)
		{
			float dist = anim->player.GetVariable("dist");
			float alpha = anim->player.GetVariable("alpha", 1);
			ui::draw::RectCutoutCol(anim->baseRect.ExtendBy(ui::AABB2f::UniformBorder(dist)), anim->baseRect, ui::Color4f(1, alpha));
		}
		anims.erase(std::remove_if(anims.begin(), anims.end(), [](const std::unique_ptr<ActivationAnimData>& anim)
		{
			return anim->player.GetVariable("alpha", 1) <= 0;
		}), anims.end());
	}

	std::vector<std::unique_ptr<ActivationAnimData>> anims;
};
void Demo_ButtonPressHighlight(ui::UIContainer* ctx)
{
	ctx->Make<ButtonPressHighlightDemo>();
}

