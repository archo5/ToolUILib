
#include "Textbox.h"

#include "../Model/Native.h"
#include "../Render/RenderText.h"


namespace ui {

struct TextRange
{
	size_t start;
	size_t end;

	StringView sv(StringView text)
	{
		return text.substr(start, end - start);
	}
};

struct TextLines
{
	Array<TextRange> lines;

	size_t FindLine(size_t pos) const
	{
		if (lines.Size() <= 1)
			return 0;
		for (size_t i = 0; i < lines.Size(); i++)
		{
			if (lines[i].start <= pos && pos < lines[i].end)
				return i;
		}
		if (lines.Last().end == pos)
			return lines.Size() - 1;
		// error
		assert(!"bad position");
		return 0;
	}
	void Recalculate(StringView s, Font* font, int size, float maxWidth, bool multiline)
	{
		if (multiline)
			RecalculateMultiline(s, font, size, maxWidth);
		else
			SetOne(s);
	}
	void SetOne(StringView s)
	{
		lines = { TextRange{ 0, s.size() } };
	}
	void RecalculateMultiline(StringView s, Font* font, int size, float maxWidth)
	{
		lines.Clear();
		lines.Append({ 0, 0 });

		size_t lastBreakPos = 0;
		float widthToLastBreak = 0;
		float widthSoFar = 0;
		uint32_t prevCh = 0;

		TextMeasureBegin(font, size);
		UTF8Iterator it(s);
		for (;;)
		{
			uint32_t charStart = it.pos;
			uint32_t ch = it.Read();
			if (ch == UTF8Iterator::END)
				break;
			lines.Last().end = it.pos;
			if (ch == '\n')
			{
				lines.Append({ it.pos, it.pos });

				TextMeasureReset();
				widthToLastBreak = 0;
				widthSoFar = 0;
			}
			else
			{
				// TODO full unicode?
				if (ch == ' ' || prevCh == ' ')
				{
					widthToLastBreak = widthSoFar;
					lastBreakPos = charStart;
				}
				float w = TextMeasureAddChar(ch);
				if (w > maxWidth && lines.Last().end != lines.Last().start)
				{
					uint32_t p = widthToLastBreak > 0 ? lastBreakPos : charStart;

					it.pos = p;
					lines.Last().end = p;
					lines.Append({ p, p });

					TextMeasureReset();
					widthToLastBreak = 0;
					widthSoFar = 0;
				}
				widthSoFar = w;
			}
			prevCh = ch;
		}
		TextMeasureEnd();

		if (lines.Last().start == lines.Last().end && (lines.size() == 1 || !lines[lines.size() - 2].sv(s).ends_with("\n")))
			lines.RemoveLast();
	}
};

struct TextboxImpl
{
	std::string text;
	std::string placeholder;
	size_t origStartCursor = 0;
	size_t startCursor = 0;
	size_t endCursor = 0;
	bool showCaretState = false;
	bool hadFocusOnFirstClick = false;
	bool multiline = false;
	unsigned lastPressRepeatCount = 0;

	TextLines lines;
};

Textbox::Textbox()
{
	_impl = new TextboxImpl;
}

Textbox::~Textbox()
{
	delete _impl;
}

void Textbox::OnReset()
{
	FrameElement::OnReset();

	flags |= UIObject_DB_FocusOnLeftClick | UIObject_DB_CaptureMouseOnLeftClick;
	SetDefaultFrameStyle(DefaultFrameStyle::TextBox);

	_impl->placeholder = {};
}

void Textbox::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();

	auto* font = frameStyle.font.GetFont();
	int size = frameStyle.font.size;

