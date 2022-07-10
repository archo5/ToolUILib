
#include "FileTreeDataSource.h"

#include "../Core/Logging.h"

#include <time.h>


namespace ui {

LogCategory LOG_FTDS("FileTreeDataSource");


static std::string SizeToText(uint64_t size)
{
	if (size < 1024)
		return std::to_string(size) + " B";
	if (size < 1024 * 1024)
		return ui::Format("%.2f KB", size / 1024.0);
	if (size < 1024 * 1024 * 1024)
		return ui::Format("%.2f MB", size / (1024.0 * 1024.0));
	return ui::Format("%.2f GB", size / (1024.0 * 1024.0 * 1024.0));
}

static std::string TimestampToText(uint64_t unixMS)
{
	time_t t = unixMS / 1000;
	tm tm = {};
	localtime_s(&tm, &t);
	char buf[32];
	strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &tm);
	return buf;
}


void FileTreeDataSource::File::InitFrom(const ui::StringView& fullPath, bool recursive)
{
	name = ui::to_string(fullPath.after_last("/"));
	size = ui::GetFileSize(fullPath);
	modTimeUnixMS = ui::GetFileModTimeUnixMS(fullPath);
	isDir = !!(ui::GetFileAttributes(fullPath) & ui::FA_Directory);

	subfiles.clear();
	if (recursive && isDir)
	{
		auto dih = ui::CreateDirectoryIterator(fullPath);
		std::string file;
		while (dih->GetNext(file))
		{
			auto chFullPath = ui::PathJoin(fullPath, file);
			auto* F = new File;
			F->InitFrom(chFullPath, true);
			subfiles.push_back(F);
		}
	}
}

FileTreeDataSource::FileTreeDataSource(std::string rootPath) : _rootPath(rootPath)
{
	_dcw = ui::CreateDirectoryChangeWatcher(rootPath, this);
	_root = new File;
	_root->InitFrom(rootPath, true);
}

std::string FileTreeDataSource::GetColName(size_t col)
{
	switch (col)
	{
	case COL_Name: return "Name";
	case COL_Size: return "Size";
	case COL_LastModified: return "Last modified";
	default: return "???";
	}
}

std::string FileTreeDataSource::GetText(uintptr_t id, size_t col)
{
	auto* F = GetFileFromID(id);
	switch (col)
	{
	case COL_Name: return F->name;
	case COL_Size:
		if (F->isDir)
			return ui::Format("%zu files", F->subfiles.size());
		else
			return SizeToText(F->size);
	case COL_LastModified: return TimestampToText(F->modTimeUnixMS);
	default: return "???";
	}
}

size_t FileTreeDataSource::GetChildCount(uintptr_t id)
{
	auto* F = GetFileFromID(id);
	if (!F->isDir)
		return 0;
	return F->subfiles.size();
}

uintptr_t FileTreeDataSource::GetChild(uintptr_t id, size_t which)
{
	auto* F = GetFileFromID(id);
	return reinterpret_cast<uintptr_t>(F->subfiles[which].get_ptr());
}

bool FileTreeDataSource::IsOpen(uintptr_t id)
{
	return GetFileFromID(id)->open;
}

void FileTreeDataSource::ClearSelection()
{
	for (const auto& kvp : _selected)
		kvp.key->selected = false;
	_selected.clear();
}

bool FileTreeDataSource::GetSelectionState(uintptr_t item)
{
	auto* F = GetFileFromID(item);
	return F->selected;
}

void FileTreeDataSource::SetSelectionState(uintptr_t item, bool sel)
{
	auto* F = GetFileFromID(item);
	if (F->selected != sel)
	{
		F->selected = sel;
		if (sel)
			_selected.insert(F, {});
		else
			_selected.erase(F);
	}
}

ui::RCHandle<FileTreeDataSource::File> FileTreeDataSource::GetFileFromPath(ui::StringView path, bool rebuildIfMissing)
{
	auto name = path.after_last("/");
	auto parent = name.size() < path.size() ? path.substr(0, path.size() - name.size() - 1) : ui::StringView();
	auto parentFile = parent.empty() ? _root : GetFileFromPath(parent, rebuildIfMissing);
	if (!parentFile)
		return nullptr;
	bool secondTry = false;
	for (;;)
	{
		if (parentFile->isDir)
		{
			for (auto& chf : parentFile->subfiles)
			{
				if (chf->name == name)
					return chf;
			}
		}
		if (secondTry)
			break;
		else
			secondTry = true;
		if (rebuildIfMissing)
		{
			parentFile->InitFrom(ui::to_string(_rootPath, path), true);
			onChange.Call();
		}
	}
	return nullptr;
}

void FileTreeDataSource::OnFileAdded(ui::StringView path)
{
	auto F = GetFileFromPath(ui::PathGetParent(path), true);
	if (!F)
	{
		LogWarn(LOG_FTDS, "missing file: %.*s", int(path.size()), path.data());
		return;
	}
	if (!F->isDir)
	{
		F->InitFrom(ui::PathJoin(_rootPath, ui::PathGetParent(path)), true);
		onChange.Call();
		if (!F->isDir)
		{
			LogWarn(LOG_FTDS, "expected directory: %.*s", int(path.size()), path.data());
			return;
		}
	}
	auto name = path.after_last("/");
	for (auto& chf : F->subfiles)
	{
		if (chf->name == name)
		{
			F->subfiles.erase(F->subfiles.begin() + (&chf - F->subfiles.data()));
			break;
		}
	}
	auto* chf = new File;
	chf->InitFrom(ui::PathJoin(_rootPath, path));
	F->subfiles.push_back(chf);
	onChange.Call();
}

void FileTreeDataSource::OnFileRemoved(ui::StringView path)
{
	auto F = GetFileFromPath(ui::PathGetParent(path), true);
	if (!F)
	{
		LogWarn(LOG_FTDS, "missing file: %.*s", int(path.size()), path.data());
		return;
	}
	if (!F->isDir)
	{
		F->InitFrom(ui::PathJoin(_rootPath, ui::PathGetParent(path)), true);
		onChange.Call();
		if (!F->isDir)
		{
			LogWarn(LOG_FTDS, "expected directory: %.*s", int(path.size()), path.data());
			return;
		}
	}
	auto name = path.after_last("/");
	for (auto& chf : F->subfiles)
	{
		if (chf->name == name)
		{
			F->subfiles.erase(F->subfiles.begin() + (&chf - F->subfiles.data()));
			break;
		}
	}
	onChange.Call();
}

void FileTreeDataSource::OnFileChanged(ui::StringView path)
{
	auto F = GetFileFromPath(path, true);
	if (!F)
	{
		LogWarn(LOG_FTDS, "missing file: %.*s", int(path.size()), path.data());
		return;
	}
	if (F->isDir)
		return; // changes covered by the two callbacks above
	F->InitFrom(ui::PathJoin(_rootPath, path));
	onChange.Call();
}

} // ui
