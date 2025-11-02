
#include "pch.h"

#include "../lib-src/Model/Native.h"
#include "../lib-src/Core/SerializationBKVT.h"
#include "../lib-src/Core/SerializationDATO.h"
#include "../lib-src/Core/TweakableValue.h"
#include "../lib-src/Core/TriangulatorComplex.h"
#include "../lib-src/Render/RHI.h"


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
		if (ui::imButton(ui::DefaultIconStyle::Play))
		{
			animPlayer.SetVariable("test", 0);
			animPlayer.PlayAnim(anim);
		}
		if (ui::imButton(ui::DefaultIconStyle::Stop))
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


struct VExpandAnimTest : ui::Buildable
{
	bool expand = true;
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();
		{
			ui::imEditBool(expand, "Expand", ui::TreeStateToggleSkin());
			WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
			WPush<ui::VExpandContainer>().Init(expand, 0.4f);
			{
				WPush<ui::StackTopDownLayoutElement>();
				{
					ui::imButton("Button 1");
					ui::imButton("Button 2");
				}
				WPop();
			}
			WPop();
			WPop();
			ui::imButton("Button 3");
		}
		WPop();
	}
};
void Test_VExpandAnim()
{
	ui::Make<VExpandAnimTest>();
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


struct FullscreenTest : ui::Buildable
{
	struct MonitorInfo
	{
		std::string name;
		ui::AABB2i rect;
		ui::gfx::MonitorID id;
	};

	ui::Array<MonitorInfo> monitors;

	FullscreenTest()
	{
		ReloadMonitorInfo();
	}

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

#if 0
		{
			if (ui::imButton("1 (reset)"))
				GetNativeWindow()->SetInnerUIScale(1);
			if (ui::imButton("0.75"))
				GetNativeWindow()->SetInnerUIScale(0.75f);
			if (ui::imButton("0.5"))
				GetNativeWindow()->SetInnerUIScale(0.5f);
			if (ui::imButton("0.25"))
				GetNativeWindow()->SetInnerUIScale(0.25f);
			ui::MakeWithTextf<ui::LabelFrame>("- scale: %.2f", GetNativeWindow()->GetInnerUIScale());
		}
#endif

		if (ui::imButton("Refresh screen info"))
		{
			ReloadMonitorInfo();
		}

		{
			ui::MakeWithText<ui::LabelFrame>("Windowed:");

			if (auto monid = ui::gfx::Monitors::FindFromWindow(GetNativeWindow()))
			{
				if (ui::imButton("Current screen"))
				{
					auto rect = ui::gfx::Monitors::GetScreenArea(monid);
					SetWindowed();
				}
			}

			for (auto& m : monitors)
			{
				if (ui::imButton(m.name.c_str()))
				{
					SetWindowed();
					auto* W = GetNativeWindow();
					W->SetInnerPosition(m.rect.GetCenter() - W->GetInnerSize().ToVec2() / 2);
				}
			}
		}

		{
			ui::MakeWithText<ui::LabelFrame>("Borderless:");

			if (auto monid = ui::gfx::Monitors::FindFromWindow(GetNativeWindow()))
			{
				if (ui::imButton("Current screen"))
				{
					auto rect = ui::gfx::Monitors::GetScreenArea(monid);
					SetBorderless(rect);
				}
			}

			for (auto& m : monitors)
			{
				if (ui::imButton(m.name.c_str()))
					SetBorderless(m.rect);
			}
		}

		{
			ui::MakeWithText<ui::LabelFrame>("Exclusive:");

			if (auto monid = ui::gfx::Monitors::FindFromWindow(GetNativeWindow()))
			{
				if (ui::imButton("Current screen"))
				{
					SetExclusive(monid);
				}
			}

			for (auto& m : monitors)
			{
				if (ui::imButton(m.name.c_str()))
					SetExclusive(m.id);
			}
		}

		WPop();
	}

	void ReloadMonitorInfo()
	{
		monitors.Clear();

		for (auto id : ui::gfx::Monitors::All())
		{
			auto rect = ui::gfx::Monitors::GetScreenArea(id);
			auto name = ui::gfx::Monitors::GetName(id);
			bool prim = ui::gfx::Monitors::IsPrimary(id);

			std::string info = ui::Format("\"%s\"%s (%d;%d - %d;%d)",
				name.c_str(),
				prim ? " (primary)" : "",
				rect.x0, rect.y0, rect.x1, rect.y1);
			monitors.Append({ info, rect, id });
		}
	}

	void SetWindowed()
	{
		auto* W = GetNativeWindow();
		W->StopExclusiveFullscreen();
		W->SetState(ui::WindowState::Normal);
		W->SetStyle(ui::WindowStyle::WS_Default);
	}

	void SetBorderless(ui::AABB2i rect)
	{
		auto* W = GetNativeWindow();
		W->StopExclusiveFullscreen();
		W->SetStyle(ui::WindowStyle::WS_None);
		W->SetInnerPosition(rect.GetCenter() - W->GetInnerSize().ToVec2() / 2);
		W->SetState(ui::WindowState::Maximized);
	}

	void SetExclusive(ui::gfx::MonitorID monitor)
	{
		auto* W = GetNativeWindow();
		W->SetStyle(ui::WindowStyle::WS_None);
		//auto rect = ui::Monitors::GetScreenArea(monitor);
		//W->SetInnerPosition(rect.GetCenter() - W->GetInnerSize().ToVec2() / 2);
		W->StartExclusiveFullscreen({ {}, monitor });
		//W->StartExclusiveFullscreen({ { 1920, 1080 }, monitor });
	}
};
void Test_Fullscreen()
{
	WMake<FullscreenTest>();
}


