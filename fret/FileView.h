
#pragma once
#include "pch.h"

struct Workspace;
struct OpenedFile;
struct HexViewer;

struct FileView : ui::Node
{
	void Render(UIContainer* ctx) override;

	Workspace* workspace = nullptr;
	OpenedFile* of = nullptr;

	HexViewer* curHexViewer = nullptr;
};

