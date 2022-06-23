
#pragma once
#include "../Model/Controls.h"


namespace ui {

struct Textbox : FrameElement
{
	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	bool IsLongSelection() const { return startCursor != endCursor; }
	StringView GetSelectedText() const;
	void EnterText(const char* str);
	void EraseSelection();

	size_t _FindCursorPos(float vpx);

	const std::string& GetText() const { return _text; }
	Textbox& SetText(StringView s);
	Textbox& SetPlaceholder(StringView s);

	Textbox& Init(float& val);
	template <size_t N> Textbox& Init(char(&val)[N])
	{
		SetText(val);
		HandleEvent(EventType::Change) = [this, &val](Event&)
		{
			auto& t = GetText();
			size_t s = min(t.size(), N - 1);
			memcpy(val, t.c_str(), s);
			val[s] = 0;
		};
		return *this;
	}

	std::string _text;
	std::string _placeholder;
	size_t _origStartCursor = 0;
	size_t startCursor = 0;
	size_t endCursor = 0;
	bool showCaretState = false;
	bool _hadFocusOnFirstClick = false;
	unsigned _lastPressRepeatCount = 0;
	float accumulator = 0;
};

struct TextboxPlaceholder : Modifier
{
	StringView placeholder;

	TextboxPlaceholder(StringView pch) : placeholder(pch) {}

	void Apply(UIObject* obj) const override
	{
		if (auto* tb = dynamic_cast<Textbox*>(obj))
			tb->SetPlaceholder(placeholder);
	}
};

} // ui
