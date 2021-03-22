
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


struct OSCommunicationTest : ui::Node
{
	OSCommunicationTest()
	{
		animReq.callback = [this]() { Rerender(); };
		animReq.BeginAnimation();
	}
	void Render(UIContainer* ctx) override
	{
		{
			ui::Property::Scope ps(ctx, "\bClipboard");
			bool hasText = ui::Clipboard::HasText();
			ui::imm::EditBool(ctx, hasText, nullptr, { ui::Enable(false) });
			ui::imm::EditString(ctx, clipboardData.c_str(), [this](const char* v) { clipboardData = v; });
			if (ui::imm::Button(ctx, "Read", { ui::Width(style::Coord::Fraction(0.1f)) }))
				clipboardData = ui::Clipboard::GetText();
			if (ui::imm::Button(ctx, "Write", { ui::Width(style::Coord::Fraction(0.1f)) }))
				ui::Clipboard::SetText(clipboardData);
		}

		ctx->Textf("time (ms): %u, double click time (ms): %u",
			unsigned(ui::platform::GetTimeMs()),
			unsigned(ui::platform::GetDoubleClickTime())) + ui::Padding(5);

		auto pt = ui::platform::GetCursorScreenPos();
		auto col = ui::platform::GetColorAtScreenPos(pt);
		ctx->Textf("cursor pos:[%d;%d] color:[%d;%d;%d;%d]",
			pt.x, pt.y,
			col.r, col.g, col.b, col.a) + ui::Padding(5);
		ctx->Make<ui::ColorInspectBlock>()->SetColor(col);

		if (ui::imm::Button(ctx, "Show error message"))
			ui::platform::ShowErrorMessage("Error", "Message");
	}

	ui::AnimationCallbackRequester animReq;

	std::string clipboardData;
};
void Test_OSCommunication(UIContainer* ctx)
{
	ctx->Make<OSCommunicationTest>();
}


struct FileSelectionWindowTest : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		ctx->Text("Check for change");
		ui::imm::PropText(ctx, "Current working directory", ui::GetWorkingDirectory().c_str());

		ctx->Text("Inputs");
		ui::Property::Begin(ctx, "Filters");
		ctx->PushBox();
		{
			auto* se = ctx->Make<ui::SequenceEditor>();
			se->SetSequence(Allocate<ui::StdSequence<decltype(fsw.filters)>>(fsw.filters));
			se->itemUICallback = [this](UIContainer* ctx, ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* filter = static_cast<ui::FileSelectionWindow::Filter*>(ptr);
				ui::imm::PropEditString(ctx, "\bName", filter->name.c_str(), [filter](const char* v) { filter->name = v; });
				ui::imm::PropEditString(ctx, "\bExts", filter->exts.c_str(), [filter](const char* v) { filter->exts = v; });
			};
			if (ui::imm::Button(ctx, "Add"))
				fsw.filters.push_back({});
		}
		ctx->Pop();
		ui::Property::End(ctx);

		ui::imm::PropEditString(ctx, "Default extension", fsw.defaultExt.c_str(), [&](const char* s) { fsw.defaultExt = s; });
		ui::imm::PropEditString(ctx, "Title", fsw.title.c_str(), [&](const char* s) { fsw.title = s; });
		ui::Property::Begin(ctx, "Options");
		ui::imm::EditFlag(ctx, fsw.flags, unsigned(ui::FileSelectionWindow::MultiSelect), "Multi-select", {}, ui::imm::ButtonStateToggleSkin());
		ui::imm::EditFlag(ctx, fsw.flags, unsigned(ui::FileSelectionWindow::CreatePrompt), "Create prompt", {}, ui::imm::ButtonStateToggleSkin());
		ui::Property::End(ctx);

		ctx->Text("Inputs / outputs");
		ui::imm::PropEditString(ctx, "Current directory", fsw.currentDir.c_str(), [&](const char* s) { fsw.currentDir = s; });
		ui::Property::Begin(ctx, "Selected files");
		ctx->PushBox();
		{
			auto* se = ctx->Make<ui::SequenceEditor>();
			se->SetSequence(Allocate<ui::StdSequence<decltype(fsw.selectedFiles)>>(fsw.selectedFiles));
			se->itemUICallback = [this](UIContainer* ctx, ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* file = static_cast<std::string*>(ptr);
				ui::imm::PropEditString(ctx, "\bFile", file->c_str(), [file](const char* v) { *file = v; });
			};
			if (ui::imm::Button(ctx, "Add"))
				fsw.selectedFiles.push_back({});
		}
		ctx->Pop();
		ui::Property::End(ctx);

		ctx->Text("Controls");
		ui::Property::Begin(ctx, "Open file selection window");
		if (ui::imm::Button(ctx, "Open"))
			Show(false);
		if (ui::imm::Button(ctx, "Save"))
			Show(true);
		ui::Property::End(ctx);

		ctx->Text("Outputs");
		ui::imm::PropText(ctx, "Last returned value", lastRet);
	}

	void Show(bool save)
	{
		lastRet = fsw.Show(save) ? "true" : "false";
	}

	ui::FileSelectionWindow fsw;

	const char* lastRet = "-";
};
void Test_FileSelectionWindow(UIContainer* ctx)
{
	ctx->Make<FileSelectionWindowTest>();
}

