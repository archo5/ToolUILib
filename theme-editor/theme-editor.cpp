
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

struct TE_SlicedImageElement : UIElement
{
	void OnPaint() override
	{
		UIRect r = GetContentRect();
		float iw = _image->GetWidth();
		float ih = _image->GetHeight();
		float x0 = roundf((r.x0 + r.x1 - _width) * 0.5f);
		float y0 = roundf((r.y0 + r.y1 - _height) * 0.5f);
		UIRect outer = { x0, y0, x0 + _width, y0 + _height };
		UIRect inner = outer.ShrinkBy({ float(_left), float(_top), float(_right), float(_bottom) });
		UIRect texinner = { invlerp(0, iw, _left), invlerp(0, ih, _top), invlerp(iw, 0, _right), invlerp(ih, 0, _bottom) };
		draw::RectColTex9Slice(outer, inner, ui::Color4b::White(), _image->_texture, { 0, 0, 1, 1 }, texinner);
	}
	TE_SlicedImageElement* SetImage(Image* img)
	{
		_image = img;
		return this;
	}
	TE_SlicedImageElement* SetTargetSize(int w, int h)
	{
		_width = w;
		_height = h;
		return this;
	}
	TE_SlicedImageElement* SetBorderSizes(int l, int t, int r, int b)
	{
		_left = l;
		_top = t;
		_right = r;
		_bottom = b;
		return this;
	}

	Image* _image = nullptr;
	int _width = 0;
	int _height = 0;
	int _left = 0;
	int _top = 0;
	int _right = 0;
	int _bottom = 0;
};

struct TE_MainPreviewNode : Node
{
	void Render(UIContainer* ctx) override
	{
		Subscribe(DCT_NodePreviewInvalidated);
		Subscribe(DCT_EditProcGraph);
		Subscribe(DCT_EditProcGraphNode);

		ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
		{
			ctx->Text("Preview") + Padding(5);
			if (theme->curPreviewImage && imm::Button(ctx, "Reset to template"))
			{
				theme->SetCurPreviewImage(nullptr);
			}
		}
		ctx->Pop();

		if (tmpl->renderSettings.layer)
		{
			ctx->PushBox() + Layout(style::layouts::EdgeSlice());
			{
				ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
				{
					imm::RadioButton(ctx, g_previewMode, TEPM_Original, "Original", {}, imm::ButtonStateToggleSkin());
					imm::RadioButton(ctx, g_previewMode, TEPM_Sliced, "Sliced", {}, imm::ButtonStateToggleSkin());
				}
				ctx->Pop();
				if (g_previewMode == TEPM_Original)
				{
					ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
					{
						imm::RadioButton(ctx, g_previewScaleMode, ScaleMode::None, "No scaling", {}, imm::ButtonStateToggleSkin());
						imm::RadioButton(ctx, g_previewScaleMode, ScaleMode::Fit, "Fit", {}, imm::ButtonStateToggleSkin());
					}
					ctx->Pop();
				}
				if (g_previewMode == TEPM_Sliced)
				{
					ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
					{
						imm::PropEditInt(ctx, "\bWidth", g_previewSlicedWidth, { MinWidth(20) });
						imm::PropEditInt(ctx, "\bHeight", g_previewSlicedHeight, { MinWidth(20) });
					}
					ctx->Pop();
				}

				*ctx->Push<ListBox>() + Height(style::Coord::Percent(95)); // TODO

				Canvas canvas;
#if 0
				auto rc = imgNode->MakeRenderContext();
				if (auto* prevImg = theme->curPreviewImage)
					rc.overrides = prevImg->overrides[theme->curVariation].get();
#endif
				auto& rs = tmpl->renderSettings;
				rs.layer->Render(canvas, tmpl->GetRenderContext());
				auto* img = Allocate<Image>(canvas);
				if (g_previewMode == TEPM_Original)
				{
					*ctx->Make<ImageElement>()
						->SetImage(img)
						->SetScaleMode(g_previewScaleMode)
						+ ui::Width(style::Coord::Percent(100))
						+ ui::Height(style::Coord::Percent(100));
				}
				if (g_previewMode == TEPM_Sliced)
				{
					*ctx->Make<TE_SlicedImageElement>()
						->SetImage(img)
						->SetTargetSize(g_previewSlicedWidth, g_previewSlicedHeight)
						->SetBorderSizes(rs.l, rs.t, rs.r, rs.b)
						+ ui::Width(style::Coord::Percent(100))
						+ ui::Height(style::Coord::Percent(100));
				}

				ctx->Pop();
			}
			ctx->Pop();
		}
	}

