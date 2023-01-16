
#include "pch.h"

#include "../lib-src/Model/Native.h"
#include "../lib-src/Core/SerializationBKVT.h"


struct BasicEasingAnimTest : ui::Buildable
{
	BasicEasingAnimTest()
	{
		animPlayer.onAnimUpdate = [this]() { Rebuild(); };
		anim = new ui::AnimEaseLinear("test", 123, 1);
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::LabeledProperty::Begin("Control");
		if (ui::imm::Button(ui::DefaultIconStyle::Play))
		{
			animPlayer.SetVariable("test", 0);
			animPlayer.PlayAnim(anim);
		}
		if (ui::imm::Button(ui::DefaultIconStyle::Stop))
		{
			animPlayer.StopAnim(anim);
		}
		ui::MakeWithText<ui::FrameElement>(std::to_string(animPlayer.GetVariable("test")))
			.SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		ui::LabeledProperty::End();
		sliderVal = animPlayer.GetVariable("test");
		ui::Make<ui::Slider>().Init(sliderVal, { 0, 123, 0 });

		WPop();
	}

	ui::AnimPlayer animPlayer;
	ui::AnimPtr anim;
	float sliderVal = 0;
};
void Test_BasicEasingAnim()
{
	ui::Make<BasicEasingAnimTest>();
}


struct ThreadWorkerTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::MakeWithText<ui::Button>("Do it").HandleEvent(ui::EventType::Activate) = [this](ui::Event&)
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
						Rebuild();
					});
#pragma warning (disable:4996)
					_sleep(20);
				}
			}, true);
		};
		auto& pb = ui::MakeWithText<ui::ProgressBar>(progress < 1 ? "Processing..." : "Done");
		pb.progress = progress;

		WPop();
	}

	float progress = 0;

	ui::WorkerQueue wq;
};
void Test_ThreadWorker()
{
	ui::Make<ThreadWorkerTest>();
}


struct ThreadedImageRenderingTest : ui::Buildable
{
	void Build() override
	{
		ui::BuildMulticastDelegateAdd(ui::OnWindowResized, [this](ui::NativeWindowBase* w)
		{
			if (w == GetNativeWindow())
				Rebuild();
		});

		auto& img = ui::Make<ui::ImageElement>();
		img.SetLayoutMode(ui::ImageLayoutMode::Fill);
		img.SetScaleMode(ui::ScaleMode::Fill);
		img.SetImage(image);

		ui::Application::PushEvent(this, [this, &img]()
		{
			auto cr = img.GetFinalRect();
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
					image = ui::draw::ImageCreateFromCanvas(canvas);
					Rebuild();
				});
			}, true);
		});
	}

	ui::WorkerQueue wq;
	ui::draw::ImageHandle image;
};
void Test_ThreadedImageRendering()
{
	ui::Make<ThreadedImageRenderingTest>();
}


struct OSCommunicationTest : ui::Buildable
{
	OSCommunicationTest()
	{
		animReq.callback = [this]() { Rebuild(); };
		animReq.BeginAnimation();

		using namespace ui::platform;
		icons[0] = LoadFileIcon("gui-tests/rsrc/iss96.png");
		icons[1] = LoadFileIcon("gui-tests/rsrc");
		icons[2] = LoadFileIcon(".png", FileIconType::GenericFile);
		icons[3] = LoadFileIcon(" ", FileIconType::GenericDir);
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		{
			ui::LabeledProperty::Scope ps("\bClipboard");
			auto tmpl = ui::StackExpandLTRLayoutElement::GetSlotTemplate();
			bool hasText = ui::Clipboard::HasText();
			ui::imm::EditBool(hasText, nullptr, { ui::Enable(false) });
			ui::imm::EditString(clipboardData.c_str(), [this](const char* v) { clipboardData = v; });

			tmpl->SetScaleWeight(0.1f);
			if (ui::imm::Button("Read"))
				clipboardData = ui::Clipboard::GetText();

			tmpl->SetScaleWeight(0.1f);
			if (ui::imm::Button("Write"))
				ui::Clipboard::SetText(clipboardData);
		}

		ui::MakeWithTextf<ui::LabelFrame>("time (ms): %u, double click time (ms): %u",
			unsigned(ui::platform::GetTimeMs()),
			unsigned(ui::platform::GetDoubleClickTime()));

		auto pt = ui::platform::GetCursorScreenPos();
		auto col = ui::platform::GetColorAtScreenPos(pt);
		ui::MakeWithTextf<ui::LabelFrame>("cursor pos:[%d;%d] color:[%d;%d;%d;%d]",
			pt.x, pt.y,
			col.r, col.g, col.b, col.a);
		ui::Make<ui::ColorInspectBlock>().SetColor(col);

		auto* win = ui::NativeWindowBase::FindFromScreenPos(pt);
		ui::MakeWithTextf<ui::LabelFrame>("window under cursor: %p (%s)", win, win ? win->GetTitle().c_str() : "-");

		if (ui::imm::Button("Show error message"))
			ui::platform::ShowErrorMessage("Error", "Message");

		if (ui::imm::Button("Browse to file"))
			ui::platform::BrowseToFile("gui-theme2.tga");

		WPush<ui::StackLTRLayoutElement>();
		{
			for (auto icon : icons)
			{
				if (icon)
				{
					WText("Icon images:");
					for (auto& entry : icon->bitmapImageEntries)
					{
						ui::Make<ui::ImageElement>().SetImage(entry->image);
					}
				}
				else
					WText("Icon failed to load");
			}
		}
		WPop();

		WPop();
	}

