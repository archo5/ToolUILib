
#include "TECore.h"
#include "TENodes.h"
#include "TETheme.h"


enum TE_PreviewMode
{
	TEPM_Original,
	TEPM_Sliced,
};
static TE_PreviewMode g_previewMode = TEPM_Original;
static ScaleMode g_previewScaleMode = ScaleMode::None;
static unsigned g_previewZoomPercent = 100;
static unsigned g_previewSlicedWidth = 128;
static unsigned g_previewSlicedHeight = 32;

struct TE_SlicedImageElement : ui::FillerElement
{
	draw::ImageHandle _image;
	int _width = 0;
	int _height = 0;
	int _left = 0;
	int _top = 0;
	int _right = 0;
	int _bottom = 0;

	void OnReset() override
	{
		FillerElement::OnReset();

		_image = {};
		_width = 0;
		_height = 0;
		_left = 0;
		_top = 0;
		_right = 0;
		_bottom = 0;
	}
	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		UIRect r = GetFinalRect();
		float iw = _image->GetWidth();
		float ih = _image->GetHeight();
		float x0 = roundf((r.x0 + r.x1 - _width) * 0.5f);
		float y0 = roundf((r.y0 + r.y1 - _height) * 0.5f);
		UIRect outer = { x0, y0, x0 + _width, y0 + _height };
		UIRect inner = outer.ShrinkBy({ float(_left), float(_top), float(_right), float(_bottom) });
		UIRect texinner = { invlerp(0, iw, _left), invlerp(0, ih, _top), invlerp(iw, 0, _right), invlerp(ih, 0, _bottom) };
		draw::RectColTex9Slice(outer, inner, Color4b::White(), _image, { 0, 0, 1, 1 }, texinner);
	}
	TE_SlicedImageElement& SetImage(draw::IImage* img)
	{
		_image = img;
		return *this;
	}
	TE_SlicedImageElement& SetTargetSize(int w, int h)
	{
		_width = w;
		_height = h;
		return *this;
	}
	TE_SlicedImageElement& SetBorderSizes(int l, int t, int r, int b)
	{
		_left = l;
		_top = t;
		_right = r;
		_bottom = b;
		return *this;
	}
};

struct TE_MainPreviewNode : Buildable
{
	TE_Template* tmpl;

	void Build() override
	{
		auto rebuildFn = [this]() { Rebuild(); };
		BuildMulticastDelegateAddNoArgs(OnNodePreviewInvalidated, rebuildFn);
		BuildMulticastDelegateAddNoArgs(OnProcGraphEdit, rebuildFn);
		BuildMulticastDelegateAddNoArgs(OnProcGraphNodeEdit, rebuildFn);

		Push<EdgeSliceLayoutElement>();

		Push<StackExpandLTRLayoutElement>();
		{
			MakeWithText<LabelFrame>("Preview");
			if (tmpl->curPreviewImage && imButton("Reset to template"))
			{
				tmpl->SetCurPreviewImage(nullptr);
			}
		}
		Pop();

		if (tmpl->renderSettings.layer)
		{
			{
				Push<StackExpandLTRLayoutElement>();
				{
					imRadioButton(g_previewMode, TEPM_Original, "Original", {}, ButtonStateToggleSkin());
					imRadioButton(g_previewMode, TEPM_Sliced, "Sliced", {}, ButtonStateToggleSkin());
				}
				Pop();
				if (g_previewMode == TEPM_Original)
				{
					Push<StackExpandLTRLayoutElement>();
					{
						imRadioButton(g_previewScaleMode, ScaleMode::None, "No scaling", {}, ButtonStateToggleSkin());
						imRadioButton(g_previewScaleMode, ScaleMode::Fit, "Fit", {}, ButtonStateToggleSkin());
					}
					Pop();
				}
				if (g_previewMode == TEPM_Sliced)
				{
					Push<StackExpandLTRLayoutElement>();
					{
						imm::PropEditInt("\bWidth", g_previewSlicedWidth, { SetMinWidth(20) });
						imm::PropEditInt("\bHeight", g_previewSlicedHeight, { SetMinWidth(20) });
					}
					Pop();
				}

				Push<ListBoxFrame>();

				Canvas canvas;
#if 0
				auto rc = imgNode->MakeRenderContext();
				if (auto* prevImg = theme->curPreviewImage)
					rc.overrides = prevImg->overrides[theme->curVariation].get();
#endif
				auto& rs = tmpl->renderSettings;
				rs.layer->Render(canvas, tmpl->GetRenderContext());
				auto img = draw::ImageCreateFromCanvas(canvas, draw::TexFlags::NoFilter);
				if (g_previewMode == TEPM_Original)
				{
					Make<ImageElement>()
						.SetImage(img)
						.SetScaleMode(g_previewScaleMode)
						.SetLayoutMode(ImageLayoutMode::Fill);
				}
				if (g_previewMode == TEPM_Sliced)
				{
					Make<TE_SlicedImageElement>()
						.SetImage(img)
						.SetTargetSize(g_previewSlicedWidth, g_previewSlicedHeight)
						.SetBorderSizes(rs.l, rs.t, rs.r, rs.b);
				}

				Pop();
			}
		}

		Pop();
	}
};