struct OSCommunicationTest : ui::Buildable
{
	ui::Array<std::string> monitors;
	ui::Array<ui::gfx::GraphicsAdapters::Info> gfxAdapters;

	OSCommunicationTest()
	{
		animReq.callback = [this]() { Rebuild(); };
		animReq.BeginAnimation();

		ReloadMonitorInfo();
		ReloadGfxAdapterInfo();

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
			ui::imEnable(false), ui::imEditBool(hasText);
			ui::imEditString(clipboardData);

			tmpl->SetScaleWeight(0.1f);
			if (ui::imButton("Read"))
				clipboardData = ui::Clipboard::GetText();

			tmpl->SetScaleWeight(0.1f);
			if (ui::imButton("Write"))
				ui::Clipboard::SetText(clipboardData);
		}

		WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		WPush<ui::StackTopDownLayoutElement>();
		{
			if (ui::imButton("Reload"))
			{
				ReloadMonitorInfo();
				ReloadGfxAdapterInfo();
			}

			WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::ListBox);
			WPush<ui::StackTopDownLayoutElement>();
			{
				for (auto& m : monitors)
					WMakeWithText<ui::LabelFrame>(m);
			}
			WPop();
			WPop();
			
			WPush<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::ListBox);
			WPush<ui::StackTopDownLayoutElement>();
			{
				for (auto& g : gfxAdapters)
				{
					auto text = g.name;
					text += " monitors:(";
					for (auto mon : g.monitors)
					{
						text += mon.name;
						text += ",";
					}
					if (text.back() == ',')
						text.pop_back();
					text.push_back(')');
					WMakeWithText<ui::LabelFrame>(text);
				}
			}
			WPop();
			WPop();
		}
		WPop();
		WPop();

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

		if (ui::imButton("Show error message"))
			ui::platform::ShowErrorMessage("Error", "Message");

		if (ui::imButton("Browse to file"))
			ui::platform::BrowseToFile("gui-theme2.tga");

		if (ui::imButton("Open URL"))
			ui::platform::OpenURL("https://example.com/");

		WPush<ui::StackLTRLayoutElement>();
		{
			if (ui::imButton("Limit cursor to this window"))
				GetNativeWindow()->ClipCursorToThisWindow();
			if (ui::imButton("Remove cursor limit"))
				ui::NativeWindowBase::SetCursorClipWindow(nullptr);
			if (ui::imButton("Minimize"))
				GetNativeWindow()->SetState(ui::WindowState::Minimized);
			if (ui::imButton("Maximize"))
				GetNativeWindow()->SetState(ui::WindowState::Maximized);
			if (ui::imButton("Restore"))
				GetNativeWindow()->SetState(ui::WindowState::Normal);
		}
		WPop();

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

	void ReloadMonitorInfo()
	{
		monitors.Clear();

		for (auto id : ui::gfx::Monitors::All())
		{
			auto rect = ui::gfx::Monitors::GetScreenArea(id);
			auto name = ui::gfx::Monitors::GetName(id);
			bool prim = ui::gfx::Monitors::IsPrimary(id);

			std::string info = ui::Format("Monitor %p%s: \"%s\" (%d;%d - %d;%d)",
				(void*)id,
				prim ? " (primary)" : "",
				name.c_str(),
				rect.x0, rect.y0, rect.x1, rect.y1);
			monitors.Append(info);

			puts(info.c_str());
			auto res = ui::gfx::Monitors::GetAvailableDisplayModes(id);
			for (auto r : res)
			{
				printf("- resolution: %dx%d (%d Hz)\n", int(r.width), int(r.height), int(r.refreshRate.num));
			}
		}
	}

	void ReloadGfxAdapterInfo()
	{
		gfxAdapters = ui::gfx::GraphicsAdapters::All();
		for (auto& A : gfxAdapters)
		{
			printf("Adapter: \"%s\"\n", A.name.c_str());
			for (auto& M : A.monitors)
			{
				printf("  Monitor: \"%s\" (%p) [%zu modes]\n", M.name.c_str(), (void*)M.id, M.displayModes.Size());
				for (auto& DM : M.displayModes)
				{
					printf("    Display mode: %dx%d, %g Hz\n", int(DM.width), int(DM.height), double(DM.refreshRate.num) / DM.refreshRate.denom);
				}
			}
		}
	}

	ui::AnimationCallbackRequester animReq;

	std::string clipboardData;

	ui::draw::ImageSetHandle icons[4];
};
void Test_OSCommunication()
{
	ui::Make<OSCommunicationTest>();
}


