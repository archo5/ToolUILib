
#pragma once
#include "Platform.h"
#include "String.h"
#include "Delegate.h"

#include <stdarg.h>


namespace ui {

enum class LogLevel : uint8_t
{
	Off,
	Error, // a clear failure
	Warn,  // may or may not be an error depending on intent
	Info,  // notes an event that can be important to a non-developer power user
	Debug, // an event that is likely to only be important to a developer in specific circumstances
	All,
};

struct LogCategory
{
	const char* const name;

	LogLevel level = LogLevel::Warn;
	MulticastDelegate<LogLevel, StringView> onCategoryLogMessage;

	LogCategory(const char* nm, LogLevel lev = LogLevel::Warn) : name(nm), level(lev) {}
};

extern MulticastDelegate<LogLevel, const LogCategory&, StringView> OnAnyLogMessage;
extern LogLevel GlobalLogLevel;

bool CanLogDynLev(LogLevel level, const LogCategory& category);
bool CanLogError(const LogCategory& category);
bool CanLogWarn(const LogCategory& category);
bool CanLogInfo(const LogCategory& category);
bool CanLogDebug(const LogCategory& category);

void LogDynLevVA(LogLevel level, const LogCategory& category, const char* fmt, va_list args);
void LogDynLev(LogLevel level, const LogCategory& category, const char* fmt, ...);

void LogError(const LogCategory& category, const char* fmt, ...);
void LogWarn(const LogCategory& category, const char* fmt, ...);
void LogInfo(const LogCategory& category, const char* fmt, ...);
void LogDebug(const LogCategory& category, const char* fmt, ...);

} // ui
