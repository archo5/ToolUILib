
#pragma once
#include "pch.h"

struct OpenedFile;


struct TabInspect : ui::Node
{
	void Render(UIContainer* ctx) override;

	OpenedFile* of = nullptr;
};
