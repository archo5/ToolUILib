
#pragma once

#include "String.h"


namespace ui {

// returns an empty string if root was passed
std::string PathGetParent(StringView path);
std::string PathJoin(StringView a, StringView b);
bool PathIsAbsolute(StringView path);
std::string PathGetAbsolute(StringView path);
bool PathIsRelativeTo(StringView path, StringView relativeTo);

std::string ReadTextFile(StringView path);
bool WriteTextFile(StringView path, StringView text);
std::string ReadBinaryFile(StringView path);
bool WriteBinaryFile(StringView path, const void* data, size_t size);

bool DirectoryExists(StringView path);
bool CreateDirectory(StringView path);
bool CreateMissingDirectories(StringView path);
bool CreateMissingParentDirectories(StringView path);

std::string GetWorkingDirectory();
bool SetWorkingDirectory(StringView sv);

struct DirectoryIterator
{
	struct DirectoryIteratorImpl* _impl;

	DirectoryIterator(StringView path);
	~DirectoryIterator();

	bool GetNext(std::string& retFile);
};

enum FileAttributes
{
	FA_Exists = 1 << 0,
	FA_Directory = 1 << 1,
	FA_Symlink = 1 << 2,
};
unsigned GetFileAttributes(StringView path);
uint64_t GetFileModTimeUTC(StringView path);

} // ui
