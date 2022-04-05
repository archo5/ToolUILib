
#pragma once

#include "Objects.h"
#include "System.h"


namespace ui {

struct Panel : UIElement
{
	void OnReset() override;
};

struct Header : UIElement
{
	void OnReset() override;
};

struct Button : UIElement
{
	void OnReset() override;
};


struct StateButtonBase : UIElement
{
	void OnReset() override;

	virtual uint8_t GetState() const = 0;

	ui::Rangef GetFullEstimatedWidth(const ui::Size2f& containerSize, ui::EstSizeType type, bool forParentLayout = true) override
	{
		return firstChild->GetFullEstimatedWidth(containerSize, type, forParentLayout);
	}
	ui::Rangef GetFullEstimatedHeight(const ui::Size2f& containerSize, ui::EstSizeType type, bool forParentLayout = true) override
	{
		return firstChild->GetFullEstimatedHeight(containerSize, type, forParentLayout);
	}
	void OnLayout(const ui::UIRect& rect, const ui::Size2f& containerSize) override
	{
		firstChild->PerformLayout(rect, containerSize);
		finalRectCP = finalRectC = firstChild->finalRectCP;
	}
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

struct StateToggleVisualBase : UIElement
{
	void OnPaint(const UIPaintContext& ctx) override;
};

struct CheckboxIcon : StateToggleVisualBase
{
	void OnReset() override;
};

struct RadioButtonIcon : StateToggleVisualBase
{
	void OnReset() override;
};

struct TreeExpandIcon : StateToggleVisualBase
{
	void OnReset() override;
};

struct StateButtonSkin : StateToggleVisualBase
{
	void OnReset() override;
};


struct ListBox : UIElement
{
	void OnReset() override;
};

struct Selectable : UIElement
{
	void OnReset() override;
	Selectable& Init(bool selected)
	{
		SetFlag(UIObject_IsChecked, selected);
		return *this;
	}
};

struct ProgressBar : UIElement
{
	StyleBlockRef completionBarStyle;
	float progress = 0.5f;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
};

struct FloatLimits
{
	double min = -DBL_MAX;
	double max = DBL_MAX;
	double step = 0;
};

struct Slider : UIElement
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

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

	StyleAccessor GetTrackStyle() { return StyleAccessor(trackStyle, this); }
	StyleAccessor GetTrackFillStyle() { return StyleAccessor(trackFillStyle, this); }
	StyleAccessor GetThumbStyle() { return StyleAccessor(thumbStyle, this); }

	double _value = 0;
	FloatLimits _limits = { 0, 1, 0 };

	StyleBlockRef trackStyle;
	StyleBlockRef trackFillStyle;
	StyleBlockRef thumbStyle;

	float _mxoff = 0;
};


struct Property : UIElement
{
	struct Scope
	{
		Scope(const char* lblstr = nullptr)
		{
			Begin();
			label = lblstr ? &Label(lblstr) : nullptr;
		}
		~Scope()
		{
			End();
		}

		UIObject* label;
	};

	void OnReset() override;
	static void Begin(const char* label = nullptr);
	static void End();

	static UIObject& Label(const char* label);
	static UIObject& MinLabel(const char* label);
};

struct PropertyList : UIElement
{
	void OnReset() override;
	UIRect CalcPaddingRect(const UIRect& expTgtRect) override;

	StyleAccessor GetDefaultLabelStyle() { return StyleAccessor(_defaultLabelStyle, this); }
	void SetDefaultLabelStyle(const StyleBlockRef& s) { _defaultLabelStyle = s; }

	Coord splitPos = Coord::Percent(40);
	Coord minSplitPos = 0;

	StyleBlockRef _defaultLabelStyle;
	float _calcSplitX = 0;
};

struct LabeledProperty : UIElement
{
	struct Scope
	{
		Scope(const char* lblstr = nullptr)
		{
			label = &Begin(lblstr);
		}
		~Scope()
		{
			End();
		}

		UIObject* label;
	};

