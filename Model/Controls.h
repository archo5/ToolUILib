
#pragma once

#include "Objects.h"
#include "System.h"


namespace ui {

struct Panel : UIElement
{
	Panel();
};

struct Header : UIElement
{
	void OnInit() override;
};

struct Button : UIElement
{
	Button();
};


struct StateButtonBase : UIElement
{
	StateButtonBase();

	virtual uint8_t GetState() const = 0;
};

struct StateToggleBase : StateButtonBase
{
	void OnEvent(UIEvent& e) override;

	virtual bool OnActivate() = 0;
};

struct StateToggle : StateToggleBase
{
	StateToggle* InitReadOnly(uint8_t state);
	StateToggle* InitEditable(uint8_t state, uint8_t maxNumStates = 2);

	void OnSerialize(IDataSerializer& s) override;

	uint8_t GetState() const override { return _state; }
	StateToggle* SetState(uint8_t state);
	bool OnActivate() override;

	uint8_t _state = 0;
	uint8_t _maxNumStates = 0;
};

template <class T>
struct CheckboxFlagT : StateToggleBase
{
	CheckboxFlagT* Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return this;
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

	T* _iptr = nullptr;
	T _value = {};
};

template <>
struct CheckboxFlagT<bool> : StateToggleBase
{
	CheckboxFlagT* Init(bool& bref)
	{
		_bptr = &bref;
		return this;
	}

	bool OnActivate() override
	{
		if (!_bptr)
			return false;
		*_bptr ^= true;
		return true;
	}
	uint8_t GetState() const override { return _bptr && *_bptr; }

	bool* _bptr = nullptr;
};

template <class T>
struct RadioButtonT : StateToggleBase
{
	RadioButtonT* Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return this;
	}

	uint8_t GetState() const override { return _iptr && *_iptr == _value; }
	bool OnActivate() override
	{
		if (!_iptr)
			return false;
		*_iptr = _value;
		return true;
	}

	T* _iptr = nullptr;
	T _value = {};
};

struct StateToggleVisualBase : UIElement
{
	void OnPaint() override;
};

struct CheckboxIcon : StateToggleVisualBase
{
	CheckboxIcon();
};

struct RadioButtonIcon : StateToggleVisualBase
{
	RadioButtonIcon();
};

struct TreeExpandIcon : StateToggleVisualBase
{
	TreeExpandIcon();
};

struct StateButtonSkin : StateToggleVisualBase
{
	StateButtonSkin();
};


struct ListBox : UIElement
{
	ListBox();
};

struct Selectable : UIElement
{
	void OnInit() override;
	Selectable* Init(bool selected)
	{
		SetFlag(UIObject_IsChecked, selected);
		return this;
	}
};

struct ProgressBar : UIElement
{
	ProgressBar();
	void OnPaint() override;

	style::BlockRef completionBarStyle;
	float progress = 0.5f;
};

struct FloatLimits
{
	double min = -DBL_MAX;
	double max = DBL_MAX;
	double step = 0;
};

struct Slider : UIElement
{
	void OnInit() override;
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

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
		HandleEvent(UIEventType::Change) = [this, &vp](UIEvent&) { vp = (float)GetValue(); };
		return *this;
	}
	Slider& Init(double& vp, FloatLimits limits = { 0, 1, 0 })
	{
		SetLimits(limits);
		SetValue(vp);
		HandleEvent(UIEventType::Change) = [this, &vp](UIEvent&) { vp = GetValue(); };
		return *this;
	}

	style::Accessor GetTrackStyle() { return style::Accessor(trackStyle, this); }
	style::Accessor GetTrackFillStyle() { return style::Accessor(trackFillStyle, this); }
	style::Accessor GetThumbStyle() { return style::Accessor(thumbStyle, this); }

	double _value = 0;
	FloatLimits _limits = { 0, 1, 0 };

	style::BlockRef trackStyle;
	style::BlockRef trackFillStyle;
	style::BlockRef thumbStyle;

	float _mxoff = 0;
};


