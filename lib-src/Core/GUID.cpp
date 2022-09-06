
#include "GUID.h"

#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")


namespace ui {

static_assert(sizeof(GUID) == 16, "wrong GUID struct size");

GUID GUID::New()
{
	UUID id = {};
	UuidCreate(&id);
	GUID ret;
	memcpy(&ret, &id, 16);
	return ret;
}

bool GUID::ValidateStringHex(StringView str)
{
	if (str.size() < 32)
		return false;
	for (int i = 0; i < 32; i++)
		if (!IsHexDigit(str[i]))
			return false;
	return true;
}

static int g_guidHyphenPositions[4] = { 8, 13, 18, 23 };
static int g_guidHexSplitPositions[32] =
{
	0, 1, 2, 3, 4, 5, 6, 7,
	9, 10, 11, 12,
	14, 15, 16, 17,
	19, 20, 21, 22,
	24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
};

bool GUID::ValidateStringHexSplit(StringView str)
{
	if (str.size() < 36)
		return false;
	for (int i = 0; i < 4; i++)
		if (str[g_guidHyphenPositions[i]] != '-')
			return false;
	for (int i = 0; i < 32; i++)
		if (!IsHexDigit(str[g_guidHexSplitPositions[i]]))
			return false;
	return true;
}

GUID GUID::FromStringHex(StringView str)
{
	if (str.size() < 32)
		return {};
	GUID ret;
	for (int i = 0; i < 16; i++)
		ret.v8[i] = (DecodeHexChar(str[i * 2]) << 4) | DecodeHexChar(str[i * 2 + 1]);
	return ret;
}

GUID GUID::FromStringHexSplit(StringView str)
{
	if (str.size() < 36)
		return {};
	for (int i = 0; i < 4; i++)
		if (str[g_guidHyphenPositions[i]] != '-')
			return {};
	GUID ret;
	for (int i = 0; i < 16; i++)
	{
		ret.v8[i] = (DecodeHexChar(str[g_guidHexSplitPositions[i * 2]]) << 4)
			| DecodeHexChar(str[g_guidHexSplitPositions[i * 2 + 1]]);
	}
	return ret;
}

void GUID::ToStringHex(char out[32], bool upper) const
{
	const char* cs = upper ? "0123456789ABCDEF" : "0123456789abcdef";
	for (int i = 0; i < 16; i++)
	{
		out[i * 2 + 0] = cs[v8[i] >> 4];
		out[i * 2 + 1] = cs[v8[i] & 0xf];
	}
}

void GUID::ToStringHexSplit(char out[36], bool upper) const
{
	const char* cs = upper ? "0123456789ABCDEF" : "0123456789abcdef";
	for (int i = 0; i < 4; i++)
		out[g_guidHyphenPositions[i]] = '-';
	for (int i = 0; i < 16; i++)
	{
		int p = g_guidHexSplitPositions[i * 2];
		out[p + 0] = cs[v8[i] >> 4];
		out[p + 1] = cs[v8[i] & 0xf];
	}
}

std::string GUID::ToStringHex(bool upper) const
{
	char buf[32];
	ToStringHex(buf, upper);
	return { buf, 32 };
}

std::string GUID::ToStringHexSplit(bool upper) const
{
	char buf[36];
	ToStringHexSplit(buf, upper);
	return { buf, 36 };
}

} // ui
