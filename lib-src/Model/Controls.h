
#pragma once

#include "Objects.h"
#include "System.h"
#include "../Core/Optional.h"
#include "../Core/StaticID.h"


namespace ui {

struct FrameStyle
{
	static constexpr const char* NAME = "FrameStyle";

	AABB2f padding;
	PainterHandle backgroundPainter;
	FontSettings font;
	Optional<Color4b> textColor;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

template <class T> struct PaddingStyleMixin
{
private:
	UI_FORCEINLINE AABB2f& _GetPadding() { return static_cast<T*>(this)->GetPadding(); }
public:
	UI_FORCEINLINE const AABB2f& GetPadding() const { return const_cast<PaddingStyleMixin*>(this)->_GetPadding(); }

	UI_FORCEINLINE T& SetPadding(float w) { _GetPadding() = UIRect::UniformBorder(w); return *static_cast<T*>(this); }
	UI_FORCEINLINE T& SetPadding(float x, float y) { _GetPadding() = { x, y, x, y }; return *static_cast<T*>(this); }
	UI_FORCEINLINE T& SetPadding(float l, float t, float r, float b) { _GetPadding() = { l, t, r, b }; return *static_cast<T*>(this); }
	UI_FORCEINLINE T& SetPadding(const UIRect& r) { _GetPadding() = r; return *static_cast<T*>(this); }

	UI_FORCEINLINE T& SetPaddingLeft(float p) { _GetPadding().x0 = p; return *static_cast<T*>(this); }
	UI_FORCEINLINE T& SetPaddingTop(float p) { _GetPadding().y0 = p; return *static_cast<T*>(this); }
	UI_FORCEINLINE T& SetPaddingRight(float p) { _GetPadding().x1 = p; return *static_cast<T*>(this); }
	UI_FORCEINLINE T& SetPaddingBottom(float p) { _GetPadding().y1 = p; return *static_cast<T*>(this); }
};

enum class DefaultFrameStyle
{
	Label,
	Header,
	GroupBox,
	Button,
	Selectable,
	DropdownButton,
	ListBox,
	TextBox,
	ProcGraphNode,
	Checkerboard,
};

struct FrameElement : UIObjectSingleChild, PaddingStyleMixin<FrameElement>
{
	// avoiding naming conflicts with elements who might want to reuse FrameElement but add separate styling for the contents
	FrameStyle frameStyle;

	// for PaddingStyleMixin
	UI_FORCEINLINE AABB2f& GetPadding() { return frameStyle.padding; }

	AABB2f GetContentRect() { return GetFinalRect().ShrinkBy(frameStyle.padding); }

	void OnReset() override;

	ContentPaintAdvice PaintFrame() { return PaintFrame(this); }
	ContentPaintAdvice PaintFrame(const PaintInfo& info);
	void OnPaint(const UIPaintContext& ctx) override;
	void PaintChildren(const UIPaintContext& ctx, const ContentPaintAdvice& cpa);

	const FontSettings* _GetFontSettings() const override;
	Size2f GetReducedContainerSize(Size2f size);
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	FrameElement& RemoveFrameStyle();
	FrameElement& SetFrameStyle(const StaticID<FrameStyle>& id);
	FrameElement& SetDefaultFrameStyle(DefaultFrameStyle style);
};

struct IconStyle
{
	static constexpr const char* NAME = "IconStyle";

	Size2f size;
	PainterHandle painter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

enum class DefaultIconStyle
{
	Checkbox,
	RadioButton,
	TreeExpand,

	Add,
	Close,
	Delete,
	Play,
	Pause,
	Stop,
};

struct IconElement : UIObjectNoChildren
{
	IconStyle style;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	IconElement& SetStyle(const StaticID<IconStyle>& id);
	IconElement& SetDefaultStyle(DefaultIconStyle style);
};

struct LabelFrame : FrameElement
{
	void OnReset() override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
};

struct Header : FrameElement
{
	void OnReset() override;
};

struct Button : FrameElement
{
	void OnReset() override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


struct StateButtonBase : WrapperElement
{
	void OnReset() override;

