
#pragma once
#include "pch.h"

#include "TableWithOffsets.h"

struct OpenedFile;


struct TabMarkers : ui::Buildable, TableWithOffsets
{
	void Build() override;

	OpenedFile* of = nullptr;
};
