
#include "FileSystem.h"

#include "Logging.h"
#include "Threading.h"
#include "WindowsUtils.h"

#include "../Model/Native.h"

#include "../../ThirdParty/miniz.h"

#include <thread>


#undef CreateDirectory
#undef GetFileAttributes


namespace ui {

std::string PathFromSystem(StringView path)
{
	auto pcopy = to_string(path);
	NormalizePath(pcopy);
	return pcopy;
}

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
	HANDLE handle = INVALID_HANDLE_VALUE;

	DirectoryIteratorImpl(StringView srcPath)
	{
		path = UTF8toWCHAR(srcPath);
		path.append(L"/*");
	}
	~DirectoryIteratorImpl()
	{
		if (handle != INVALID_HANDLE_VALUE)
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
		if (handle == INVALID_HANDLE_VALUE)
		{
			handle = FindFirstFileW(path.c_str(), &findData);
			success = handle != INVALID_HANDLE_VALUE;
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


static LogCategory LOG_DIR_CHANGE_WATCHER("DirChgWatch");

struct DirectoryChangeWatcherImpl : IDirectoryChangeWatcher
{
	std::wstring _path;
	IDirectoryChangeListener* _listener;
	std::thread _thread;
	HANDLE _quitEvent;

	struct CHANGE_BUF
	{
		FILE_NOTIFY_INFORMATION _align;
		char _pad[16384 - sizeof(_align)];
	};

	DirectoryChangeWatcherImpl(StringView path, IDirectoryChangeListener* listener) : _listener(listener)
	{
		_path = UTF8toWCHAR(path);
		_quitEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
		_thread = std::thread([this]() { Run(); });
	}

	~DirectoryChangeWatcherImpl()
	{
		::SetEvent(_quitEvent);
		_thread.join();
		CloseHandle(_quitEvent);
	}

	void Run()
	{
		DWORD flags =
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE;
#if 0
		HANDLE h = ::FindFirstChangeNotificationW(_path.c_str(), TRUE, flags);
		for (;;)
		{
			HANDLE waitHandles[] = { _quitEvent, h };
			auto ret = ::WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0)
				break;
			if (ret != WAIT_OBJECT_0 + 1)
				continue;

			::FindNextChangeNotification(h);
		}
		::FindCloseChangeNotification(h);
#endif
		HANDLE h = ::CreateFileW(
			_path.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED | FILE_FLAG_BACKUP_SEMANTICS,
			nullptr);
		if (h == INVALID_HANDLE_VALUE)
		{
			LogWarn(LOG_DIR_CHANGE_WATCHER, "CreateFileW failed (error %08X) on %s", GetLastError(), WCHARtoUTF8s(_path).c_str());
			return;
		}

		HANDLE complEv = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);

		bool error = false;
		while (::WaitForSingleObject(_quitEvent, 0) != WAIT_OBJECT_0)
		{
			CHANGE_BUF buf;
			OVERLAPPED ovr = {};
			ovr.hEvent = complEv;
			if (!::ReadDirectoryChangesW(h, &buf, sizeof(buf), TRUE, flags, nullptr, &ovr, nullptr))
			{
				if (!error)
				{
					LogWarn(LOG_DIR_CHANGE_WATCHER, "ReadDirectoryChangesW failed (%08X)", GetLastError());
					error = true;
				}
				::WaitForSingleObject(_quitEvent, 1000);
				continue;
			}
			HANDLE waitHandles[] = { _quitEvent, complEv };
			auto ret = ::WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
			if (ret == WAIT_OBJECT_0)
				break;
			if (ret != WAIT_OBJECT_0 + 1)
				continue;

			DWORD size = 0;
			if (!::GetOverlappedResult(h, &ovr, &size, FALSE))
				LogError(LOG_DIR_CHANGE_WATCHER, "GetOverlappedResult failed!");

			if (size == 0)
			{
				LogError(LOG_DIR_CHANGE_WATCHER, "returned 0 bytes, changes were too big for the buffer!");
				continue;
			}
			else
			{
				LogInfo(LOG_DIR_CHANGE_WATCHER, "returned %d bytes", int(size));
			}

			auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buf);
			for (;;)
			{
				auto path = WCHARtoUTF8(info->FileName, info->FileNameLength / sizeof(WCHAR));
				if (CanLogInfo(LOG_DIR_CHANGE_WATCHER))
				{
					const char* type = "<unrecognized file action>";
					switch (info->Action)
					{
					case FILE_ACTION_ADDED: type = "ADDED"; break;
					case FILE_ACTION_REMOVED: type = "REMOVED"; break;
					case FILE_ACTION_MODIFIED: type = "MODIFIED"; break;
					case FILE_ACTION_RENAMED_OLD_NAME: type = "RENAMED_OLD_NAME"; break;
					case FILE_ACTION_RENAMED_NEW_NAME: type = "RENAMED_NEW_NAME"; break;
					}
					LogInfo(
						LOG_DIR_CHANGE_WATCHER,
						"%s: [%u]%s",
						type,
						unsigned(info->FileNameLength),
						path.c_str());
				}

				path = PathFromSystem(path);
				switch (info->Action)
				{
				case FILE_ACTION_ADDED:
				case FILE_ACTION_RENAMED_NEW_NAME:
					Application::PushEvent([this, path{ std::move(path) }]() { _listener->OnFileAdded(path); });
					break;
				case FILE_ACTION_REMOVED:
				case FILE_ACTION_RENAMED_OLD_NAME:
					Application::PushEvent([this, path{ std::move(path) }]() { _listener->OnFileRemoved(path); });
					break;
				case FILE_ACTION_MODIFIED:
					Application::PushEvent([this, path{ std::move(path) }]() { _listener->OnFileChanged(path); });
					break;
				}

				if (info->NextEntryOffset == 0)
					break;
				info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(info) + info->NextEntryOffset);
			}
		}

