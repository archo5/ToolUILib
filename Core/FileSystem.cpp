
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


static IOResult IOErrorFromErrno()
{
	switch (errno)
	{
	case ENOENT: return IOResult::FileNotFound;
	default: return IOResult::Unknown;
	}
}

static FileReadResult ReadFile(StringView path, const char* mode)
{
	FILE* f = fopen(CStr<MAX_PATH>(path), mode);
	if (!f)
		return { IOErrorFromErrno() };

	RCHandle<OwnedMemoryBuffer> ret = new OwnedMemoryBuffer;
	fseek(f, 0, SEEK_END);
	if (auto s = ftell(f))
	{
		ret->Alloc(s);

		fseek(f, 0, SEEK_SET);
		s = fread(ret->data, 1, s, f);
		ret->size = s;
	}
	fclose(f);

	return { IOResult::Success, ret };
}

FileReadResult ReadTextFile(StringView path)
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

FileReadResult ReadBinaryFile(StringView path)
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


struct DirectoryIteratorImpl : IDirectoryIterator
{
	std::wstring path;
	WIN32_FIND_DATAW findData = {};
	HANDLE handle = nullptr;

	DirectoryIteratorImpl(StringView srcPath)
	{
		path = UTF8toWCHAR(srcPath);
		path.append(L"/*");
	}
	~DirectoryIteratorImpl()
	{
		if (handle)
			FindClose(handle);
	}

	bool GetNext(std::string& retFile) override;
};

bool DirectoryIteratorImpl::GetNext(std::string& retFile)
{
	if (path.empty())
		return false;

	bool success;
	for (;;)
	{
		if (handle == nullptr)
		{
			handle = FindFirstFileW(path.c_str(), &findData);
			success = handle != nullptr;
		}
		else
			success = FindNextFileW(handle, &findData) != FALSE;

		if (!success)
			break;

		if (wcscmp(findData.cFileName, L".") != 0 &&
			wcscmp(findData.cFileName, L"..") != 0)
			break; // found an actual entry
	}

	if (!success)
		return false;

	retFile = WCHARtoUTF8(findData.cFileName);
	return true;
}

DirectoryIteratorHandle CreateDirectoryIterator(StringView path)
{
	return new DirectoryIteratorImpl(path);
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
	UI_DEFER(CloseHandle(hfile));
	FILETIME t;
	if (!::GetFileTime(hfile, nullptr, nullptr, &t))
		return 0;
	ULARGE_INTEGER uli;
	uli.HighPart = t.dwHighDateTime;
	uli.LowPart = t.dwLowDateTime;
	return uli.QuadPart / 10000; // to milliseconds
}


FileReadResult FileSourceSequence::ReadTextFile(StringView path)
{
	FileReadResult ret;
	for (auto& fs : fileSystems)
	{
		ret = fs->ReadTextFile(path);
		if (ret.result == IOResult::Success)
			return ret;
	}
	return ret;
}

FileReadResult FileSourceSequence::ReadBinaryFile(StringView path)
{
	FileReadResult ret;
	for (auto& fs : fileSystems)
	{
		ret = fs->ReadBinaryFile(path);
		if (ret.result == IOResult::Success)
			return ret;
	}
	return ret;
}

struct AllFileSourceIterator : IDirectoryIterator
{
	std::string path;
	RCHandle<FileSourceSequence> fs;
	size_t curNum = 0;
	DirectoryIteratorHandle cdi;

	bool GetNext(std::string& retFile) override
	{
		for (;;)
		{
			if (cdi)
			{
				if (cdi->GetNext(retFile))
					return true;
				else
					cdi = nullptr;
			}
			else
			{
				if (curNum >= fs->fileSystems.size())
					return false;
				cdi = fs->fileSystems[curNum++]->CreateDirectoryIterator(path);
			}
		}
	}
};

DirectoryIteratorHandle FileSourceSequence::CreateDirectoryIterator(StringView path)
{
	auto* it = new AllFileSourceIterator;
	it->path.assign(path.data(), path.size());
	it->fs = this;
	return it;
}


struct FileSystemSource : IFileSource
{
	std::string rootPath;

	FileSystemSource(StringView root)
	{
		rootPath = PathGetAbsolute(root);
	}

	std::string GetRealPath(StringView path)
	{
		if (PathIsAbsolute(path))
			return { path.data(), path.size() };
		return PathJoin(rootPath, path);
	}

	FileReadResult ReadTextFile(StringView path)
	{
		return ui::ReadTextFile(GetRealPath(path));
	}
	FileReadResult ReadBinaryFile(StringView path)
	{
		return ui::ReadBinaryFile(GetRealPath(path));
	}
	DirectoryIteratorHandle CreateDirectoryIterator(StringView path)
	{
		return ui::CreateDirectoryIterator(GetRealPath(path));
	}
};

FileSourceHandle CreateFileSystemSource(StringView rootPath)
{
	return new FileSystemSource(rootPath);
}


static FileSourceHandle g_currentFS;
static RCHandle<FileSourceSequence> g_mainFS;

FileSourceSequence* FSGetDefault()
{
	return g_mainFS;
}

IFileSource* FSGetCurrent()
{
	return g_currentFS;
}

void FSSetCurrent(IFileSource* fs)
{
	g_currentFS = fs;
}

struct FSInitFree
{
	FSInitFree()
	{
		g_mainFS = new FileSourceSequence;
		g_currentFS = g_mainFS;
	}
	~FSInitFree()
	{
		g_currentFS = nullptr;
		g_mainFS = nullptr;
	}
}
g_FSInitializer;

} // ui
