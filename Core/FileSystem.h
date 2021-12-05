
#pragma once

#include "String.h"
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
	virtual void* GetData() const = 0;
	virtual size_t GetSize() const = 0;

	StringView GetStringView() const { return { static_cast<const char*>(GetData()), GetSize() }; }
};
using BufferHandle = RCHandle<IBuffer>;

struct OwnedMemoryBuffer : IBuffer
{
	void* data = nullptr;
	size_t size = 0;

	~OwnedMemoryBuffer() { delete[] data; }

	void* GetData() const override { return data; }
	size_t GetSize() const override { return size; }

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
std::string PathGetParent(StringView path);
std::string PathJoin(StringView a, StringView b);
bool PathIsAbsolute(StringView path);
std::string PathGetAbsolute(StringView path);
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

std::string GetWorkingDirectory();
bool SetWorkingDirectory(StringView sv);

enum FileAttributes
{
	FA_Exists = 1 << 0,
	FA_Directory = 1 << 1,
	FA_Symlink = 1 << 2,
};
unsigned GetFileAttributes(StringView path);
uint64_t GetFileModTimeUTC(StringView path);

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


struct IFileSource : RefCountedST
{
	virtual FileReadResult ReadTextFile(StringView path) = 0;
	virtual FileReadResult ReadBinaryFile(StringView path) = 0;
	virtual DirectoryIteratorHandle CreateDirectoryIterator(StringView path) = 0;
};
using FileSourceHandle = RCHandle<IFileSource>;

struct FileSourceSequence : IFileSource
{
	std::vector<FileSourceHandle> fileSystems;

	FileReadResult ReadTextFile(StringView path) override;
	FileReadResult ReadBinaryFile(StringView path) override;
	DirectoryIteratorHandle CreateDirectoryIterator(StringView path) override;
};

FileSourceHandle CreateFileSystemSource(StringView rootPath);


FileSourceSequence* FSGetDefault();
IFileSource* FSGetCurrent();
void FSSetCurrent(IFileSource* fs);

inline FileReadResult FSReadTextFile(StringView path) { return FSGetCurrent()->ReadTextFile(path); }
inline FileReadResult FSReadBinaryFile(StringView path) { return FSGetCurrent()->ReadBinaryFile(path); }
inline DirectoryIteratorHandle FSCreateDirectoryIterator(StringView path) { return FSGetCurrent()->CreateDirectoryIterator(path); }

} // ui
