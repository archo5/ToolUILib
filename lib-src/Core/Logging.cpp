
#include "Logging.h"

#include <stdio.h>
#include <time.h>


extern "C"
{
	void __stdcall OutputDebugStringA(const char*);
	unsigned long __stdcall GetCurrentThreadId();
}


namespace ui {

static tm CurTime()
{
	time_t src = time(nullptr);
	tm t = {};
	if (localtime_s(&t, &src))
		t = {};
	return t;
}

static const char* g_levelNames[] =
{
	"?????",
	"ERROR",
	" WARN",
	" info",
	"debug",
	"-all-",
};

void LogDynLevVA(LogLevel level, const LogCategory& category, const char* fmt, va_list args)
{
	char buf[1024];
	tm t = CurTime();
	size_t at = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
	at += snprintf(buf + at, sizeof(buf) - at, " % 5ld %s|%s ", GetCurrentThreadId(), g_levelNames[int(level)], category.name);
	at += vsnprintf(buf + at, sizeof(buf) - at, fmt, args);
	if (at + 2 > sizeof(buf))
		at = sizeof(buf) - 2;
	buf[at++] = '\n';
	buf[at] = '\0';

	fwrite(buf, 1, at, stdout);
	OutputDebugStringA(buf);
}

void LogDynLev(LogLevel level, const LogCategory& category, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	LogDynLevVA(level, category, fmt, args);
	va_end(args);
}

void LogError(const LogCategory& category, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	LogDynLevVA(LogLevel::Error, category, fmt, args);
	va_end(args);
}

void LogWarn(const LogCategory& category, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	LogDynLevVA(LogLevel::Warn, category, fmt, args);
	va_end(args);
}

void LogInfo(const LogCategory& category, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	LogDynLevVA(LogLevel::Info, category, fmt, args);
	va_end(args);
}

void LogDebug(const LogCategory& category, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	LogDynLevVA(LogLevel::Debug, category, fmt, args);
	va_end(args);
}

} // ui
