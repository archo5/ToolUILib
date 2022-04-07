
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <functional>
#include <algorithm>
#include <vector>
#include <unordered_set>

#include "../GUI.h"

#include "../Model/WIP.h"


inline ui::RadioButtonT<int>& BasicRadioButton(const char* text, int& iref, int val)
{
	auto& rb = ui::Push<ui::RadioButtonT<int>>().Init(iref, val);
	ui::Push<ui::StackLTRLayoutElement>();
	ui::Make<ui::RadioButtonIcon>();
	ui::Text(text) + ui::SetPadding(4);
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

struct TESTWrapper : ui::UIElement
{
	ui::Rangef GetFullEstimatedWidth(const ui::Size2f& containerSize, ui::EstSizeType type, bool forParentLayout = true) override
	{
		return firstChild->GetFullEstimatedWidth(containerSize, type, forParentLayout);
	}
	ui::Rangef GetFullEstimatedHeight(const ui::Size2f& containerSize, ui::EstSizeType type, bool forParentLayout = true) override
	{
		return firstChild->GetFullEstimatedHeight(containerSize, type, forParentLayout);
	}
	void OnLayout(const ui::UIRect& rect, const ui::Size2f& containerSize) override
	{
		firstChild->PerformLayout(rect, containerSize);
		finalRectCP = finalRectC = firstChild->finalRectCP;
	}
};

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
	return e;
}
