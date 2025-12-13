
#pragma once
#include "../Model/Controls.h"


namespace ui {

struct Textbox : FrameElement
{
	Textbox();
	~Textbox();

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	EstSizeRange CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	bool IsMultiline() const;
	Textbox& SetMultiline(bool ml);

	bool IsLongSelection() const;
	StringView GetSelectedText() const;
	void EnterText(const char* str);
	void EraseSelection();

	size_t _FindCursorPos(Point2f vp);

	const std::string& GetText() const;
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

	struct TextboxImpl* _impl;
};

} // ui
