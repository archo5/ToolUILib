
#pragma once
#include <inttypes.h>
#include <type_traits>
#include <string>
#include "String.h"

class SettingsSerializer
{
public:
	virtual bool IsWriter() = 0;
	virtual void SetSubdirectory(StringView) = 0;
	virtual void _SerializeInt(StringView name, int64_t&) = 0;
	virtual void _SerializeFloat(StringView name, double&) = 0;
	virtual void SerializeString(StringView name, std::string&) = 0;

	template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
	void SerializeInt(StringView name, T& v)
	{
		int64_t i = v;
		_SerializeInt(name, i);
		v = i;
	}

	template <class T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
	void SerializeFloat(StringView name, T& v)
	{
		double f = v;
		_SerializeFloat(name, f);
		v = f;
	}
};

class Registry
{
public:
};