struct TE_ImageEditorNode : Buildable
{
	TE_Theme* theme;
	TE_Template* tmpl;

	void Build() override
	{
		BuildMulticastDelegateAdd(OnActiveImageChanged, [this]() { Rebuild(); });

		Push<StackTopDownLayoutElement>();

		Push<StackLTRLayoutElement>();
		{
			MakeWithText<LabelFrame>("Images");
			if (imButton("Add"))
			{
				auto img = std::make_shared<TE_Image>();
				img->name = "<unnamed>";
				tmpl->images.InsertAt(0, img);
				Rebuild();
			}
		}
		Pop();

		auto& imged = Make<SequenceEditor>();
		imged.itemLayoutPreset = EditorItemContentsLayoutPreset::None;
		imged.SetSequence(UI_BUILD_ALLOC(ArraySequence<decltype(tmpl->images)>)(tmpl->images));
		imged.itemUICallback = [this](SequenceEditor* se, size_t idx, void* ptr)
		{
			EditImage(static_cast<std::shared_ptr<TE_Image>*>(ptr)->get(), se, idx, ptr);
		};

		Pop();
	}

	void EditImage(TE_Image* img, SequenceEditor* se, size_t idx, void* ptr)
	{
		Push<StackTopDownLayoutElement>();
		{
			Push<StackExpandLTRLayoutElement>();
			{
				auto stmpl = ui::StackExpandLTRLayoutElement::GetSlotTemplate();

				stmpl->DisableScaling();
				imEditBool(img->expanded, {}, {}, TreeStateToggleSkin());

				stmpl->DisableScaling();
				if (imRadioButtonRaw(tmpl->curPreviewImage == img, "P", {}, ButtonStateToggleSkin()))
				{
					tmpl->SetCurPreviewImage(img);
				}

				imEditString(StdStringRW(img->name));
				se->OnBuildDeleteButton();
			}
			Pop();

			if (img->expanded)
			{
				Push<FrameElement>()
					.SetDefaultFrameStyle(DefaultFrameStyle::GroupBox)
					.HandleEvent() = [this](Event& e)
				{
					if (e.type == EventType::Change || e.type == EventType::Commit || e.type == EventType::IMChange)
						tmpl->InvalidateAllNodes();
				};
				Push<StackTopDownLayoutElement>();
				{
					auto& ovr = img->overrides[theme->curVariation];
					if (!ovr)
						ovr = std::make_shared<TE_Overrides>();

					auto& coed = Make<SequenceEditor>();
					// TODO making Allocate automatically work in the current node would avoid this bug (Allocate instead of GetCurrentNode()->)
					coed.SetSequence(UI_BUILD_ALLOC(ArraySequence<decltype(ovr->colorOverrides)>)(ovr->colorOverrides));
					coed.itemUICallback = [](SequenceEditor* se, size_t idx, void* ptr)
					{
						auto& co = *static_cast<TE_ColorOverride*>(ptr);
						if (auto ncr = co.ncref.lock())
							MakeWithText<LabelFrame>(ncr->name);
						else
							EditNCRef(co.ncref);

						ui::Push<ui::SizeConstraintElement>().SetWidth(40);
						imEditColor(co.color, false);
						ui::Pop();
					};

					if (imButton("Add override"))
					{
						Array<MenuItem> items;
						for (auto& col : theme->curTemplate->colors)
						{
							bool added = false;
							for (auto& acol : ovr->colorOverrides)
							{
								if (col == acol.ncref.lock())
								{
									added = true;
									break;
								}
							}
							if (!added)
							{
								items.Append(MenuItem(col->name).Func([ovr, col]() { ovr->colorOverrides.Append({ col, col->color }); }));
							}
						}
						if (items.IsEmpty())
							items.Append(MenuItem("- no overrides left to add -", {}, true));
						Menu(items).Show(this);
					}
				}
				Pop();
				Pop();
			}
		}
		Pop();
	}
};

static bool showRenderSettings = true;
static bool showColors = true;
static float hSplit[2] = { 0.6f, 0.8f };
static float vSplitPreview[1] = { 0.5f };

struct TE_TemplateEditorNode : Buildable
{
	TE_Theme* theme;
	TE_Template* tmpl;

