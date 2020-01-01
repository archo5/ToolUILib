
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

class ProgressBar : public UIElement
{
public:
	ProgressBar();
	void OnPaint() override;

	style::BlockRef completionBarStyle;
	float progress = 0.5f;
};

class Slider : public UIElement
{
public:
	Slider();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	Slider* Init(float* vp, double vmin = 0, double vmax = 1, double step = 0);

	float PosToQ(float x);
	double QToValue(float q);
	float ValueToQ(double v);
	double PosToValue(float x) { return QToValue(PosToQ(x)); }

	double minValue = 0;
	double maxValue = 1;
	double trackStep = 0;
	float* valuePtr = nullptr;

	style::BlockRef trackStyle;
	style::BlockRef trackFillStyle;
	style::BlockRef thumbStyle;

	float _mxoff = 0;
};

struct SplitPane : UIElement
{
	SplitPane();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnLayout(const UIRect& rect) override;
	float GetFullEstimatedWidth(float containerWidth, float containerHeight) override;
	float GetFullEstimatedHeight(float containerWidth, float containerHeight) override;

	float GetSplit(unsigned which);
	SplitPane* SetSplit(unsigned which, float f, bool clamp = true);
	bool GetDirection() const { return _verticalSplit; }
	SplitPane* SetDirection(bool vertical);

	style::BlockRef vertSepStyle; // for horizontal splitting
	style::BlockRef horSepStyle; // for vertical splitting
	std::vector<float> _splits;
	bool _verticalSplit = false;
	SubUI<uint16_t> _splitUI;
	float _dragOff = 0;
};

class ScrollArea : public UIElement
{
public:
	ScrollArea();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnLayout(const UIRect& rect) override;

	style::BlockRef trackVStyle;
	style::BlockRef thumbVStyle;
	float yoff = 23;
};

class TabGroup : public UIElement
{
public:
	TabGroup();
	void OnInit() override;
	void OnPaint() override;

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

	bool IsLongSelection() const { return startCursor != endCursor; }
	void EnterText(const char* str);
	void EraseSelection();

	int _FindCursorPos(float vpx);

	std::string text;
	int startCursor = 0;
	int endCursor = 0;
	bool showCaretState = false;
};

struct CollapsibleTreeNode : UIElement
{
	CollapsibleTreeNode();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	bool open = false;
	bool _hovered = false;
};

class TableDataSource
{
public:
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
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void Render(UIContainer* ctx) override;

	TableDataSource* GetDataSource() const { return _dataSource; }
	void SetDataSource(TableDataSource* src);

	style::BlockRef cellStyle;
	style::BlockRef rowHeaderStyle;
	style::BlockRef colHeaderStyle;

private:
	TableDataSource* _dataSource;
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
