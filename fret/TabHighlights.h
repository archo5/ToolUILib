
#pragma once
#include "pch.h"

struct OpenedFile;


struct TabHighlights : ui::Node
{
	void Render(UIContainer* ctx) override;

	OpenedFile* of = nullptr;
};
