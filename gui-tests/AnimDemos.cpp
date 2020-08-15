
#include "pch.h"


struct SlidingHighlightAnimDemo : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ui::imm::RadioButton(ctx, layout, 0, "No button", { ui::Style(ui::Theme::current->button) });
		ui::imm::RadioButton(ctx, layout, 1, "Left", { ui::Style(ui::Theme::current->button) });
		ui::imm::RadioButton(ctx, layout, 2, "Right", { ui::Style(ui::Theme::current->button) });

		tgt = nullptr;
		if (layout != 0)
		{
			ctx->PushBox() + ui::StackingDirection(layout == 1 ? style::StackingDirection::LeftToRight : style::StackingDirection::RightToLeft);
			tgt = ctx->MakeWithText<ui::Button>(layout == 1 ? "Left" : "Right button");
			ctx->Pop();
		}
	}
	void OnLayoutChanged() override
	{
		UIRect tr = GetTargetRect();
		if (memcmp(&targetLayout, &tr, sizeof(tr)) != 0)
		{
			startLayout = GetCurrentRect();
			targetLayout = tr;
			startTime = ui::platform::GetTimeMs();
			endTime = startTime + 1000;
			//puts("START ANIM");
			this->system->eventSystem.SetTimer(this, 1.0f / 60.0f);
		}
	}
	void OnPaint() override
	{
		ui::Node::OnPaint();

		auto r = GetCurrentRect();
		r = r.ShrinkBy(UIRect::UniformBorder(1));

		ui::Color4f colLine(1, 0, 0);
		ui::draw::LineCol(r.x0, r.y0, r.x1, r.y0, 1, colLine);
		ui::draw::LineCol(r.x0, r.y1, r.x1, r.y1, 1, colLine);
		ui::draw::LineCol(r.x0, r.y0, r.x0, r.y1, 1, colLine);
		ui::draw::LineCol(r.x1, r.y0, r.x1, r.y1, 1, colLine);
	}
	void OnEvent(UIEvent& e) override
	{
		ui::Node::OnEvent(e);

		if (e.type == UIEventType::Timer)
		{
			//puts("TIMER");
			if (ui::platform::GetTimeMs() - startTime < endTime - startTime)
			{
				//puts("INVRESET");
				GetNativeWindow()->InvalidateAll();
				this->system->eventSystem.SetTimer(this, 1.0f / 60.0f);
			}
		}
	}

	UIRect GetCurrentRect()
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
			lerp(startLayout.x0, targetLayout.x0, q),
			lerp(startLayout.y0, targetLayout.y0, q),
			lerp(startLayout.x1, targetLayout.x1, q),
			lerp(startLayout.y1, targetLayout.y1, q),
		};
	}
	UIRect GetTargetRect()
	{
		if (tgt)
			return tgt->GetBorderRect();
		auto r = GetBorderRect();
		r.y1 = r.y0;
		return r;
	}

	UIObject* tgt = nullptr;

	int layout = 0;
	UIRect startLayout = {};
	UIRect targetLayout = {};
	uint32_t startTime = 0;
	uint32_t endTime = 0;
};
void Demo_SlidingHighlightAnim(UIContainer* ctx)
{
	ctx->Make<SlidingHighlightAnimDemo>();
}

