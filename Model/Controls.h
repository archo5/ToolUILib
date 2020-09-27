
#pragma once

#include "Objects.h"


namespace ui {

struct Panel : UIElement
{
	Panel();
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
	Selectable();
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
	static void Make(UIContainer* ctx, const char* label, std::function<void()> content)
	{
		Begin(ctx, label);
		content();
		End(ctx);
	}

	static UIObject& Label(UIContainer* ctx, const char* label);
	static UIObject& MinLabel(UIContainer* ctx, const char* label);

	static void EditFloat(UIContainer* ctx, const char* label, float* v);
	static void EditFloat2(UIContainer* ctx, const char* label, float* v);
	static void EditFloat3(UIContainer* ctx, const char* label, float* v);
	static void EditFloat4(UIContainer* ctx, const char* label, float* v);
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
	Textbox& SetText(const std::string& s);

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
	int startCursor = 0;
	int endCursor = 0;
	bool showCaretState = false;
	float accumulator = 0;
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
