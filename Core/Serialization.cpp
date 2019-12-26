
#include "Serialization.h"
#include <Windows.h>


class SettingsBackend
{
public:
	virtual int64_t ReadInt(StringView path) = 0;
	virtual void WriteInt(StringView path, int64_t) = 0;
	virtual double ReadFloat(StringView path) = 0;
	virtual void WriteFloat(StringView path, double) = 0;
	virtual std::string ReadString(StringView path) = 0;
	virtual void WriteString(StringView path, StringView) = 0;
};


class RegistryBackendWin32Registry : SettingsBackend
{
	int64_t ReadInt(StringView path) override
	{
		DWORD type = 0, size = 0;
		int64_t value = 0;
		auto ret = RegGetValueA(HKEY_LOCAL_MACHINE, "Software/UITEST", path.data(), RRF_RT_REG_DWORD | RRF_RT_REG_QWORD | RRF_ZEROONFAILURE, &type, &value, &size);
		if (ret == ERROR_SUCCESS)
			return value;
		printf("failed to read: %08X\n", ret);
		return 0;
	}
	void WriteInt(StringView path, int64_t) override
	{
	}
	double ReadFloat(StringView path) override
	{
	}
	void WriteFloat(StringView path, double) override
	{
	}
	std::string ReadString(StringView path) override
	{
	}
	void WriteString(StringView path, StringView) override
	{
	}
};


class SettingsHierarchySerializerBase : public SettingsSerializer
{
public:
	SettingsHierarchySerializerBase(SettingsBackend* be, StringView basePath) :
		_backend(be),
		_basePath(basePath.data(), basePath.size()),
		_curPath(_basePath)
	{
	}
	std::string GetFullPath(StringView name)
	{
		std::string out = _curPath;
		out.push_back('/');
		out.append(name.data(), name.size());
		return out;
	}
	void SetSubdirectory(StringView name) override
	{
		_curPath = _basePath;
		_curPath.push_back('/');
		_curPath.append(name.data(), name.size());
	}

protected:
	SettingsBackend* _backend;
	std::string _basePath;
	std::string _curPath;
};

class Reader : public SettingsHierarchySerializerBase
{
public:
	bool IsWriter() override { return false; }
	void _SerializeInt(StringView name, int64_t& v) override { v = _backend->ReadInt(GetFullPath(name)); }
	void _SerializeFloat(StringView name, double& v) override { v = _backend->ReadFloat(GetFullPath(name)); }
	void SerializeString(StringView name, std::string& v) override { v = _backend->ReadString(GetFullPath(name)); }
};

class Writer : public SettingsHierarchySerializerBase
{
public:
	bool IsWriter() override { return true; }
	void _SerializeInt(StringView name, int64_t& v) override { _backend->WriteInt(GetFullPath(name), v); }
	void _SerializeFloat(StringView name, double& v) override { _backend->WriteFloat(GetFullPath(name), v); }
	void SerializeString(StringView name, std::string& v) override { _backend->WriteString(GetFullPath(name), v); }
};