	virtual uint8_t GetState() const = 0;
};

struct StateToggleBase : StateButtonBase
{
	void OnEvent(Event& e) override;

	virtual bool OnActivate() = 0;
};

struct StateToggle : StateToggleBase
{
	uint8_t _state = 0;
	uint8_t _maxNumStates = 0;

	StateToggle& InitReadOnly(uint8_t state);
	StateToggle& InitEditable(uint8_t state, uint8_t maxNumStates = 2);

	uint8_t GetState() const override { return _state; }
	StateToggle& SetState(uint8_t state);
	bool OnActivate() override;
};

template <class T>
struct CheckboxFlagT : StateToggleBase
{
	T* _iptr = nullptr;
	T _value = {};

	CheckboxFlagT& Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return *this;
	}

	uint8_t GetState() const override { return _iptr && (*_iptr & _value) == _value; }
	bool OnActivate() override
	{
		if (!_iptr)
			return false;
		if (!GetState())
			*_iptr |= _value;
		else
			*_iptr &= T(~_value);
		return true;
	}

	void OnReset() override
	{
		StateToggleBase::OnReset();

		_iptr = nullptr;
		_value = {};
	}
};

template <>
struct CheckboxFlagT<bool> : StateToggleBase
{
	bool* _bptr = nullptr;

	CheckboxFlagT& Init(bool& bref)
	{
		_bptr = &bref;
		return *this;
	}

	bool OnActivate() override
	{
		if (!_bptr)
			return false;
		*_bptr ^= true;
		return true;
	}
	uint8_t GetState() const override { return _bptr && *_bptr; }

	void OnReset() override
	{
		StateToggleBase::OnReset();

		_bptr = nullptr;
	}
};

template <class T>
struct RadioButtonT : StateToggleBase
{
	T* _iptr = nullptr;
	T _value = {};

	RadioButtonT& Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return *this;
	}

	uint8_t GetState() const override { return _iptr && *_iptr == _value; }
	bool OnActivate() override
	{
		if (!_iptr)
			return false;
		*_iptr = _value;
		return true;
	}

	void OnReset() override
	{
		StateToggleBase::OnReset();

		_iptr = nullptr;
		_value = {};
	}
};

struct StateToggleIconBase : IconElement
{
	void OnPaint(const UIPaintContext& ctx) override;
};

struct StateToggleFrameBase : FrameElement
{
	void OnPaint(const UIPaintContext& ctx) override;
};

struct CheckboxIcon : StateToggleIconBase
{
	void OnReset() override;
};

struct RadioButtonIcon : StateToggleIconBase
{
	void OnReset() override;
};

struct TreeExpandIcon : StateToggleIconBase
{
	void OnReset() override;
};

struct StateButtonSkin : StateToggleFrameBase
{
	void OnReset() override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


struct ListBoxFrame : FrameElement
{
	void OnReset() override;
};

struct Selectable : FrameElement
{
	void OnReset() override;
	Selectable& Init(bool selected)
	{
		SetFlag(UIObject_IsChecked, selected);
		return *this;
	}
};


struct ProgressBarStyle
{
	static constexpr const char* NAME = "ProgressBarStyle";

	AABB2f padding;
	AABB2f fillMargin;
	PainterHandle backgroundPainter;
	PainterHandle fillPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct ProgressBar : UIObjectSingleChild
{
	ProgressBarStyle style;
	float progress = 0.5f;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;

	Size2f GetReducedContainerSize(Size2f size);
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
};


struct FloatLimits
{
	double min = -DBL_MAX;
	double max = DBL_MAX;
	double step = 0;
};

struct SliderStyle
{
	static constexpr const char* NAME = "SliderStyle";

