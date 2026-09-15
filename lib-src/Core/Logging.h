
#pragma once
#include "Platform.h"
#include "String.h"
#include "Delegate.h"

#include <stdarg.h>


namespace ui {
namespace log {

enum class LogLevel : uint8_t
{
	Off,
	Error, // a clear failure
	Warn,  // may or may not be an error depending on intent
	Info,  // notes an event that can be important to a non-developer power user
	Debug, // an event that is likely to only be important to a developer in specific circumstances
	All,
};

struct UI_API LogMsgRef
{
	StringView full;
	StringView msgonly;
};

struct UI_API LogCategory
{
	const char* const name;

	LogLevel level = LogLevel::Warn;
	MulticastDelegate<LogLevel, LogMsgRef> onCategoryLogMessage;

	explicit LogCategory(const char* nm, LogLevel lev = LogLevel::Warn) : name(nm), level(lev) {}
};

UI_API extern MulticastDelegate<LogLevel, const LogCategory&, LogMsgRef> OnAnyLogMessage;
UI_API extern LogLevel GlobalLogLevel;

UI_API bool CanLogDynLev(LogLevel level, const LogCategory& category);
UI_API bool CanLogError(const LogCategory& category);
UI_API bool CanLogWarn(const LogCategory& category);
UI_API bool CanLogInfo(const LogCategory& category);
UI_API bool CanLogDebug(const LogCategory& category);

UI_API void LogDynLevVA(LogLevel level, const LogCategory& category, const char* fmt, va_list args);
UI_API void LogDynLev(LogLevel level, const LogCategory& category, const char* fmt, ...);

UI_API void LogError(const LogCategory& category, const char* fmt, ...);
UI_API void LogWarn(const LogCategory& category, const char* fmt, ...);
UI_API void LogInfo(const LogCategory& category, const char* fmt, ...);
UI_API void LogDebug(const LogCategory& category, const char* fmt, ...);

} // log
using namespace ui::log;
} // ui
