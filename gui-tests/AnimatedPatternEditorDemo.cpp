
#include "pch.h"

#include "../lib-src/Core/MathExpr.h"
#include "../lib-src/Editors/TreeEditor.h"
#include "../lib-src/Editors/CurveEditor.h"


using namespace ui;

static MulticastDelegate<> AnimPatternChanged;


static constexpr int NUM_CURVES = 16;


struct APGlobalSettings
{
	int imageRes = 128;
	int numSteps = 16;
	float duration = 2;

	Sequence01Curve curves[NUM_CURVES];

	APGlobalSettings()
	{
		for (int i = 0; i < NUM_CURVES; i++)
		{
			curves[i].points.Append({ 0, 0, 0 });
			curves[i].points.Append({ 1, 1, 1 });
		}
	}
	void UI()
	{
		imLabel("Image resolution"), imEditInt(imageRes, {}, {}, { 32, 512 });
		imLabel("Step count"), imEditInt(numSteps, {}, {}, { 1, 64 });
		imm::PropEditFloat("Duration (sec)", duration, {}, {}, { 1.0f, 30.0f });
	}
};


struct APMathExprDataSource : IMathExprDataSource
{
	enum Variables
	{
		T,
		ImageRes,
		NumSteps,
		Duration,
		Curve0,
		Curve15 = Curve0 + NUM_CURVES - 1,
	};

	float t = 0;
	float evalCurves[NUM_CURVES] = {};
	APGlobalSettings* settings = nullptr;

	void EarlyEval()
	{
		for (int i = 0; i < NUM_CURVES; i++)
			evalCurves[i] = settings->curves[i].Evaluate(t);
	}

	ID FindVariable(const char* name) override
	{
		if (IsNameEqualTo(name, "t")) return T;
		if (IsNameEqualTo(name, "width") ||
			IsNameEqualTo(name, "height")) return ImageRes;
		if (IsNameEqualTo(name, "numSteps")) return NumSteps;
		if (IsNameEqualTo(name, "duration")) return Duration;
		for (int i = 0; i < NUM_CURVES; i++)
		{
			char cmp[7] = "curve0";
			cmp[5] = "0123456789ABCDEF"[i];
			if (IsNameEqualTo(name, cmp)) return Curve0 + i;
		}
		return NOT_FOUND;
	}
	float GetVariable(ID id) override
	{
		switch (id)
		{
		case T: return t;
		case ImageRes: return settings->imageRes;
		case NumSteps: return settings->numSteps;
		case Duration: return settings->duration;
		default:
			if (id >= Curve0 && id <= Curve15)
				return evalCurves[id - Curve0];
			return 0;
		}
	}
};


struct MathExprStr : IMathExprErrorOutput
{
	std::string expr;
	std::string error;
	MathExpr compiled;

	void SetExpr(StringView str)
	{
		expr = ui::to_string(str);
		error.clear();
		APMathExprDataSource compileOnlyDS;
		compiled.Compile(str, &compileOnlyDS, this);
	}
	bool IMUI(const char* label)
	{
		bool ret = (imLabel(label), imEditString(BRW(expr, [this](StringView v) { SetExpr(v); })));
		if (!error.empty())
		{
			auto& fe = MakeWithText<LabelFrame>(error);
			fe.frameStyle.textColor.SetValue(Color4b(200, 0, 0));
		}
		return ret;
	}

	void OnError(int pos, const char* err) override
	{
		error = Format("Error: [%d] %s", pos, err);
	}
};


struct APLayer
{
	Array<APLayer*> children;

	~APLayer()
	{
		for (APLayer* n : children)
			delete n;
	}
	virtual APLayer* CloneBase() = 0;
	APLayer* Clone()
	{
		APLayer* tmp = CloneBase();
		for (APLayer*& n : tmp->children)
			n = n->Clone();
		return tmp;
	}

	virtual const char* ItemName() = 0;
	virtual void InlineUI()
	{
		MakeWithText<ui::LabelFrame>(ItemName());
	}
	virtual void FullUI()
	{
		Text("TODO:");
		Text(typeid(*this).name());
	}
	virtual void Eval(APMathExprDataSource* meds) = 0;
	void EvalAll(APMathExprDataSource* meds)
	{
		Eval(meds);
		for (auto* ch : children)
			ch->EvalAll(meds);
	}
	virtual float Sample(float spx, float spy) = 0;
};


