
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
};

class CheckableBase : public UIElement
{
public:
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	virtual void OnSelect() = 0;
	virtual bool IsSelected() = 0;
};

class CheckboxBase : public CheckableBase
{
public:
	CheckboxBase();
};

class Checkbox : public CheckboxBase
{
public:
	virtual void OnSelect() override { SetChecked(!GetChecked()); }
	virtual bool IsSelected() override { return GetChecked(); }

	Checkbox* SetChecked(bool v)
	{
		_isChecked = v;
		return this;
	}
	bool GetChecked() const
	{
		return _isChecked;
	}

private:
	bool _isChecked = false;
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

} // ui

struct UITextbox : UIElement
{
	UITextbox();
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

struct UICollapsibleTreeNode : UIElement
{
	UICollapsibleTreeNode();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	bool open = false;
	bool _hovered = false;
};

struct UITable : UINode
{
};
