
#pragma once

#include "Tables.h"
#include "../Core/FileSystem.h"


namespace ui {

struct FileTreeDataSource : ui::TreeDataSource, ui::ISelectionStorage, ui::IDirectoryChangeListener
{
	struct File : ui::RefCountedST
	{
		std::string name;
		uint64_t size;
		uint64_t modTimeUnixMS;
		bool isDir = false;
		bool selected = false;
		bool open = true;

		std::vector<ui::RCHandle<File>> subfiles;

		void InitFrom(const ui::StringView& fullPath, bool recursive = false);
	};

	std::string _rootPath;
	ui::RCHandle<File> _root;
	ui::DirectoryChangeWatcherHandle _dcw;
	ui::HashMap<File*, ui::NoValue> _selected;
	ui::MulticastDelegate<> onChange;

	FileTreeDataSource(std::string rootPath);

	// TreeDataSource utils
	File* GetFileFromID(uintptr_t id)
	{
		if (id == ROOT)
			return _root;
		return reinterpret_cast<File*>(id);
	}

	// TreeDataSource
	enum Columns
	{
		COL_Name,
		COL_Size,
		COL_LastModified,

		COL_COUNT,
	};

	size_t GetNumCols() override { return COL_COUNT; }
	std::string GetColName(size_t col) override;
	std::string GetText(uintptr_t id, size_t col) override;
	size_t GetChildCount(uintptr_t id) override;
	uintptr_t GetChild(uintptr_t id, size_t which) override;
	bool IsOpen(uintptr_t id) override;

	// ISelectionStorage
	void ClearSelection() override;
	bool GetSelectionState(uintptr_t item) override;
	void SetSelectionState(uintptr_t item, bool sel) override;

	// IDirectoryChangeListener utils
	ui::RCHandle<File> GetFileFromPath(ui::StringView path, bool rebuildIfMissing);

	// IDirectoryChangeListener
	void OnFileAdded(ui::StringView path) override;
	void OnFileRemoved(ui::StringView path) override;
	void OnFileChanged(ui::StringView path) override;
};

} // ui