struct APL_Combiner : APLayer
{
	enum Mode
	{
		Union, // 1- multiply of 1- child values
		Intersect, // multiply values of children
		Subtract, // subtract 1+ from 0
	};
	Mode mode = Union;
	bool invert = false;

	const char* ItemName() override
	{
		switch (mode)
		{
		case Union: return "[inv] Combiner: Union" + (invert ? 0 : 6);
		case Intersect: return "[inv] Combiner: Intersect" + (invert ? 0 : 6);
		case Subtract: return "[inv] Combiner: Subtract" + (invert ? 0 : 6);
		default: return "[inv] Combiner: ???" + (invert ? 0 : 6);
		}
	}
	void FullUI() override
	{
		imLabel("Mode"), imDropdownMenuList(mode, UI_BUILD_ALLOC(ZeroSepCStrOptionList)("Union\0Intersect\0Subtract\0"));
		imLabel("Invert"), imEditBool(invert);
	}
	APLayer* CloneBase() override
	{
		return new APL_Combiner(*this);
	}
	void Eval(APMathExprDataSource* meds) override
	{
	}
	float Sample(float spx, float spy) override
	{
		if (children.IsEmpty())
			return invert ? 1 : 0;

		float ret = 1;
		if (mode == Union)
		{
			for (auto* ch : children)
				ret *= 1 - ch->Sample(spx, spy);
			ret = 1 - ret;
		}
		else if (mode == Intersect)
		{
			for (auto* ch : children)
				ret *= ch->Sample(spx, spy);
		}
		else if (mode == Subtract)
		{
			ret = children[0]->Sample(spx, spy);
			for (size_t i = 1; i < children.size(); i++)
				ret *= 1 - children[i]->Sample(spx, spy);
		}

		if (invert)
			ret = 1 - ret;
		return ret;
	}
};


struct APL_Shape : APLayer
{
};


struct APL_Shape_Square : APL_Shape
{
	MathExprStr x;
	MathExprStr y;
	MathExprStr halfSize;
	MathExprStr angle;

	float e_x, e_y, e_halfSize, e_cosAngle, e_sinAngle;

	APL_Shape_Square(APGlobalSettings& settings)
	{
		char bfr[32];

		snprintf(bfr, 32, "%g", settings.imageRes / 2.0f);
		x.SetExpr(bfr);
		y.SetExpr(bfr);

		snprintf(bfr, 32, "%g", settings.imageRes / 4.0f);
		halfSize.SetExpr(bfr);

		angle.SetExpr("0");
	}
	const char* ItemName() override
	{
		return "Square";
	}
	APLayer* CloneBase() override
	{
		return new APL_Shape_Square(*this);
	}
	void Eval(APMathExprDataSource* meds) override
	{
		e_x = x.compiled.Evaluate(meds);
		e_y = y.compiled.Evaluate(meds);
		e_halfSize = halfSize.compiled.Evaluate(meds);
		float e_angle = angle.compiled.Evaluate(meds);
		e_cosAngle = cosf(-e_angle * DEG2RAD);
		e_sinAngle = sinf(-e_angle * DEG2RAD);
	}
	float Sample(float spx, float spy) override
	{
		float sprx = spx - e_x;
		float spry = spy - e_y;

		float dx = sprx * e_cosAngle - spry * e_sinAngle;
		float dy = sprx * e_sinAngle + spry * e_cosAngle;

		return clamp(e_halfSize - max(fabsf(dx), fabsf(dy)) - 0.5f, 0.0f, 1.0f);
	}
	void FullUI() override
	{
		x.IMUI("X");
		y.IMUI("Y");
		halfSize.IMUI("Half-size");
		angle.IMUI("Angle");
	}
};


static Color4b defImage(255, 0, 0, 127);
struct AnimPattern : ITree
{
	Array<APLayer*> rootLayers;
	APGlobalSettings globalSettings;

	APLayer* selectedLayer = nullptr;
	int curCurve = 0;
	float curTime = 0;
	bool playing = false;
	uint32_t prevTime = platform::GetTimeMs();

	draw::ImageHandle image = draw::ImageCreateRGBA8(1, 1, &defImage);

	~AnimPattern()
	{
		Clear();
	}

	void Clear()
	{
		for (APLayer* r : rootLayers)
			delete r;
		rootLayers.Clear();
	}

	void SetPlaying(bool play)
	{
		playing = play;
		if (play)
			prevTime = platform::GetTimeMs();
	}

	void Update()
	{
		if (playing)
		{
			auto t = platform::GetTimeMs();
			float dt = (t - prevTime) * 0.001f;
			prevTime = t;

			curTime += dt;
			curTime = fmodf(curTime, globalSettings.duration);
		}
	}