struct SysDirPathsTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();
#define SDPE(name) WText(#name ": " + ui::GetSystemDirPath(ui::SystemDirectoryType::name))
		SDPE(Documents);
		SDPE(Desktop);
		SDPE(Downloads);
		SDPE(Pictures);
		SDPE(Videos);
		SDPE(UserDataRoot);

		SDPE(Windows_LocalAppData);
		SDPE(Windows_LocalLowAppData);
		SDPE(Windows_RoamingAppData);
#undef SDPE
		WPop();
	}
};
void Test_SysDirPaths()
{
	ui::Make<SysDirPathsTest>();
}


struct FileSelectionWindowTest : ui::Buildable
{
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		ui::Text("Check for change");
		ui::imLabel("Current working directory"), ui::StdText(ui::GetWorkingDirectory());

		ui::Text("Inputs");
		ui::LabeledProperty::Begin("Filters");
		ui::Push<ui::StackTopDownLayoutElement>();
		{
			auto& se = ui::Make<ui::SequenceEditor>();
			se.SetSequence(UI_BUILD_ALLOC(ui::ArraySequence<decltype(fsw.filters)>)(fsw.filters));
			se.itemUICallback = [this](ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* filter = static_cast<ui::FileSelectionWindow::Filter*>(ptr);
				ui::imLabel("\bName"), ui::imEditString(filter->name);
				ui::imLabel("\bExts"), ui::imEditString(filter->exts);
			};
			if (ui::imButton("Add"))
				fsw.filters.Append({});
		}
		ui::Pop();
		ui::LabeledProperty::End();