	TE_Theme* theme;
	TE_Template* tmpl;
};

static bool showRenderSettings = true;
static bool showColors = true;
struct TE_TemplateEditorNode : Node
{
	void Render(UIContainer* ctx) override
	{
		auto* hsp = ctx->Push<SplitPane>();
		{
			ctx->Push<ListBox>();
			{
				auto* pge = ctx->Make<ProcGraphEditor>();
				*pge + Height(style::Coord::Percent(100));
				pge->Init(tmpl);
			}
			ctx->Pop();

			ctx->PushBox();
			{
				auto* vsp = ctx->Push<SplitPane>();
				{
					auto* preview = ctx->Make<TE_MainPreviewNode>();
					preview->theme = theme;
					preview->tmpl = tmpl;

					ctx->PushBox()
						+ EventHandler(UIEventType::IMChange, [this](UIEvent&) { tmpl->InvalidateAllNodes(); });
					{
						imm::EditBool(ctx, showRenderSettings, "Render settings", {}, imm::TreeStateToggleSkin());
						if (showRenderSettings)
						{
							tmpl->renderSettings.UI(ctx);
						}

						imm::EditBool(ctx, showColors, "Colors", {}, imm::TreeStateToggleSkin());
						if (showColors)
						{
							auto* ced = ctx->Make<SequenceEditor>();
							*ced + Height(style::Coord::Percent(100));
							ced->SetSequence(Allocate<StdSequence<decltype(tmpl->colors)>>(tmpl->colors));
							ced->itemUICallback = [this](UIContainer* ctx, SequenceEditor* se, size_t idx, void* ptr)
							{
								auto& NC = *static_cast<std::shared_ptr<TE_NamedColor>*>(ptr);
								imm::EditColor(ctx, NC->color);
								imm::EditString(ctx, NC->name.c_str(), [&NC](const char* v) { NC->name = v; });
							};
						}
					}
					ctx->Pop();
				}
				ctx->Pop();
				vsp->SetDirection(true);
				vsp->SetSplits({ 0.5f });
			}
			ctx->Pop();
		}
		ctx->Pop();
		hsp->SetSplits({ 0.8f });
	}

	TE_Theme* theme;
	TE_Template* tmpl;
};

struct TE_ImageEditorNode : Node
{
	static constexpr bool Persistent = true;

	TE_Theme* theme;

	void Render(UIContainer* ctx) override
	{
		Subscribe(DCT_ChangeActiveImage);

		ctx->PushBox() + StackingDirection(style::StackingDirection::LeftToRight);
		{
			ctx->Text("Images") + Padding(5);
			if (imm::Button(ctx, "Add"))
			{
				auto img = std::make_shared<TE_Image>();
				img->name = "<unnamed>";
				theme->images.insert(theme->images.begin(), img);
				Rerender();
			}
		}
		ctx->Pop();

		auto* imged = ctx->Make<SequenceEditor>();
		*imged + Height(style::Coord::Percent(100));
		imged->showDeleteButton = false;
		imged->SetSequence(Allocate<StdSequence<decltype(theme->images)>>(theme->images));
		imged->itemUICallback = [this](UIContainer* ctx, SequenceEditor* se, size_t idx, void* ptr)
		{
			EditImage(ctx, static_cast<std::shared_ptr<TE_Image>*>(ptr)->get(), se, idx, ptr);
		};
	}

