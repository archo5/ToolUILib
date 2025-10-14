
#include "Logging.h"

#include "../../ThirdParty/stb_sprintf.h"

#include <time.h>


extern "C"
{
	void __stdcall OutputDebugStringA(const char*);
	unsigned long __stdcall GetCurrentThreadId();
}


namespace ui {
namespace log {

MulticastDelegate<LogLevel, const LogCategory&, LogMsgRef> OnAnyLogMessage;
LogLevel GlobalLogLevel = LogLevel::All;

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

bool CanLogDynLev(LogLevel level, const LogCategory& category)
{
	if (level == LogLevel::Off)
		return false;
	if (level > GlobalLogLevel)
		return false;
	if (level > category.level)
		return false;
	return true;
}

bool CanLogError(const LogCategory& category)
{
	return CanLogDynLev(LogLevel::Error, category);
}

bool CanLogWarn(const LogCategory& category)
{
	return CanLogDynLev(LogLevel::Warn, category);
}

bool CanLogInfo(const LogCategory& category)
{
	return CanLogDynLev(LogLevel::Info, category);
}

bool CanLogDebug(const LogCategory& category)
{
	return CanLogDynLev(LogLevel::Debug, category);
}

void LogDynLevVA(LogLevel level, const LogCategory& category, const char* fmt, va_list args)
{
	if (!CanLogDynLev(level, category))
		return;

	char buf[1024];
	tm t = CurTime();
	size_t at = strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
	at += stbsp_snprintf(buf + at, sizeof(buf) - at, " % 5ld %s|%s ", GetCurrentThreadId(), g_levelNames[int(level)], category.name);
	const char* msgonly = buf + at;
	at += stbsp_vsnprintf(buf + at, sizeof(buf) - at, fmt, args);
	if (at + 2 > sizeof(buf))
		at = sizeof(buf) - 2;

	buf[at] = '\0';

	LogMsgRef mref = {};
	{
		mref.full = StringView(buf, at);
		mref.msgonly = StringView(msgonly, size_t(buf + at - msgonly));
	}
	category.onCategoryLogMessage.Call(level, mref);
	OnAnyLogMessage.Call(level, category, mref);

	buf[at++] = '\n';
	buf[at] = '\0';

	fwrite(buf, 1, at, stderr);
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

} // log
} // ui
