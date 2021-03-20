
#include "FileSystem.h"

#include "WindowsUtils.h"


#undef GetFileAttributes


namespace ui {

std::string ReadTextFile(const char* path)
{
	FILE* f = fopen(path, "r");
	if (!f)
		return {};
	std::string data;
	fseek(f, 0, SEEK_END);
	if (auto s = ftell(f))
	{
		data.resize(s);
		fseek(f, 0, SEEK_SET);
		s = fread(&data[0], 1, s, f);
		data.resize(s);
	}
	fclose(f);
	return data;
}

bool WriteTextFile(const char* path, StringView text)
{
	FILE* f = fopen(path, "w");
	if (!f)
		return false;
	bool success = fwrite(text.data(), text.size(), 1, f) != 0;
	fclose(f);
	return success;
}


struct DirectoryIteratorImpl
{
	std::wstring path;
	WIN32_FIND_DATAW findData = {};
	HANDLE handle = nullptr;

	~DirectoryIteratorImpl()
	{
		if (handle)
			FindClose(handle);
	}
};

DirectoryIterator::DirectoryIterator(const char* path) : _impl(new DirectoryIteratorImpl)
{
	_impl->path = UTF8toWCHAR(path);
	_impl->path.append(L"/*");
}

DirectoryIterator::~DirectoryIterator()
{
	delete _impl;
}

bool DirectoryIterator::GetNext(std::string& retFile)
{
	if (_impl->path.empty())
		return false;

	bool success;
	if (_impl->handle == nullptr)
	{
		_impl->handle = FindFirstFileW(_impl->path.c_str(), &_impl->findData);
		success = _impl->handle != nullptr;
	}
	else
		success = FindNextFileW(_impl->handle, &_impl->findData) != FALSE;
	
	if (!success)
		return false;

	retFile = WCHARtoUTF8(_impl->findData.cFileName);
	return true;
}


unsigned GetFileAttributes(const char* path)
{
	auto attr = GetFileAttributesW(UTF8toWCHAR(path).c_str());
	unsigned ret = 0;
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		ret |= FA_Directory;
	if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
		ret |= FA_Symlink;
	return ret;
}

uint64_t GetFileModTimeUTC(const char* path)
{
	HANDLE hfile = CreateFileW(UTF8toWCHAR(path).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hfile)
		return 0;
	FILETIME t;
	if (!::GetFileTime(hfile, nullptr, nullptr, &t))
		return 0;
	ULARGE_INTEGER uli;
	uli.HighPart = t.dwHighDateTime;
	uli.LowPart = t.dwLowDateTime;
	return uli.QuadPart / 10000; // to milliseconds
}

} // ui
