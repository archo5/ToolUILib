
#pragma once
#include "pch.h"

struct OpenedFile;


struct TabHighlights : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override;

	OpenedFile* of = nullptr;
};
