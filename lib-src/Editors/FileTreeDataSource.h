
#pragma once

#include "Tables.h"
#include "../Core/FileSystem.h"
#include "../Core/HashSet.h"


namespace ui {

struct FileTreeDataSource : TreeDataSource, ISelectionStorage, IDirectoryChangeListener
{
	struct File : RefCountedST
	{
		std::string name;
		uint64_t size;
		uint64_t modTimeUnixMS;
		bool isDir = false;
		bool selected = false;
		bool open = true;
		bool _triedLoadingIcon = false;

		draw::ImageSetHandle _cachedIcon;

		Array<RCHandle<File>> subfiles;

		void InitFrom(const StringView& fullPath, bool recursive = false);
		draw::ImageSetHandle GetIcon();
	};

	std::string _rootPath;
	RCHandle<File> _root;
	DirectoryChangeWatcherHandle _dcw;
	HashSet<File*> _selected;
	MulticastDelegate<> onChange;

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
	draw::ImageSetHandle GetIcon(uintptr_t id, size_t col) override;
	size_t GetChildCount(uintptr_t id) override;
	uintptr_t GetChild(uintptr_t id, size_t which) override;
	bool IsOpen(uintptr_t id) override;
	void ToggleOpenState(uintptr_t id) override;

	// ISelectionStorage
	void ClearSelection() override;
	bool GetSelectionState(uintptr_t item) override;
	void SetSelectionState(uintptr_t item, bool sel) override;

	// IDirectoryChangeListener utils
	RCHandle<File> GetFileFromPath(StringView path, bool rebuildIfMissing);

	// IDirectoryChangeListener
	void OnFileAdded(StringView path) override;
	void OnFileRemoved(StringView path) override;
	void OnFileChanged(StringView path) override;
};

} // ui