		CloseHandle(complEv);
	}
};

DirectoryChangeWatcherHandle CreateDirectoryChangeWatcher(StringView path, IDirectoryChangeListener* listener)
{
	return new DirectoryChangeWatcherImpl(path, listener);
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


struct ZipFileSource : IFileSource
{
	struct Entry
	{
		mz_uint id = -1;
		bool dir = false;
		std::vector<std::string> dirEntries;
	};

	mz_zip_archive arch;
	BufferHandle memory;
	HashMap<std::string, Entry> fileMap;

	~ZipFileSource()
	{
		mz_zip_reader_end(&arch);
	}

	Entry& InsertEntry(const std::string& name)
	{
		auto parent = PathGetParent(name);
		if (parent != "" && parent != ".")
		{
			auto& pe = InsertEntry(parent);
			pe.dir = true;
			pe.dirEntries.push_back(name);
		}
		return fileMap[name];
	}

	void Init()
	{
		auto numFiles = mz_zip_reader_get_num_files(&arch);
		for (mz_uint i = 0; i < numFiles; i++)
		{
			char name[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE + 1];
			auto written = mz_zip_reader_get_filename(&arch, i, name, MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE + 1);
			if (written >= 2 &&
				name[written - 2] == '/')
				name[written - 2] = 0;

			Entry& e = InsertEntry(name);
			e.id = i;
			e.dir = mz_zip_reader_is_file_a_directory(&arch, i);
		}
	}

	static size_t NormalizeNewlines(char* data, size_t size)
	{
		char* start = data;
		char* end = data + size;
		char* dstdata = data;
		for (; data != end; data++, dstdata++)
		{
			if (*data == '\r')
			{
				if (data + 1 != end && data[1] == '\n')
				{
					dstdata--;
					continue;
				}
				else
				{
					*dstdata = '\n';
				}
			}
			else
				*dstdata = *data;
		}
		return dstdata - start;
	}

	FileReadResult ReadTextFile(StringView path)
	{
		auto res = ReadBinaryFile(path);
		if (res.data)
		{
			static_cast<OwnedMemoryBuffer*>(&*res.data)->size = NormalizeNewlines(static_cast<char*>(res.data->GetData()), res.data->GetSize());
		}
		return res;
	}
	FileReadResult ReadBinaryFile(StringView path)
	{
		auto it = fileMap.find(to_string(path));
		if (it.is_valid() && it->value.dir == false && it->value.id != mz_uint(-1))
		{
			mz_zip_archive_file_stat s;
			if (mz_zip_reader_file_stat(&arch, it->value.id, &s))
			{
				auto buf = AsRCHandle(new OwnedMemoryBuffer);
				size_t fileSize = mz_uint(s.m_uncomp_size);
				buf->size = fileSize;
				buf->Alloc(fileSize);
				if (mz_zip_reader_extract_to_mem_no_alloc(&arch, it->value.id, buf->data, buf->size, 0, nullptr, 0))
					return { IOResult::Success, buf };
			}
		}
		return { IOResult::FileNotFound };
	}

	struct DirIter : IDirectoryIterator
	{
		RCHandle<ZipFileSource> zfs;
		Entry* entry = nullptr;
		size_t at = 0;

		bool GetNext(std::string& retFile)
		{
			if (at < entry->dirEntries.size())
			{
				retFile = entry->dirEntries[at++];
				return true;
			}
			return false;
		}
	};
	DirectoryIteratorHandle CreateDirectoryIterator(StringView path)
	{
		auto it = fileMap.find(to_string(path));
		if (it.is_valid() && it->value.dir)
		{
			auto ret = AsRCHandle(new DirIter);
			ret->zfs = this;
			ret->entry = &it->value;
			return ret;
		}
		return new NullDirectoryIterator;
	}
};

FileSourceHandle CreateZipFileSource(StringView path)
{
	auto h = AsRCHandle(new ZipFileSource);
	mz_zip_zero_struct(&h->arch);
	mz_zip_reader_init_file(&h->arch, CStr<MAX_PATH>(path), 0);
	h->Init();
	return h;
}

FileSourceHandle CreateZipFileMemorySource(BufferHandle memory)
{
	auto h = AsRCHandle(new ZipFileSource);
	h->memory = memory;
	mz_zip_zero_struct(&h->arch);
	mz_zip_reader_init_mem(&h->arch, memory->GetData(), memory->GetSize(), 0);
	h->Init();
	return h;
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
