
#pragma once
#include "pch.h"

#include "TableWithOffsets.h"

struct OpenedFile;


struct TabMarkers : ui::Node, TableWithOffsets
{
	void Render(UIContainer* ctx) override;

	OpenedFile* of = nullptr;
};
