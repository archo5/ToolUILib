
#include "Controls.h"
#include "Layout.h"
#include "System.h"
#include "Theme.h"


namespace ui {

Panel::Panel()
{
	styleProps = ui::Theme::current->panel;
}


Button::Button()
{
	styleProps = ui::Theme::current->button;
}

void Button::OnEvent(UIEvent& e)
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


void CheckableBase::OnPaint()
{
	style::PaintInfo info(this);
	if (IsSelected())
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);

	PaintChildren();
}

void CheckableBase::OnEvent(UIEvent& e)
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
		e.context->OnChange(this);
	}
}


CheckboxBase::CheckboxBase()
{
	styleProps = ui::Theme::current->checkbox;
}


RadioButtonBase::RadioButtonBase()
{
	styleProps = ui::Theme::current->radioButton;
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
	styleProps = ui::Theme::current->collapsibleTreeNode;
}

void UICollapsibleTreeNode::OnPaint()
{
	style::PaintInfo info(this);
	info.state &= ~style::PS_Hover;
	if (_hovered)
		info.state |= style::PS_Hover;
	if (open)
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);
	PaintChildren();
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