	void PreviewUI()
	{
		Push<StackExpandLTRLayoutElement>();

		auto tmpl = StackExpandLTRLayoutElement::GetSlotTemplate();

		tmpl->DisableScaling();
		MakeWithText<Header>("Preview");

		tmpl->DisableScaling();
		if (imButton(playing ? DefaultIconStyle::Pause : DefaultIconStyle::Play))
			SetPlaying(!playing);

		auto& slider = Make<Slider>().SetValue(curTime / globalSettings.duration);
		slider + AddEventHandler(EventType::Change, [this, &slider](Event& e) { curTime = globalSettings.duration * slider.GetValue(); });
		Pop();

		Make<ImageElement>()
			.SetImage(image)
			.SetAlphaBackgroundEnabled(true)
			.SetLayoutMode(ImageLayoutMode::Fill);
	}

	void PropertyUI()
	{
		Push<ScrollArea>();
		Push<StackTopDownLayoutElement>();

		if (selectedLayer)
		{
			Push<StackExpandLTRLayoutElement>();
			MakeWithText<Header>(Format("Properties - %s", selectedLayer->ItemName()));
			AddWrappedIn<Button>(New<IconElement>().SetDefaultStyle(DefaultIconStyle::Close))
				+ ui::AddEventHandler(EventType::Activate, [this](Event&) { selectedLayer = nullptr; AnimPatternChanged.Call(); });
			Pop();

			selectedLayer->FullUI();
		}
		else
		{
			MakeWithText<Header>("Global settings");

			globalSettings.UI();
		}

		Pop();
		Pop();
	}

	void CurveEditorUI()
	{
		Push<StackLTRLayoutElement>();
		MakeWithText<Header>("Curves");

		for (int i = 0; i < NUM_CURVES; i++)
		{
			char tmp[32];
			snprintf(tmp, 32, "%X", i);
			imRadioButton(curCurve, i, tmp, {}, ButtonStateToggleSkin());
		}
		Pop();

		auto* cv = UI_BUILD_ALLOC(Sequence01CurveView)();
		cv->curve = &globalSettings.curves[curCurve];
		auto& ced = Make<CurveEditorElement>();
		ced.curveView = cv;
		ced.viewport = { 0, 0, float(globalSettings.numSteps), 1 };
		ced.settings.snapX = 1;
	}

	void GenerateImage(Canvas& outCanvas)
	{
		int imageRes = globalSettings.imageRes;
		if (outCanvas.GetWidth() != imageRes ||
			outCanvas.GetHeight() != imageRes)
			outCanvas.SetSize(imageRes, imageRes);

		APMathExprDataSource meds;
		meds.t = curTime * globalSettings.numSteps / globalSettings.duration;
		meds.settings = &globalSettings;
		meds.EarlyEval();

		for (auto* rl : rootLayers)
			rl->EvalAll(&meds);

		auto* px = outCanvas.GetPixels();
		for (int y = 0; y < imageRes; y++)
		{
			for (int x = 0; x < imageRes; x++)
			{
				float s = 1;
				for (auto* rl : rootLayers)
					s *= 1 - rl->Sample(x + 0.5f, y + 0.5f);
				s = 1 - s;
				px[x + y * imageRes] = Color4f(s, 1).GetColor32();
			}
		}
	}

	struct NodeLoc
	{
		Array<APLayer*>* arr;
		size_t idx;

		APLayer* Get() const { return arr->At(idx); }
	};
	NodeLoc FindNode(TreePathRef path)
	{
		if (path.size() == 1)
			return { &rootLayers, path.Last() };
		else
		{
			auto parent = FindNode(path.WithoutLast());
			return { &(*parent.arr)[parent.idx]->children, path.Last() };
		}
	}

	void IterateChildren(TreePathRef path, IterationFunc&& fn) override
	{
		if (path.IsEmpty())
		{
			for (auto* node : rootLayers)
				fn(node);
		}
		else
		{
			auto loc = FindNode(path);
			for (auto* node : loc.Get()->children)
				fn(node);
		}
	}
	size_t GetChildCount(TreePathRef path) override
	{
		if (path.IsEmpty())
			return rootLayers.size();
		auto loc = FindNode(path);
		return loc.Get()->children.size();
	}

