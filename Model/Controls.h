
#pragma once

#include "Objects.h"


struct UIPanel : UIElement
{
	UIPanel();
	void OnPaint() override;
};

struct UIButton : UIElement
{
	UIButton();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	bool pressed = false;
};

struct UICheckbox : UIElement
{
	UICheckbox();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;
	//void OnLayout(const UIRect& rect) override;

	UICheckbox* SetValue(bool v)
	{
		checked = v;
		return this;
	}
	bool GetValue() const
	{
		return checked;
	}

	bool checked = false;
};

struct UIRadioButton : UIElement
{
	UIRadioButton();
	void OnPaint() override;
	void OnEvent(UIEvent& e) override;

	UIRadioButton* SetValue(bool v)
	{
		checked = v;
		return this;
	}
	bool checked = false;
};

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
