
#include "Controls.h"
#include "Layout.h"
#include "System.h"


UIPanel::UIPanel()
{
	GetStyle().SetBoxSizing(style::BoxSizing::BorderBox);
	GetStyle().SetMargin(2);
	GetStyle().SetPadding(6);
}

void UIPanel::OnPaint()
{
	auto r = GetPaddingRect();
	DrawThemeElement(TE_Panel, r.x0, r.y0, r.x1, r.y1);
	UIElement::OnPaint();
}


UIButton::UIButton()
{
	GetStyle().SetLayout(style::Layout::InlineBlock);
	GetStyle().SetBoxSizing(style::BoxSizing::BorderBox);
	GetStyle().SetPadding(5);
}

void UIButton::OnPaint()
{
	auto r = GetPaddingRect();
	DrawThemeElement(
		IsInputDisabled() ? TE_ButtonDisabled :
		IsClicked() ? TE_ButtonPressed : 
		IsHovered() ? TE_ButtonHover : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
	UIElement::OnPaint();
}

void UIButton::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		system->eventSystem.SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::Activate)
	{
		if (IsInputDisabled())
			e.handled = true;
	}
}


UICheckbox::UICheckbox()
{
	GetStyle().SetLayout(style::Layout::InlineBlock);
	//GetStyle().SetWidth(style::Coord::Percent(12));
	//GetStyle().SetHeight(style::Coord::Percent(12));
	GetStyle().SetWidth(GetFontHeight() + 5 + 5);
	GetStyle().SetHeight(GetFontHeight() + 5 + 5);
}

void UICheckbox::OnPaint()
{
	auto r = GetPaddingRect();
	float w = std::min(r.GetWidth(), r.GetHeight());
	DrawThemeElement(
		IsInputDisabled() ? TE_CheckBgrDisabled :
		IsClicked(0) ? TE_CheckBgrPressed :
		IsHovered() ? TE_CheckBgrHover : TE_CheckBgrNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
	if (checked)
		DrawThemeElement(IsInputDisabled() ? TE_CheckMarkDisabled : TE_CheckMark, r.x0, r.y0, r.x0 + w, r.y0 + w);
	UIElement::OnPaint();
}

void UICheckbox::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		system->eventSystem.SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::Activate)
	{
		if (IsInputDisabled())
		{
			e.handled = true;
			return;
		}
		checked = !checked;
		e.context->OnChange(this);
	}
}

#if 0
void UICheckbox::OnLayout(const UIRect& rect)
{
	finalRect = rect;

	auto width = GetStyle().GetWidth();
	if (width.IsDefined())
		finalRect.x1 = finalRect.x0 + ResolveUnits(width, rect.x1 - rect.x0);

	finalRect.y1 = finalRect.y0 + 24;
	auto height = GetStyle().GetHeight();
	if (height.IsDefined())
		finalRect.y1 = finalRect.y0 + ResolveUnits(height, rect.x1 - rect.x0);
}
#endif


namespace ui {

RadioButtonBase::RadioButtonBase()
{
	GetStyle().SetLayout(style::Layout::InlineBlock);
	//GetStyle().SetWidth(style::Coord::Percent(12));
	//GetStyle().SetHeight(style::Coord::Percent(12));
	GetStyle().SetWidth(GetFontHeight() + 5 + 5);
	GetStyle().SetHeight(GetFontHeight() + 5 + 5);
}

void RadioButtonBase::OnPaint()
{
	auto r = GetPaddingRect();
	if (buttonStyle)
	{
		DrawThemeElement(
			IsInputDisabled() ? TE_ButtonDisabled :
			IsClicked(0) || IsSelected() ? TE_ButtonPressed :
			IsHovered() ? TE_ButtonHover : TE_ButtonNormal, r.x0, r.y0, r.x1, r.y1);
	}
	else
	{
		float w = std::min(r.GetWidth(), r.GetHeight());
		DrawThemeElement(
			IsInputDisabled() ? TE_RadioBgrDisabled :
			IsClicked(0) ? TE_RadioBgrPressed :
			IsHovered() ? TE_RadioBgrHover : TE_RadioBgrNormal, r.x0, r.y0, r.x0 + w, r.y0 + w);
		if (IsSelected())
			DrawThemeElement(IsInputDisabled() ? TE_RadioMarkDisabled : TE_RadioMark, r.x0, r.y0, r.x0 + w, r.y0 + w);
	}
	UIElement::OnPaint();
}

void RadioButtonBase::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		system->eventSystem.SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::Activate)
	{
		if (IsInputDisabled())
		{
			e.handled = true;
			return;
		}
		OnSelect();
	}
}

void RadioButtonBase::SetButtonStyle()
{
	GetStyle().SetLayout(style::Layout::InlineBlock);
	GetStyle().SetBoxSizing(style::BoxSizing::BorderBox);
	GetStyle().SetPadding(5);
	GetStyle().SetWidth(style::Coord::Undefined());
	GetStyle().SetHeight(style::Coord::Undefined());
	buttonStyle = true;
}

} // ui


UITextbox::UITextbox()
{
	GetStyle().SetLayout(style::Layout::InlineBlock);
	GetStyle().SetBoxSizing(style::BoxSizing::BorderBox);
	GetStyle().SetPadding(5);
	GetStyle().SetWidth(120);
	GetStyle().SetHeight(GetFontHeight() + 5 + 5);
}

