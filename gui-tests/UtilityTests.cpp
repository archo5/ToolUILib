
#include "pch.h"


struct BasicEasingAnimTest : ui::Node
{
	BasicEasingAnimTest()
	{
		animPlayer.onAnimUpdate = [this]() { Rerender(); };
		anim = std::make_shared<ui::AnimEaseLinear>("test", 123, 1);
	}
	void Render(UIContainer* ctx) override
	{
		ui::Property::Begin(ctx, "Control");
		if (ui::imm::Button(ctx, "Play"))
		{
			animPlayer.SetVariable("test", 0);
			animPlayer.PlayAnim(anim);
		}
		if (ui::imm::Button(ctx, "Stop"))
		{
			animPlayer.StopAnim(anim);
		}
		ctx->MakeWithText<ui::Panel>(std::to_string(animPlayer.GetVariable("test")));
		ui::Property::End(ctx);
		sliderVal = animPlayer.GetVariable("test");
		ctx->Make<ui::Slider>()->Init(sliderVal, { 0, 123, 0 });
	}

	ui::AnimPlayer animPlayer;
	ui::AnimPtr anim;
	float sliderVal = 0;
};
void Test_BasicEasingAnim(UIContainer* ctx)
{
	ctx->Make<BasicEasingAnimTest>();
}


struct ThreadWorkerTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->MakeWithText<ui::Button>("Do it")->HandleEvent(UIEventType::Activate) = [this](UIEvent&)
		{
			wq.Push([this]()
			{
				for (int i = 0; i <= 100; i++)
				{
					if (wq.HasItems() || wq.IsQuitting())
						return;
					ui::Application::PushEvent(this, [this, i]()
					{
						progress = i / 100.0f;
						Rerender();
					});
#pragma warning (disable:4996)
					_sleep(20);
				}
			}, true);
		};
		auto* pb = ctx->MakeWithText<ui::ProgressBar>(progress < 1 ? "Processing..." : "Done");
		pb->progress = progress;
	}

	float progress = 0;

	WorkerQueue wq;
};
void Test_ThreadWorker(UIContainer* ctx)
{
	ctx->Make<ThreadWorkerTest>();
}


struct ThreadedImageRenderingTest : ui::Node
{
	~ThreadedImageRenderingTest()
	{
		delete image;
	}
	void Render(UIContainer* ctx) override
	{
		Subscribe(ui::DCT_ResizeWindow, GetNativeWindow());

		auto* img = ctx->Make<ui::ImageElement>();
		img->GetStyle().SetWidth(style::Coord::Percent(100));
		img->GetStyle().SetHeight(style::Coord::Percent(100));
		img->SetScaleMode(ui::ScaleMode::Fill);
		img->SetImage(image);

		ui::Application::PushEvent(this, [this, img]()
		{
			auto cr = img->GetContentRect();
			int tw = cr.GetWidth();
			int th = cr.GetHeight();

			if (image && image->GetWidth() == tw && image->GetHeight() == th)
				return;

			wq.Push([this, tw, th]()
			{
				ui::Canvas canvas(tw, th);

				auto* px = canvas.GetPixels();
				for (uint32_t y = 0; y < th; y++)
				{
					if (wq.HasItems() || wq.IsQuitting())
						return;

					for (uint32_t x = 0; x < tw; x++)
					{
						float res = (((x & 1) + (y & 1)) & 1) ? 1.0f : 0.1f;
						float s = sinf(x * 0.02f) * 0.2f + 0.5f;
						double q = 1 - fabsf(y / float(th) - s) / 0.1f;
						if (q < 0)
							q = 0;
						res *= 1 - q;
						res += 0.5f * q;
						uint8_t c = res * 255;
						px[x + y * tw] = 0xff000000 | (c << 16) | (c << 8) | c;
					}
				}

				ui::Application::PushEvent(this, [this, canvas{ std::move(canvas) }]()
				{
					delete image;
					image = new ui::Image(canvas);
					Rerender();
				});
			}, true);
		});
	}

	WorkerQueue wq;
	ui::Image* image = nullptr;
};
void Test_ThreadedImageRendering(UIContainer* ctx)
{
	ctx->Make<ThreadedImageRenderingTest>();
}

