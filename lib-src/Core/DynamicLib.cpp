
#include "DynamicLib.h"

#include "Logging.h"
#include "WindowsUtils.h"


namespace ui {

LogCategory LOG_DYNLIB("DynLib");

DynamicLib* DynamicLibLoad(StringView path)
{
	auto* ret = reinterpret_cast<DynamicLib*>(::LoadLibraryW(UTF8toWCHAR(path).c_str()));
	if (ret)
	{
		if (CanLogInfo(LOG_DYNLIB))
			LogInfo(LOG_DYNLIB, "DynamicLibLoad(\"%s\") succeeded => %p", to_string(path).c_str(), ret);
	}
	else
	{
		if (CanLogWarn(LOG_DYNLIB))
			LogWarn(LOG_DYNLIB, "failed to load dynamic lib via LoadLibraryW(\"%s\")", to_string(path).c_str());
	}
	return ret;
}

void DynamicLibUnload(DynamicLib* lib)
{
	if (::FreeLibrary(reinterpret_cast<HMODULE>(lib)))
	{
		if (CanLogInfo(LOG_DYNLIB))
			LogInfo(LOG_DYNLIB, "DynamicLibUnload(%p) succeeded", lib);
	}
	else
	{
		if (CanLogWarn(LOG_DYNLIB))
			LogWarn(LOG_DYNLIB, "failed to free dynamic lib via FreeLibrary");
	}
}

void* DynamicLibGetSymbol(DynamicLib* lib, const char* name)
{
	void* ret = ::GetProcAddress(reinterpret_cast<HMODULE>(lib), name);
	if (CanLogDebug(LOG_DYNLIB))
		LogDebug(LOG_DYNLIB, "DynamicLibGetSymbol(%p, \"%s\") => %p", lib, name, ret);
	return ret;
}

} // ui
