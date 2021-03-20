
#pragma once
#include <Windows.h>


static std::wstring UTF8toWCHAR(StringView sv)
{
	std::wstring ret;
	int size = MultiByteToWideChar(CP_UTF8, 0, sv.data(), sv.size(), nullptr, 0);
	if (size > 0)
	{
		ret.resize(size);
		if (MultiByteToWideChar(CP_UTF8, 0, sv.data(), sv.size(), &ret[0], ret.size()) == size)
			return ret;
	}
	return {};
}

static std::string WCHARtoUTF8(const WCHAR* buf, size_t size = SIZE_MAX)
{
	if (size == SIZE_MAX)
		size = wcslen(buf);
	std::string ret;
	int mbsize = WideCharToMultiByte(CP_UTF8, 0, buf, size, nullptr, 0, nullptr, nullptr);
	if (mbsize > 0)
	{
		ret.resize(mbsize);
		if (WideCharToMultiByte(CP_UTF8, 0, buf, size, &ret[0], ret.size(), nullptr, nullptr) == mbsize)
			return ret;
	}
	return {};
}

static std::string WCHARtoUTF8s(const std::wstring& s)
{
	return WCHARtoUTF8(s.c_str(), s.size());
}