	float minSize = 20;
	float trackSize = 0;
	AABB2f trackMargin = AABB2f::UniformBorder(0);
	AABB2f trackFillMargin = AABB2f::UniformBorder(0);
	AABB2f thumbExtend = AABB2f::UniformBorder(0);
	PainterHandle backgroundPainter;
	PainterHandle trackBasePainter;
	PainterHandle trackFillPainter;
	PainterHandle thumbPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct Slider : UIObjectNoChildren
{
	SliderStyle style;

	double _value = 0;
	FloatLimits _limits = { 0, 1, 0 };
	float _mxoff = 0;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(style.minSize); }

	double PosToQ(double x);
	double QToValue(double q);
	double ValueToQ(double v);
	double PosToValue(double x) { return QToValue(PosToQ(x)); }

	double GetValue() const { return _value; }
	Slider& SetValue(double v) { _value = v; return *this; }
	FloatLimits GetLimits() const { return _limits; }
	Slider& SetLimits(FloatLimits limits) { _limits = limits; return *this; }

	Slider& Init(float& vp, FloatLimits limits = { 0, 1, 0 })
	{
		SetLimits(limits);
		SetValue(vp);
		HandleEvent(EventType::Change) = [this, &vp](Event&) { vp = (float)GetValue(); };
		return *this;
	}
	Slider& Init(double& vp, FloatLimits limits = { 0, 1, 0 })
	{
		SetLimits(limits);
		SetValue(vp);
		HandleEvent(EventType::Change) = [this, &vp](Event&) { vp = GetValue(); };
		return *this;
	}
};


struct PropertyList : HFillVWrapElement // TODO should this be a stack layout?
{
	void OnReset() override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	Coord splitPos = Coord::Percent(40);
	Coord minSplitPos = 0;
	FrameStyle defaultLabelStyle;

	float _calcSplitX = 0;
};

struct LabeledProperty : HFillVWrapElement
{
	enum ContentLayoutType
	{
		OneElement,
		StackExpandLTR,
	};

	struct Scope
	{
		ContentLayoutType layoutType;
		LabeledProperty* label;

		Scope(StringView lblstr = {}, ContentLayoutType layout = StackExpandLTR) : layoutType(layout)
		{
			label = &Begin(lblstr, layoutType);
		}
		~Scope()
		{
			End(layoutType);
		}
	};

	static LabeledProperty& Begin(StringView label = {}, ContentLayoutType layout = StackExpandLTR);
	static void End(ContentLayoutType layout = StackExpandLTR);

	LabeledProperty()
	{
		flags |= UIObject_NeedsTreeUpdates;
	}

	void OnReset() override;
	void OnEnterTree() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
	void OnEvent(Event& e) override;

	StringView GetText() const { return _labelText; }
	LabeledProperty& SetText(StringView text);

	bool IsBrief() const { return _isBrief; }
	LabeledProperty& SetBrief(bool b) { _isBrief = b; return *this; }

	const FrameStyle& FindCurrentLabelStyle() const;
	const FrameStyle& GetLabelStyle();
	LabeledProperty& SetLabelStyle(const FrameStyle& s);

	LabeledProperty& SetLabelBold(bool b = true) { return SetLabelFontWeight(b ? FontWeight::Bold : FontWeight::Normal); }
	LabeledProperty& SetLabelFontWeight(FontWeight w)
	{
		_labelStyle.font.weight = w;
		_preferOwnLabelStyle = true;
		return *this;
	}

	float GetLastSeparatorPositionX() const { return _lastSepX; }

	FrameStyle _labelStyle;
	std::string _labelText;
	PropertyList* _propList = nullptr;
	float _lastSepX = 0;
	bool _isBrief = false;
	bool _preferOwnLabelStyle = false;
	Tooltip::BuildFunc _tooltipBuildFunc;
};


struct ScrollbarData
{
	UIObject* owner;
	UIRect rect;
	float viewportSize;
	float contentSize;
	float& contentOff;
};

struct ScrollbarStyle
{
	static constexpr const char* NAME = "ScrollbarStyle";

