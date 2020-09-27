
#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <functional>
#include <algorithm>
#include <vector>
#include <unordered_set>

#include "../GUI.h"

inline ui::RadioButtonT<int>* BasicRadioButton(UIContainer* ctx, const char* text, int& iref, int val)
{
	auto* rb = ctx->Push<ui::RadioButtonT<int>>()->Init(iref, val);
	ctx->Make<ui::RadioButtonIcon>();
	ctx->Text(text) + ui::Padding(4);
	ctx->Pop();
	return rb;
}

inline ui::RadioButtonT<int>* BasicRadioButton2(UIContainer* ctx, const char* text, int& iref, int val)
{
	auto* rb = ctx->Push<ui::RadioButtonT<int>>()->Init(iref, val);
	ctx->MakeWithText<ui::StateButtonSkin>(text);
	ctx->Pop();
	return rb;
}
