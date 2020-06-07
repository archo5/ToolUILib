
#pragma once
#include "pch.h"

struct Workspace;


struct TabImages : ui::Node
{
	void Render(UIContainer* ctx) override;

	Workspace* workspace = nullptr;
};