	float width = 20;
	AABB2f trackFillMargin = {};
	float thumbMinLength = 0;
	PainterHandle trackPainter;
	PainterHandle thumbPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct ScrollbarV
{
	ScrollbarStyle style;
	SubUI<int> uiState;
	float dragStartContentOff = 0;
	float dragStartCursorPos = 0;

	ScrollbarV();
	void OnReset();
	Coord GetWidth();
	UIRect GetThumbRect(const ScrollbarData& info);
	void OnPaint(const ScrollbarData& info);
	bool OnEvent(const ScrollbarData& info, Event& e);
};

struct ScrollArea : WrapperElement
{
	Size2f estContentSize = {};
	float yoff = 0;
	ScrollbarV sbv;

	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;
	void OnReset() override;
};


struct BackgroundBlocker : FillerElement
{
	Color4b _color = Color4b::Zero();

	BackgroundBlocker& SetColor(Color4b c) { _color = c; return *this; }

	void OnReset() override;
	void OnEvent(Event& e) override;
	void OnPaint(const UIPaintContext& ctx) override;

	void OnLayout(const UIRect& rect, LayoutInfo info) override;
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override { return Rangef::AtLeast(0); }

	void OnButton();
};


struct DropdownMenu : Buildable
{
	bool enableTooltip = true;

	void Build() override;
	void OnEvent(Event& e) override;

	virtual void OnBuildButton();
	virtual void OnBuildMenuWithLayout();
	virtual UIObject& OnBuildMenu();
	virtual void OnKeyActionEvent(ui::Event& e) {}

	virtual void OnBuildButtonContents() = 0;
	virtual void OnBuildMenuContents() = 0;
};


struct OptionList
{
	typedef void ElementFunc(const void* ptr, uintptr_t id);

	virtual void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) = 0;
	virtual uintptr_t FindAdjacent(uintptr_t start, int delta) = 0;
	virtual void BuildElement(const void* ptr, uintptr_t id, bool list) = 0;
	virtual void BuildEmptyButtonContents();
};

struct CStrOptionList : OptionList
{
	struct CountInfo
	{
		size_t count;
		bool hasEmpty;
	};
	uintptr_t emptyValue = UINTPTR_MAX;

	void BuildElement(const void* ptr, uintptr_t id, bool list) override;
	uintptr_t FindAdjacent(uintptr_t start, int delta) override;
	virtual CountInfo GetCount() const = 0;
};

struct ZeroSepCStrOptionList : CStrOptionList
{
	const char* str = nullptr;
	size_t size = SIZE_MAX;
	const char* emptyStr = nullptr;

	ZeroSepCStrOptionList(const char* s, const char* empty = nullptr);
	ZeroSepCStrOptionList(size_t sz, const char* s, const char* empty = nullptr) : str(s), size(sz), emptyStr(empty) {}

	CountInfo GetCount() const override { return { size, !!emptyStr }; }
	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
	void BuildEmptyButtonContents() override;
};

struct NullTerminated {};

struct CStrArrayOptionList : CStrOptionList
{
	const char* const* arr = nullptr;
	size_t size = SIZE_MAX;
	const char* emptyStr = nullptr;

	CStrArrayOptionList(NullTerminated, const char* const* a, const char* empty = nullptr);
	CStrArrayOptionList(size_t sz, const char* const* a, const char* empty = nullptr);
	template <size_t N>
	UI_FORCEINLINE CStrArrayOptionList(const char* const (&a)[N], const char* empty = nullptr) : CStrArrayOptionList(N, a, empty) {}

	CountInfo GetCount() const override { return { size, !!emptyStr }; }
	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
	void BuildEmptyButtonContents() override;
};

struct StringArrayOptionList : CStrOptionList
{
	Array<std::string> options;
	Optional<std::string> emptyStr;