struct Property : UIElement
{
	struct Scope
	{
		Scope(UIContainer* c, const char* lblstr = nullptr) : ctx(c)
		{
			Begin(ctx);
			label = lblstr ? &Label(ctx, lblstr) : nullptr;
		}
		~Scope()
		{
			End(ctx);
		}

		UIContainer* ctx;
		UIObject* label;
	};

	Property();
	static void Begin(UIContainer* ctx, const char* label = nullptr);
	static void End(UIContainer* ctx);

	static UIObject& Label(UIContainer* ctx, const char* label);
	static UIObject& MinLabel(UIContainer* ctx, const char* label);

	static void EditFloat(UIContainer* ctx, const char* label, float* v);
	static void EditFloat2(UIContainer* ctx, const char* label, float* v);
	static void EditFloat3(UIContainer* ctx, const char* label, float* v);
	static void EditFloat4(UIContainer* ctx, const char* label, float* v);
};

struct PropertyList : UIElement
{
	void OnInit() override;
	UIRect CalcPaddingRect(const UIRect& expTgtRect) override;

	style::Accessor GetDefaultLabelStyle() { return style::Accessor(_defaultLabelStyle, this); }
	void SetDefaultLabelStyle(const style::BlockRef& s) { _defaultLabelStyle = s; }

	style::Coord splitPos = style::Coord::Percent(40);
	style::Coord minSplitPos = 0;

	style::BlockRef _defaultLabelStyle;
	float _calcSplitX = 0;
};

struct LabeledProperty : UIElement
{
	struct Scope
	{
		Scope(UIContainer* c, const char* lblstr = nullptr) : ctx(c)
		{
			label = Begin(ctx, lblstr);
		}
		~Scope()
		{
			End(ctx);
		}

		UIContainer* ctx;
		UIObject* label;
	};

	static LabeledProperty* Begin(UIContainer* ctx, const char* label = nullptr);
	static void End(UIContainer* ctx);

	void OnInit() override;
	void OnPaint() override;
	UIRect CalcPaddingRect(const UIRect& expTgtRect) override;

	StringView GetText() const { return _labelText; }
	LabeledProperty& SetText(StringView text);

	bool IsBrief() const { return _isBrief; }
	LabeledProperty& SetBrief(bool b) { _isBrief = b; return *this; }

	style::Accessor GetLabelStyle() { return style::Accessor(_labelStyle, this); }
	LabeledProperty& SetLabelStyle(const style::BlockRef& s) { _labelStyle = s; return *this; }

	style::BlockRef _labelStyle;
	std::string _labelText;
	PropertyList* _propList = nullptr;
	bool _isBrief = false;
};


struct SplitPane : UIElement
{
	SplitPane();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override;
	void OnSerialize(IDataSerializer& s) override;
	Range<float> GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type, bool forParentLayout) override;
	Range<float> GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type, bool forParentLayout) override;

	SplitPane* SetSplits(std::initializer_list<float> splits, bool firstTimeOnly = true);
	SplitPane* SetDirection(bool vertical);

	style::BlockRef vertSepStyle; // for horizontal splitting
	style::BlockRef horSepStyle; // for vertical splitting
	std::vector<float> _splits;
	bool _splitsSet = false;
	bool _verticalSplit = false;
	SubUI<uint16_t> _splitUI;
	float _dragOff = 0;
};

struct ScrollbarData
{
	UIObject* owner;
	UIRect rect;
	float viewportSize;
	float contentSize;
	float& contentOff;
};

struct ScrollbarV
{
	ScrollbarV();
	style::Coord GetWidth();
	UIRect GetThumbRect(const ScrollbarData& info);
	void OnPaint(const ScrollbarData& info);
	void OnEvent(const ScrollbarData& info, UIEvent& e);

	style::BlockRef trackVStyle;
	style::BlockRef thumbVStyle;
	SubUI<int> uiState;
	float dragStartContentOff;
	float dragStartCursorPos;
};

