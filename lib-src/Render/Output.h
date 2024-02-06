
#pragma once

#include "../Core/String.h"
#include "../Core/Math.h"


namespace ui {
struct NativeWindowBase;
namespace gfx {


struct RefreshRate
{
	u32 num = 0;
	u32 denom = 1;

	bool operator == (const RefreshRate& o) const
	{
		// do not do more advanced checks since a driver may change behavior based on exact numbers
		return num == o.num && denom == o.denom;
	}
	bool operator != (const RefreshRate& o) const { return !(*this == o); }

	bool EqualTo(const RefreshRate& o) const
	{
		return num * o.denom == o.num * denom;
	}
};

struct DisplayMode
{
	u32 width = 0;
	u32 height = 0;
	RefreshRate refreshRate = { 0, 1 };
};

typedef struct Monitor_* MonitorID;
struct Monitors
{
	static Array<MonitorID> All();
	static MonitorID Primary();
	static MonitorID FindFromPoint(Vec2i point, bool nearest = false);
	static MonitorID FindFromWindow(NativeWindowBase* w);

	static bool IsPrimary(MonitorID id);
	static AABB2i GetScreenArea(MonitorID id);
	static std::string GetName(MonitorID id);
	static Array<DisplayMode> GetAvailableDisplayModes(MonitorID id);
};

struct ExclusiveFullscreenInfo
{
	Size2i size;
	MonitorID monitor = nullptr;
	RefreshRate refreshRate = { 0, 1 };

	bool operator == (const ExclusiveFullscreenInfo& o) const
	{
		return size == o.size && monitor == o.monitor && refreshRate == o.refreshRate;
	}
	bool operator != (const ExclusiveFullscreenInfo& o) const { return !(*this == o); }
};


struct GraphicsAdapters
{
	enum InfoFlags
	{
		Info_Monitors = 1 << 0,
		Info_MonitorNames = Info_Monitors | (1 << 1),
		Info_DisplayModes = Info_Monitors | (1 << 2),
		Info_All = Info_Monitors | Info_MonitorNames | Info_DisplayModes,
	};

	struct Info
	{
		struct Monitor
		{
			MonitorID id;
			std::string name;
			Array<DisplayMode> displayModes;
		};
		std::string name;
		Array<Monitor> monitors;
	};

	static Array<Info> All(u32 flags = Info_All);

	static void GetInitial(int& index, StringView& name);
	static bool IsInitialLocked();
	static int GetLockedInitialAdapter();
	static bool SetInitialByName(StringView name); // empty = default
	static bool SetInitialByIndex(int index); // -1 = default
};


} // gfx
} // ui
