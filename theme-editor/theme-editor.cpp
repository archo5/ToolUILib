
#include "TECore.h"
#include "TENodes.h"
#include "TETheme.h"

#include "../Model/WIP.h"


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
		UIRect r = GetContentRect();
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
		TEMP_LAYOUT_MODE = FILLER;
		Subscribe(DCT_NodePreviewInvalidated);
		Subscribe(DCT_EditProcGraph);
		Subscribe(DCT_EditProcGraphNode);

		Push<EdgeSliceLayoutElement>();

		Push<StackExpandLTRLayoutElement>();
		{
			MakeWithText<LabelFrame>("Preview");
			if (tmpl->curPreviewImage && imm::Button("Reset to template"))
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
					imm::RadioButton(g_previewMode, TEPM_Original, "Original", {}, imm::ButtonStateToggleSkin());
					imm::RadioButton(g_previewMode, TEPM_Sliced, "Sliced", {}, imm::ButtonStateToggleSkin());
				}
				Pop();
				if (g_previewMode == TEPM_Original)
				{
					Push<StackExpandLTRLayoutElement>();
					{
						imm::RadioButton(g_previewScaleMode, ScaleMode::None, "No scaling", {}, imm::ButtonStateToggleSkin());
						imm::RadioButton(g_previewScaleMode, ScaleMode::Fit, "Fit", {}, imm::ButtonStateToggleSkin());
					}
					Pop();
				}
				if (g_previewMode == TEPM_Sliced)
				{
					Push<LabeledProperty>();
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
		Subscribe(DCT_ChangeActiveImage);

		Push<StackLTRLayoutElement>();
		{
			MakeWithText<LabelFrame>("Images");
			if (imm::Button("Add"))
			{
				auto img = std::make_shared<TE_Image>();
				img->name = "<unnamed>";
				tmpl->images.insert(tmpl->images.begin(), img);
				Rebuild();
			}
		}
		Pop();

		auto& imged = Make<SequenceEditor>();
		imged.itemLayoutPreset = EditorItemContentsLayoutPreset::None;
		imged.SetSequence(Allocate<StdSequence<decltype(tmpl->images)>>(tmpl->images));
		imged.itemUICallback = [this](SequenceEditor* se, size_t idx, void* ptr)
		{
			EditImage(static_cast<std::shared_ptr<TE_Image>*>(ptr)->get(), se, idx, ptr);
		};
	}

	void EditImage(TE_Image* img, SequenceEditor* se, size_t idx, void* ptr)
	{
		Push<StackTopDownLayoutElement>();
		{
			Push<StackExpandLTRLayoutElement>();
			{
				imm::EditBool(img->expanded, nullptr, { SetWidth(Coord::Fraction(0)) }, imm::TreeStateToggleSkin());
				if (imm::RadioButtonRaw(tmpl->curPreviewImage == img, "P", { SetWidth(Coord::Fraction(0)) }, imm::ButtonStateToggleSkin()))
				{
					tmpl->SetCurPreviewImage(img);
				}
				imm::EditString(img->name.c_str(), [&img](const char* v) { img->name = v; });
				se->OnBuildDeleteButton();
			}
			Pop();

			if (img->expanded)
			{
				Push<Panel>().HandleEvent() = [this](Event& e)
				{
					if (e.type == EventType::Change || e.type == EventType::Commit || e.type == EventType::IMChange)
						tmpl->InvalidateAllNodes();
				};
				{
					auto& ovr = img->overrides[theme->curVariation];
					if (!ovr)
						ovr = std::make_shared<TE_Overrides>();

					auto& coed = Make<SequenceEditor>();
					// TODO making Allocate automatically work in the current node would avoid this bug (Allocate instead of GetCurrentNode()->)
					coed.SetSequence(ui::BuildAlloc<StdSequence<decltype(ovr->colorOverrides)>>(ovr->colorOverrides));
					coed.itemUICallback = [](SequenceEditor* se, size_t idx, void* ptr)
					{
						auto& co = *static_cast<TE_ColorOverride*>(ptr);
						if (auto ncr = co.ncref.lock())
							MakeWithText<LabelFrame>(ncr->name);
						else
							EditNCRef(co.ncref);
						imm::EditColor(co.color, false, { SetWidth(40) });
					};

					if (imm::Button("Add override"))
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
				Pop();
			}
		}
		Pop();
	}
};