class ScrollArea : public UIElement
{
public:
	ScrollArea();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override;
	void OnSerialize(IDataSerializer& s) override;

	float yoff = 23;
	ScrollbarV sbv;
};

class TabGroup : public UIElement
{
public:
	TabGroup();

	struct TabButtonBase* _activeBtn = nullptr;
};

class TabButtonList : public UIElement
{
public:
	TabButtonList();
	void OnPaint() override;
};

struct TabButtonBase : UIElement
{
	TabButtonBase();
	void OnDestroy() override;
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	virtual void OnSelect() {}
	virtual bool IsSelected() const { return !!(flags & UIObject_IsChecked); }
};

struct TabButton : TabButtonBase
{
	TabButton* Init(bool selected)
	{
		SetFlag(UIObject_IsChecked, selected);
		if (selected)
			if (auto* g = FindParentOfType<TabGroup>())
				g->_activeBtn = this;
		return this;
	}
	// implement Activate event to set source state
};

template <class T>
struct TabButtonT : TabButtonBase
{
	TabButtonT* Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		if (iref == value)
			if (auto* g = FindParentOfType<TabGroup>())
				g->_activeBtn = this;
		return this;
	}

	void OnSelect() override
	{
		if (!_iptr)
			return;
		*_iptr = _value;
		RerenderNode();
	}
	bool IsSelected() const override { return _iptr && *_iptr == _value; }

	T* _iptr = nullptr;
	T _value = {};
};

class TabPanel : public UIElement
{
public:
	TabPanel();
	void OnPaint() override;
};

struct Textbox : UIElement
{
	Textbox();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnSerialize(IDataSerializer& s) override;

	bool IsLongSelection() const { return startCursor != endCursor; }
	StringView GetSelectedText() const;
	void EnterText(const char* str);
	void EraseSelection();

	int _FindCursorPos(float vpx);

	const std::string& GetText() const { return _text; }
	Textbox& SetText(StringView s);
	Textbox& SetPlaceholder(StringView s);

	Textbox& Init(float& val);
	template <size_t N> Textbox& Init(char (&val)[N])
	{
		SetText(val);
		HandleEvent(UIEventType::Change) = [this, &val](UIEvent&)
		{
			auto& t = GetText();
			size_t s = min(t.size(), N - 1);
			memcpy(val, t.c_str(), s);
			val[s] = 0;
		};
		return *this;
	}

	std::string _text;
	std::string _placeholder;
	int startCursor = 0;
	int endCursor = 0;
	bool showCaretState = false;
	float accumulator = 0;
};

struct TextboxPlaceholder : Modifier
{
	StringView placeholder;

	TextboxPlaceholder(StringView pch) : placeholder(pch) {}

	void Apply(UIObject* obj) const override
	{
		if (auto* tb = dynamic_cast<Textbox*>(obj))
			tb->SetPlaceholder(placeholder);
	}
};

struct CollapsibleTreeNode : UIElement
{
	CollapsibleTreeNode();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnSerialize(IDataSerializer& s) override;

	bool open = false;
	bool _hovered = false;
};


struct BackgroundBlocker : UIElement
{
	void OnInit() override;
	void OnEvent(UIEvent& e) override;
	void OnButton();

	style::RectAnchoredPlacement _fullScreenPlacement;
};


struct DropdownMenu : ui::Node
{
	void Render(UIContainer* ctx) override;
	void OnEvent(UIEvent& e) override;

	virtual void OnBuildButton(UIContainer* ctx);
	virtual void OnBuildMenuWithLayout(UIContainer* ctx);
	virtual UIObject& OnBuildMenu(UIContainer* ctx);

	virtual void OnBuildButtonContents(UIContainer* ctx) = 0;
	virtual void OnBuildMenuContents(UIContainer* ctx) = 0;
};


struct OptionList
{
	typedef void ElementFunc(const void* ptr, uintptr_t id);

	virtual void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) = 0;
	virtual void BuildElement(UIContainer* ctx, const void* ptr, uintptr_t id, bool list) = 0;
};