	void Build() override
	{
		Push<SplitPane>().Init(Direction::Horizontal, hSplit);
		{
			Push<ListBoxFrame>();
			{
				auto& pge = Make<ProcGraphEditor>();
				pge.Init(tmpl);
			}
			Pop();

			auto& ien = Make<TE_ImageEditorNode>();
			ien.theme = theme;
			ien.tmpl = tmpl;

			Push<StackTopDownLayoutElement>();
			{
				Push<SplitPane>().Init(Direction::Vertical, vSplitPreview);
				{
					auto& preview = Make<TE_MainPreviewNode>();
					preview.tmpl = tmpl;

					Push<PropertyList>()
						+ AddEventHandler(EventType::IMChange, [this](Event&) { tmpl->InvalidateAllNodes(); });
					Push<StackTopDownLayoutElement>();
					{
						imEditBool(showRenderSettings, "Render settings", {}, TreeStateToggleSkin());
						if (showRenderSettings)
						{
							tmpl->renderSettings.UI();
						}

						imEditBool(showColors, "Colors", {}, TreeStateToggleSkin());
						if (showColors)
						{
							auto& ced = Make<SequenceEditor>();
							ced.SetSequence(UI_BUILD_ALLOC(ArraySequence<decltype(tmpl->colors)>)(tmpl->colors));
							ced.itemUICallback = [this](SequenceEditor* se, size_t idx, void* ptr)
							{
								auto& NC = *static_cast<std::shared_ptr<TE_NamedColor>*>(ptr);
								imEditColor(NC->color);
								imEditString(StdStringRW(NC->name));
							};
						}
					}
					Pop();
					Pop();
				}
				Pop();
			}
			Pop();
		}
		Pop();
	}
};

struct TE_ThemeEditorNode : Buildable
{
	void Build() override
	{
		Array<MenuItem> topMenu;
		topMenu.Append(MenuItem("File"));
		topMenu.Last().submenu.Append(MenuItem("New").Func([this]() { theme->Clear(); Rebuild(); }));
		topMenu.Last().submenu.Append(MenuItem("Open").Func([this]() { theme->LoadFromFile("sample.ths"); Rebuild(); }));
		topMenu.Last().submenu.Append(MenuItem("Save").Func([this]() { theme->SaveToFile("sample.ths"); Rebuild(); }));
		UI_BUILD_ALLOC(TopMenu)(GetNativeWindow(), topMenu);

		Push<LayerLayoutElement>();

		auto& tp = Push<TabbedPanel>();
		for (TE_Template* tmpl : theme->templates)
		{
			if (editNameTemplate != tmpl)
			{
				auto& w = PushNoAppend<WrapperElement>();
				auto& txt = Text(tmpl->name);
				//txt.flags |= UIObject_DB_CaptureMouseOnLeftClick;
				txt.HandleEvent(EventType::Click) = [this, tmpl](Event& e)
				{
					if (e.numRepeats == 2)
					{
						editNameTemplate = tmpl;
						Rebuild();
					}
				};
				Pop();
				tp.AddUITab(&w, uintptr_t(tmpl));
			}
			else
			{
				auto efn = [this](Event& e)
				{
					if (e.type == EventType::LostFocus)
					{
						editNameTemplate = nullptr;
						Rebuild();
					}
				};

				auto& w = PushNoAppend<WrapperElement>();
				Push<SizeConstraintElement>().SetWidth(100);
				imEditString(StdStringRW(tmpl->name), { AddEventHandler(efn) });
				Pop();
				Pop();
				tp.AddUITab(&w, uintptr_t(tmpl));
			}
		}
		tp.SetActiveTabByEnumValue(theme->curTemplate);
		tp.HandleEvent(&tp, EventType::SelectionChange) = [this, &tp](Event&)
		{
			theme->curTemplate = tp.GetCurrentTabEnumValue<TE_Template*>();
			Rebuild();
		};

		auto& w = PushNoAppend<StackLTRLayoutElement>();
		if (imButton("+"))
		{
			auto* p = new TE_Template(theme);
			p->name = "<name>";
			theme->templates.Append(p);
			Rebuild();
		}
		Pop();
		tp.SetTabBarExtension(&w);

		if (theme->curTemplate)
		{
			auto& pen = Make<TE_TemplateEditorNode>();
			pen.theme = theme;
			pen.tmpl = theme->curTemplate;
		}
		Pop();

		Make<DefaultOverlayBuilder>();

		Pop();
	}

	TE_Theme* theme;
	TE_Template* editNameTemplate = nullptr;
};

int uimain(int argc, char* argv[])
{
	Application app(argc, argv);
	NativeMainWindow mw;
	mw.SetTitle("Theme Editor");
	mw.SetInnerSize(1280, 720);
	TE_Theme theme;
	auto* ten = CreateUIObject<TE_ThemeEditorNode>();
	ten->theme = &theme;
	mw.SetContents(ten, true);
	mw.SetVisible(true);
	return app.Run();
}
