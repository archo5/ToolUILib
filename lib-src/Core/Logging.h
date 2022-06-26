
#pragma once
#include "Platform.h"

#include <stdarg.h>


namespace ui {

enum class LogLevel : uint8_t
{
	Off,
	Error,
	Warn,
	Info,
	Debug,
	All,
};

struct LogCategory
{
	const char* name;

	LogCategory(const char* nm) : name(nm) {}
};

void LogDynLevVA(LogLevel level, const LogCategory& category, const char* fmt, va_list args);
void LogDynLev(LogLevel level, const LogCategory& category, const char* fmt, ...);

void LogError(const LogCategory& category, const char* fmt, ...);
void LogWarn(const LogCategory& category, const char* fmt, ...);
void LogInfo(const LogCategory& category, const char* fmt, ...);
void LogDebug(const LogCategory& category, const char* fmt, ...);

} // ui