		ui::imLabel("Default extension"), ui::imEditString(fsw.defaultExt);
		ui::imLabel("Title"), ui::imEditString(fsw.title);
		ui::LabeledProperty::Begin("Options");
		ui::imEditFlag(fsw.flags, unsigned(ui::FileSelectionWindow::MultiSelect), "Multi-select", ui::ButtonStateToggleSkin());
		ui::imEditFlag(fsw.flags, unsigned(ui::FileSelectionWindow::CreatePrompt), "Create prompt", ui::ButtonStateToggleSkin());
		ui::LabeledProperty::End();

		ui::Text("Inputs / outputs");
		ui::imLabel("Current directory"), ui::imEditString(fsw.currentDir);
		ui::LabeledProperty::Begin("Selected files");
		ui::Push<ui::StackTopDownLayoutElement>();
		{
			auto& se = ui::Make<ui::SequenceEditor>();
			se.SetSequence(UI_BUILD_ALLOC(ui::ArraySequence<decltype(fsw.selectedFiles)>)(fsw.selectedFiles));
			se.itemUICallback = [this](ui::SequenceEditor* se, size_t idx, void* ptr)
			{
				auto* file = static_cast<std::string*>(ptr);
				ui::imLabel("\bFile"), ui::imEditString(*file);
			};
			if (ui::imButton("Add"))
				fsw.selectedFiles.Append({});
		}
		ui::Pop();
		ui::LabeledProperty::End();

		ui::Text("Controls");
		ui::LabeledProperty::Begin("Open file selection window");
		if (ui::imButton("Open"))
			Show(false);
		if (ui::imButton("Save"))
			Show(true);
		ui::LabeledProperty::End();

		ui::Text("Outputs");
		ui::imLabel("Last returned value"), ui::StdText(lastRet);

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
			ui::Vec2f vec2f;
			ui::AABB2i aabb2i;

			void OnSerialize(ui::IObjectIterator& oi, const ui::FieldInfo& FI)
			{
				oi.BeginObject(FI, "Data");
				ui::OnField(oi, "integer", integer);
				ui::OnField(oi, "unsignedInteger", unsignedInteger);
				ui::OnField(oi, "sixtyFourBitInteger", sixtyFourBitInteger);
				ui::OnField(oi, "sixtyFourBitUnsignedInteger", sixtyFourBitUnsignedInteger);
				ui::OnField(oi, "singlePrecisionfloatingPointNumber", singlePrecisionfloatingPointNumber);
				ui::OnField(oi, "doublePrecisionfloatingPointNumber", doublePrecisionfloatingPointNumber);
				ui::OnField(oi, "vec2f", vec2f);
				ui::OnField(oi, "aabb2i", aabb2i);
				oi.EndObject();
			}

			bool operator == (const Data& o) const
			{
				return integer == o.integer
					&& unsignedInteger == o.unsignedInteger
					&& sixtyFourBitInteger == o.sixtyFourBitInteger
					&& sixtyFourBitUnsignedInteger == o.sixtyFourBitUnsignedInteger
					&& singlePrecisionfloatingPointNumber == o.singlePrecisionfloatingPointNumber
					&& doublePrecisionfloatingPointNumber == o.doublePrecisionfloatingPointNumber
					&& vec2f == o.vec2f
					&& aabb2i == o.aabb2i;
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
					d.vec2f = ui::Vec2f(v, v);
					d.aabb2i = ui::AABB2i(v, v, v, v);
				}
				datas.Append(d);
			}
		}
	};

	ui::Array<std::string> results;

	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();
		if (ui::imButton("Run tests"))
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
			size_t size = 0;
			double t0 = ui::hqtime();
			{
				ui::JSONSerializerObjectIterator ser;
				ser.indent = nullptr;
				case1.OnSerialize(ser, {});
				size = ser.GetData().size();
			}
			double t1 = ui::hqtime();
			results.Append(ui::Format("case 1(1000) | JSON | serialize: %.2f ms (%zu bytes)", (t1 - t0) * 1000, size));
		}
		{
			size_t size = 0;
			double t0 = ui::hqtime();
			{
				ui::BKVTSerializer ser;
				case1.OnSerialize(ser, {});
				size = ser.GetData(true).Size();
			}
			double t1 = ui::hqtime();
			results.Append(ui::Format("case 1(1000) | BKVT | serialize: %.2f ms (%zu bytes)", (t1 - t0) * 1000, size));
		}
		{
			size_t size = 0;
			double t0 = ui::hqtime();
			{
				ui::DATOSerializer ser;
				case1.OnSerialize(ser, {});
				size = ser.GetData().Size();
			}
			double t1 = ui::hqtime();
			results.Append(ui::Format("case 1(1000) | DATO | serialize: %.2f ms (%zu bytes)", (t1 - t0) * 1000, size));
		}

		results.Append("-----");

		{
			ui::JSONSerializerObjectIterator ser;
			ser.indent = nullptr;
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
			//for (int i = 0; i < 1000; i++)
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
		{
			ui::DATOSerializer ser("DATO");
			case1.OnSerialize(ser, {});
			auto data = ser.GetData();
			Case1 dest;

			double t0 = ui::hqtime();
			//for (int i = 0; i < 1000; i++)
			{
				ui::DATOUnserializer uns;
				if (!uns.Init(data))
					results.Append("DATO init error");
				dest.OnSerialize(uns, {});
			}
			double t1 = ui::hqtime();
			if (!(case1 == dest))
				results.Append("DATO parse output mismatch");
			results.Append(ui::Format("case 1(1000) | DATO | UNserialize: %.2f ms", (t1 - t0) * 1000));
		}
	}
};
void Test_SerializationSpeed()
{
	ui::Make<SerializationSpeed>();
}


