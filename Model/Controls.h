
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

private:
	float _mxoff = 0;
};

class ScrollArea : public UIElement
{
public:
	ScrollArea();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	void OnLayout(const UIRect& rect) override;

private:
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

	int active;
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

class Table : public ui::Node
{
public:
	Table();
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

} // ui
