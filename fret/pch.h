
#pragma once

#pragma warning(disable:4996)
#define NOMINMAX
#include <winsock2.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "../GUI.h"

enum GlobalEvents
{
	GlobalEvent_OpenImage,
	GlobalEvent_OpenImageRsrcEditor,
};