	ui::AnimationCallbackRequester animReq;

	std::string clipboardData;

	ui::draw::ImageSetHandle icons[4];
};
void Test_OSCommunication()
{
	ui::Make<OSCommunicationTest>();
}


struct FileSelectionWindowTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Check for change");
		ui::imm::PropText("Current working directory", ui::GetWorkingDirectory().c_str());

		ui::Text("Inputs");
		ui::LabeledProperty::Begin("Filters");
		ui::Push<ui::StackTopDownLayoutElement>();
		{
			auto& se = ui::Make<ui::SequenceEditor>();
			se.SetSequence(Allocate<ui::ArraySequence<decltype(fsw.filters)>>(fsw.filters));
			se.itemUICallback = [this](ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* filter = static_cast<ui::FileSelectionWindow::Filter*>(ptr);
				ui::imm::PropEditString("\bName", filter->name.c_str(), [filter](const char* v) { filter->name = v; });
				ui::imm::PropEditString("\bExts", filter->exts.c_str(), [filter](const char* v) { filter->exts = v; });
			};
			if (ui::imm::Button("Add"))
				fsw.filters.Append({});
		}
		ui::Pop();
		ui::LabeledProperty::End();

		ui::imm::PropEditString("Default extension", fsw.defaultExt.c_str(), [&](const char* s) { fsw.defaultExt = s; });
		ui::imm::PropEditString("Title", fsw.title.c_str(), [&](const char* s) { fsw.title = s; });
		ui::LabeledProperty::Begin("Options");
		ui::imm::EditFlag(fsw.flags, unsigned(ui::FileSelectionWindow::MultiSelect), "Multi-select", {}, ui::imm::ButtonStateToggleSkin());
		ui::imm::EditFlag(fsw.flags, unsigned(ui::FileSelectionWindow::CreatePrompt), "Create prompt", {}, ui::imm::ButtonStateToggleSkin());
		ui::LabeledProperty::End();

		ui::Text("Inputs / outputs");
		ui::imm::PropEditString("Current directory", fsw.currentDir.c_str(), [&](const char* s) { fsw.currentDir = s; });
		ui::LabeledProperty::Begin("Selected files");
		ui::Push<ui::StackTopDownLayoutElement>();
		{
			auto& se = ui::Make<ui::SequenceEditor>();
			se.SetSequence(Allocate<ui::ArraySequence<decltype(fsw.selectedFiles)>>(fsw.selectedFiles));
			se.itemUICallback = [this](ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* file = static_cast<std::string*>(ptr);
				ui::imm::PropEditString("\bFile", file->c_str(), [file](const char* v) { *file = v; });
			};
			if (ui::imm::Button("Add"))
				fsw.selectedFiles.Append({});
		}
		ui::Pop();
		ui::LabeledProperty::End();

		ui::Text("Controls");
		ui::LabeledProperty::Begin("Open file selection window");
		if (ui::imm::Button("Open"))
			Show(false);
		if (ui::imm::Button("Save"))
			Show(true);
		ui::LabeledProperty::End();

		ui::Text("Outputs");
		ui::imm::PropText("Last returned value", lastRet);

		WPop();
	}

	void Show(bool save)
	{
		lastRet = fsw.Show(save) ? "true" : "false";
	}

	ui::FileSelectionWindow fsw;

	const char* lastRet = "-";
};
void Test_FileSelectionWindow()
{
	ui::Make<FileSelectionWindowTest>();
}


struct DirectoryChangeWatcherTest : EventsTest, ui::IDirectoryChangeListener
{
	ui::DirectoryChangeWatcherHandle dcwh;

	void OnFileAdded(ui::StringView path) override
	{
		WriteMsg("OnFileAdded:\"%.*s\"", int(path.size()), path.data());
	}
	void OnFileRemoved(ui::StringView path) override
	{
		WriteMsg("OnFileRemoved:\"%.*s\"", int(path.size()), path.data());
	}
	void OnFileChanged(ui::StringView path) override
	{
		WriteMsg("OnFileChanged:\"%.*s\"", int(path.size()), path.data());
	}
	DirectoryChangeWatcherTest()
	{
		dcwh = ui::CreateDirectoryChangeWatcher("build", this);
	}
};
void Test_DirectoryChangeWatcher()
{
	ui::Make<DirectoryChangeWatcherTest>();
}


namespace ui {
double hqtime();
} // ui
struct SerializationSpeed : ui::Buildable
{
	struct Case1
	{
		struct Data
		{
			int integer;
			unsigned unsignedInteger;
			ui::i64 sixtyFourBitInteger;
			ui::u64 sixtyFourBitUnsignedInteger;
			float singlePrecisionfloatingPointNumber;
			double doublePrecisionfloatingPointNumber;