	bool usePlaceholder = !IsFocused() && _impl->text.empty();
	StringView text = usePlaceholder ? _impl->placeholder : _impl->text;
	auto textColor = usePlaceholder ? Color4b(255, 128) : cpa.HasTextColor() ? cpa.GetTextColor() : Color4b::White(); // TODO to theme
	_impl->lines.Recalculate(text, font, size, GetContentRect().GetWidth(), _impl->multiline);
	{
		auto r = GetContentRect();
		float y = r.y0;
		for (TextRange line : _impl->lines.lines)
		{
			draw::TextLine(font, size, r.x0, y, line.sv(text).rtrim(), textColor, TextBaseline::Top, &r);
			y += size;
		}

		if (IsFocused())
		{
			auto startCursor = _impl->startCursor;
			auto endCursor = _impl->endCursor;

			if (startCursor != endCursor)
			{
				size_t minpos = startCursor < endCursor ? startCursor : endCursor;
				size_t maxpos = startCursor > endCursor ? startCursor : endCursor;
				size_t minline = _impl->lines.FindLine(minpos);
				size_t maxline = _impl->lines.FindLine(maxpos);
				for (size_t line = minline; line <= maxline; line++)
				{
					auto L = _impl->lines.lines[line];
					size_t minLpos = ui::max(L.start, minpos);
					size_t maxLpos = ui::min(L.end, maxpos);

					float x0 = GetTextWidth(font, size, text.substr(L.start, minLpos - L.start));
					float x1 = GetTextWidth(font, size, text.substr(L.start, maxLpos - L.start));
					float y = r.y0 + line * size;

					AABB2f hlrect = { r.x0 + x0, y, r.x0 + x1, y + size };
					hlrect = hlrect.Intersect(r);

					draw::RectCol(hlrect, Color4f(0.5f, 0.7f, 0.9f, 0.4f));
				}
			}

			if (_impl->showCaretState)
			{
				size_t line = _impl->lines.FindLine(endCursor);
				float x = 0;
				if (line < _impl->lines.lines.Size())
				{
					auto L = _impl->lines.lines[line];
					x = GetTextWidth(font, size, text.substr(L.start, endCursor - L.start));
				}
				float y = r.y0 + line * size;
				if (r.Contains(Vec2f(r.x0 + x, y)))
					draw::RectCol(r.x0 + x, y, r.x0 + x + 1, y + size, Color4b::White());
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

static int GetCharClass(StringView str, size_t pos)
{
	UTF8Iterator it(str);
	it.pos = pos;
	uint32_t c = it.Read();

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
	auto& hadFocusOnFirstClick = tb._impl->hadFocusOnFirstClick;
	auto& lastPressRepeatCount = tb._impl->lastPressRepeatCount;
	auto& origStartCursor = tb._impl->origStartCursor;
	auto& startCursor = tb._impl->startCursor;
	auto& endCursor = tb._impl->endCursor;
	auto& text = tb._impl->text;

	tb._impl->lines.Recalculate(text, tb.frameStyle.font.GetFont(), tb.frameStyle.font.size, tb.GetContentRect().GetWidth(), tb._impl->multiline);

	if (start)
	{
		lastPressRepeatCount = e.numRepeats - 1;
		origStartCursor = tb._FindCursorPos(e.position);
		if (e.numRepeats == 1)
			hadFocusOnFirstClick = tb.IsFocused();
	}
	unsigned mode = (lastPressRepeatCount + (hadFocusOnFirstClick ? 0 : 2)) % 3;
	if (mode != 2)
	{
		startCursor = origStartCursor;
		endCursor = tb._FindCursorPos(e.position);
		if (mode == 1)
		{
			startCursor = PrevWord(text, startCursor);
			endCursor = NextWord(text, endCursor);
		}
	}
	else
	{
		startCursor = 0;
		endCursor = text.size();
	}
}

void Textbox::OnEvent(Event& e)
{
	auto& startCursor = _impl->startCursor;
	auto& endCursor = _impl->endCursor;
	auto& text = _impl->text;

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
		_impl->showCaretState = !_impl->showCaretState;
		if (system) // TODO better check? needed?
			GetNativeWindow()->InvalidateAll(); // TODO localized
		if (IsFocused())
			e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == EventType::GotFocus)
	{
		_impl->showCaretState = true;
		GetNativeWindow()->InvalidateAll(); // TODO localized
		startCursor = 0;
		endCursor = text.size();
		e.context->SetTimer(this, 0.5f);
	}
	else if (e.type == EventType::LostFocus)
	{
		e.context->OnCommit(this);
		e.StopPropagation();
	}
	else if (e.type == EventType::KeyAction)
	{
		switch (e.GetKeyAction())
		{
		case KeyAction::Enter:
			if (_impl->multiline)
			{
				if (!IsInputDisabled())
					EnterText("\n");
			}
			else
				e.context->SetKeyboardFocus(nullptr);
			break;

		case KeyAction::PrevLetter:
			if (IsLongSelection() && !e.GetKeyActionModifier())
			{
				startCursor = endCursor = startCursor < endCursor ? startCursor : endCursor;
			}
			else
			{
				endCursor = PrevChar(text, endCursor);
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
				endCursor = NextChar(text, endCursor);
				if (!e.GetKeyActionModifier())
					startCursor = endCursor;
			}
			break;

		case KeyAction::PrevWord:
			endCursor = PrevWord(text, endCursor);
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;
		case KeyAction::NextWord:
			endCursor = NextWord(text, endCursor);
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
			endCursor = text.size();
			if (!e.GetKeyActionModifier())
				startCursor = endCursor;
			break;

		case KeyAction::Cut:
			Clipboard::SetText(GetSelectedText());
			if (!IsInputDisabled())
			{
				EraseSelection();
				_OnChangeStyle();
				e.context->OnChange(this);
			}
			break;
		case KeyAction::Copy:
			Clipboard::SetText(GetSelectedText());
			break;
		case KeyAction::SelectAll:
			startCursor = 0;
			endCursor = text.size();
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
						to = PrevChar(text, endCursor);
						text.erase(to, endCursor - to);
						startCursor = endCursor = to;
						break;
					case KeyAction::DelNextLetter:
						to = NextChar(text, endCursor);
						text.erase(endCursor, to - endCursor);
						break;
					case KeyAction::DelPrevWord:
						to = PrevWord(text, endCursor);
						text.erase(to, endCursor - to);
						startCursor = endCursor = to;
						break;
					case KeyAction::DelNextWord:
						to = NextWord(text, endCursor);
						text.erase(endCursor, to - endCursor);
						break;
					}
				}
				_OnChangeStyle();
				e.context->OnChange(this);
				break;
			case KeyAction::Paste:
				EnterText(Clipboard::GetText().c_str());
				break;
			}
		}
		e.StopPropagation();
	}
	else if (e.type == EventType::TextInput)
	{
		if (!IsInputDisabled())
		{
			char ch[5];
			if (e.GetUTF32Char() >= 32 && e.GetUTF32Char() != 127 && e.GetUTF8Text(ch))
				EnterText(ch);
			e.StopPropagation();
		}
	}
	else if (e.type == EventType::KeyDown || e.type == EventType::KeyUp)
	{
		// TODO limit the scope of these events to capture only the processed keys
		if (e.shortCode != ui::KSC_Escape)
			e.StopPropagation();
	}
}

