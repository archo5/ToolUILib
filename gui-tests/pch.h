
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <functional>
#include <algorithm>
#include <vector>
#include <unordered_set>

#include "../GUI.h"

inline ui::RadioButtonT<int>& BasicRadioButton(const char* text, int& iref, int val)
{
	auto& rb = ui::Push<ui::RadioButtonT<int>>().Init(iref, val);
	ui::Make<ui::RadioButtonIcon>();
	ui::Text(text) + ui::SetPadding(4);
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
