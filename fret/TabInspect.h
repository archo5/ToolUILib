
#pragma once
#include "pch.h"

struct OpenedFile;


struct TabInspect : ui::Buildable
{
	void Build() override;

	OpenedFile* of = nullptr;
};
