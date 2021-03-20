
#pragma once

#include "String.h"


namespace ui {

std::string ReadTextFile(const char* path);
bool WriteTextFile(const char* path, StringView text);

struct DirectoryIterator
{
	struct DirectoryIteratorImpl* _impl;

	DirectoryIterator(const char* path);
	~DirectoryIterator();

	bool GetNext(std::string& retFile);
};

enum FileAttributes
{
	FA_Directory,
	FA_Symlink,
};
unsigned GetFileAttributes(const char* path);
uint64_t GetFileModTimeUTC(const char* path);

} // ui