struct Settings
{
	static constexpr const char* Type = "test_settings_group";

	bool val1 = true;
	int val2 = 3;
	std::string val3 = "test";

	void Serialize(ui::IObjectIterator& oi)
	{
		ui::OnField(oi, "val1", val1);
		ui::OnField(oi, "val2", val2);
		ui::OnField(oi, "val3", val3);
	}
};
UI_CFG_TWEAKABLE_SET_DECLARE(TestTweakableSet);
ui::Tweakable<Settings> g_twkSettings(TestTweakableSet());
UI_CFG_TWEAKABLE_SET_DEFINE(TestTweakableSet);
struct ConfigTweakableTest : ui::Buildable
{
	ui::ConfigDB_JSONFile cfgdb = { TestTweakableSet() };

	void OnEnable() override
	{
		cfgdb.filePath = "tweakable_test.json";
		cfgdb.Load();
	}
	void OnDisable() override
	{
		cfgdb.Save();
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();

		bool edit = false;
		ui::imLabel("Value 1"),
			edit |= ui::imEditBool(g_twkSettings.val1);
		ui::imLabel("Value 2"),
			edit |= ui::imEditInt(g_twkSettings.val2);
		ui::imLabel("Value 3"),
			edit |= ui::imEditString(g_twkSettings.val3);
		if (edit)
			g_twkSettings.SetDirty();

		WPop();
	}
};
void Test_ConfigTweakable()
{
	ui::Make<ConfigTweakableTest>();
}


struct TweakableValuesTest : ui::Buildable
{
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		float scale = UI_TWEAKABLE_VALUE(10);
		ui::Color4b col(UI_TWEAKABLE_VALUE(200, "r"), UI_TWEAKABLE_VALUE(100, "g"), UI_TWEAKABLE_VALUE(35, "b"));
		ui::draw::RectCol({ scale, scale, scale * 2, scale * 2 }, col);
	}
	void OnEnable() override
	{
		ui::TweakableValueSetMinReloadInterval(1000);
	}
	void OnDisable() override
	{
		ui::TweakableValueSetMinReloadInterval(-1);
	}
	void Build() override
	{
	}
};
void Test_TweakableValues()
{
	ui::Make<TweakableValuesTest>();
}


struct TriangulatorComplexTest : ui::Buildable
{
	struct Tri
	{
		ui::Vec2f pos[3];