	CountInfo GetCount() const override { return { options.Size(), emptyStr.HasValue() }; }
	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
	void BuildEmptyButtonContents() override;
};


struct DropdownMenuList : DropdownMenu
{
	OptionList* _options = nullptr;
	uintptr_t _selected = UINTPTR_MAX;

	DropdownMenuList& SetOptions(OptionList* o) { _options = o; return *this; }
	DropdownMenuList& SetSelected(uintptr_t s) { _selected = s; return *this; }
	uintptr_t GetSelected() { return _selected; }

	void OnReset() override;
	void OnBuildButtonContents() override;
	void OnBuildMenuContents() override;
	void OnKeyActionEvent(ui::Event& e) override;

	virtual void OnBuildEmptyButtonContents();
	virtual void OnBuildMenuElement(const void* ptr, uintptr_t id);
};

namespace imm {

template <class MT, class T> imCtrlInfoT<MT> imDropdownMenuListCustom(T& val, OptionList* ol, ModInitList mods = {})
{
	auto& ddml = Make<MT>();
	ddml.SetOptions(ol);
	for (auto& mod : mods)
		mod->Apply(&ddml);

	if (ddml.flags & UIObject_AfterIMEdit)
	{
		ddml._OnIMChange();
		ddml.flags &= ~UIObject_AfterIMEdit;
	}
	bool edited = false;
	if (ddml.flags & UIObject_IsEdited)
	{
		val = T(ddml.GetSelected());
		ddml.flags &= ~UIObject_IsEdited;
		ddml.flags |= UIObject_AfterIMEdit;
		ddml.RebuildContainer();
		edited = true;
	}
	else
		ddml.SetSelected(uintptr_t(val));

	ddml.HandleEvent(EventType::Commit) = [&ddml](Event& e)
	{
		if (e.target != &ddml)
			return;
		e.current->flags |= UIObject_IsEdited;
		e.current->RebuildContainer();
	};

	return { edited, &ddml };
}
template <class T> imCtrlInfoT<DropdownMenuList> imDropdownMenuList(T& val, OptionList* ol, ModInitList mods = {})
{
	return imDropdownMenuListCustom<DropdownMenuList>(val, ol, mods);
}

} // imm


struct ModalWindowContainer
{
	Array<UIObject*> modalWindows;
	MulticastDelegate<> onChangeModalWindows;

	ModalWindowContainer() {}
	ModalWindowContainer(const ModalWindowContainer&) = delete;
	ModalWindowContainer(ModalWindowContainer&&) = delete;
	ModalWindowContainer& operator = (const ModalWindowContainer&) = delete;
	ModalWindowContainer& operator = (ModalWindowContainer&&) = delete;

	~ModalWindowContainer() { DestroyAll(); }

	void AddModalWindow(UIObject* mwo);
	void DestroyAll();
	void BuildUI();

	static void CloseModalWindow(UIObject* root);
};

struct ModalWindowOpenInfo
{
	NativeWindowBase* targetWindow;
	UIObject* modalWindowContents;

	UIObject* TryGetModalWindowContents(NativeWindowBase* myWindow);
};

namespace _ {
extern MulticastDelegate<ModalWindowOpenInfo&> OnModalWindowOpened;
} // _

struct ModalWindowRenderer : Buildable
{
	ui::ModalWindowContainer _mwc;

	void OnReset() override;
	void Build() override;
};

bool OpenModalWindow(NativeWindowBase* inNativeWindow, UIObject* modalWindowContents);


struct OverlayInfoPlacement : IPlacement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const override;
};

struct DefaultOverlayBuilder : Buildable
{
	bool drawTooltip = true;
	bool drawDragDrop = true;
	OverlayInfoPlacement placement;

	void OnReset() override
	{
		Buildable::OnReset();

		flags |= UIObject_HitTestPassthrough;

		drawTooltip = true;
		drawDragDrop = true;
	}

	void Build() override;
};

} // ui
