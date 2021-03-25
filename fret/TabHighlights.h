
#pragma once
#include "pch.h"

struct OpenedFile;


struct TabHighlights : ui::Buildable
{
	void Build() override;

	OpenedFile* of = nullptr;
};