	void EditImage(UIContainer* ctx, TE_Image* img, SequenceEditor* se, size_t idx, void* ptr)
	{
		ctx->PushBox();
		{
			ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
			{
				imm::EditBool(ctx, img->expanded, nullptr, { Width(style::Coord::Fraction(0)) }, imm::TreeStateToggleSkin());
				if (imm::RadioButtonRaw(ctx, theme->curPreviewImage == img, "P", { Width(style::Coord::Fraction(0)) }, imm::ButtonStateToggleSkin()))
				{
					theme->SetCurPreviewImage(img);
				}
				imm::EditString(ctx, img->name.c_str(), [&img](const char* v) { img->name = v; });
				se->OnBuildDeleteButton(ctx, idx);
			}
			ctx->Pop();

			if (img->expanded)
			{
				ctx->Push<Panel>()->HandleEvent() = [this](UIEvent& e)
				{
					if (e.type == UIEventType::Change || e.type == UIEventType::Commit || e.type == UIEventType::IMChange)
						theme->curTemplate->InvalidateAllNodes();
				};
				{
					auto& ovr = img->overrides[theme->curVariation];
					if (!ovr)
						ovr = std::make_shared<TE_Overrides>();

					auto* coed = ctx->Make<SequenceEditor>();
					// TODO making Allocate automatically work in the current node would avoid this bug (Allocate instead of ctx->GetCurrentNode()->)
					coed->SetSequence(ctx->GetCurrentNode()->Allocate<StdSequence<decltype(ovr->colorOverrides)>>(ovr->colorOverrides));
					coed->itemUICallback = [](UIContainer* ctx, SequenceEditor* se, size_t idx, void* ptr)
					{
						auto& co = *static_cast<TE_ColorOverride*>(ptr);
						if (auto ncr = co.ncref.lock())
							*ctx->MakeWithText<BoxElement>(ncr->name) + Padding(5);
						else
							EditNCRef(ctx, co.ncref);
						imm::EditColor(ctx, co.color, { Width(40) });
					};

					if (imm::Button(ctx, "Add override"))
					{
						std::vector<MenuItem> items;
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
								items.push_back(MenuItem(col->name).Func([ovr, col]() { ovr->colorOverrides.push_back({ col, col->color }); }));
							}
						}
						if (items.empty())
							items.push_back(MenuItem("- no overrides left to add -", {}, true));
						Menu(items).Show(this);
					}
				}
				ctx->Pop();
			}
		}
		ctx->Pop();
	}
};

struct TE_ThemeEditorNode : Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		ctx->Push<MenuBarElement>();
		{
			ctx->Push<MenuItemElement>()->SetText("File");
			{
				ctx->Make<MenuItemElement>()->SetText("New").Func([this]() { theme->Clear(); Rerender(); });
				ctx->Make<MenuItemElement>()->SetText("Open").Func([this]() { theme->LoadFromFile("sample.ths"); Rerender(); });
				ctx->Make<MenuItemElement>()->SetText("Save").Func([this]() { theme->SaveToFile("sample.ths"); Rerender(); });
			}
			ctx->Pop();
		}
		ctx->Pop();

		auto* hsp = ctx->Push<SplitPane>();
		{
			ctx->Make<TE_ImageEditorNode>()->theme = theme;

			*ctx->Push<TabGroup>() + Height(style::Coord::Percent(100)) + Layout(style::layouts::EdgeSlice());
			{
				ctx->Push<TabButtonList>();
				{
					for (TE_Template* tmpl : theme->templates)
					{
						ctx->Push<TabButtonT<TE_Template*>>()->Init(theme->curTemplate, tmpl);
						if (editNameTemplate != tmpl)
						{
							ctx->Text(tmpl->name).HandleEvent(UIEventType::Click) = [this, tmpl](UIEvent& e)
							{
								if (e.numRepeats == 2)
								{
									editNameTemplate = tmpl;
									Rerender();
								}
							};
						}
						else
						{
							auto efn = [this](UIEvent& e)
							{
								if (e.type == UIEventType::LostFocus)
								{
									editNameTemplate = nullptr;
									Rerender();
								}
							};
							imm::EditString(
								ctx,
								tmpl->name.c_str(),
								[tmpl](const char* v) { tmpl->name = v; },
								{ Width(100), EventHandler(efn) });
						}
						ctx->Pop();
					}
				}
				if (imm::Button(ctx, "+"))
				{
					auto* p = new TE_Template(theme);
					p->name = "<name>";
					theme->templates.push_back(p);
				}
				ctx->Pop();

				if (theme->curTemplate)
				{
					*ctx->Push<TabPanel>() + Height(style::Coord::Percent(100));
					{
						auto* pen = ctx->Make<TE_TemplateEditorNode>();
						*pen + Height(style::Coord::Percent(100));
						pen->theme = theme;
						pen->tmpl = theme->curTemplate;
					}
					ctx->Pop();
				}
			}
			ctx->Pop();
		}
		ctx->Pop();
		hsp->SetSplits({ 0.2f });
	}

	TE_Theme* theme;
	TE_Template* editNameTemplate = nullptr;
};

struct ThemeEditorMainWindow : NativeMainWindow
{
	ThemeEditorMainWindow()
	{
		SetTitle("Theme Editor");
		SetSize(1280, 720);
	}
	void OnRender(UIContainer* ctx)
	{
		auto* ten = ctx->Make<TE_ThemeEditorNode>();
		*ten + Height(style::Coord::Percent(100));
		ten->theme = &theme;

		ctx->Make<DefaultOverlayRenderer>();
	}

	TE_Theme theme;
};

int uimain(int argc, char* argv[])
{
	Application app(argc, argv);
	ThemeEditorMainWindow mw;
	mw.SetVisible(true);
	return app.Run();
}