struct CStrOptionList : OptionList
{
	void BuildElement(UIContainer* ctx, const void* ptr, uintptr_t id, bool list) override;
};

struct ZeroSepCStrOptionList : CStrOptionList
{
	const char* str = nullptr;
	size_t size = SIZE_MAX;

	ZeroSepCStrOptionList(const char* s) : str(s) {}
	ZeroSepCStrOptionList(size_t sz, const char* s) : str(s), size(sz) {}

	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
};

struct NullTerminated {};

struct CStrArrayOptionList : CStrOptionList
{
	const char** arr = nullptr;
	size_t size = SIZE_MAX;

	CStrArrayOptionList(const char** a, NullTerminated) : arr(a) {}
	template <size_t N>
	CStrArrayOptionList(const char* (&a)[N]) : arr(a), size(N) {}
	CStrArrayOptionList(size_t sz, const char** a) : arr(a), size(sz) {}

	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
};


struct DropdownMenuList : DropdownMenu
{
	static constexpr bool Persistent = false;

	OptionList* _options = nullptr;
	uintptr_t _selected = UINTPTR_MAX;

	DropdownMenuList& SetOptions(OptionList* o) { _options = o; return *this; }
	DropdownMenuList& SetSelected(uintptr_t s) { _selected = s; return *this; }
	uintptr_t GetSelected() { return _selected; }

	void OnSerialize(IDataSerializer& s) override;
	void OnBuildButtonContents(UIContainer* ctx) override;
	void OnBuildMenuContents(UIContainer* ctx) override;

	virtual void OnBuildEmptyButtonContents(UIContainer* ctx);
	virtual void OnBuildMenuElement(UIContainer* ctx, const void* ptr, uintptr_t id);
};

namespace imm {

template <class MT, class T> bool DropdownMenuListCustom(UIContainer* ctx, T& val, OptionList* ol, ModInitList mods = {})
{
	auto* ddml = ctx->Make<MT>();
	ddml->SetOptions(ol);
	for (auto& mod : mods)
		mod->Apply(ddml);

	bool edited = false;
	if (ddml->flags & UIObject_IsEdited)
	{
		val = T(ddml->GetSelected());
		ddml->flags &= ~UIObject_IsEdited;
		ddml->_OnIMChange();
		edited = true;
	}
	else
		ddml->SetSelected(uintptr_t(val));

	ddml->HandleEvent(UIEventType::Commit) = [ddml](UIEvent& e)
	{
		if (e.target != ddml)
			return;
		e.current->flags |= UIObject_IsEdited;
		e.current->RerenderContainerNode();
	};

	return edited;
}
template <class T> bool DropdownMenuList(UIContainer* ctx, T& val, OptionList* ol, ModInitList mods = {})
{
	return DropdownMenuListCustom<ui::DropdownMenuList>(ctx, val, ol, mods);
}

template <class MT, class T> bool PropDropdownMenuListCustom(UIContainer* ctx, const char* label, T& val, OptionList* ol, ModInitList mods = {})
{
	LabeledProperty::Scope ps(ctx, label);
	return DropdownMenuListCustom<MT, T>(ctx, val, ol, mods);
}
template <class T> bool PropDropdownMenuList(UIContainer* ctx, const char* label, T& val, OptionList* ol, ModInitList mods = {})
{
	LabeledProperty::Scope ps(ctx, label);
	return DropdownMenuList<T>(ctx, val, ol, mods);
}

} // imm


struct OverlayInfoPlacement : style::Placement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) override;
};

struct OverlayInfoFrame : UIElement
{
	OverlayInfoFrame();
	OverlayInfoPlacement placement;
};

struct TooltipFrame : OverlayInfoFrame
{
	TooltipFrame();
};

struct DragDropDataFrame : OverlayInfoFrame
{
	DragDropDataFrame();
};

struct DefaultOverlayRenderer : Node
{
	void Render(UIContainer* ctx) override;

	bool drawTooltip = true;
	bool drawDragDrop = true;
};

} // ui
