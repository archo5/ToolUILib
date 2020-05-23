
#pragma once

#include "Objects.h"


namespace ui {

class Panel : public UIElement
{
public:
	Panel();
};

class Button : public UIElement
{
public:
	Button();
	void OnEvent(UIEvent& e) override;

	std::function<void()> onClick;
};

class CheckableBase : public UIElement
{
public:
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	virtual void OnSelect() = 0;
	virtual bool IsSelected() = 0;

	// TODO generalize efficiently
	std::function<void()> onChange;
};

class CheckboxBase : public CheckableBase
{
public:
	CheckboxBase();
};

// TODO
class CheckboxData : public CheckboxBase
{
public:
	CheckboxData* Init(bool val)
	{
		value = val;
		return this;
	}
	void OnSerialize(IDataSerializer& s) override { s << value; }

	virtual void OnSelect() override { value ^= true; }
	virtual bool IsSelected() override { return value; }

	bool value;
};

class Checkbox : public CheckboxBase
{
public:
	Checkbox* Init(bool& bref)
	{
		_bptr = &bref;
		return this;
	}

	virtual void OnSelect() override { if (_bptr) *_bptr ^= true; }
	virtual bool IsSelected() override { return _bptr && *_bptr; }

private:
	bool* _bptr = nullptr;
};

class RadioButtonBase : public CheckableBase
{
public:
	RadioButtonBase();
};

template <class T>
class RadioButtonT : public RadioButtonBase
{
public:
	RadioButtonT* Init(T& iref, T value)
	{
		_iptr = &iref;
		_value = value;
		return this;
	}

	void OnSelect() override { if (_iptr) *_iptr = _value; }
	bool IsSelected() override { return _iptr && *_iptr == _value; }

	T* _iptr = nullptr;
	T _value = {};
};

class ListBox : public UIElement
{
public:
	ListBox();
};

class SelectableBase : public UIElement
{
public:
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	virtual void OnSelect(bool) = 0;
	virtual bool IsSelected() = 0;

