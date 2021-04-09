
#include "FileSystem.h"

#include "WindowsUtils.h"


#undef CreateDirectory
#undef GetFileAttributes


namespace ui {

std::string PathGetParent(StringView path)
{
	while (path.ends_with("/"))
		path = path.substr(0, path.size() - 1);
	if (path == ".." || path.ends_with("/.."))
		return to_string(path, "/..");
	if (path == "." || path == "")
		return "..";
	auto lastSep = path.find_last_at("/");
	if (lastSep == SIZE_MAX)
		return ".";
	if (lastSep == 0 || path[lastSep - 1] == ':')
		return ""; // end of absolute path
	return to_string(path.substr(0, lastSep));
}

std::string PathJoin(StringView a, StringView b)
{
	while (a.ends_with("/"))
		a = a.substr(0, a.size() - 1);
	while (b.starts_with("/"))
		b = b.substr(1);
	return to_string(a, "/", b);
}

bool PathIsAbsolute(StringView path)
{
	if (path.starts_with("/"))
		return true;
	if (path.size() >= 3 && path[1] == ':' && path[2] == '/')
		return true;
	return false;
}

std::string PathGetAbsolute(StringView path)
{
	if (PathIsAbsolute(path))
		return to_string(path);
	auto cwd = GetWorkingDirectory();
	return PathJoin(cwd, path);
}

std::string PathGetRelativeTo(StringView path, StringView relativeTo)
{
	auto abspath = PathGetAbsolute(path);
	auto absrelto = PathGetAbsolute(relativeTo);
	if (!StringView(abspath).ends_with("/"))
		abspath.push_back('/');
	if (!StringView(absrelto).ends_with("/"))
		absrelto.push_back('/');
	std::string backtrack;
	while (absrelto.size() > 1)
	{
		if (StringView(abspath).starts_with(absrelto))
			return ui::to_string(backtrack, StringView(abspath).substr(absrelto.size()).until_last("/"));
		backtrack += "../";
		absrelto = PathGetParent(absrelto);
		absrelto.push_back('/');
	}
	return abspath;
}

bool PathIsRelativeTo(StringView path, StringView relativeTo)
{
	auto abspath = PathGetAbsolute(path);
	auto absrelto = PathGetAbsolute(relativeTo);
	if (!StringView(abspath).ends_with("/"))
		abspath.push_back('/');
	if (!StringView(absrelto).ends_with("/"))
		absrelto.push_back('/');
	return StringView(abspath).starts_with(absrelto);
}


static std::string ReadFile(StringView path, const char* mode)
{
	FILE* f = fopen(CStr<MAX_PATH>(path), mode);
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

std::string ReadTextFile(StringView path)
{
	return ReadFile(path, "r");
}

bool WriteTextFile(StringView path, StringView text)
{
	FILE* f = fopen(CStr<MAX_PATH>(path), "w");
	if (!f)
		return false;
	bool success = fwrite(text.data(), text.size(), 1, f) != 0;
	fclose(f);
	return success;
}

std::string ReadBinaryFile(StringView path)
{
	return ReadFile(path, "rb");
}

bool WriteBinaryFile(StringView path, const void* data, size_t size)
{
	FILE* f = fopen(CStr<MAX_PATH>(path), "wb");
	if (!f)
		return false;
	bool success = fwrite(data, size, 1, f) != 0;
	fclose(f);
	return success;
}


bool DirectoryExists(StringView path)
{
	auto attr = ::GetFileAttributesW(UTF8toWCHAR(path).c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool CreateDirectory(StringView path)
{
	return ::CreateDirectoryW(UTF8toWCHAR(path).c_str(), nullptr) != FALSE;
}

bool CreateMissingDirectories(StringView path)
{
	if (!DirectoryExists(path))
	{
		auto parent = PathGetParent(path);
		if (!parent.empty())
			if (!CreateMissingDirectories(parent))
				return false;
		return CreateDirectory(path);
	}
	return true;
}

bool CreateMissingParentDirectories(StringView path)
{
	return CreateMissingDirectories(PathGetParent(path));
}


std::string GetWorkingDirectory()
{
	DWORD len = ::GetCurrentDirectoryW(0, nullptr);
	if (len > 0)
	{
		std::wstring ret;
		ret.resize(len);
		DWORD len2 = ::GetCurrentDirectoryW(ret.size(), &ret[0]);
		if (len2 > 0)
		{
			ret.resize(len2);
			NormalizePath(ret);
			return WCHARtoUTF8s(ret);
		}
	}
	return {};
}

bool SetWorkingDirectory(StringView sv)
{
	return ::SetCurrentDirectoryW(UTF8toWCHAR(sv).c_str()) != FALSE;
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

DirectoryIterator::DirectoryIterator(StringView path) : _impl(new DirectoryIteratorImpl)
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
	for (;;)
	{
		if (_impl->handle == nullptr)
		{
			_impl->handle = FindFirstFileW(_impl->path.c_str(), &_impl->findData);
			success = _impl->handle != nullptr;
		}
		else
			success = FindNextFileW(_impl->handle, &_impl->findData) != FALSE;

		if (!success)
			break;

		if (wcscmp(_impl->findData.cFileName, L".") != 0 &&
			wcscmp(_impl->findData.cFileName, L"..") != 0)
			break; // found an actual entry
	}

	if (!success)
		return false;

	retFile = WCHARtoUTF8(_impl->findData.cFileName);
	return true;
}


unsigned GetFileAttributes(StringView path)
{
	auto attr = GetFileAttributesW(UTF8toWCHAR(path).c_str());
	if (attr == INVALID_FILE_ATTRIBUTES)
		return 0;
	unsigned ret = FA_Exists;
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		ret |= FA_Directory;
	if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
		ret |= FA_Symlink;
	return ret;
}

uint64_t GetFileModTimeUTC(StringView path)
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
