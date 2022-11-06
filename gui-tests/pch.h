
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <functional>
#include <algorithm>

#include "../lib-src/GUI.h"


inline ui::RadioButtonT<int>& BasicRadioButton(const char* text, int& iref, int val)
{
	auto& rb = ui::Push<ui::RadioButtonT<int>>().Init(iref, val);
	ui::Push<ui::StackLTRLayoutElement>();
	ui::Make<ui::RadioButtonIcon>();
	ui::MakeWithText<ui::LabelFrame>(text);
	ui::Pop();
	ui::Pop();
	return rb;
}

inline ui::RadioButtonT<int>& BasicRadioButton2(const char* text, int& iref, int val)
{
	auto& rb = ui::Push<ui::RadioButtonT<int>>().Init(iref, val);
	ui::MakeWithText<ui::StateButtonSkin>(text);
	ui::Pop();
	return rb;
}

inline bool& GetWrapperSetting()
{
	static bool wrapperSetting;
	return wrapperSetting;
}

using TESTWrapper = ui::WrapperElement;

template <class E> inline E& WPush()
{
	if (GetWrapperSetting())
		ui::Push<TESTWrapper>();
	return ui::Push<E>();
}

inline void WPop()
{
	ui::Pop();
	if (GetWrapperSetting())
		ui::Pop();
}

template <class E> inline E& WMake()
{
	if (GetWrapperSetting())
		ui::Push<TESTWrapper>();
	auto& e = ui::Make<E>();
	if (GetWrapperSetting())
		ui::Pop();
	return e;
}

inline ui::TextElement& WText(ui::StringView s)
{
	auto& T = WMake<ui::TextElement>();
	T.SetText(s);
	return T;
}

template <class E> inline E& WMakeWithText(ui::StringView s)
{
	auto& e = WPush<E>();
	WText(s);
	WPop();
	return e;
}


struct EventsTest : ui::Buildable
{
	static constexpr unsigned MAX_MESSAGES = 50;

	std::string msgBuf[MAX_MESSAGES];
	unsigned writePos = 0;

	void WriteMsg(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, 1024, fmt, args);
		va_end(args);
		msgBuf[writePos++] = buf;
		writePos %= MAX_MESSAGES;
		GetNativeWindow()->InvalidateAll();
	}

	void OnPaint(const ui::UIPaintContext& ctx) override
	{
		auto* font = ui::GetFont(ui::FONT_FAMILY_SANS_SERIF);
		for (unsigned i = 0; i < MAX_MESSAGES; i++)
		{
			unsigned idx = (MAX_MESSAGES * 2 + writePos - i - 1) % MAX_MESSAGES;
			ui::draw::TextLine(font, 12, 0, GetFinalRect().y1 - i * 12, msgBuf[idx], ui::Color4f::White());
		}
	}
	void Build() override
	{
	}
};
