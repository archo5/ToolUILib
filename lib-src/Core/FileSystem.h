
#pragma once

#include "String.h"
#include "Array.h"
#include "RefCounted.h"


namespace ui {

enum class IOResult : uint8_t
{
	Success = 0,
	Unknown,
	FileNotFound,
};

struct IBuffer : RefCountedMT
{
	virtual void* Data() const = 0;
	virtual size_t Size() const = 0;

	StringView GetStringView() const { return { static_cast<const char*>(Data()), Size() }; }
};
using BufferHandle = RCHandle<IBuffer>;

struct BasicMemoryBuffer : IBuffer
{
	void* data = nullptr;
	size_t size = 0;

	void* Data() const override { return data; }
	size_t Size() const override { return size; }
};

struct OwnedMemoryBuffer : BasicMemoryBuffer
{
	~OwnedMemoryBuffer() { delete[] data; }

	void Alloc(size_t s)
	{
		data = new char[s];
	}
};

struct FileReadResult
{
	IOResult result;
	BufferHandle data;
};

// returns an empty string if root was passed
UI_API std::string PathFromSystem(StringView path);
UI_API std::string PathGetParent(StringView path);
UI_API std::string PathJoin(StringView a, StringView b);
UI_API bool PathIsAbsolute(StringView path);
UI_API std::string PathGetAbsolute(StringView path);
UI_API std::string PathGetResolvedIfReachable(StringView path);
UI_API std::string PathGetRelativeTo(StringView path, StringView relativeTo);
UI_API bool PathIsRelativeTo(StringView path, StringView relativeTo);

UI_API FileReadResult ReadTextFile(StringView path);
UI_API bool WriteTextFile(StringView path, StringView text);
UI_API FileReadResult ReadBinaryFile(StringView path);
UI_API bool WriteBinaryFile(StringView path, const void* data, size_t size);

UI_API bool DirectoryExists(StringView path);
UI_API bool CreateDirectory(StringView path);
UI_API bool CreateMissingDirectories(StringView path);
UI_API bool CreateMissingParentDirectories(StringView path);

UI_API bool DeleteFile(StringView path);
UI_API bool DeleteDirectory(StringView path);
UI_API bool MoveFile(StringView from, StringView to);

UI_API std::string GetWorkingDirectory();
UI_API bool SetWorkingDirectory(StringView sv);

enum class SystemDirectoryType : u8
{
	Documents,
	Desktop,
	Downloads,
	Pictures,
	Videos,
	UserDataRoot,

	Windows_LocalAppData,
	Windows_LocalLowAppData,
	Windows_RoamingAppData,
};
UI_API std::string GetSystemDirPath(SystemDirectoryType sdt);

enum FileAttributes
{
	FA_Exists = 1 << 0,
	FA_Directory = 1 << 1,
	FA_Symlink = 1 << 2,
};
UI_API unsigned GetFileAttributes(StringView path);
UI_API uint64_t GetFileSize(StringView path);
UI_API uint64_t GetFileModTimeUTC(StringView path);
UI_API uint64_t GetFileModTimeUnixMS(StringView path);
UI_API u64 GetTimeUnixMS();

// TODO allow single user only
struct IDirectoryIterator : RefCountedST
{
	virtual bool GetNext(std::string& retFile) = 0;
};
using DirectoryIteratorHandle = RCHandle<IDirectoryIterator>;
UI_API DirectoryIteratorHandle CreateDirectoryIterator(StringView path);


struct NullDirectoryIterator : IDirectoryIterator
{
	bool GetNext(std::string&) override
	{
		return false;
	}
};


struct IDirectoryChangeListener
{
	// in this context, "file" refers to any type of file (could be directory, symlink etc.)
	// "path" is relative to the listened path
	// the file may be gone by the time this function is called
	virtual void OnFileAdded(StringView path) = 0;
	virtual void OnFileRemoved(StringView path) = 0;
	virtual void OnFileChanged(StringView path) = 0;
};
struct IDirectoryChangeWatcher : RefCountedST
{
};
using DirectoryChangeWatcherHandle = RCHandle<IDirectoryChangeWatcher>;
UI_API DirectoryChangeWatcherHandle CreateDirectoryChangeWatcher(StringView path, IDirectoryChangeListener* listener);


struct IFileSource : RefCountedST
{
	virtual FileReadResult ReadTextFile(StringView path) = 0;
	virtual FileReadResult ReadBinaryFile(StringView path) = 0;
	virtual unsigned GetFileAttributes(StringView path) = 0;
	virtual DirectoryIteratorHandle CreateDirectoryIterator(StringView path) = 0;
};
using FileSourceHandle = RCHandle<IFileSource>;

struct FileSourceSequence : IFileSource
{
	Array<FileSourceHandle> fileSystems;

	FileReadResult ReadTextFile(StringView path) override;
	FileReadResult ReadBinaryFile(StringView path) override;
	unsigned GetFileAttributes(StringView path) override;
	DirectoryIteratorHandle CreateDirectoryIterator(StringView path) override;
};

UI_API FileSourceHandle CreateFileSystemSource(StringView rootPath);
UI_API FileSourceHandle CreateZipFileSource(StringView path);
UI_API FileSourceHandle CreateZipFileMemorySource(BufferHandle memory);


UI_API FileSourceSequence* FSGetDefault();
UI_API IFileSource* FSGetCurrent();
UI_API void FSSetCurrent(IFileSource* fs);

UI_API inline FileReadResult FSReadTextFile(StringView path) { return FSGetCurrent()->ReadTextFile(path); }
UI_API inline FileReadResult FSReadBinaryFile(StringView path) { return FSGetCurrent()->ReadBinaryFile(path); }
UI_API inline DirectoryIteratorHandle FSCreateDirectoryIterator(StringView path) { return FSGetCurrent()->CreateDirectoryIterator(path); }

} // ui
