
#pragma once
#include "pch.h"

struct Workspace;
struct OpenedFile;
struct HexViewer;
struct DDStruct;

struct FileView : ui::Node
{
	void Render(UIContainer* ctx) override;

	void HexViewer_OnRightClick();

	DDStruct* CreateBlankStruct(int64_t pos);
	DDStruct* CreateStructFromMarkers(int64_t pos);
	void CreateImage(int64_t pos, StringView fmt);
	void GoToOffset(int64_t pos);

	Workspace* workspace = nullptr;
	OpenedFile* of = nullptr;

	HexViewer* curHexViewer = nullptr;
};

