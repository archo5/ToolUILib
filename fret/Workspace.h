
#pragma once
#include "pch.h"

#include "HexViewer.h"
#include "FileReaders.h"


enum class SubtabType
{
	Inspect = 0,
	Highlights = 1,
	Markers = 2,
	Structures = 3,
	Images = 4,
};

struct OpenedFile
{
	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);

	DDFile* ddFile = nullptr;
	uint64_t fileID = 0;
	HexViewerState hexViewerState;
	HighlightSettings highlightSettings;
};

struct Workspace
{
	Workspace()
	{
		ddiSrc.dataDesc = &desc;
		ddimgSrc.dataDesc = &desc;
	}
	~Workspace()
	{
		Clear();
	}

	void Clear()
	{
		desc.Clear();
		for (auto* F : openedFiles)
			delete F;
		openedFiles.clear();
	}

	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);
	bool LoadFromFile(ui::StringView path);
	bool SaveToFile(ui::StringView path);

	std::vector<OpenedFile*> openedFiles;
	int curOpenedFile = 0;
	SubtabType curSubtab = SubtabType::Markers;
	DataDesc desc;
	DataDescInstanceSource ddiSrc;
	DataDescImageSource ddimgSrc;

	// runtime cache
	CachedImage cachedImg;
};
