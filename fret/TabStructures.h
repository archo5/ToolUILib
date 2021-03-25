
#pragma once
#include "pch.h"
#include "TableWithOffsets.h"

struct Workspace;


struct TabStructures : ui::Buildable, TableWithOffsets
{
	void Build() override;

	Workspace* workspace = nullptr;
};
