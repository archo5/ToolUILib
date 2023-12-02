
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
std::string PathFromSystem(StringView path);
std::string PathGetParent(StringView path);
std::string PathJoin(StringView a, StringView b);
bool PathIsAbsolute(StringView path);
std::string PathGetAbsolute(StringView path);
std::string PathGetResolvedIfReachable(StringView path);
std::string PathGetRelativeTo(StringView path, StringView relativeTo);
bool PathIsRelativeTo(StringView path, StringView relativeTo);

FileReadResult ReadTextFile(StringView path);
bool WriteTextFile(StringView path, StringView text);
FileReadResult ReadBinaryFile(StringView path);
bool WriteBinaryFile(StringView path, const void* data, size_t size);

bool DirectoryExists(StringView path);
bool CreateDirectory(StringView path);
bool CreateMissingDirectories(StringView path);
bool CreateMissingParentDirectories(StringView path);

bool DeleteFile(StringView path);
bool DeleteDirectory(StringView path);
bool MoveFile(StringView from, StringView to);

std::string GetWorkingDirectory();
bool SetWorkingDirectory(StringView sv);

enum FileAttributes
{
	FA_Exists = 1 << 0,
	FA_Directory = 1 << 1,
	FA_Symlink = 1 << 2,
};
unsigned GetFileAttributes(StringView path);
uint64_t GetFileSize(StringView path);
uint64_t GetFileModTimeUTC(StringView path);
uint64_t GetFileModTimeUnixMS(StringView path);
u64 GetTimeUnixMS();

// TODO allow single user only
struct IDirectoryIterator : RefCountedST
{
	virtual bool GetNext(std::string& retFile) = 0;
};
using DirectoryIteratorHandle = RCHandle<IDirectoryIterator>;
DirectoryIteratorHandle CreateDirectoryIterator(StringView path);


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
DirectoryChangeWatcherHandle CreateDirectoryChangeWatcher(StringView path, IDirectoryChangeListener* listener);


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

FileSourceHandle CreateFileSystemSource(StringView rootPath);
FileSourceHandle CreateZipFileSource(StringView path);
FileSourceHandle CreateZipFileMemorySource(BufferHandle memory);


FileSourceSequence* FSGetDefault();
IFileSource* FSGetCurrent();
void FSSetCurrent(IFileSource* fs);

inline FileReadResult FSReadTextFile(StringView path) { return FSGetCurrent()->ReadTextFile(path); }
inline FileReadResult FSReadBinaryFile(StringView path) { return FSGetCurrent()->ReadBinaryFile(path); }
inline DirectoryIteratorHandle FSCreateDirectoryIterator(StringView path) { return FSGetCurrent()->CreateDirectoryIterator(path); }

} // ui