	static LabeledProperty& Begin(const char* label = nullptr);
	static void End();

	LabeledProperty()
	{
		flags |= UIObject_NeedsTreeUpdates;
	}

	void OnReset() override;
	void OnEnterTree() override;
	void OnPaint(const UIPaintContext& ctx) override;
	UIRect CalcPaddingRect(const UIRect& expTgtRect) override;

	StringView GetText() const { return _labelText; }
	LabeledProperty& SetText(StringView text);

	bool IsBrief() const { return _isBrief; }
	LabeledProperty& SetBrief(bool b) { _isBrief = b; return *this; }

	StyleBlock* FindCurrentLabelStyle() const;
	StyleAccessor GetLabelStyle();
	LabeledProperty& SetLabelStyle(const StyleBlockRef& s) { _labelStyle = s; return *this; }

	StyleBlockRef _labelStyle;
	std::string _labelText;
	PropertyList* _propList = nullptr;
	bool _isBrief = false;
};


struct SplitPane : UIElement
{
	SplitPane();
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
	void OnLayout(const UIRect& rect, const Size2f& containerSize) override;
	void OnReset() override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	void CustomAppendChild(UIObject* obj) override;
	UIObject* FindLastChildContainingPos(Point2f pos) const override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	Rangef GetFullEstimatedWidth(const Size2f& containerSize, EstSizeType type, bool forParentLayout) override;
	Rangef GetFullEstimatedHeight(const Size2f& containerSize, EstSizeType type, bool forParentLayout) override;

	SplitPane* SetSplits(std::initializer_list<float> splits, bool firstTimeOnly = true);
	SplitPane* SetDirection(bool vertical);

	StyleBlockRef vertSepStyle; // for horizontal splitting
	StyleBlockRef horSepStyle; // for vertical splitting
	std::vector<UIObject*> _children;
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
	void OnReset();
	Coord GetWidth();
	UIRect GetThumbRect(const ScrollbarData& info);
	void OnPaint(const ScrollbarData& info);
	bool OnEvent(const ScrollbarData& info, Event& e);

	StyleBlockRef trackVStyle;
	StyleBlockRef thumbVStyle;
	SubUI<int> uiState;
	float dragStartContentOff;
	float dragStartCursorPos;
};

struct ScrollArea : UIElement
{
	Size2f estContentSize = {};
	float yoff = 0;
	ScrollbarV sbv;

	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
	void OnLayout(const UIRect& rect, const Size2f& containerSize) override;
	void OnReset() override;
};

struct TabGroup : UIElement
{
	struct TabButtonBase* _activeBtn;

	void OnReset() override;
};

struct TabButtonList : UIElement
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
};

struct TabButtonBase : UIElement
{
	TabButtonBase()
	{
		flags |= UIObject_NeedsTreeUpdates;
	}

	void OnReset() override;
	void OnExitTree() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	virtual void OnSelect() {}
	virtual bool IsSelected() const { return !!(flags & UIObject_IsChecked); }
};

struct TabButton : TabButtonBase
{
	TabButton& Init(bool selected)
	{
		SetFlag(UIObject_IsChecked, selected);
		if (selected)
			if (auto* g = FindParentOfType<TabGroup>())
				g->_activeBtn = this;
		return *this;
	}
	// implement Activate event to set source state
};

template <class T>
struct TabButtonT : TabButtonBase
{
	T* _iptr = nullptr;
	T _value = {};

	TabButtonT& Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		if (iref == value)
			if (auto* g = FindParentOfType<TabGroup>())
				g->_activeBtn = this;
		return *this;
	}

	void OnSelect() override
	{
		if (!_iptr)
			return;
		*_iptr = _value;
		Rebuild();
	}
	bool IsSelected() const override { return _iptr && *_iptr == _value; }

	void OnReset() override
	{
		TabButtonBase::OnReset();

		_iptr = nullptr;
		_value = {};
	}
};

struct TabPanel : UIElement
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
};