	std::function<void()> onChange;
};

class Selectable : public SelectableBase
{
public:
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

namespace imm {

bool Button(UIContainer* ctx, const char* text, std::initializer_list<ui::Modifier*> mods = {});
bool EditBool(UIContainer* ctx, bool& val, std::initializer_list<ui::Modifier*> mods = {});
bool EditInt(UIContainer* ctx, UIObject* dragObj, int& val, std::initializer_list<ui::Modifier*> mods = {}, int speed = 1, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool EditInt(UIContainer* ctx, UIObject* dragObj, unsigned& val, std::initializer_list<ui::Modifier*> mods = {}, unsigned speed = 1, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool EditInt(UIContainer* ctx, UIObject* dragObj, int64_t& val, std::initializer_list<ui::Modifier*> mods = {}, int64_t speed = 1, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool EditInt(UIContainer* ctx, UIObject* dragObj, uint64_t& val, std::initializer_list<ui::Modifier*> mods = {}, uint64_t speed = 1, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool EditFloat(UIContainer* ctx, UIObject* dragObj, float& val, std::initializer_list<ui::Modifier*> mods = {}, float speed = 1, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool EditString(UIContainer* ctx, const char* text, const std::function<void(const char*)>& retfn, std::initializer_list<ui::Modifier*> mods = {});

bool PropButton(UIContainer* ctx, const char* label, const char* text, std::initializer_list<ui::Modifier*> mods = {});
bool PropEditBool(UIContainer* ctx, const char* label, bool& val, std::initializer_list<ui::Modifier*> mods = {});
bool PropEditInt(UIContainer* ctx, const char* label, int& val, std::initializer_list<ui::Modifier*> mods = {}, int speed = 1, int vmin = INT_MIN, int vmax = INT_MAX, const char* fmt = "%d");
bool PropEditInt(UIContainer* ctx, const char* label, unsigned& val, std::initializer_list<ui::Modifier*> mods = {}, unsigned speed = 1, unsigned vmin = 0, unsigned vmax = UINT_MAX, const char* fmt = "%u");
bool PropEditInt(UIContainer* ctx, const char* label, int64_t& val, std::initializer_list<ui::Modifier*> mods = {}, int64_t speed = 1, int64_t vmin = INT64_MIN, int64_t vmax = INT64_MAX, const char* fmt = "%" PRId64);
bool PropEditInt(UIContainer* ctx, const char* label, uint64_t& val, std::initializer_list<ui::Modifier*> mods = {}, uint64_t speed = 1, uint64_t vmin = 0, uint64_t vmax = UINT64_MAX, const char* fmt = "%" PRIu64);
bool PropEditFloat(UIContainer* ctx, const char* label, float& val, std::initializer_list<ui::Modifier*> mods = {}, float speed = 1, float vmin = -FLT_MAX, float vmax = FLT_MAX, const char* fmt = "%g");
bool PropEditString(UIContainer* ctx, const char* label, const char* text, const std::function<void(const char*)>& retfn, std::initializer_list<ui::Modifier*> mods = {});

} // imm

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
	float dragOff;
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
	void OnInit() override;
	void OnPaint() override;
	void OnSerialize(IDataSerializer& s) override;

	int active = 0;
	class TabButton* _activeBtn = nullptr;
	int _curButton = 0;
	int _curPanel = 0;
};

class TabList : public UIElement
{
public:
	TabList();
	void OnPaint() override;
};

class TabButton : public UIElement
{
public:
	TabButton();
	void OnInit() override;
	void OnDestroy() override;
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	int id;
};

class TabPanel : public UIElement
{
public:
	TabPanel();
	void OnInit() override;
	void OnPaint() override;

	int id;
};

struct Textbox : UIElement
{
	Textbox();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnSerialize(IDataSerializer& s) override;

	bool IsLongSelection() const { return startCursor != endCursor; }
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

struct Selection1D
{
	Selection1D();
	~Selection1D();
	void OnSerialize(IDataSerializer& s);
	void Clear();
	bool AnySelected();
	uintptr_t GetFirstSelection();
	bool IsSelected(uintptr_t id);
	void SetSelected(uintptr_t id, bool sel);

	struct Selection1DImpl* _impl;
};

struct TableDataSource
{
	virtual size_t GetNumRows() = 0;
	virtual size_t GetNumCols() = 0;
	virtual std::string GetRowName(size_t row) = 0;
	virtual std::string GetColName(size_t col) = 0;
	virtual std::string GetText(size_t row, size_t col) = 0;
};

class TableView : public ui::Node
{
public:
	TableView();
	~TableView();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnSerialize(IDataSerializer& s) override;
	void Render(UIContainer* ctx) override;

	TableDataSource* GetDataSource() const;
	void SetDataSource(TableDataSource* src);
	void CalculateColumnWidths(bool includeHeader = true, bool firstTimeOnly = true);

	bool IsValidRow(uintptr_t pos);
	size_t GetRowAt(float y);
	size_t GetHoverRow() const;

	style::BlockRef cellStyle;
	style::BlockRef rowHeaderStyle;
	style::BlockRef colHeaderStyle;
	Selection1D selection;

private:
	struct TableViewImpl* _impl;
};

class TreeDataSource
{
public:
	static constexpr uintptr_t ROOT = uintptr_t(-1);

	virtual size_t GetNumCols() = 0;
	virtual std::string GetColName(size_t col) = 0;
	virtual size_t GetChildCount(uintptr_t id) = 0;
	virtual uintptr_t GetChild(uintptr_t id, size_t which) = 0;
	virtual std::string GetText(uintptr_t id, size_t col) = 0;
};

class TreeView : public ui::Node
{
public:
	struct PaintState;

	TreeView();
	~TreeView();
	void OnPaint() override;
	void _PaintOne(uintptr_t id, int lvl, PaintState& ps);
	void OnEvent(UIEvent& e) override;
	void Render(UIContainer* ctx) override;

	TreeDataSource* GetDataSource() const;
	void SetDataSource(TreeDataSource* src);
	void CalculateColumnWidths(bool firstTimeOnly = true);

	style::BlockRef cellStyle;
	style::BlockRef expandButtonStyle;
	style::BlockRef colHeaderStyle;

private:
	struct TreeViewImpl* _impl;
};

} // ui
