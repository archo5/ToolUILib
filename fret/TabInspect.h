
#pragma once
#include "pch.h"

struct OpenedFile;


struct TabInspect : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override;

	OpenedFile* of = nullptr;
};