			void OnSerialize(ui::IObjectIterator& oi, const ui::FieldInfo& FI)
			{
				oi.BeginObject(FI, "Data");
				ui::OnField(oi, "integer", integer);
				ui::OnField(oi, "unsignedInteger", unsignedInteger);
				ui::OnField(oi, "sixtyFourBitInteger", sixtyFourBitInteger);
				ui::OnField(oi, "sixtyFourBitUnsignedInteger", sixtyFourBitUnsignedInteger);
				ui::OnField(oi, "singlePrecisionfloatingPointNumber", singlePrecisionfloatingPointNumber);
				ui::OnField(oi, "doublePrecisionfloatingPointNumber", doublePrecisionfloatingPointNumber);
				oi.EndObject();
			}

			bool operator == (const Data& o) const
			{
				return integer == o.integer
					&& unsignedInteger == o.unsignedInteger
					&& sixtyFourBitInteger == o.sixtyFourBitInteger
					&& sixtyFourBitUnsignedInteger == o.sixtyFourBitUnsignedInteger
					&& singlePrecisionfloatingPointNumber == o.singlePrecisionfloatingPointNumber
					&& doublePrecisionfloatingPointNumber == o.doublePrecisionfloatingPointNumber;
			}
		};

		ui::Array<std::string> strings;
		ui::Array<int> ints;
		ui::Array<Data> datas;

		void OnSerialize(ui::IObjectIterator& oi, const ui::FieldInfo& FI)
		{
			if (FI.NeedObject())
				oi.BeginObject(FI, "Case1");

			ui::OnField(oi, "strings", strings);
			ui::OnField(oi, "ints", ints);
			ui::OnField(oi, "datas", datas);

			if (FI.NeedObject())
				oi.EndObject();
		}

		bool operator == (const Case1& o) const
		{
			return strings == o.strings && ints == o.ints && datas == o.datas;
		}

		void Generate(int n)
		{
			strings.Clear();
			ints.Clear();
			datas.Clear();

			for (int i = 0; i < n; i++)
			{
				int v = int(ui::i64(ui::hqtime()) * 1000) % 8999 + 1000;
				strings.Append(ui::Format("%d", v));
				ints.Append(v);
				Data d = {};
				{
					d.integer = v;
					d.unsignedInteger = v;
					d.sixtyFourBitInteger = v;
					d.sixtyFourBitUnsignedInteger = v;
					d.singlePrecisionfloatingPointNumber = v;
					d.doublePrecisionfloatingPointNumber = v;
				}
				datas.Append(d);
			}
		}
	};

	ui::Array<std::string> results;

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();
		if (ui::imm::Button("Run tests"))
			RunTests();
		for (auto& r : results)
			WText(r);
		WPop();
	}
	void RunTests()
	{
		results.Clear();

		Case1 case1;
		case1.Generate(1000);

		{
			double t0 = ui::hqtime();
			{
				ui::JSONSerializerObjectIterator ser;
				case1.OnSerialize(ser, {});
				ser.GetData();
			}
			double t1 = ui::hqtime();
			results.Append(ui::Format("case 1(1000) | JSON | serialize: %.2f ms", (t1 - t0) * 1000));
		}
		{
			double t0 = ui::hqtime();
			{
				ui::BKVTSerializer ser;
				case1.OnSerialize(ser, {});
				ser.GetData(true);
			}
			double t1 = ui::hqtime();
			results.Append(ui::Format("case 1(1000) | BKVT | serialize: %.2f ms", (t1 - t0) * 1000));
		}

		results.Append("-----");

		{
			ui::JSONSerializerObjectIterator ser;
			case1.OnSerialize(ser, {});
			auto& data = ser.GetData();
			Case1 dest;

			double t0 = ui::hqtime();
			{
				ui::JSONUnserializerObjectIterator uns;
				if (!uns.Parse(data))
					results.Append("JSON parse error");
				dest.OnSerialize(uns, {});
			}
			double t1 = ui::hqtime();
			if (!(case1 == dest))
				results.Append("JSON parse output mismatch");
			results.Append(ui::Format("case 1(1000) | JSON | UNserialize: %.2f ms", (t1 - t0) * 1000));
		}
		{
			ui::BKVTSerializer ser;
			case1.OnSerialize(ser, {});
			auto data = ser.GetData(true);
			Case1 dest;

			double t0 = ui::hqtime();
			{
				ui::BKVTUnserializer uns;
				if (!uns.Init(data, true))
					results.Append("BKVT init error");
				dest.OnSerialize(uns, {});
			}
			double t1 = ui::hqtime();
			if (!(case1 == dest))
				results.Append("BKVT parse output mismatch");
			results.Append(ui::Format("case 1(1000) | BKVT | UNserialize: %.2f ms", (t1 - t0) * 1000));
		}
	}
};
void Test_SerializationSpeed()
{
	ui::Make<SerializationSpeed>();
}

