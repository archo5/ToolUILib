
#pragma once
#include "pch.h"

struct Workspace;


struct TabImages : ui::Buildable
{
	void Build(ui::UIContainer* ctx) override;

	Workspace* workspace = nullptr;
};