		void Shrink(float amt)
		{
			using namespace ui;
			Vec2f pa = pos[0];
			Vec2f pb = pos[1];
			Vec2f pc = pos[2];
			Vec2f da = (pb - pa).Normalized();
			Vec2f db = (pc - pb).Normalized();
			Vec2f dc = (pa - pc).Normalized();
			Vec2f ta = da.Perp();
			Vec2f tb = db.Perp();
			Vec2f tc = dc.Perp();
			Vec2f ha = (ta + tc).Normalized();
			Vec2f hb = (tb + ta).Normalized();
			Vec2f hc = (tc + tb).Normalized();
			float sa = amt / Vec2Dot(ta, ha);
			float sb = amt / Vec2Dot(tb, hb);
			float sc = amt / Vec2Dot(tc, hc);
			pos[0] += ha * sa;
			pos[1] += hb * sb;
			pos[2] += hc * sc;
		}
	};

	bool shrinktris = true;
	ui::Vec2f userpos = { 500, 350 };
	ui::Vec2f useroff = {};
	ui::TriangulatorComplex* TC = ui::TriangulatorComplex_Create();
	ui::Array<Tri> triangles;
	ui::HashSet<ui::u32> usedverts;

	void DumpResult()
	{
		using namespace ui;

		TriangulatorComplex_Triangulate(TC);

		triangles.Clear();
		usedverts.Clear();
		u32 tricount = TriangulatorComplex_GetTriangleCount(TC);
		for (u32 t = 0; t < tricount; t++)
		{
			u32 verts[3] = {};
			bool ret = TriangulatorComplex_GetTriangle(TC, t, verts); assert(ret);
			usedverts.Insert(verts[0]);
			usedverts.Insert(verts[1]);
			usedverts.Insert(verts[2]);
			u32 dummy;
			Vec2f v0, v1, v2;
			ret = TriangulatorComplex_GetVertexInfo(TC, verts[0], v0, dummy); assert(ret);
			ret = TriangulatorComplex_GetVertexInfo(TC, verts[1], v1, dummy); assert(ret);
			ret = TriangulatorComplex_GetVertexInfo(TC, verts[2], v2, dummy); assert(ret);
			triangles.Append({ v0, v1, v2 });
		}
	}

	struct PolyAndHoles
	{
		ui::Array<ui::Vec2f> mainpoly;
		ui::Array<ui::Array<ui::Vec2f>> holepolys;
		ui::Array<ui::Array<ui::Vec2f>> cutpolys;
		ui::Array<ui::Vec2f> cutpoints;

		bool IsInside(ui::Vec2f point) const
		{
			if (!ui::TriangulatorComplexUtil_PointInOrNearPolygon(point, mainpoly, 0.0001f))
				return false;

			for (auto& hp : holepolys)
			{
				if (ui::TriangulatorComplexUtil_PointInButNotNearPolygon(point, hp, 0.0001f))
					return false;
			}

			return true;
		}
		static bool IsInsideFunc(void* userdata, ui::Vec2f point)
		{
			return static_cast<PolyAndHoles*>(userdata)->IsInside(point);
		}
	};

	void InitPNH(PolyAndHoles& pnh)
	{
		for (auto& hp : pnh.holepolys)
			for (auto& hpv : hp)
				hpv += useroff;
		using namespace ui;
		TriangulatorComplex_Reset(TC);
		TriangulatorComplex_SetInsideFunc(TC, &pnh, &PolyAndHoles::IsInsideFunc);
		TriangulatorComplex_AddPolygon(TC, pnh.mainpoly);
		for (auto& hp : pnh.holepolys)
			TriangulatorComplex_AddPolygon(TC, hp);
		for (auto& cp : pnh.cutpolys)
			TriangulatorComplex_AddPolygon(TC, cp);
		TriangulatorComplex_AddPoints(TC, pnh.cutpoints);
	}

	void DoTest_BasicRect()
	{
		using namespace ui;
		TriangulatorComplex_Reset(TC);
		Array<Vec2f> verts = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		TriangulatorComplex_SetInsideFunc(TC, &verts, [](void* userdata, Vec2f point) -> bool
		{
			return TriangulatorComplexUtil_PointInOrNearPolygon(point, *(Array<Vec2f>*)userdata, 0.0001f);
		});
		TriangulatorComplex_AddPolygon(TC, verts);

		DumpResult();
	}

