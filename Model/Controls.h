
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

struct Table : UINode
{
};

} // ui