EstSizeRange Textbox::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float minWidth = frameStyle.font.size * 2 + frameStyle.padding.x0 + frameStyle.padding.x1;
	return FrameElement::CalcEstimatedWidth(containerSize, type).WithSoftMin(minWidth);
}

EstSizeRange Textbox::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	bool usePlaceholder = !IsFocused() && _impl->text.empty();
	StringView text = usePlaceholder ? _impl->placeholder : _impl->text;
	float maxWidth = GetContentRect().GetWidth();
	if (maxWidth <= 0)
		maxWidth = containerSize.x - frameStyle.padding.x0 - frameStyle.padding.x1;
	_impl->lines.Recalculate(text, frameStyle.font.GetFont(), frameStyle.font.size, maxWidth, _impl->multiline);

	float minHeight = frameStyle.font.size * ui::max(size_t(1), _impl->lines.lines.size());

	minHeight += frameStyle.padding.y0 + frameStyle.padding.y1;
	return FrameElement::CalcEstimatedHeight(containerSize, type).WithSoftMin(minHeight);
}

bool Textbox::IsMultiline() const
{
	return _impl->multiline;
}

Textbox& Textbox::SetMultiline(bool ml)
{
	_impl->multiline = ml;
	return *this;
}

bool Textbox::IsLongSelection() const
{
	return _impl->startCursor != _impl->endCursor;
}

StringView Textbox::GetSelectedText() const
{
	auto startCursor = _impl->startCursor;
	auto endCursor = _impl->endCursor;
	int min = startCursor < endCursor ? startCursor : endCursor;
	int max = startCursor > endCursor ? startCursor : endCursor;
	return StringView(_impl->text.data() + min, max - min);
}

void Textbox::EnterText(const char* str)
{
	EraseSelection();
	size_t num = strlen(str);
	_impl->text.insert(_impl->endCursor, str, num);
	_impl->startCursor = _impl->endCursor += num;

	_OnChangeStyle();

	system->eventSystem.OnChange(this);
}

void Textbox::EraseSelection()
{
	if (IsLongSelection())
	{
		auto& startCursor = _impl->startCursor;
		auto& endCursor = _impl->endCursor;
		int min = startCursor < endCursor ? startCursor : endCursor;
		int max = startCursor > endCursor ? startCursor : endCursor;
		_impl->text.erase(min, max - min);
		startCursor = endCursor = min;
	}
}

size_t Textbox::_FindCursorPos(Point2f vp)
{
	if (_impl->text.empty())
		return 0;

	auto r = GetContentRect();
	size_t line = vp.y < r.y0 ? 0 : min(size_t(floorf((vp.y - r.y0) / frameStyle.font.size)), _impl->lines.lines.size() - 1);
	auto L = _impl->lines.lines[line];

	auto lineText = L.sv(_impl->text);
	if (lineText.ends_with("\n"))
	{
		lineText = lineText.substr(0, lineText.size() - 1);
		if (lineText.ends_with("\r"))
			lineText = lineText.substr(0, lineText.size() - 1);
	}

	TextMeasureBegin(frameStyle.font.GetFont(), frameStyle.font.size);
	float lpx = vp.x - r.x0;
	float bestDist = fabsf(lpx);
	size_t bestPos = 0;
	UTF8Iterator it(lineText);
	for (;;)
	{
		uint32_t c = it.Read();
		if (c == UTF8Iterator::END)
			break;

		float nw = TextMeasureAddChar(c);
		float nd = fabsf(lpx - nw);
		if (nd <= bestDist)
		{
			bestDist = nd;
			bestPos = it.pos;
		}
		else
			break; // distance increasing so no value in processing the rest
	}
	TextMeasureEnd();

	return L.start + bestPos;
}

const std::string& Textbox::GetText() const
{
	return _impl->text;
}

Textbox& Textbox::SetText(StringView s)
{
	if (InUse())
		return *this;
	auto& text = _impl->text;
	text <<= s;
	if (_impl->startCursor > text.size())
		_impl->startCursor = text.size();
	if (_impl->endCursor > text.size())
		_impl->endCursor = text.size();
	return *this;
}

Textbox& Textbox::SetPlaceholder(StringView s)
{
	_impl->placeholder <<= s;
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
