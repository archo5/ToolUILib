
#include "pch.h"
#include "Workspace.h"


void OpenedFile::Load(NamedTextSerializeReader& r)
{
	r.BeginDict("");
	fileID = r.ReadUInt64("fileID");
	hexViewerState.basePos = r.ReadUInt64("basePos");
	hexViewerState.byteWidth = r.ReadUInt("byteWidth");
	highlightSettings.Load("highlighter", r);
	r.EndDict();
}

void OpenedFile::Save(NamedTextSerializeWriter& w)
{
	w.BeginDict("");
	w.WriteInt("fileID", fileID);
	w.WriteInt("basePos", hexViewerState.basePos);
	w.WriteInt("byteWidth", hexViewerState.byteWidth);
	highlightSettings.Save("highlighter", w);
	w.EndDict();
}

void Workspace::Load(NamedTextSerializeReader& r)
{
	Clear();
	r.BeginDict("workspace");

	desc.Load("desc", r);
	for (auto* F : desc.files)
	{
		std::string path = F->path;
		if (path.size() < 2 || path[1] != ':')
			path = "FRET_Plugins/" + path;
		F->dataSource = new FileDataSource(path.c_str());
		F->mdSrc.dataSource = F->dataSource;
	}

	r.BeginArray("openedFiles");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);

		auto* F = new OpenedFile;
		F->Load(r);
		F->ddFile = desc.FindFileByID(F->fileID);
		openedFiles.push_back(F);

		r.EndEntry();
	}
	r.EndArray();

	r.EndDict();
}

void Workspace::Save(NamedTextSerializeWriter& w)
{
	w.BeginDict("workspace");
	w.WriteString("version", "1");

	desc.Save("desc", w);

	w.BeginArray("openedFiles");
	for (auto* F : openedFiles)
		F->Save(w);
	w.EndArray();

	w.EndDict();
}