	void ClearSelection() override
	{
		selectedLayer = nullptr;
	}
	bool GetSelectionState(TreePathRef path) override
	{
		auto loc = FindNode(path);
		return loc.Get() == selectedLayer;
	}
	void SetSelectionState(TreePathRef path, bool sel) override
	{
		auto loc = FindNode(path);
		if (sel)
			selectedLayer = loc.Get();
	}

	uint32_t lastVer = ContextMenu::Get().GetVersion();
	void FillItemContextMenu(MenuItemCollection& mic, TreePathRef path) override
	{
		lastVer = mic.GetVersion();
		GenerateInsertMenu(mic, path);
	}
	void FillListContextMenu(MenuItemCollection& mic) override
	{
		if (lastVer != mic.GetVersion())
			GenerateInsertMenu(mic, {});
		//mic.basePriority += MenuItemCollection::BASE_ADVANCE;
		//mic.Add("Run script") = [this]() { RunScript(); };
	}
	void GenerateInsertMenu(MenuItemCollection& mic, TreePathRef path)
	{
		mic.basePriority += MenuItemCollection::BASE_ADVANCE;

		mic.Add("- Shapes -", true);
		mic.NewSection();
		mic.Add("Square") = [this, path]() { Insert(path, new APL_Shape_Square(globalSettings)); };
		mic.NewSection();

		mic.Add("- Modifiers -", true);
		mic.NewSection();
		mic.Add("Combiner") = [this, path]() { Insert(path, new APL_Combiner); };
		mic.NewSection();
	}
	void Insert(TreePathRef path, APLayer* node)
	{
		if (path.IsEmpty())
			rootLayers.Append(node);
		else
			FindNode(path).Get()->children.Append(node);
		AnimPatternChanged.Call();
	}

	void Remove(TreePathRef path) override
	{
		auto loc = FindNode(path);
		delete loc.Get();
		if (selectedLayer == loc.Get())
			selectedLayer = nullptr;
		loc.arr->RemoveAt(loc.idx);
	}
	void Duplicate(TreePathRef path) override
	{
		auto loc = FindNode(path);
		loc.arr->Append(loc.Get()->Clone());
	}
	void MoveTo(TreePathRef node, TreePathRef dest) override
	{
		auto srcLoc = FindNode(node);
		auto* srcNode = srcLoc.Get();
		srcLoc.arr->RemoveAt(srcLoc.idx);

		auto destLoc = FindNode(dest);
		destLoc.arr->InsertAt(destLoc.idx, srcNode);
	}
};


static float hsplit[1] = { 0.7f };
static float vsplit1[1] = { 0.6f };
static float vsplit2[1] = { 0.3f };

struct AnimatedPatternEditor : Buildable, AnimationRequester
{
	void OnAnimationFrame() override
	{
		animPattern.Update();

		Canvas canvas;
		animPattern.GenerateImage(canvas);
		animPattern.image = draw::ImageCreateFromCanvas(canvas);

		//GetNativeWindow()->InvalidateAll();
		Rebuild();
	}
	void Build() override
	{
		BuildMulticastDelegateAdd(AnimPatternChanged, [this]() { Rebuild(); });
		BeginAnimation();

		Push<SplitPane>().Init(Direction::Horizontal, hsplit);
		{
			Push<SplitPane>().Init(Direction::Vertical, vsplit1);
			{
				Push<EdgeSliceLayoutElement>();
				{
					animPattern.PreviewUI();
				}
				Pop();

				Push<EdgeSliceLayoutElement>();
				{
					animPattern.CurveEditorUI();
				}
				Pop();
			}
			Pop();

			Push<SplitPane>().Init(Direction::Vertical, vsplit2);
			{
				Push<EdgeSliceLayoutElement>();
				{
					animPattern.PropertyUI();
				}
				Pop();

				Push<StackTopDownLayoutElement>();
				{
					MakeWithText<Header>("Layers");

					auto& te = Make<TreeEditor>();
					te.SetTree(&animPattern);
					te.itemUICallback = [](TreeEditor* te, TreePathRef path, void* data)
					{
						static_cast<APLayer*>(data)->InlineUI();
					};
					te.HandleEvent(EventType::SelectionChange) = [this](Event&) { Rebuild(); };
				}
				Pop();
			}
			Pop();
		}
		Pop();
	}
	void OnEvent(Event& e) override
	{
		Buildable::OnEvent(e);
		if (e.type == EventType::IMChange)
			Rebuild();
	}

	AnimPattern animPattern;
};
void Demo_AnimatedPatternEditor()
{
	Make<AnimatedPatternEditor>();
}
