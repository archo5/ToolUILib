
#include "pch.h"

#include <memory>


struct SlidingHighlightAnimDemo : ui::Buildable
{
	SlidingHighlightAnimDemo()
	{
		animReq.callback = [this]() { GetNativeWindow()->InvalidateAll(); };
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::imm::RadioButton(layout, 0, "No button", {}, ui::imm::ButtonStateToggleSkin());
		ui::imm::RadioButton(layout, 1, "Left", {}, ui::imm::ButtonStateToggleSkin());
		ui::imm::RadioButton(layout, 2, "Right", {}, ui::imm::ButtonStateToggleSkin());

		tgt = nullptr;
		if (layout != 0)
		{
			WPush<ui::SizeConstraintElement>().SetHeight(100);
			WPush<ui::EdgeSliceLayoutElement>();
			auto tmpl = ui::EdgeSliceLayoutElement::GetSlotTemplate();
			tmpl->edge = layout == 1 ? ui::Edge::Left : ui::Edge::Right;
			tgt = &ui::MakeWithText<ui::Button>(layout == 1 ? "Left" : "Right button");
			WPop();
			WPop();
		}

		WPop();
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
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);

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
			return tgt->GetFinalRect();
		auto r = GetFinalRect();
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
void Demo_SlidingHighlightAnim()
{
	ui::Make<SlidingHighlightAnimDemo>();
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
		aad->baseRect = button.GetFinalRect();
		aad->player.onAnimUpdate = [this]() { GetNativeWindow()->InvalidateAll(); };

		ui::AnimPtr activationAnim = new ui::ParallelAnimation
			(std::initializer_list<ui::AnimPtr>{
			new ui::SequenceAnimation
				(std::initializer_list<ui::AnimPtr>{
				new ui::AnimSetValue("dist", 0),
					new ui::AnimEaseLinear("dist", 10, 1),
			}),
				new ui::SequenceAnimation
					(std::initializer_list<ui::AnimPtr>{
					new ui::AnimSetValue("alpha", 1),
						new ui::AnimEaseOutCubic("alpha", 0, 1),
				}),
		});
		aad->player.PlayAnim(activationAnim);

		anims.Append(std::move(aad));
	}
	void AddActivationAnim(ui::Button& button)
	{
		button + ui::AddEventHandler(ui::EventType::Activate, [this, &button](ui::Event&) { PlayActivationAnim(button); });
	}

	void Build() override
	{
		ui::Push<ui::PaddingElement>().SetPadding(30);
		ui::Push<ui::StackTopDownLayoutElement>();

		AddActivationAnim(ui::MakeWithText<ui::Button>("Press me"));
		AddActivationAnim(ui::MakeWithText<ui::Button>("...or me"));

		ui::Pop();
		ui::Pop();
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);
		for (const auto& anim : anims)
		{
			float dist = anim->player.GetVariable("dist");
			float alpha = anim->player.GetVariable("alpha", 1);
			ui::draw::RectCutoutCol(anim->baseRect.ExtendBy(ui::AABB2f::UniformBorder(dist)), anim->baseRect, ui::Color4f(1, alpha));
		}
		for (size_t i = 0; i < anims.size(); i++)
		{
			if (anims[i]->player.GetVariable("alpha", 1) <= 0)
				anims.RemoveAt(i--);
		}
	}

	ui::Array<std::unique_ptr<ActivationAnimData>> anims;
};
void Demo_ButtonPressHighlight()
{
	ui::Make<ButtonPressHighlightDemo>();
}


struct FancyButtonDemo : ui::Buildable
{
	struct FancyButtonPainter : ui::IPainter
	{
		ui::ContentPaintAdvice Paint(const ui::PaintInfo& info) override
		{
			// the storage can be extended to support more than one simultaneous animation
			static uint32_t lastClickTime = ui::platform::GetTimeMs() - 1000;
			static ui::Vec2f lastClickPos;
			static ui::AnimationCallbackRequester lastClickAnimReq;
			auto* w = info.obj->system->nativeWindow;
			lastClickAnimReq.callback = [w]() { w->InvalidateAll(); };

			auto cp = info.obj->system->eventSystem.prevMousePos;
			uint32_t timeDiff = ui::platform::GetTimeMs() - lastClickTime;
			bool animating = timeDiff < 1000;

			if (info.IsDown())
			{
				lastClickPos = cp;
				lastClickTime = ui::platform::GetTimeMs();
			}
			else lastClickAnimReq.SetAnimating(animating);

			auto r = info.rect;
			ui::Color4b color(200, 40, 0);
			if (info.IsDown())
				color = ui::Color4b(160, 20, 0);
			else if (info.IsHovered())
				color = ui::Color4b(220, 80, 40);

			float off = info.IsDown() ? 2 : 4;
			float offtop = info.IsDown() ? 2 : 0;
			ui::draw::RectCol(r.x0, r.y0 + offtop, r.x1, r.y1, color);
			ui::draw::RectCol(r.x0, r.y1 - off, r.x1, r.y1, ui::Color4b(40, 5, 0));

			ui::draw::PushScissorRect(r.ShrinkBy({ 0, offtop, 0, off }));

			if (!info.IsDown() && animating)
				ui::draw::AACircleLineCol(lastClickPos, timeDiff * 0.001f * 200, timeDiff * 0.001f * 90, ui::Color4b(255, 100 * ui::invlerp(1000, 0, timeDiff)));

			ui::draw::PopScissorRect();

			ui::ContentPaintAdvice cpa;
			cpa.SetTextColor({ 240, 240, 240 });
			if (info.IsHovered())
				cpa.SetTextColor({ 255, 255, 255 });
			if (info.IsDown())
			{
				cpa.offset.y += 2;
				cpa.SetTextColor({ 180, 130, 120 });
			}
			return cpa;
		}
	};

	void Build() override
	{
		WPush<ui::PaddingElement>().SetPadding(30);
		WPush<ui::StackTopDownLayoutElement>();

		auto& btn = WPush<ui::Button>();
		btn.frameStyle.backgroundPainter = new FancyButtonPainter();
		btn.SetPadding(5, 5, 5, 9);
		WText("Fancy button");
		WPop();

		WPop();
		WPop();
	}
};
void Demo_FancyButton()
{
	ui::Make<FancyButtonDemo>();
}

