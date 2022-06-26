
#include "Textbox.h"

#include "../Model/Native.h"


namespace ui {

void Textbox::OnReset()
{
	FrameElement::OnReset();

	flags |= UIObject_DB_FocusOnLeftClick | UIObject_DB_CaptureMouseOnLeftClick;
	SetDefaultFrameStyle(DefaultFrameStyle::TextBox);

	_placeholder = {};
}

void Textbox::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();

	{
		auto* font = frameStyle.font.GetFont();
		int size = frameStyle.font.size;

		auto r = GetContentRect();
		if (!_placeholder.empty() && !IsFocused() && _text.empty())
			draw::TextLine(font, size, r.x0, r.y1 - (r.y1 - r.y0 - size) / 2, _placeholder, Color4b(255, 128));
		if (!_text.empty())
			draw::TextLine(font, size, r.x0, r.y1 - (r.y1 - r.y0 - size) / 2, _text, Color4b::White());

		if (IsFocused())
		{
			if (startCursor != endCursor)
			{
				int minpos = startCursor < endCursor ? startCursor : endCursor;
				int maxpos = startCursor > endCursor ? startCursor : endCursor;
				float x0 = GetTextWidth(font, size, StringView(_text).substr(0, minpos));
				float x1 = GetTextWidth(font, size, StringView(_text).substr(0, maxpos));

				draw::RectCol(r.x0 + x0, r.y0, r.x0 + x1, r.y1, Color4f(0.5f, 0.7f, 0.9f, 0.4f));
			}

			if (showCaretState)
			{
				float x = GetTextWidth(font, size, StringView(_text).substr(0, endCursor));
				draw::RectCol(r.x0 + x, r.y0, r.x0 + x + 1, r.y1, Color4b::White());
			}
		}
	}

	PaintChildren(ctx, cpa);
}

static size_t PrevChar(StringView str, size_t pos)
{
	if (pos == 0)
		return 0;
	pos--;
	while (pos > 0 && (str[pos] & 0xC0) == 0x80)
		pos--;
	return pos;
}

static size_t NextChar(StringView str, size_t pos)
{
	if (pos == str.size())
		return pos;
	pos++;
	while (pos < str.size() && (str[pos] & 0xC0) == 0x80)
		pos++;
	return pos;
}

static uint32_t GetUTF8(StringView str, size_t pos)
{
	if (pos >= str.size())
		return UINT32_MAX;

	char c0 = str[pos];
	if (!(c0 & 0x80))
		return c0;

	if (pos + 1 >= str.size())
		return UINT32_MAX;
	char c1 = str[pos + 1];
	if ((c1 & 0xC0) != 0x80)
		return UINT32_MAX;

	if ((c0 & 0xE0) == 0xC0)
		return (c0 & ~0xE0) | ((c1 & ~0xC0) << 5);

	if (pos + 2 >= str.size())
		return UINT32_MAX;
	char c2 = str[pos + 2];
	if ((c2 & 0xC0) != 0x80)
		return UINT32_MAX;

	if ((c0 & 0xF0) == 0xE0)
		return (c0 & ~0xF0) | ((c1 & ~0xC0) << 4) | ((c2 & ~0xC0) << 10);

	if (pos + 3 >= str.size())
		return UINT32_MAX;
	char c3 = str[pos + 3];
	if ((c3 & 0xC0) != 0x80)
		return UINT32_MAX;

	if ((c0 & 0xF8) == 0xF0)
		return (c0 & ~0xF0) | ((c1 & ~0xC0) << 3) | ((c2 & ~0xC0) << 9) | ((c2 & ~0xC0) << 15);

	return UINT32_MAX;
}

static int GetCharClass(StringView str, size_t pos)
{
	uint32_t c = GetUTF8(str, pos);
	// TODO full unicode
	if (IsSpace(c)) // (ASCII)
		return 0;
	// punctuation (ASCII)
	if ((c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126))
		return 2;
	// anything else is considered to be a word
	return 1;
}

static bool IsWordBreak(int cca, int ccb)
{
	if (cca == ccb) // same class - ignore
		return false;
	if (ccb == 0) // trailing space - ignore
		return false;
	return true;
}