void UITextbox::OnPaint()
{
	// background
	{
		auto r = GetPaddingRect();
		DrawThemeElement(TE_Textbox, r.x0, r.y0, r.x1, r.y1);
	}

	{
		auto r = GetContentRect();
		DrawTextLine(r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text.c_str(), 1, 1, 1);

		if (IsFocused())
		{
			if (startCursor != endCursor)
			{
				int minpos = startCursor < endCursor ? startCursor : endCursor;
				int maxpos = startCursor > endCursor ? startCursor : endCursor;
				float x0 = GetTextWidth(text.c_str(), minpos);
				float x1 = GetTextWidth(text.c_str(), maxpos);

				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(0.5f, 0.7f, 0.9f, 0.4f);
				br.Quad(r.x0 + x0, r.y0, r.x0 + x1, r.y1, 0, 0, 1, 1);
				br.End();
			}

			if (showCaretState)
			{
				float x = GetTextWidth(text.c_str(), endCursor);
				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(1, 1, 1);
				br.Quad(r.x0 + x, r.y0, r.x0 + x + 1, r.y1, 0, 0, 1, 1);
				br.End();
			}
		}
	}

	UIElement::OnPaint();
}

void UITextbox::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		if (e.GetButton() == UIMouseButton::Left)
			startCursor = endCursor = _FindCursorPos(e.x);
		system->eventSystem.SetKeyboardFocus(this);
	}
	else if (e.type == UIEventType::ButtonUp)
	{
		if (e.GetButton() == UIMouseButton::Left)
			endCursor = _FindCursorPos(e.x);
	}
	else if (e.type == UIEventType::MouseMove)
	{
		if (IsClicked(0))
			endCursor = _FindCursorPos(e.x);
	}
	else if (e.type == UIEventType::Timer)
	{
		showCaretState = !showCaretState;
		if (IsFocused())
			system->eventSystem.SetTimer(this, 0.5f);
	}
	else if (e.type == UIEventType::GotFocus)
	{
		showCaretState = true;
		system->eventSystem.SetTimer(this, 0.5f);
	}
	else if (e.type == UIEventType::LostFocus)
	{
		system->eventSystem.OnCommit(this);
	}
	else if (e.type == UIEventType::KeyAction)
	{
		switch (e.GetKeyAction())
		{
		case UIKeyAction::Enter:
			system->eventSystem.SetKeyboardFocus(nullptr);
			break;
		case UIKeyAction::Backspace:
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor > 0)
				text.erase(startCursor = --endCursor, 1); // TODO unicode
			system->eventSystem.OnChange(this);
			break;
		case UIKeyAction::Delete:
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor + 1 < text.size())
				text.erase(endCursor, 1); // TODO unicode
			system->eventSystem.OnChange(this);
			break;

		case UIKeyAction::Left:
			if (IsLongSelection())
				startCursor = endCursor = startCursor < endCursor ? startCursor : endCursor;
			else if (endCursor > 0)
				startCursor = --endCursor;
			break;
		case UIKeyAction::Right:
			if (IsLongSelection())
				startCursor = endCursor = startCursor > endCursor ? startCursor : endCursor;
			else if (endCursor < text.size())
				startCursor = ++endCursor;
			break;
		case UIKeyAction::Home:
			startCursor = endCursor = 0;
			break;
		case UIKeyAction::End:
			startCursor = endCursor = text.size();
			break;

		case UIKeyAction::SelectAll:
			startCursor = 0;
			endCursor = text.size();
			break;
		}
	}
	else if (e.type == UIEventType::TextInput)
	{
		char ch[5];
		if (e.GetUTF32Char() >= 32 && e.GetUTF8Text(ch))
			EnterText(ch);
	}
}

void UITextbox::EnterText(const char* str)
{
	EraseSelection();
	size_t num = strlen(str);
	text.insert(endCursor, str, num);
	startCursor = endCursor += num;
	system->eventSystem.OnChange(this);
}

void UITextbox::EraseSelection()
{
	if (IsLongSelection())
	{
		int min = startCursor < endCursor ? startCursor : endCursor;
		int max = startCursor > endCursor ? startCursor : endCursor;
		text.erase(min, max - min);
		startCursor = endCursor = min;
	}
}

int UITextbox::_FindCursorPos(float vpx)
{
	auto r = GetContentRect();
	// TODO kerning
	float x = r.x0;
	for (size_t i = 0; i < text.size(); i++)
	{
		float lw = GetTextWidth(&text[i], 1);
		if (vpx < x + lw * 0.5f)
			return i;
		x += lw;
	}
	return text.size();
}


UICollapsibleTreeNode::UICollapsibleTreeNode()
{
	GetStyle().SetLayout(style::Layout::Stack);
	//GetStyle().SetPadding(1);
	GetStyle().SetPaddingLeft(GetFontHeight());
}

void UICollapsibleTreeNode::OnPaint()
{
	auto r = GetPaddingRect();
	DrawThemeElement(_hovered ?
		open ? TE_TreeTickOpenHover : TE_TreeTickClosedHover :
		open ? TE_TreeTickOpenNormal : TE_TreeTickClosedNormal, r.x0, r.y0, r.x0 + GetFontHeight(), r.y0 + GetFontHeight());
	UIElement::OnPaint();
}

void UICollapsibleTreeNode::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::MouseEnter || e.type == UIEventType::MouseMove)
	{
		auto r = GetPaddingRect();
		float h = GetFontHeight();
		_hovered = e.x >= r.x0 && e.x < r.x0 + h && e.y >= r.y0 && e.y < r.y0 + h;
	}
	if (e.type == UIEventType::MouseLeave)
	{
		_hovered = false;
	}
	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && _hovered)
	{
		open = !open;
		e.GetTargetNode()->Rerender();
	}
}
