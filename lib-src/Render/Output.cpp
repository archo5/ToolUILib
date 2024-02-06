
#include "Output.h"

#include "../Core/Logging.h"
#include "../Model/Native.h"

#ifdef _WIN32
#define UNICODE 1
#include <windows.h>
#include "../Core/WindowsUtils.h"
#endif


namespace ui {
extern LogCategory LOG_WIN32; // TODO move?
namespace gfx {


static BOOL CALLBACK MonitorEnumProc(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM lp)
{
	auto* ret = reinterpret_cast<Array<MonitorID>*>(lp);
	auto id = reinterpret_cast<MonitorID>(monitor);
	if (rect->left == 0 && rect->top == 0)
		ret->InsertAt(0, id); // make primary first
	else
		ret->Append(id);
	return TRUE;
}

Array<MonitorID> Monitors::All()
{
	Array<MonitorID> ret;
	if (!::EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&ret)))
	{
		LogWarn(LOG_WIN32, "EnumDisplayMonitors failed!");
	}
	return ret;
}

MonitorID Monitors::Primary()
{
	return MonitorID(::MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
}

MonitorID Monitors::FindFromPoint(Vec2i point, bool nearest)
{
	return MonitorID(::MonitorFromPoint({ point.x, point.y }, nearest ? MONITOR_DEFAULTTONEAREST : MONITOR_DEFAULTTONULL));
}

MonitorID Monitors::FindFromWindow(NativeWindowBase* w)
{
	return MonitorID(::MonitorFromWindow(HWND(w->GetNativeHandle()), MONITOR_DEFAULTTONEAREST));
}

bool Monitors::IsPrimary(MonitorID id)
{
	MONITORINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);

	if (::GetMonitorInfoW(HMONITOR(id), &info))
	{
		auto r = info.rcMonitor;
		return r.left == 0 && r.top == 0;
	}
	return false;
}

AABB2i Monitors::GetScreenArea(MonitorID id)
{
	MONITORINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);

	if (::GetMonitorInfoW(HMONITOR(id), &info))
	{
		auto r = info.rcMonitor;
		return { r.left, r.top, r.right, r.bottom };
	}
	return {};
}

std::string Monitors::GetName(MonitorID id)
{
	MONITORINFOEXW info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);

	if (::GetMonitorInfoW(HMONITOR(id), &info))
	{
		DISPLAY_DEVICEW dd;
		memset(&dd, 0, sizeof(dd));
		dd.cb = sizeof(dd);

		if (::EnumDisplayDevicesW(info.szDevice, 0, &dd, 0))
		{
			return WCHARtoUTF8(dd.DeviceString);
		}
	}
	return {};
}

Array<DisplayMode> Monitors::GetAvailableDisplayModes(MonitorID id)
{
	MONITORINFOEXW info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);

	if (::GetMonitorInfoW(HMONITOR(id), &info))
	{
		Array<DisplayMode> ret;
		ret.Reserve(35); // 28-32 for a generic 1080p monitor (different collections)
		DEVMODEW mode;
		memset(&mode, 0, sizeof(mode));
		mode.dmSize = sizeof(DEVMODEW);
		for (DWORD n = 0; ::EnumDisplaySettingsExW(info.szDevice, n, &mode, 0); n++)
		{
			if (mode.dmBitsPerPel == 32 && mode.dmDisplayFixedOutput == DMDFO_DEFAULT)
				ret.Append({ mode.dmPelsWidth, mode.dmPelsHeight, mode.dmDisplayFrequency, 1 });
		}
		return ret;
	}
	return {};
}


static int g_initialGraphicsAdapterLocked = -1;
static int g_initialGraphicsAdapterIndex = -1;
static std::string g_initialGraphicsAdapterName;

void GraphicsAdapters::GetInitial(int& index, StringView& name)
{
	index = g_initialGraphicsAdapterIndex;
	name = g_initialGraphicsAdapterName;
}

bool GraphicsAdapters::IsInitialLocked()
{
	return g_initialGraphicsAdapterLocked >= 0;
}

int GraphicsAdapters::GetLockedInitialAdapter()
{
	return g_initialGraphicsAdapterLocked;
}

bool GraphicsAdapters::SetInitialByName(StringView name)
{
	if (g_initialGraphicsAdapterLocked >= 0)
		return false;
	g_initialGraphicsAdapterIndex = -1;
	g_initialGraphicsAdapterName = ui::to_string(name);
	return true;
}

bool GraphicsAdapters::SetInitialByIndex(int index)
{
	if (g_initialGraphicsAdapterLocked >= 0)
		return false;
	g_initialGraphicsAdapterIndex = index;
	g_initialGraphicsAdapterName = {};
	return true;
}

void GraphicsAdapters_Lock(int which)
{
	assert(g_initialGraphicsAdapterLocked < 0);
	assert(which >= 0);
	g_initialGraphicsAdapterLocked = which;
}

void GraphicsAdapters_Unlock()
{
	assert(g_initialGraphicsAdapterLocked >= 0);
	g_initialGraphicsAdapterLocked = -1;
}


} // gfx
} // ui