static size_t PrevWord(StringView str, size_t pos)
{
	if (pos == 0)
		return 0;

	pos = PrevChar(str, pos);
	auto pcc = GetCharClass(str, pos);
	while (pos > 0)
	{
		size_t npp = PrevChar(str, pos);
		auto ncc = GetCharClass(str, npp);
		if (IsWordBreak(ncc, pcc))
			break;
		pcc = ncc;
		pos = npp;
	}
	return pos;
}

static size_t NextWord(StringView str, size_t pos)
{
	if (pos == str.size())
		return pos;

	auto pcc = GetCharClass(str, pos);
	pos = NextChar(str, pos);
	while (pos < str.size())
	{
		auto ncc = GetCharClass(str, pos);
		if (IsWordBreak(pcc, ncc))
			break;
		pcc = ncc;
		pos = NextChar(str, pos);
	}
	return pos;
}

static void UpdateSelection(Textbox& tb, Event& e, bool start)
{
	if (start)
	{
		tb._lastPressRepeatCount = e.numRepeats - 1;
		tb._origStartCursor = tb._FindCursorPos(e.position.x);
		if (e.numRepeats == 1)
			tb._hadFocusOnFirstClick = tb.IsFocused();
	}
	unsigned mode = (tb._lastPressRepeatCount + (tb._hadFocusOnFirstClick ? 0 : 2)) % 3;
	if (mode != 2)
	{
		tb.startCursor = tb._origStartCursor;
		tb.endCursor = tb._FindCursorPos(e.position.x);
		if (mode == 1)
		{
			tb.startCursor = PrevWord(tb._text, tb.startCursor);
			tb.endCursor = NextWord(tb._text, tb.endCursor);
		}
	}
	else
	{
		tb.startCursor = 0;
		tb.endCursor = tb._text.size();
	}
}

void Textbox::OnEvent(Event& e)
{
	if (e.type == EventType::ButtonDown)
	{
		if (e.GetButton() == MouseButton::Left)
			UpdateSelection(*this, e, true);
	}
	else if (e.type == EventType::ButtonUp)
	{
		if (e.GetButton() == MouseButton::Left)
			UpdateSelection(*this, e, false);
	}
	else if (e.type == EventType::MouseMove)
	{
		if (IsClicked(0))
			UpdateSelection(*this, e, false);
	}
	else if (e.type == EventType::SetCursor)
	{
		e.context->SetDefaultCursor(DefaultCursor::Text);
		e.StopPropagation();
	}
	else if (e.type == EventType::Timer)
	{
		showCaretState = !showCaretState;
		if (system) // TODO better check? needed?
			GetNativeWindow()->InvalidateAll(); // TODO localized
		if (IsFocused())
			e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == EventType::GotFocus)
	{
		showCaretState = true;
		GetNativeWindow()->InvalidateAll(); // TODO localized
		startCursor = 0;
		endCursor = _text.size();
		e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == EventType::LostFocus)
	{
		e.context->OnCommit(this);
	}
	else if (e.type == EventType::KeyAction)
	{
		switch (e.GetKeyAction())
		{
		case KeyAction::Enter:
			e.context->SetKeyboardFocus(nullptr);
			break;

		case KeyAction::PrevLetter:
			if (IsLongSelection() && !e.GetKeyActionModifier())
			{
				startCursor = endCursor = startCursor < endCursor ? startCursor : endCursor;
			}
			else
			{
				endCursor = PrevChar(_text, endCursor);
				if (!e.GetKeyActionModifier())
					startCursor = endCursor;
			}
			break;
		case KeyAction::NextLetter:
			if (IsLongSelection() && !e.GetKeyActionModifier())
			{
				startCursor = endCursor = startCursor > endCursor ? startCursor : endCursor;
			}
			else
			{
				endCursor = NextChar(_text, endCursor);
				if (!e.GetKeyActionModifier())
					startCursor = endCursor;
			}
			break;

		case KeyAction::PrevWord:
			endCursor = PrevWord(_text, endCursor);
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;
		case KeyAction::NextWord:
			endCursor = NextWord(_text, endCursor);
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;

		case KeyAction::GoToLineStart:
		case KeyAction::GoToStart:
			endCursor = 0;
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;
		case KeyAction::GoToLineEnd:
		case KeyAction::GoToEnd:
			endCursor = _text.size();
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;

		case KeyAction::Cut:
			Clipboard::SetText(GetSelectedText());
			if (!IsInputDisabled())
			{
				EraseSelection();
				e.context->OnChange(this);
			}
			break;
		case KeyAction::Copy:
			Clipboard::SetText(GetSelectedText());
			break;
		case KeyAction::SelectAll:
			startCursor = 0;
			endCursor = _text.size();
			break;
		}

		if (!IsInputDisabled())
		{
			switch (e.GetKeyAction())
			{
			case KeyAction::DelPrevLetter:
			case KeyAction::DelNextLetter:
			case KeyAction::DelPrevWord:
			case KeyAction::DelNextWord:
				if (IsLongSelection())
				{
					EraseSelection();
				}
				else
				{
					size_t to;
					switch (e.GetKeyAction())
					{
					case KeyAction::DelPrevLetter:
						to = PrevChar(_text, endCursor);
						_text.erase(to, endCursor - to);
						startCursor = endCursor = to;
						break;
					case KeyAction::DelNextLetter:
						to = NextChar(_text, endCursor);
						_text.erase(endCursor, to - endCursor);
						break;
					case KeyAction::DelPrevWord:
						to = PrevWord(_text, endCursor);
						_text.erase(to, endCursor - to);
						startCursor = endCursor = to;
						break;
					case KeyAction::DelNextWord:
						to = NextWord(_text, endCursor);
						_text.erase(endCursor, to - endCursor);
						break;
					}
				}
				e.context->OnChange(this);
				break;
			case KeyAction::Paste:
				EnterText(Clipboard::GetText().c_str());
				break;
			}
		}
	}
	else if (e.type == EventType::TextInput)
	{
		if (!IsInputDisabled())
		{
			char ch[5];
			if (e.GetUTF32Char() >= 32 && e.GetUTF32Char() != 127 && e.GetUTF8Text(ch))
				EnterText(ch);
		}
	}
}

