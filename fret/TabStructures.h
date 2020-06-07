
#pragma once
#include "pch.h"
#include "TableWithOffsets.h"

struct Workspace;


struct TabStructures : ui::Node, TableWithOffsets
{
	void Render(UIContainer* ctx) override;

	Workspace* workspace = nullptr;
};