struct Textbox : UIElement
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	bool IsLongSelection() const { return startCursor != endCursor; }
	StringView GetSelectedText() const;
	void EnterText(const char* str);
	void EraseSelection();

	size_t _FindCursorPos(float vpx);

	const std::string& GetText() const { return _text; }
	Textbox& SetText(StringView s);
	Textbox& SetPlaceholder(StringView s);

	Textbox& Init(float& val);
	template <size_t N> Textbox& Init(char (&val)[N])
	{
		SetText(val);
		HandleEvent(EventType::Change) = [this, &val](Event&)
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
	size_t _origStartCursor = 0;
	size_t startCursor = 0;
	size_t endCursor = 0;
	bool showCaretState = false;
	bool _hadFocusOnFirstClick = false;
	unsigned _lastPressRepeatCount = 0;
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
	bool open = false;
	bool _hovered = false;

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
};


struct BackgroundBlocker : UIElement
{
	RectAnchoredPlacement _fullScreenPlacement;

	void OnReset() override;
	void OnEvent(Event& e) override;
	void OnButton();
};


struct DropdownMenu : Buildable
{
	void Build() override;
	void OnEvent(Event& e) override;

	virtual void OnBuildButton();
	virtual void OnBuildMenuWithLayout();
	virtual UIObject& OnBuildMenu();

	virtual void OnBuildButtonContents() = 0;
	virtual void OnBuildMenuContents() = 0;
};


struct OptionList
{
	typedef void ElementFunc(const void* ptr, uintptr_t id);

	virtual void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) = 0;
	virtual void BuildElement(const void* ptr, uintptr_t id, bool list) = 0;
};

struct CStrOptionList : OptionList
{
	void BuildElement(const void* ptr, uintptr_t id, bool list) override;
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
	const char* const* arr = nullptr;
	size_t size = SIZE_MAX;

	CStrArrayOptionList(const char* const* a, NullTerminated) : arr(a) {}
	template <size_t N>
	CStrArrayOptionList(const char* const (&a)[N]) : arr(a), size(N) {}
	CStrArrayOptionList(size_t sz, const char* const* a) : arr(a), size(sz) {}

	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
};

struct StringArrayOptionList : CStrOptionList
{
	std::vector<std::string> options;

	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override;
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

	virtual void OnBuildEmptyButtonContents();
	virtual void OnBuildMenuElement(const void* ptr, uintptr_t id);
};

namespace imm {

template <class MT, class T> bool DropdownMenuListCustom(T& val, OptionList* ol, ModInitList mods = {})
{
	auto& ddml = Make<MT>();
	ddml.SetOptions(ol);
	for (auto& mod : mods)
		mod->Apply(&ddml);

	bool edited = false;
	if (ddml.flags & UIObject_IsEdited)
	{
		val = T(ddml.GetSelected());
		ddml.flags &= ~UIObject_IsEdited;
		ddml._OnIMChange();
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

	return edited;
}
template <class T> bool DropdownMenuList(T& val, OptionList* ol, ModInitList mods = {})
{
	return DropdownMenuListCustom<ui::DropdownMenuList>(val, ol, mods);
}

template <class MT, class T> bool PropDropdownMenuListCustom(const char* label, T& val, OptionList* ol, ModInitList mods = {})
{
	LabeledProperty::Scope ps(label);
	return DropdownMenuListCustom<MT, T>(val, ol, mods);
}
template <class T> bool PropDropdownMenuList(const char* label, T& val, OptionList* ol, ModInitList mods = {})
{
	LabeledProperty::Scope ps(label);
	return DropdownMenuList<T>(val, ol, mods);
}

} // imm


struct OverlayInfoPlacement : IPlacement
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
	void OnReset() override;
};

struct DragDropDataFrame : OverlayInfoFrame
{
	void OnReset() override;
};

struct DefaultOverlayBuilder : Buildable
{
	bool drawTooltip = true;
	bool drawDragDrop = true;

	void OnReset() override
	{
		Buildable::OnReset();

		drawTooltip = true;
		drawDragDrop = true;
	}

	void Build() override;
};

} // ui