Rangef Textbox::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float minWidth = frameStyle.font.size * 2 + frameStyle.padding.x0 + frameStyle.padding.x1;
	return FrameElement::CalcEstimatedWidth(containerSize, type).Intersect(Rangef::AtLeast(minWidth));
}

Rangef Textbox::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float minHeight = frameStyle.font.size + frameStyle.padding.y0 + frameStyle.padding.y1;
	return FrameElement::CalcEstimatedHeight(containerSize, type).Intersect(Rangef::AtLeast(minHeight));
}

StringView Textbox::GetSelectedText() const
{
	int min = startCursor < endCursor ? startCursor : endCursor;
	int max = startCursor > endCursor ? startCursor : endCursor;
	return StringView(_text.data() + min, max - min);
}

void Textbox::EnterText(const char* str)
{
	EraseSelection();
	size_t num = strlen(str);
	_text.insert(endCursor, str, num);
	startCursor = endCursor += num;
	system->eventSystem.OnChange(this);
}

void Textbox::EraseSelection()
{
	if (IsLongSelection())
	{
		int min = startCursor < endCursor ? startCursor : endCursor;
		int max = startCursor > endCursor ? startCursor : endCursor;
		_text.erase(min, max - min);
		startCursor = endCursor = min;
	}
}

size_t Textbox::_FindCursorPos(float vpx)
{
	auto r = GetContentRect();
	// TODO kerning
	float x = r.x0;
	for (size_t i = 0; i < _text.size(); i++)
	{
		float lw = GetTextWidth(frameStyle.font.GetFont(), frameStyle.font.size, StringView(_text).substr(i, 1));
		if (vpx < x + lw * 0.5f)
			return i;
		x += lw;
	}
	return _text.size();
}

Textbox& Textbox::SetText(StringView s)
{
	if (InUse())
		return *this;
	_text.assign(s.data(), s.size());
	if (startCursor > _text.size())
		startCursor = _text.size();
	if (endCursor > _text.size())
		endCursor = _text.size();
	return *this;
}

Textbox& Textbox::SetPlaceholder(StringView s)
{
	_placeholder.assign(s.data(), s.size());
	return *this;
}

Textbox& Textbox::Init(float& val)
{
	if (!InUse())
	{
		char bfr[64];
		snprintf(bfr, 64, "%g", val);
		SetText(bfr);
	}
	HandleEvent(EventType::Change) = [this, &val](Event&) { val = atof(GetText().c_str()); };
	return *this;
}

} // ui
