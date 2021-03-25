
#pragma once
#include "pch.h"

struct Workspace;


struct TabImages : ui::Buildable
{
	void Build() override;

	Workspace* workspace = nullptr;
};
