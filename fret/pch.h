
#pragma once

#pragma warning(disable:4996)
#define NOMINMAX
#include <winsock2.h>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "../GUI.h"

enum GlobalEvents
{
	GlobalEvent_OpenImage,
	GlobalEvent_OpenImageRsrcEditor,
};

template <class T>
struct WindowT : ui::NativeMainWindow
{
	void OnRender(UIContainer* ctx) override
	{
		rootNode = ctx->Make<T>();
	}
	T* rootNode = nullptr;
};

using NamedTextSerializeReader = ui::NamedTextSerializeReader;
using NamedTextSerializeWriter = ui::NamedTextSerializeWriter;