static bool showRenderSettings = true;
static bool showColors = true;
struct TE_TemplateEditorNode : Buildable
{
	TE_Theme* theme;
	TE_Template* tmpl;

	void Build() override
	{
		auto& hsp = Push<SplitPane>();
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
				auto& vsp = Push<SplitPane>();
				{
					auto& preview = Make<TE_MainPreviewNode>();
					preview.tmpl = tmpl;

					Push<PropertyList>()
						+ AddEventHandler(EventType::IMChange, [this](Event&) { tmpl->InvalidateAllNodes(); });
					{
						imm::EditBool(showRenderSettings, "Render settings", {}, imm::TreeStateToggleSkin());
						if (showRenderSettings)
						{
							tmpl->renderSettings.UI();
						}

						imm::EditBool(showColors, "Colors", {}, imm::TreeStateToggleSkin());
						if (showColors)
						{
							auto& ced = Make<SequenceEditor>();
							ced.SetSequence(Allocate<StdSequence<decltype(tmpl->colors)>>(tmpl->colors));
							ced.itemUICallback = [this](SequenceEditor* se, size_t idx, void* ptr)
							{
								auto& NC = *static_cast<std::shared_ptr<TE_NamedColor>*>(ptr);
								imm::EditColor(NC->color);
								imm::EditString(NC->name.c_str(), [&NC](const char* v) { NC->name = v; });
							};
						}
					}
					Pop();
				}
				Pop();
				vsp.SetDirection(true);
				vsp.SetSplits({ 0.5f });
			}
			Pop();
		}
		Pop();
		hsp.SetSplits({ 0.6f, 0.8f });
	}
};

struct TE_ThemeEditorNode : Buildable
{
	void Build() override
	{
		std::vector<MenuItem> topMenu;
		topMenu.push_back(MenuItem("File"));
		topMenu.back().submenu.push_back(MenuItem("New").Func([this]() { theme->Clear(); Rebuild(); }));
		topMenu.back().submenu.push_back(MenuItem("Open").Func([this]() { theme->LoadFromFile("sample.ths"); Rebuild(); }));
		topMenu.back().submenu.push_back(MenuItem("Save").Func([this]() { theme->SaveToFile("sample.ths"); Rebuild(); }));
		Allocate<TopMenu>(GetNativeWindow(), topMenu);

		//auto& tp = Push<TabbedPanel>();
		Push<TabGroup>();
		{
#if 0
			for (TE_Template* tmpl : theme->templates)
				tp.AddEnumTab(tmpl->name, tmpl);
			tp.SetActiveTabByEnumValue(theme->curTemplate);
			tp.HandleEvent(&tp, EventType::Change) = [this](Event&)
			{
			};
#endif
			Push<TabButtonList>();
			{
				for (TE_Template* tmpl : theme->templates)
				{
					Push<TabButtonT<TE_Template*>>().Init(theme->curTemplate, tmpl);
					if (editNameTemplate != tmpl)
					{
						auto& txt = Text(tmpl->name);
						txt.flags |= UIObject_DB_CaptureMouseOnLeftClick;
						txt.HandleEvent(EventType::Click) = [this, tmpl](Event& e)
						{
							if (e.numRepeats == 1)
							{
								editNameTemplate = tmpl;
								Rebuild();
							}
						};
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
						imm::EditString(
							tmpl->name.c_str(),
							[tmpl](const char* v) { tmpl->name = v; },
							{ SetWidth(100), AddEventHandler(efn) });
					}
					Pop();
				}
			}
			if (imm::Button("+"))
			{
				auto* p = new TE_Template(theme);
				p->name = "<name>";
				theme->templates.push_back(p);
			}
			Pop();

			if (theme->curTemplate)
			{
				Push<TabPanel>();
				{
					auto& pen = Make<TE_TemplateEditorNode>();
					pen.theme = theme;
					pen.tmpl = theme->curTemplate;
				}
				Pop();
			}
		}
		Pop();
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
	void OnBuild() override
	{
		auto& ten = Make<TE_ThemeEditorNode>();
		ten.theme = &theme;

		Make<DefaultOverlayBuilder>();
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