	void DoTest_RectDummyPP()
	{
		using namespace ui;
		TriangulatorComplex_Reset(TC);
		Array<Vec2f> verts = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		TriangulatorComplex_SetInsideFunc(TC, &verts, [](void* userdata, Vec2f point) -> bool
		{
			return TriangulatorComplexUtil_PointInOrNearPolygon(point, *(Array<Vec2f>*)userdata, 0.0001f);
		});
		TriangulatorComplex_AddPolygon(TC, verts);
		TriangulatorComplex_AddPolygon(TC, { Vec2f(300, 300), Vec2f(400, 300), Vec2f(350, 350) });
		TriangulatorComplex_AddPoints(TC, { Vec2f(350, 320), Vec2f(350, 250) });

		DumpResult();
	}

	void DoTest_RectEdgePoints()
	{
		using namespace ui;
		TriangulatorComplex_Reset(TC);
		Array<Vec2f> verts = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		TriangulatorComplex_SetInsideFunc(TC, &verts, [](void* userdata, Vec2f point) -> bool
		{
			return TriangulatorComplexUtil_PointInOrNearPolygon(point, *(Array<Vec2f>*)userdata, 0.0001f);
		});
		TriangulatorComplex_AddPolygon(TC, verts);
		TriangulatorComplex_AddPoints(TC, { Vec2f(200, 350), Vec2f(350, 400) });

		DumpResult();
	}

	void DoTest_RectPointExplore()
	{
		using namespace ui;
		TriangulatorComplex_Reset(TC);
		Array<Vec2f> verts = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		TriangulatorComplex_SetInsideFunc(TC, &verts, [](void* userdata, Vec2f point) -> bool
		{
			return TriangulatorComplexUtil_PointInOrNearPolygon(point, *(Array<Vec2f>*)userdata, 0.0001f);
		});
		TriangulatorComplex_AddPolygon(TC, verts);
		TriangulatorComplex_AddPoints(TC, { Vec2f(500, 250), userpos });

		DumpResult();
	}

