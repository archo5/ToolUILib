
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
	void OnEvent(UIEvent& e) override;
};

struct CheckableBase : UIElement
{
	CheckableBase();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	virtual void OnSelect() {}
	virtual bool IsSelected() const { return !!(flags & UIObject_IsChecked); }
};

struct CheckboxBase : CheckableBase
{
	CheckboxBase();
};

struct Checkbox : CheckboxBase
{
	Checkbox* Init(bool checked)
	{
		SetFlag(UIObject_IsChecked, checked);
		return this;
	}
	void OnSelect() override;
	// implement Activate event to flip source state
};

struct CheckboxBoolState : CheckboxBase
{
	bool GetChecked() const { return _value; }
	CheckboxBoolState* SetChecked(bool val)
	{
		_value = val;
		return this;
	}
	void OnSerialize(IDataSerializer& s) override { s << _value; }

	virtual void OnSelect() override { _value ^= true; }
	virtual bool IsSelected() const override { return _value; }

	bool _value;
};

template <class T>
struct CheckboxFlagT : CheckboxBase
{
	CheckboxFlagT* Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return this;
	}

	void OnSelect() override
	{
		if (!_iptr)
			return;
		if (!IsSelected())
			*_iptr |= _value;
		else
			*_iptr &= ~_value;
	}
	bool IsSelected() const override { return _iptr && (*_iptr & _value) == _value; }

	T* _iptr = nullptr;
	T _value = {};
};

struct RadioButtonBase : CheckableBase
{
	RadioButtonBase();
};

struct RadioButton : RadioButtonBase
{
	RadioButton* Init(bool selected)
	{
		SetFlag(UIObject_IsChecked, selected);
		return this;
	}
	void OnSelect() override;
	// implement Activate event to set source state
};

template <class T>
struct RadioButtonT : RadioButtonBase
{
	RadioButtonT* Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return this;
	}

	void OnSelect() override { if (_iptr) *_iptr = _value; }
	bool IsSelected() const override { return _iptr && *_iptr == _value; }

	T* _iptr = nullptr;
	T _value = {};
};

struct ListBox : UIElement
{
	ListBox();
};

struct SelectableBase : UIElement
{
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	virtual void OnSelect(bool) = 0;
	virtual bool IsSelected() = 0;

	std::function<void()> onChange;
};

struct Selectable : SelectableBase
{
	Selectable();
	Selectable* Init(bool& bref)
	{
		_bptr = &bref;
		return this;
	}

	virtual void OnSelect(bool v) override { if (_bptr) *_bptr = v; }
	virtual bool IsSelected() override { return _bptr && *_bptr; }

private:
	bool* _bptr = nullptr;
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

	double _value = 0;
	FloatLimits _limits = { 0, 1, 0 };

	style::BlockRef trackStyle;
	style::BlockRef trackFillStyle;
	style::BlockRef thumbStyle;

	float _mxoff = 0;
};

struct Property : UIElement
{
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
	Range<float> GetFullEstimatedWidth(const Size<float>& containerSize, style::EstSizeType type) override;
	Range<float> GetFullEstimatedHeight(const Size<float>& containerSize, style::EstSizeType type) override;

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
			size_t s = std::min(t.size(), N - 1);
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

struct OverlayInfoFrame : UIElement
{
	void OnLayout(const UIRect& rect, const Size<float>& containerSize) override;
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