	void DoTest_RectAndHoleRect()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(300, 250), Vec2f(400, 250), Vec2f(400, 350), Vec2f(300, 350) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectAndDupHoleRect()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(300, 250), Vec2f(400, 250), Vec2f(400, 350), Vec2f(300, 350) });
		pnh.holepolys.Append({ Vec2f(300, 250), Vec2f(400, 250), Vec2f(400, 350), Vec2f(300, 350) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectAndHoleRectPeekingOut()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(300, 250), Vec2f(400, 250), Vec2f(400, 500), Vec2f(300, 500) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectAndHoleRectEdgeOverlap()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(300, 250), Vec2f(400, 250), Vec2f(400, 400), Vec2f(300, 400) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectAndHoleRectEdgeOverlap2X()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(250, 250), Vec2f(300, 250), Vec2f(300, 400), Vec2f(250, 400) });
		pnh.holepolys.Append({ Vec2f(400, 250), Vec2f(450, 250), Vec2f(450, 400), Vec2f(400, 400) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectAndCutterRectEdgeOvrUP()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.cutpolys.Append({ Vec2f(350, 150), Vec2f(550, 300), Vec2f(350, 450), Vec2f(150, 300) });
		pnh.cutpoints.Append(userpos);

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectAndOverlappingHoleRects()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(300, 250), Vec2f(400, 250), Vec2f(400, 350), Vec2f(300, 350) });
		pnh.holepolys.Append({ Vec2f(350, 300), Vec2f(450, 300), Vec2f(450, 380), Vec2f(350, 380) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectBiggerHoleCover()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(100, 100), Vec2f(600, 100), Vec2f(600, 500), Vec2f(100, 500) });

		InitPNH(pnh);
		DumpResult();
	}

	void DoTest_RectExactHoleCover()
	{
		using namespace ui;
		PolyAndHoles pnh;
		pnh.mainpoly = { Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) };
		pnh.holepolys.Append({ Vec2f(200, 200), Vec2f(500, 200), Vec2f(500, 400), Vec2f(200, 400) });

		InitPNH(pnh);
		DumpResult();
	}

	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		ui::Buildable::OnPaint(ctx);

		for (auto t : triangles)
		{
			if (shrinktris)
			{
				t.Shrink(1);
			}
			ui::gfx::Vertex verts[3] = {};
			verts[0].x = t.pos[0].x;
			verts[0].y = t.pos[0].y;
			verts[0].col = { 255, 0, 0, 127 };
			verts[1].x = t.pos[1].x;
			verts[1].y = t.pos[1].y;
			verts[1].col = { 0, 255, 0, 127 };
			verts[2].x = t.pos[2].x;
			verts[2].y = t.pos[2].y;
			verts[2].col = { 0, 0, 255, 127 };
			ui::gfx::SetTexture(nullptr);
			ui::gfx::DrawTriangles(verts, 3);
			ui::gfx::RestoreRenderStates();
		}

		auto* font = ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF);
		int tsize = 7;
		for (ui::u32 v : usedverts)
		{
			ui::Vec2f pos;
			ui::u32 node = 0;
			if (ui::TriangulatorComplex_GetVertexInfo(TC, v, pos, node))
			{
				while (node != ui::TriangulatorComplex_NodeID_NULL)
				{
					ui::TriangulatorComplex_VertexInfoNode vinfo = {};
					if (ui::TriangulatorComplex_GetVertexInfoNode(TC, node, vinfo))
					{
						auto text = ui::Format("p=%d e=%d q=%g", int(vinfo.polyID), int(vinfo.edgeID), vinfo.edgeQ);
						ui::draw::TextLine(font, tsize, pos.x, pos.y, text, {}, ui::TextBaseline::Top);
						pos.y += tsize;
						node = vinfo.nextnode;
					}
				}
			}
		}
	}
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();
		WPush<ui::StackExpandLTRLayoutElement>();
		ui::imEditBool(shrinktris, "Shrink tris");
		ui::StdText("User pos.:");
		ui::imEditVec2f(userpos, { 0.01f });
		ui::StdText("User off.:");
		ui::imEditVec2f(useroff, { 1e-6f });
		WPop();
		WPush<ui::SizeConstraintElement>().SetMaxWidth(200);
		WPush<ui::StackTopDownLayoutElement>();
		{
			if (ui::imButton("Basic rect")) DoTest_BasicRect();
			if (ui::imButton("Rect + dummy polygon and points")) DoTest_RectDummyPP();
			if (ui::imButton("Rect + dummy points on edge")) DoTest_RectEdgePoints();
			if (ui::imButton("Rect + dummy user point")) DoTest_RectPointExplore();
			if (ui::imButton("Rect + hole rect")) DoTest_RectAndHoleRect();
			if (ui::imButton("Rect + duplicated hole rect")) DoTest_RectAndDupHoleRect();
			if (ui::imButton("Rect + hole rect peeking out")) DoTest_RectAndHoleRectPeekingOut();
			if (ui::imButton("Rect + hole rect edge overlap")) DoTest_RectAndHoleRectEdgeOverlap();
			if (ui::imButton("Rect + hole rect (2x) edge overlap")) DoTest_RectAndHoleRectEdgeOverlap2X();
			if (ui::imButton("Rect + cutter rect edge ovr + UP")) DoTest_RectAndCutterRectEdgeOvrUP();
			if (ui::imButton("Rect + overlapping hole rects")) DoTest_RectAndOverlappingHoleRects();
			if (ui::imButton("Rect + bigger hole cover")) DoTest_RectBiggerHoleCover();
			if (ui::imButton("Rect + exact hole cover")) DoTest_RectExactHoleCover();
		}
		WPop();
		WPop();
		WPop();
	}
};
void Test_TriangulatorComplex()
{
	ui::Make<TriangulatorComplexTest>();
}
